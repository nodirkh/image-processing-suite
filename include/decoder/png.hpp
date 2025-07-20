#ifndef IPS_PNG_DECODER
#define IPS_PNG_DECODER

#include <zlib.h>

#include <array>
#include <cstdint>
#include <expected>
#include <fstream>
#include <iostream>
#include <memory>
#include <string_view>
#include <tuple>
#include <vector>

namespace ips
{

enum class PNGError
{
    SUCCESS = 200,
    FILE_NOT_FOUND = 404,
    INVALID_MAGIC,
    CORRUPTED_HEADER,
    CORRUPTED_DATA,
    UNSUPPORTED_FORMAT,
    DECOMPRESSION_FAILED,
    DECODE_FAILED,
    INVALID_DIMENSIONS
};
namespace decode
{

static constexpr std::array<uint8_t, 8> PNG_MAGIC = {0x89, 0x50, 0x4E, 0x47,
                                                     0x0D, 0x0A, 0x1A, 0x0A};

static constexpr uint32_t MAX_DIM = 65535;

static inline uint32_t calculateCRC(const std::vector<uint8_t> &Data,
                                    const std::string_view Type)
{
    const size_t dataSize = Data.size();
    const size_t totalSize = 4 + dataSize;

    std::vector<uint8_t> crcData;
    crcData.reserve(totalSize);

    crcData.insert(crcData.end(), Type.begin(), Type.end());

    crcData.insert(crcData.end(), Data.begin(), Data.end());

    return static_cast<uint32_t>(crc32(0, crcData.data(), crcData.size()));
}

static inline constexpr uint8_t PaethPredictor(int a, int b, int c)
{
    const int p = a + b - c;
    const int pa = std::abs(p - a);
    const int pb = std::abs(p - b);
    const int pc = std::abs(p - c);

    if (pa <= pb && pa <= pc) return static_cast<uint8_t>(a);
    if (pb <= pc) return static_cast<uint8_t>(b);
    return static_cast<uint8_t>(c);
};

static inline constexpr bool colorValid(uint8_t depth,
                                        uint8_t colorType) noexcept
{
    switch (colorType)
    {
        case 0:  // Grayscale
            return depth == 1 || depth == 2 || depth == 4 || depth == 8 ||
                   depth == 16;
        case 2:  // RGB
            return depth == 8 || depth == 16;
        case 3:  // Indexed
            return depth == 1 || depth == 2 || depth == 4 || depth == 8;
        case 4:  // Grayscale + Alpha
            return depth == 8 || depth == 16;
        case 6:  // RGBA
            return depth == 8 || depth == 16;
        default:
            return false;
    }
}

class ByteReader
{
   public:
    explicit ByteReader(std::ifstream &file) : m_file(file) {}

    ByteReader(const ByteReader &) = delete;
    ByteReader &operator=(const ByteReader &) = delete;
    ByteReader(ByteReader &&) = default;
    ByteReader &operator=(ByteReader &&) = default;

    std::optional<uint8_t> u8()
    {
        uint8_t b;
        if (!m_file.read(reinterpret_cast<char *>(&b), 1) ||
            m_file.gcount() != 1)
        {
            return std::nullopt;
        }
        return b;
    }

    std::optional<uint32_t> u32_be()
    {
        std::array<uint8_t, 4> bytes;
        if (!m_file.read(reinterpret_cast<char *>(bytes.data()), 4) ||
            m_file.gcount() != 4)
        {
            return std::nullopt;
        }
        return (static_cast<uint32_t>(bytes[0]) << 24) |
               (static_cast<uint32_t>(bytes[1]) << 16) |
               (static_cast<uint32_t>(bytes[2]) << 8) |
               static_cast<uint32_t>(bytes[3]);
    }

    std::optional<std::string> str(size_t n)
    {
        std::string s(n, '\0');
        if (!m_file.read(s.data(), static_cast<std::streamsize>(n)) ||
            m_file.gcount() != static_cast<std::streamsize>(n))
        {
            return std::nullopt;
        }
        return s;
    }

