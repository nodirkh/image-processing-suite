#ifndef IPS_BUFFER_HPP
#define IPS_BUFFER_HPP

#include <algorithm>
#include <cstring>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <type_traits>

namespace ips
{
namespace detail
{
template <typename T>
class Buffer
{
   public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    using iterator = pointer;
    using const_iterator = const_pointer;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    Buffer() noexcept : size_(0), data_(nullptr) {}

    explicit Buffer(size_type count, const T& value = T{})
        : size_(count),
          data_(count > 0 ? std::make_unique<T[]>(count) : nullptr)
    {
        if (data_)
        {
            std::fill_n(data_.get(), size_, value);
        }
    }

    Buffer(const T* source, size_type count)
        : size_(count),
          data_(count > 0 ? std::make_unique<T[]>(count) : nullptr)
    {
        if (data_ && source)
        {
            copyData(source, count);
        }
    }

    Buffer(std::initializer_list<T> init)
        : size_(init.size()),
          data_(init.size() > 0 ? std::make_unique<T[]>(init.size()) : nullptr)
    {
        if (data_)
        {
            std::copy(init.begin(), init.end(), data_.get());
        }
    }

    Buffer(const Buffer& other)
        : size_(other.size_),
          data_(other.size_ > 0 ? std::make_unique<T[]>(other.size_) : nullptr)
    {
        if (data_ && other.data_)
        {
            copyData(other.data_.get(), size_);
        }
    }

    Buffer(Buffer&& other) noexcept
        : size_(other.size_), data_(std::move(other.data_))
    {
        other.size_ = 0;
    }

    Buffer& operator=(const Buffer& other)
    {
        if (this != &other)
        {
            if (other.size_ == 0)
            {
                clear();
            }
            else
            {
                auto new_data = std::make_unique<T[]>(other.size_);
                copyData(other.data_.get(), other.size_, new_data.get());

                size_ = other.size_;
                data_ = std::move(new_data);
            }
        }
        return *this;
    }

    Buffer& operator=(Buffer&& other) noexcept
    {
        if (this != &other)
        {
            size_ = other.size_;
            data_ = std::move(other.data_);
            other.size_ = 0;
        }
        return *this;
    }

    Buffer& operator=(std::initializer_list<T> init)
    {
        *this = Buffer(init);
        return *this;
    }

    ~Buffer() = default;

    reference at(size_type pos)
    {
        if (pos >= size_)
        {
            throw std::out_of_range("Buffer::at: index out of bounds");
        }
        return data_[pos];
    }

    const_reference at(size_type pos) const
    {
        if (pos >= size_)
        {
            throw std::out_of_range("Buffer::at: index out of bounds");
        }
        return data_[pos];
    }

    reference operator[](size_type pos) noexcept { return data_[pos]; }

    const_reference operator[](size_type pos) const noexcept
    {
        return data_[pos];
    }

    reference front()
    {
        if (empty()) throw std::runtime_error("Buffer::front: buffer is empty");
        return data_[0];
    }

    const_reference front() const
    {
        if (empty()) throw std::runtime_error("Buffer::front: buffer is empty");
        return data_[0];
    }

    reference back()
    {
        if (empty()) throw std::runtime_error("Buffer::back: buffer is empty");
        return data_[size_ - 1];
    }

    const_reference back() const
    {
        if (empty()) throw std::runtime_error("Buffer::back: buffer is empty");
        return data_[size_ - 1];
    }

    pointer data() noexcept { return data_.get(); }
    const_pointer data() const noexcept { return data_.get(); }

    iterator begin() noexcept { return data_.get(); }
    const_iterator begin() const noexcept { return data_.get(); }
    const_iterator cbegin() const noexcept { return data_.get(); }

    iterator end() noexcept { return data_.get() + size_; }
    const_iterator end() const noexcept { return data_.get() + size_; }
    const_iterator cend() const noexcept { return data_.get() + size_; }

    reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
    const_reverse_iterator rbegin() const noexcept
    {
        return const_reverse_iterator(end());
    }
    const_reverse_iterator crbegin() const noexcept
    {
        return const_reverse_iterator(end());
    }

    reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
    const_reverse_iterator rend() const noexcept
    {
        return const_reverse_iterator(begin());
    }
    const_reverse_iterator crend() const noexcept
    {
        return const_reverse_iterator(begin());
    }

    bool empty() const noexcept { return size_ == 0; }
    size_type size() const noexcept { return size_; }
    size_type max_size() const noexcept
    {
        return std::numeric_limits<size_type>::max() / sizeof(T);
    }
    size_type byte_size() const noexcept { return size_ * type_size; }

    void clear() noexcept
    {
        size_ = 0;
        data_.reset();
    }

    void fill(const T& value)
    {
        if (data_)
        {
            std::fill_n(data_.get(), size_, value);
        }
    }

    void zero() noexcept
    {
        if (data_)
        {
            if constexpr (std::is_trivially_copyable_v<T>)
            {
                std::memset(data_.get(), 0, byte_size());
            }
            else
            {
                std::fill_n(data_.get(), size_, T{});
            }
        }
    }

    void resize(size_type new_size, const T& value = T{})
    {
        if (new_size == 0)
        {
            clear();
            return;
        }

        if (new_size == size_)
        {
            return;
        }

        auto new_data = std::make_unique<T[]>(new_size);

        if (data_)
        {
            size_type copy_count = std::min(size_, new_size);
            copyData(data_.get(), copy_count, new_data.get());

            if (new_size > size_)
            {
                std::fill_n(new_data.get() + size_, new_size - size_, value);
            }
        }
        else
        {
            std::fill_n(new_data.get(), new_size, value);
        }

        size_ = new_size;
        data_ = std::move(new_data);
    }

    void assign(size_type count, const T& value)
    {
        *this = Buffer(count, value);
    }

    void assign(const T* source, size_type count)
    {
        *this = Buffer(source, count);
    }

    void assign(std::initializer_list<T> init) { *this = Buffer(init); }

    void swap(Buffer& other) noexcept
    {
        std::swap(size_, other.size_);
        std::swap(data_, other.data_);
    }

    bool operator==(const Buffer& other) const
    {
        if (size_ != other.size_) return false;
        if (data_ == other.data_) return true;
        if (!data_ || !other.data_) return false;

        return std::equal(begin(), end(), other.begin());
    }

    bool operator!=(const Buffer& other) const { return !(*this == other); }

   private:
    size_type size_;
    std::unique_ptr<T[]> data_;

    void copyData(const T* source, size_type count, T* dest = nullptr)
    {
        if (!dest) dest = data_.get();

        if constexpr (std::is_trivially_copyable_v<T>)
        {
            std::memcpy(dest, source, count * sizeof(T));
        }
        else
        {
            std::copy_n(source, count, dest);
        }
    }

    static constexpr size_type type_size = sizeof(T);
};

template <typename T>
void swap(Buffer<T>& lhs, Buffer<T>& rhs) noexcept
{
    lhs.swap(rhs);
}

}  // namespace detail
}  // namespace ips

#endif  // IPS_BUFFER_HPP