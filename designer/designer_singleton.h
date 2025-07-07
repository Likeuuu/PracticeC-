#pragma once

#include <cstring>
#include <iostream>

#define DEBUG
#include <cassert>


namespace lxm
{

class SingleTon
{
public:
    static SingleTon& getInstance()
    {
        static SingleTon instance;
        return instance;
    };

private:
    SingleTon() = default;
    SingleTon(const SingleTon& other)  = delete;
    SingleTon& operator=(const SingleTon& other) = delete;
};

int singleton()
{
// singleton
    SingleTon& ton1 = SingleTon::getInstance();
    SingleTon& ton2 = SingleTon::getInstance();

    assert(&ton1 == &ton2);

    return 0;  
};

} // lxm