    std::optional<std::vector<uint8_t>> bytes(uint32_t n)
    {
        std::vector<uint8_t> buffer(n);
        if (!m_file.read(reinterpret_cast<char *>(buffer.data()),
                         static_cast<std::streamsize>(n)) ||
            m_file.gcount() != static_cast<std::streamsize>(n))
        {
            return std::nullopt;
        }
        return buffer;
    }

   private:
    std::ifstream &m_file;
};
class PNG
{
   public:
    enum class Color : uint8_t
    {
        PNG_COLOR_GRAYSCALE,
        PNG_COLOR_RGB = 2,
        PNG_COLOR_INDEXED,
        PNG_COLOR_GRAYALPHA,
        PNG_COLOR_RGBA = 6,
    };

    PNG() {}

    explicit PNG(const std::string &path)
    {
        auto result = Open(path);
        if (PNGError::SUCCESS != result)
        {
            throw std::runtime_error("Failed to open PNG file: " + path);
        }
    }

    PNG(PNG &&) = default;
    PNG &operator=(PNG &&) = default;
    PNG(const PNG &) = delete;
    PNG &operator=(const PNG &) = delete;

    ~PNG() = default;

    PNGError Open(const std::string &path)
    {
        File = std::ifstream(path, std::ios::in | std::ios::binary);

        if (!File.is_open()) return PNGError::FILE_NOT_FOUND;

        Reader = std::make_unique<ByteReader>(File);

        auto Magic = Reader->bytes(8);  // 89 50 4e 47 0d 0a 1a 0a

        if (std::nullopt == Magic) return PNGError::CORRUPTED_HEADER;

        if (!std::equal(Magic->begin(), Magic->end(), PNG_MAGIC.begin()))
        {
            return PNGError::INVALID_MAGIC;
        }

        PNGError status = PNGError::SUCCESS;

        if ((status = readPNGHeader()) != PNGError::SUCCESS)
        {
            return status;
        }

        if ((status = readValidPNG()) != PNGError::SUCCESS)
        {
            printf("Corrupted File! \n");
            return status;
        }

        if ((status = decompressPNG()) != PNGError::SUCCESS)
        {
            printf("Cannot decompress PNG! \n");
            return status;
        }

        if ((status = decodePNG()) != PNGError::SUCCESS)
        {
            printf("Cannot decode PNG! \n");
            return status;
        }

        return PNGError::SUCCESS;
    }

    uint8_t getColorType() const noexcept { return CType; }
    uint32_t width() const noexcept { return Width; }
    uint32_t height() const noexcept { return Height; }
    uint8_t bitDepth() const noexcept { return Depth; }
    Color getColor() const noexcept { return static_cast<Color>(CType); }

    const std::vector<uint8_t> &data() const noexcept { return PNGData; }
    std::vector<uint8_t> &&moveData() noexcept { return std::move(PNGData); }

    const uint8_t *dataPointer() const noexcept { return PNGData.data(); }
    size_t dataSize() const noexcept { return PNGData.size(); }

   private:
    uint32_t Width = 0, Height = 0;
    uint8_t Depth = 0, CType = 0, CMethod = 0, FMethod = 0, Interlace = 0;

    std::unique_ptr<ByteReader> Reader;
    std::ifstream File;
    Color color;
    std::vector<uint8_t> PNGData;
    std::vector<std::tuple<uint8_t, uint8_t, uint8_t>> PLTE;
    bool hasPalette = false;

