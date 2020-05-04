#ifndef AVCODEC_LPVCITER_H
#define AVCODEC_LPVCITER_H

#include <lpvc/lpvc.h>
#include <cstddef>
#include <iterator>


class LpvcIterator final
{
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = lpvc::Color;
    using difference_type = std::ptrdiff_t;
    using pointer = lpvc::Color*;
    using reference = lpvc::Color&;

    LpvcIterator(long bitmapWidth, long lineSize, std::byte* bitmap) noexcept :
        bitmapWidth_(bitmapWidth),
        linePadding_(lineSize - bitmapWidth * sizeof(lpvc::Color)),
        bitmap_(bitmap)
    {
    }

    LpvcIterator& operator++() noexcept
    {
        bitmap_ += sizeof(lpvc::Color);

        if(++positionX_ == bitmapWidth_)
        {
            positionX_ = 0;
            bitmap_ += linePadding_;
        }

        return *this;
    }

    reference operator*() const noexcept
    {
        return *reinterpret_cast<lpvc::Color*>(bitmap_);
    }

private:
    long bitmapWidth_ = 0;
    long linePadding_ = 0;
    std::byte* bitmap_ = nullptr;
    long positionX_ = 0;
};


#endif // AVCODEC_LPVCITER_H
