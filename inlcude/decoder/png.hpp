#ifndef IPS_PNG_DECODER
#define IPS_PNG_DECODER

#include <zlib.h>

#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

namespace ips
{
namespace decode
{

static const std::vector<uint8_t> PNG_MAGIC = {0x89, 0x50, 0x4E, 0x47,
                                               0x0D, 0x0A, 0x1A, 0x0A};

static inline uint32_t calculateCRC(const std::vector<uint8_t> &Data,
                                    const std::string &Type)
{
    const size_t CSize = Data.size();

    std::vector<uint8_t> DataCRC(4 + CSize);
    std::memcpy(DataCRC.data(), Type.data(), 4);
    std::memcpy(DataCRC.data(), Data.data(), CSize);

    return static_cast<uint32_t>(crc32(0, DataCRC.data(), DataCRC.size()));
}

static inline decltype(auto) PaethPredictor(int a, int b, int c)
{
    int p = a + b - c;
    int pa = std::abs(p - a);
    int pb = std::abs(p - b);
    int pc = std::abs(p - c);

    if (pa <= pb && pa <= pc) return static_cast<uint8_t>(a);
    if (pb <= pc) return static_cast<uint8_t>(b);
    return static_cast<uint8_t>(c);
};

class ByteReader
{
   public:
    ByteReader() = delete;

    ByteReader(std::ifstream &f) : file(f) {}

    uint8_t u8()
    {
        uint8_t b;
        file.read(reinterpret_cast<char *>(&b), 1);
        return b;
    }

    uint32_t u32_be()
    {
        uint8_t bytes[4];
        file.read(reinterpret_cast<char *>(bytes), 4);
        return (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) |
               (bytes[3]);
    }

    std::string str(size_t n)
    {
        std::string s(n, '\0');
        file.read(&s[0], n);
        return s;
    }

    std::vector<uint8_t> bytes(size_t n)
    {
        std::vector<uint8_t> b(n);
        file.read(reinterpret_cast<char *>(b.data()), n);
        return b;
    }

   private:
    std::ifstream &file;
};
class PNG
{
   public:
    enum class Color
    {
        PNG_COLOR_GRAYSCALE,
        PNG_COLOR_RGB = 2,
        PNG_COLOR_INDEXED,
        PNG_COLOR_GRAYALPHA,
        PNG_COLOR_RGBA = 6,
    };

    PNG() {}

    PNG(const std::string &path)
    {
        File = std::ifstream(path, std::ios::in | std::ios::binary);
        if (nullptr != Reader) delete Reader;
        Reader = new ByteReader(File);
    }

    ~PNG()
    {
        if (Reader)
            delete Reader;
        Reader = nullptr;

        if (File.is_open())
            File.close();
    }

    bool Open(const std::string &path)
    {
        File = std::ifstream(path, std::ios::in | std::ios::binary);

        if (nullptr != Reader) delete Reader;
        Reader = new ByteReader(File);

        auto Magic = Reader->bytes(8);  // 89 50 4e 47 0d 0a 1a 0a

        if (Magic != PNG_MAGIC)
        {
            printf("Invalid PNG File! \n");
            return false;
        }

        if (!ReadPNGHeader())
        {
            printf("Corrupted File! \n");
            return false;
        }

        if (!ReadValidPNG())
        {
            printf("Corrupted File! \n");
            return false;
        }

        if (!DecompressPNG())
        {
            printf("Cannot decompress PNG! \n");
            return false;
        }

        if (!DecodePNG())
        {
            printf("Cannot decode PNG! \n");
            return false;
        }

        return true;
    }

   private:
    uint32_t Width, Height;
    uint8_t Depth, CType, CMethod, FMethod, Interlace;
    ByteReader *Reader;
    std::ifstream File;
    Color color;
    std::vector<uint8_t> PNGData;

    bool ReadPNGHeader()
    {
        auto ChunkSize = Reader->u32_be();

        if (13 != ChunkSize) return false;

        auto Type = Reader->str(4);

        if ("IHDR" != Type) return false;

        auto Data = Reader->bytes(ChunkSize);

        Width = (Data[0] << 24) | (Data[1] << 16) | (Data[2] << 8) |
                (Data[3]);  // big endian
        Height = (Data[4] << 24) | (Data[5] << 16) | (Data[6] << 8) | (Data[7]);

        if (0 == Width || 0 == Height) return false;

        Depth = Data[8];
        CType = Data[9];
        CMethod = Data[10];
        FMethod = Data[11];
        Interlace = Data[12];

        if (0 != CMethod) return false;

        if (0 != FMethod) return false;

        auto ReadCRC = Reader->u32_be();

        if (calculateCRC(Data, Type) != ReadCRC)
        {
            return false;
        }

        color = static_cast<Color>(CType);

#if defined(IPS_DEBUG)
        printf("Width %d, Height %d \n", Width, Height);
#endif

        return true;
    }