    PNGError readPNGHeader()
    {
        auto ChunkSize = Reader->u32_be();

        if (!ChunkSize || 13 != *ChunkSize) return PNGError::CORRUPTED_HEADER;

        auto Type = Reader->str(4);

        if (!Type || "IHDR" != *Type) return PNGError::CORRUPTED_HEADER;

        auto Data = Reader->bytes(*ChunkSize);

        if (!Data) return PNGError::CORRUPTED_HEADER;

        Width = ((*Data)[0] << 24) | ((*Data)[1] << 16) | ((*Data)[2] << 8) |
                ((*Data)[3]);  // big endian
        Height = ((*Data)[4] << 24) | ((*Data)[5] << 16) | ((*Data)[6] << 8) |
                 ((*Data)[7]);

        if (0 == Width || 0 == Height || Width > MAX_DIM || Height > MAX_DIM)
        {
            return PNGError::INVALID_DIMENSIONS;
        }

        Depth = (*Data)[8];
        CType = (*Data)[9];
        CMethod = (*Data)[10];
        FMethod = (*Data)[11];
        Interlace = (*Data)[12];

        if (!colorValid(Depth, CType)) return PNGError::UNSUPPORTED_FORMAT;

        if (0 != CMethod) return PNGError::UNSUPPORTED_FORMAT;

        if (0 != FMethod) return PNGError::UNSUPPORTED_FORMAT;

        auto ReadCRC = Reader->u32_be();

        if (calculateCRC(*Data, *Type) != ReadCRC)
        {
            return PNGError::CORRUPTED_HEADER;
        }

        color = static_cast<Color>(CType);

        return PNGError::SUCCESS;
    }

    PNGError readValidPNG()
    {
        PNGError status = PNGError::SUCCESS;

        PNGData.clear();
        PLTE.clear();
        hasPalette = false;

        while (true)
        {
            auto ChunkSize = Reader->u32_be();

            if (!ChunkSize)
            {
                status = PNGError::CORRUPTED_DATA;
                break;
            }

            auto Type = Reader->str(4);

            if (!Type)
            {
                status = PNGError::CORRUPTED_DATA;
                break;
            }

            if ("IEND" == *Type) break;

            auto ReadData = Reader->bytes(*ChunkSize);

            if (!ReadData)
            {
                status = PNGError::CORRUPTED_DATA;
                break;
            }

            auto ReadCRC = Reader->u32_be();

            if (!ReadCRC)
            {
                status = PNGError::CORRUPTED_DATA;
                break;
            }

            if ("IDAT" == *Type)
            {
                PNGData.insert(PNGData.end(), (*ReadData).begin(),
                               (*ReadData).end());
            }

            if ("PLTE" == *Type)
            {
                if ((*ReadData).size() % 3 != 0)
                {
                    status = PNGError::CORRUPTED_DATA;  // Corrupted palette
                    break;
                }

                for (size_t i = 0; i < (*ReadData).size(); i += 3)
                {
                    uint8_t r = (*ReadData)[i];
                    uint8_t g = (*ReadData)[i + 1];
                    uint8_t b = (*ReadData)[i + 2];
                    PLTE.emplace_back(r, g, b);
                }

                // Skip CRC
                hasPalette = true;
                continue;
            }

            if (calculateCRC(*ReadData, *Type) != ReadCRC)
            {
                return PNGError::CORRUPTED_DATA;
            }
        }

        if (CType == 3 && !hasPalette)
        {
            status = PNGError::CORRUPTED_DATA;
        }

        return status;
    }

    PNGError decompressPNG()
    {
        std::vector<uint8_t> output;

        z_stream stream = {};
        stream.next_in = const_cast<Bytef *>(PNGData.data());
        stream.avail_in = PNGData.size();

        if (inflateInit(&stream) != Z_OK) return PNGError::DECOMPRESSION_FAILED;

        uint8_t temp[4096];
        int ret;
        while (ret != Z_STREAM_END)
        {
            stream.next_out = temp;
            stream.avail_out = sizeof(temp);

            ret = inflate(&stream, Z_NO_FLUSH);
            if (ret != Z_OK && ret != Z_STREAM_END) break;

            output.insert(output.end(), temp,
                          temp + (sizeof(temp) - stream.avail_out));
        }

        if (inflateEnd(&stream) != Z_OK) return PNGError::DECOMPRESSION_FAILED;

        PNGData = std::move(output);
        return PNGError::SUCCESS;
    }

