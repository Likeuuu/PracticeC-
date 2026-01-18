

#include <cstdlib>
#include <new>
#include <iostream>
#include "Arena.h"


Arena::Arena(size_t  capacity, size_t aligments)
{
    capacity_ = align_up(capacity, aligments);
    offset_   = 0;

    base_     = static_cast<uint8_t*>(std::aligned_alloc(aligments, capacity_));

    if (!base_) throw std::bad_alloc();
}

Arena::~Arena()
{
    std::free(base_);
}

void* Arena::allocate(size_t n, size_t alignment)
{
    size_t current_base = reinterpret_cast<size_t>(base_) + offset_;
    size_t new_base     = align_up(current_base, alignment);
    size_t new_offset   = new_base - reinterpret_cast<size_t>(current_base) + n;

    if (new_offset > capacity_) throw std::bad_alloc();

    offset_ = new_offset;
    return reinterpret_cast<void*>(new_base);
}

void Arena::reset()
{
    offset_ = 0;
    return;
}

size_t Arena::capacity() const
{
    return capacity_;
}
