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