    PNGError applyPNGFilter(uint8_t FilterType, std::vector<uint8_t> &RowData,
                            size_t rowLen, const std::vector<uint8_t> &PrevRow)
    {
        if (RowData.size() != rowLen || PrevRow.size() != rowLen)
            return PNGError::DECODE_FAILED;

        switch (FilterType)
        {
            case 0:  // None
                return PNGError::SUCCESS;
            case 1:  // Sub
                for (size_t i = 1; i < rowLen; ++i)
                    RowData[i] =
                        static_cast<uint8_t>(RowData[i] + RowData[i - 1]);
                return PNGError::SUCCESS;

            case 2:  // Up
                for (size_t i = 0; i < rowLen; ++i)
                    RowData[i] = static_cast<uint8_t>(RowData[i] + PrevRow[i]);
                return PNGError::SUCCESS;

            case 3:  // Average
                for (size_t i = 0; i < rowLen; ++i)
                {
                    uint8_t left = (i > 0) ? RowData[i - 1] : 0;
                    uint8_t above = PrevRow[i];
                    RowData[i] = static_cast<uint8_t>(RowData[i] +
                                                      ((left + above) >> 1));
                }
                return PNGError::SUCCESS;

            case 4:  // Paeth
                for (size_t i = 0; i < rowLen; ++i)
                {
                    uint8_t a = (i > 0) ? RowData[i - 1] : 0;
                    uint8_t b = PrevRow[i];
                    uint8_t c = (i > 0) ? PrevRow[i - 1] : 0;
                    RowData[i] = static_cast<uint8_t>(RowData[i] +
                                                      PaethPredictor(a, b, c));
                }
                return PNGError::SUCCESS;

            default:
                return PNGError::DECODE_FAILED;
        }

        return PNGError::DECODE_FAILED;
    }

    PNGError decodePNG()
    {
        const size_t samplesPerPixel = pixelSamples();
        if (samplesPerPixel == 0)
        {
            return PNGError::UNSUPPORTED_FORMAT;
        }

        const size_t bitsPerPixel = samplesPerPixel * Depth;
        const size_t bytesPerPixel = (bitsPerPixel + 7) / 8;
        const size_t rowLen = (Width * bitsPerPixel + 7) / 8;
        const size_t stride = rowLen + 1;  // +1 for filter byte

        std::vector<uint8_t> PrevData(Width * bytesPerPixel, 0);

        std::vector<uint8_t> Output;

        if (3 == CType)
            Output.reserve(Width * Height * 3);
        else
            Output.reserve(Width * Height * bytesPerPixel);

        uint32_t Row = 0;
        for (Row; Row < Height; ++Row)
        {
            int Offset = Row * stride;

            uint8_t FilterType = PNGData[Offset];
            std::vector<uint8_t> RowData =
                std::vector<uint8_t>(PNGData.begin() + Offset + 1,
                                     PNGData.begin() + Offset + stride);

            if (PNGError::SUCCESS !=
                applyPNGFilter(FilterType, RowData, rowLen, PrevData))
                return PNGError::DECODE_FAILED;

            if (3 == CType)
            {
                for (uint8_t index : RowData)
                {
                    if (index >= PLTE.size()) return PNGError::CORRUPTED_DATA;

                    auto [r, g, b] = PLTE[index];
                    Output.push_back(r);
                    Output.push_back(g);
                    Output.push_back(b);
                }
            }
            else
                Output.insert(Output.end(), RowData.begin(), RowData.end());

            PrevData = std::move(RowData);
        }

        PNGData = std::move(Output);

        return PNGError::SUCCESS;
    }

    size_t pixelSamples() const noexcept
    {
        switch (CType)
        {
            case 0:
                return 1;  // Grayscale
            case 2:
                return 3;  // RGB
            case 3:
                return 1;  // Indexed
            case 4:
                return 2;  // Grayscale + Alpha
            case 6:
                return 4;  // RGBA
            default:
                return 0;
        }
    }
};
}  // namespace decode
}  // namespace ips

#endif  // IPS_PNG_DECODER