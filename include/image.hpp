#ifndef IPS_IMAGE_HPP
#define IPS_IMAGE_HPP

// clang-format off

#include <ctype.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <cstring>
#include <utility>
#include <variant>
#include <filesystem>

#include "buffer.hpp"
#include "decoder/png.hpp"

//clang-format on

namespace ips
{
class Image
{
public:
    enum class IMAGE_TYPE
    {
        IMAGE_U8C1,
        IMAGE_F32C1,
        IMAGE_F32C3
    };

    Image();

    Image(size_t w, IMAGE_TYPE type = IMAGE_TYPE::IMAGE_U8C1);

    Image(size_t w, size_t h, IMAGE_TYPE type = IMAGE_TYPE::IMAGE_U8C1);

    Image(size_t w, size_t h, size_t c, IMAGE_TYPE type = IMAGE_TYPE::IMAGE_U8C1);

    Image(const Image& other);

    Image(Image&& other) noexcept;

    
    Image& operator=(const Image& other);

    Image& operator=(Image&& other) noexcept;

    ~Image() = default;

    size_t width() const;
    size_t height() const;
    size_t channels() const;
    IMAGE_TYPE type() const;
    size_t size() const;
    size_t dataSize() const;
    bool empty() const;

    
    template<typename T>
    const T& at(size_t x, size_t y = 0, size_t c = 0) const;

    template<typename T>
    T& at(size_t x, size_t y = 0, size_t c = 0);

    
    const void* data() const;
    
    void* data();

    const uint8_t* dataAsUint8() const;
    
    uint8_t* dataAsUint8();

    const float* dataAsFloat() const;
    
    float* dataAsFloat();

    template<typename T>
    const T* dataAs() const;

    template<typename T>
    T* dataAs();

    void resize(size_t w, size_t h, size_t c = 0);

    template<typename T>
    void fill(const T& value);

    void zero();

    void clear();

    Image convert(IMAGE_TYPE newType) const;

    static std::optional<Image> createFromFile(const std::string& filename);

private:
    size_t Width, Height, Channels;
    IMAGE_TYPE m_type;
    
    std::variant<detail::Buffer<uint8_t>, detail::Buffer<float>> m_data;

    void allocateMemory();

    template<typename T>
    const detail::Buffer<T>& getBuffer() const;

    template<typename T>
    detail::Buffer<T>& getBuffer();

    size_t getTypeSize() const;

    size_t index(size_t x, size_t y, size_t c) const;

    void bounds(size_t x, size_t y, size_t c) const;

    template<typename T>
    void checkType() const;

    void checkChannel(size_t c, IMAGE_TYPE type) const;

    static void convertHelper(const Image& src, Image& dst);
};
}  // namespace ips

#endif