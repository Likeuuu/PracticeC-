#include <iostream>
#include <functional>


void func_pointer(int i)
{
    std::cout << i << std::endl;
};

void call_func_pointer(void (*func_pointer)(int i))
{
    func_pointer(2);
};

void call_func_lambda(std::function<void(int)> f)
{
    f(3);
};

struct fang_fun
{
    void operator()(int i)
    {
        std::cout << i << std::endl;
    }
};

void call_func_fang(fang_fun cb)
{
    cb(6);
};


int main()
{
// case 1: function pointer
    call_func_pointer(func_pointer);

// case 2 : function + lambda
    call_func_lambda([=](int i){std::cout << i << std::endl;});

// case 3 :  fang funcion
    call_func_fang(fang_fun());

    return 0;
}

// 回调函数:
// 	分为: 1. 提前定义好的函数a,  2. 之后定义的函数b.     b与a的关系就是, b调用a
// 	类似于: 函数a 是一个锦囊, 告诉函数b到时候怎么做.  而轮到函数b做事时,  把当时函数b的实际参数情况, 按着锦囊的规则来做.
