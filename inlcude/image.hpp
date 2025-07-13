#ifndef IPS_IMAGE_HPP
#define IPS_IMAGE_HPP

// clang-format off

#include <ctype.h>
#include <memory>
#include "buffer.hpp"

//clang-format on

namespace ips
{

template <typename T>
class Image
{
   public:
    Image() = default;

    Image(size_t);
    Image(size_t, size_t);
    Image(size_t, size_t, size_t);

    Image(const Image&);
    Image& operator=(const Image&);

    T& at(size_t, size_t, size_t) const;
    T at(size_t, size_t, size_t) const;

    size_t width() const;
    size_t height() const;
    size_t channel() const;

    void createImage(T*);

    size_t size() const;

   private:
    size_t w, h, c;

    std::shared_ptr<detail::Buffer<T>> m_data;
};
}  // namespace ips

#endif