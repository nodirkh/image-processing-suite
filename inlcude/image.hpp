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

    const T& At(size_t, size_t, size_t) const;
    T At(size_t, size_t, size_t);
    T& At(size_t, size_t, size_t);

    size_t Width() const;
    size_t Height() const;
    size_t Channel() const;

    void CreateImage(T*);

    size_t Size() const;

   private:
    size_t w, h, c;

    std::shared_ptr<detail::Buffer<T>> m_data;
};
}  // namespace ips

#endif