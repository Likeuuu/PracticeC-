#include <iostream>
#include <functional>


class A
{
public:
    void say(int a, int b)
    {
        std::cout << a << std::endl;
        std::cout << b << std::endl;
    }
};

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

    A a_1;
    std::function<void(int, int)> f_d = std::bind(&A::say, &a_1, std::placeholders::_1,  std::placeholders::_2);
    std::function<void(int)>      f_e = std::bind(&A::say, &a_1, 5, std::placeholders::_1);
    std::function<void(int)>      f_f = std::bind(&A::say, &a_1, std::placeholders::_1, 2);


    f_pre = std::bind(f_c, 0);
    f_pre();

    f_d(1, 1);
    f_e(1);
    f_f(1);

    return 0;
}


// function  + bind + lambda
// 	1. lambda : 临时函数, 用于
// 		1.1 stl 的排序那些
// 		1.2 回调
// 			注意: 捕获分为值与引用, 且模式const,  想要修改捕获,需要额外加mutable
// 	2. bind : 注意其参数, 普通用法(函数, 参数), 高级用法(函数地址, 变量地址, 定向占位)
// 	3. function : 一种接口. 用处 :
// 		3.1 类的变量(包括存入vec中)
// 		3.2 函数的入参 (同一接口)
