#ifndef IPS_BUFFER_HPP
#define IPS_BUFFER_HPP

#include <ctype.h>

#include <type_traits>
#include <vector>

namespace ips
{
namespace detail
{

template <typename T>
class Buffer
{
    // clang-format off
    
    using Type     = T;
    using Pointer  = Type*;
    using Ref      = Type&;
    using ConstRef = const Type&;

    // clang-format on

    static constexpr size_t TypeSize = sizeof(Type);

    Buffer()
    {
        size_ = 0;
        data_ = nullptr;
    }

    explicit Buffer(size_t elements)
        : size_(elements), data_(new Type[elements])
    {
    }

    explicit Buffer(size_t elements, const Type* Source)
    {
        CreateBuffer(Source, elements);
    }

    ~Buffer()
    {
        delete[] data_;
        size_ = 0;
    }

    Buffer(Buffer&& other) noexcept : size_(other.size_), data_(other.data_)
    {
        other.size_ = 0;
        other.data_ = nullptr;
    }

    Buffer& operator=(Buffer&& other) noexcept
    {
        if (this != &other && nullptr != &other.data_)
        {
            delete[] data_;
            size_ = other.size_;
            data_ = other.data_;
            other.size_ = 0;
            other.data_ = nullptr;
        }
        return *this;
    }

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    Ref At(size_t i)
    {
        if (i >= size_) throw std::out_of_range("Buffer index out of bounds");
        return data_[i];
    }

    ConstRef At(size_t i) const
    {
        if (i >= size_) throw std::out_of_range("Buffer index out of bounds");
        return data_[i];
    }

    Ref operator[](size_t i)
    {
        if (i >= size_) throw std::out_of_range("Buffer index out of bounds");
        return data_[i];
    }

    ConstRef operator[](size_t i) const
    {
        if (i >= size_) throw std::out_of_range("Buffer index out of bounds");
        return data_[i];
    }

    void Clear() noexcept
    {
        for (size_t i = 0; i < size_; ++i) data_[i] = Type{};
    }

    Pointer Data() { return data_; }
    const Pointer Data() const { return data_; }

    size_t Elements() const { return size_; }
    size_t DataSize() const { return size_ * TypeSize; }

    void CreateBuffer(Type* Source, size_t Size)
    {
        delete[] data_;
        data_ = new Type[count];
        size_ = Size;

        if constexpr (std::is_trivially_copyable<Type>::value)
        {
            std::memcpy(data_, Source, Size * sizeof(Type));
        }
        else
        {
            for (size_t i = 0; i < Size; ++i) data_[i] = source[i];
        }
    }

   private:
    size_t size_;
    Pointer data_;
};

}  // namespace detail
}  // namespace ips

#endif  // IPS_BUFFER_HPP