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

static inline uint32_t calculateCRC(const std::vector<uint8_t>& Data, const std::string& Type)
{
    const size_t CSize = Data.size();
    
    std::vector<uint8_t> DataCRC(4 + CSize);
    std::memcpy(DataCRC.data(), Type.data(), 4);
    std::memcpy(DataCRC.data(), Data.data(), CSize);

    return static_cast<uint32_t>(crc32(0, DataCRC.data(), DataCRC.size()));
}

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
        PNG_COLOR_RGB,
        PNG_COLOR_INDEXED,
        PNG_COLOR_GRAYALPHA,
        PNG_COLOR_RGBA,
    };

    PNG() {}

    PNG(const std::string &path)
    {
        File = std::ifstream(path, std::ios::in | std::ios::binary);
        if (nullptr != Reader) delete Reader;
        Reader = new ByteReader(File);
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
            
            if ("IDAT" == Type)
            {
                PNGData.insert(PNGData.end(), ReadData.begin(), ReadData.end());
            }
        }
    }
};
}  // namespace decode
}  // namespace ips

#endif  // IPS_PNG_DECODER