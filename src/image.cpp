#include "image.hpp"

namespace ips
{

Image::Image()
    : Width(0), Height(0), Channels(0), m_type(IMAGE_TYPE::IMAGE_U8C1)
{
}

Image::Image(size_t w, IMAGE_TYPE type)
    : Width(w), Height(1), Channels(1), m_type(type)
{
    allocateMemory();
}

Image::Image(size_t w, size_t h, IMAGE_TYPE type)
    : Width(w), Height(h), Channels(1), m_type(type)
{
    allocateMemory();
}

Image::Image(size_t w, size_t h, size_t c, IMAGE_TYPE type)
    : Width(w), Height(h), Channels(c), m_type(type)
{
    checkChannel(c, type);
    allocateMemory();
}

Image::Image(const Image& other)
    : Width(other.Width),
      Height(other.Height),
      Channels(other.Channels),
      m_type(other.m_type)
{
    switch (m_type)
    {
        case IMAGE_TYPE::IMAGE_U8C1:
            m_data = other.m_data;
            break;
        case IMAGE_TYPE::IMAGE_F32C1:
        case IMAGE_TYPE::IMAGE_F32C3:
            m_data = other.m_data;
            break;
    }
}

Image::Image(Image&& other) noexcept
    : Width(other.Width),
      Height(other.Height),
      Channels(other.Channels),
      m_type(other.m_type),
      m_data(std::move(other.m_data))
{
    other.Width = other.Height = other.Channels = 0;
}

Image& Image::operator=(const Image& other)
{
    if (this != &other)
    {
        Width = other.Width;
        Height = other.Height;
        Channels = other.Channels;
        m_type = other.m_type;

        m_data = other.m_data;
    }
    return *this;
}

Image& Image::operator=(Image&& other) noexcept
{
    if (this != &other)
    {
        Width = other.Width;
        Height = other.Height;
        Channels = other.Channels;
        m_type = other.m_type;
        m_data = std::move(other.m_data);

        other.Width = other.Height = other.Channels = 0;
    }
    return *this;
}

size_t Image::width() const { return Width; }
size_t Image::height() const { return Height; }
size_t Image::channels() const { return Channels; }
Image::IMAGE_TYPE Image::type() const { return m_type; }
size_t Image::size() const { return Width * Height * Channels; }
size_t Image::dataSize() const { return size() * getTypeSize(); }
bool Image::empty() const
{
    bool isEmpty = size() == 0;

    if (std::holds_alternative<detail::Buffer<uint8_t>>(m_data))
    {
        isEmpty |= std::get<detail::Buffer<uint8_t>>(m_data).empty();
    }
    else
    {
        isEmpty |= std::get<detail::Buffer<float>>(m_data).empty();
    }

    return isEmpty;
}

template <typename T>
const T& Image::at(size_t x, size_t y, size_t c) const
{
    bounds(x, y, c);
    checkType<T>();
    return getBuffer<T>().at(index(x, y, c));
}

template <typename T>
T& Image::at(size_t x, size_t y, size_t c)
{
    bounds(x, y, c);
    checkType<T>();
    return getBuffer<T>().at(index(x, y, c));
}

const void* Image::data() const
{
    switch (m_type)
    {
        case IMAGE_TYPE::IMAGE_U8C1:
            if (auto* buf = std::get_if<ips::detail::Buffer<uint8_t>>(&m_data))
            {
                return buf->data();
            }
            break;
        case IMAGE_TYPE::IMAGE_F32C1:
        case IMAGE_TYPE::IMAGE_F32C3:
            if (auto* buf = std::get_if<ips::detail::Buffer<float>>(&m_data))
            {
                return buf->data();
            }
            break;
    }
    return nullptr;
}

void* Image::data()
{
    switch (m_type)
    {
        case IMAGE_TYPE::IMAGE_U8C1:
            if (auto* buf = std::get_if<ips::detail::Buffer<uint8_t>>(&m_data))
            {
                return buf->data();
            }
            break;
        case IMAGE_TYPE::IMAGE_F32C1:
        case IMAGE_TYPE::IMAGE_F32C3:
            if (auto* buf = std::get_if<ips::detail::Buffer<float>>(&m_data))
            {
                return buf->data();
            }
            break;
    }
    return nullptr;
}

const uint8_t* Image::dataAsUint8() const
{
    if (m_type != IMAGE_TYPE::IMAGE_U8C1) return nullptr;
    if (auto* buf = std::get_if<ips::detail::Buffer<uint8_t>>(&m_data))
    {
        return buf->data();
    }
    return nullptr;
}

uint8_t* Image::dataAsUint8()
{
    if (m_type != IMAGE_TYPE::IMAGE_U8C1) return nullptr;
    if (auto* buf = std::get_if<ips::detail::Buffer<uint8_t>>(&m_data))
    {
        return buf->data();
    }
    return nullptr;
}

const float* Image::dataAsFloat() const
{
    if (m_type != IMAGE_TYPE::IMAGE_F32C1 && m_type != IMAGE_TYPE::IMAGE_F32C3)
        return nullptr;
    if (auto* buf = std::get_if<ips::detail::Buffer<float>>(&m_data))
    {
        return buf->data();
    }
    return nullptr;
}

float* Image::dataAsFloat()
{
    if (m_type != IMAGE_TYPE::IMAGE_F32C1 && m_type != IMAGE_TYPE::IMAGE_F32C3)
        return nullptr;
    if (auto* buf = std::get_if<ips::detail::Buffer<float>>(&m_data))
    {
        return buf->data();
    }
    return nullptr;
}

template <typename T>
const T* Image::dataAs() const
{
    checkType<T>();
    const auto& buffer = getBuffer<T>();
    return buffer.data();
}

template <typename T>
T* Image::dataAs()
{
    checkType<T>();
    auto& buffer = getBuffer<T>();
    return buffer.data();
}

void Image::resize(size_t w, size_t h, size_t c)
{
    if (c == 0) c = Channels;

    checkChannel(c, m_type);

    Width = w;
    Height = h;
    Channels = c;

    allocateMemory();
}

template <typename T>
void Image::fill(const T& value)
{
    checkType<T>();
    auto& buffer = getBuffer<T>();
    buffer.fill(value);
}

void Image::zero()
{
    switch (m_type)
    {
        case IMAGE_TYPE::IMAGE_U8C1:
            getBuffer<uint8_t>().zero();
            break;
        case IMAGE_TYPE::IMAGE_F32C1:
        case IMAGE_TYPE::IMAGE_F32C3:
            getBuffer<float>().zero();
            break;
    }
}

void Image::clear()
{
    Width = Height = Channels = 0;
    m_data = detail::Buffer<uint8_t>{};
}

Image Image::convert(IMAGE_TYPE newType) const
{
    if (empty())
    {
        return Image();
    }

    size_t newChannels = Channels;
    if (newType == IMAGE_TYPE::IMAGE_F32C3)
    {
        newChannels = 3;
    }
    else if (newType == IMAGE_TYPE::IMAGE_U8C1 ||
             newType == IMAGE_TYPE::IMAGE_F32C1)
    {
        newChannels = 1;
    }

    Image result(Width, Height, newChannels, newType);

    convertHelper(*this, result);

    return result;
}

static std::optional<Image> createFromFile(const std::string& filename)
{
    std::filesystem::path filePath(filename);

    if (".png" == filePath.extension())
    {
        auto decoder = decode::PNG();
        auto result = decoder.Open(filename);

        if (result != PNGError::SUCCESS)
        {
            return std::nullopt;
        }

        size_t numChannels = 1;

        switch (decoder.getColor())
        {
            case decode::PNG::Color::PNG_COLOR_GRAYSCALE:
                numChannels = 1;
                break;
            case decode::PNG::Color::PNG_COLOR_GRAYALPHA:
                numChannels = 2;
                break;
            case decode::PNG::Color::PNG_COLOR_RGB:
                numChannels = 3;
                break;
            case decode::PNG::Color::PNG_COLOR_RGBA:
                numChannels = 4;
                break;
            case decode::PNG::Color::PNG_COLOR_INDEXED:
                numChannels = 3;
                break;
            default:
                return std::nullopt;
        }

        Image img(decoder.width(), decoder.height(), numChannels);

        auto imgBuf = img.dataAsUint8();
        const auto& pngData = decoder.data();

        if (pngData.size() != img.size())
        {
            return std::nullopt;
        }

        memcpy(imgBuf, pngData.data(), pngData.size());

        return img;
    }

    return std::nullopt;
}

void Image::allocateMemory()
{
    if (size() == 0)
    {
        m_data = detail::Buffer<uint8_t>{};
        return;
    }

    switch (m_type)
    {
        case IMAGE_TYPE::IMAGE_U8C1:
            m_data = detail::Buffer<uint8_t>(size());
            break;
        case IMAGE_TYPE::IMAGE_F32C1:
        case IMAGE_TYPE::IMAGE_F32C3:
            m_data = detail::Buffer<float>(size());
            break;
        default:
            throw std::runtime_error("Unknown image type");
    }
}

template <typename T>
const detail::Buffer<T>& Image::getBuffer() const
{
    if (auto* buf = std::get_if<detail::Buffer<T>>(&m_data))
    {
        return *buf;
    }
    throw std::runtime_error("Buffer type mismatch");
}

template <typename T>
detail::Buffer<T>& Image::getBuffer()
{
    if (auto* buf = std::get_if<detail::Buffer<T>>(&m_data))
    {
        return *buf;
    }
    throw std::runtime_error("Buffer type mismatch");
}

size_t Image::getTypeSize() const
{
    switch (m_type)
    {
        case IMAGE_TYPE::IMAGE_U8C1:
            return sizeof(uint8_t);
        case IMAGE_TYPE::IMAGE_F32C1:
        case IMAGE_TYPE::IMAGE_F32C3:
            return sizeof(float);
        default:
            throw std::runtime_error("Unknown image type");
    }
}

size_t Image::index(size_t x, size_t y, size_t c) const
{
    return (y * Width + x) * Channels + c;
}

void Image::bounds(size_t x, size_t y, size_t c) const
{
    if (empty())
    {
        throw std::runtime_error("Cannot access empty image");
    }
    if (x >= Width)
    {
        throw std::out_of_range("X coordinate out of bounds");
    }
    if (y >= Height)
    {
        throw std::out_of_range("Y coordinate out of bounds");
    }
    if (c >= Channels)
    {
        throw std::out_of_range("Channel index out of bounds");
    }
}

template <typename T>
void Image::checkType() const
{
    bool valid = false;
    switch (m_type)
    {
        case IMAGE_TYPE::IMAGE_U8C1:
            valid = std::is_same_v<T, uint8_t>;
            break;
        case IMAGE_TYPE::IMAGE_F32C1:
        case IMAGE_TYPE::IMAGE_F32C3:
            valid = std::is_same_v<T, float>;
            break;
    }

    if (!valid)
    {
        throw std::runtime_error("Type mismatch for image format");
    }
}

void Image::checkChannel(size_t c, IMAGE_TYPE type) const
{
    switch (type)
    {
        case IMAGE_TYPE::IMAGE_U8C1:
        case IMAGE_TYPE::IMAGE_F32C1:
            if (c != 1)
            {
                throw std::invalid_argument(
                    "Single channel types must have exactly 1 channel");
            }
            break;
        case IMAGE_TYPE::IMAGE_F32C3:
            if (c != 3)
            {
                throw std::invalid_argument(
                    "F32C3 type must have exactly 3 channels");
            }
            break;
    }
}

void Image::convertHelper(const Image& src, Image& dst)
{
    if (src.m_type == dst.m_type && src.Channels == dst.Channels)
    {  // same types
        dst.m_data = src.m_data;
        return;
    }

    throw std::runtime_error("Image type conversion not yet implemented");
}

}  // namespace ips
