#include <iostream>
#include <functional>

int main()
{

// case 1: common lambda
    int a = 10;
    int b = 100;
    auto f = [&](int b) mutable
    {   
        a++;
        std::cout << a << std::endl;
        std::cout << b << std::endl;
    };

    f(b);

// case 2: function + lambda + bind
    std::function<void()> f_pre;

    int  c   = 1000;
    auto f_c = [&](int)
    {
        std::cout << c << std::endl;
    };

    f_pre = std::bind(f_c, 0);

    f_pre();




    return 0;
}


// mutable
// = &
//  want to pre
    // std::function<void()> f;  std::bind(func, param); ----> note : f is void()!!
