#pragma once

#include <cstdint>
#include <cstddef>

class Arena
{
public:
    explicit Arena(size_t  capacity, size_t aligments);
    ~Arena();

    Arena(const Arena&)             = delete;
    Arena& operator=(const Arena&)  = delete;

    Arena(Arena &&)                 = delete;
    Arena operator=(Arena &&)       = delete;

    void* allocate(size_t n, size_t alignment);

    void reset();

    // const 为什么放后面,  什么时候放前面
    size_t capacity() const;

    // static 函数声明为什么放文件;  &~(alignment - 1) 的妙用
    // 目的是根据实际的字节数和对齐值， 算出应该有的字节数
    static size_t align_up(size_t n, size_t alignment) {return (n + alignment - 1) & ~(alignment - 1);};

private:
    uint8_t* base_      = nullptr;
    size_t   capacity_  = 0;
    size_t   offset_    = 0 ;
};