    bool ReadValidPNG()
    {
        while (true)
        {
            auto ChunkSize = Reader->u32_be();
            auto Type = Reader->str(4);

            if ("IEND" == Type) break;

            auto ReadData = Reader->bytes(ChunkSize);
            auto ReadCRC = Reader->u32_be();

            if ("IDAT" == Type)
            {
                PNGData.insert(PNGData.end(), ReadData.begin(), ReadData.end());
            }

            if (calculateCRC(ReadData, Type) != ReadCRC)
            {
                return false;
            }
        }

        return true;
    }

    bool DecompressPNG()
    {
        std::vector<uint8_t> output;

        z_stream stream = {};
        stream.next_in = const_cast<Bytef *>(PNGData.data());
        stream.avail_in = PNGData.size();

        if (inflateInit(&stream) != Z_OK) return false;

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

        if (inflateEnd(&stream) != Z_OK) return false;

        PNGData = std::move(output);
        return true;
    }

    bool ApplyPNGFilter(uint8_t FilterType, std::vector<uint8_t> &RowData,
                        size_t RowLen, const std::vector<uint8_t> &PrevRow)
    {
        if (RowData.size() != RowLen || PrevRow.size() != RowLen) return false;

        switch (FilterType)
        {
            case 0:  // None
                return true;
            case 1:  // Sub
                for (size_t i = 1; i < RowLen; ++i)
                    RowData[i] =
                        static_cast<uint8_t>(RowData[i] + RowData[i - 1]);
                return true;

            case 2:  // Up
                for (size_t i = 0; i < RowLen; ++i)
                    RowData[i] = static_cast<uint8_t>(RowData[i] + PrevRow[i]);
                return true;

            case 3:  // Average
                for (size_t i = 0; i < RowLen; ++i)
                {
                    uint8_t left = (i > 0) ? RowData[i - 1] : 0;
                    uint8_t above = PrevRow[i];
                    RowData[i] = static_cast<uint8_t>(RowData[i] +
                                                      ((left + above) >> 1));
                }
                return true;

            case 4:  // Paeth
                for (size_t i = 0; i < RowLen; ++i)
                {
                    uint8_t a = (i > 0) ? RowData[i - 1] : 0;
                    uint8_t b = PrevRow[i];
                    uint8_t c = (i > 0) ? PrevRow[i - 1] : 0;
                    RowData[i] = static_cast<uint8_t>(RowData[i] +
                                                      PaethPredictor(a, b, c));
                }
                return true;

            default:
                return false;
        }
    }

    bool DecodePNG()
    {
        int64_t SamplesPerPixel = -1;
        switch (CType)
        {
            case 0:
                SamplesPerPixel = 1;
                break;
            case 2:
                SamplesPerPixel = 3;
                break;
            case 3:
                SamplesPerPixel = 1;
                break;
            case 4:
                SamplesPerPixel = 2;
                break;
            case 6:
                SamplesPerPixel = 4;
                break;
            default:
                break;
        }

        if (-1 == SamplesPerPixel) return false;

        int64_t BitsPerPixel = SamplesPerPixel * Depth;
        int64_t BytesPerPixel = (BitsPerPixel + 7) / 8;
        int64_t RowLen = Width * BytesPerPixel;
        int64_t Stride = RowLen + 1;

        std::vector<uint8_t> PrevData(Width * BytesPerPixel, 0);

        std::vector<uint8_t> Output;
        Output.reserve(Width * Height * BytesPerPixel);

        int Row = 0;
        for (Row; Row < Height; ++Row)
        {
            int Offset = Row * Stride;

            uint8_t FilterType = PNGData[Offset];
            std::vector<uint8_t> RowData =
                std::vector<uint8_t>(PNGData.begin() + Offset + 1,
                                     PNGData.begin() + Offset + Stride);

            if (!ApplyPNGFilter(FilterType, RowData, RowLen, PrevData))
                return false;

            Output.insert(Output.end(), RowData.begin(), RowData.end());

            PrevData = std::move(RowData);
        }

        PNGData = std::move(Output);

        return true;
    }
};
}  // namespace decode
}  // namespace ips

#endif  // IPS_PNG_DECODER