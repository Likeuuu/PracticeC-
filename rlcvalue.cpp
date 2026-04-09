#include <iostream>
#include <utility>
#include <cstdint>

void foo(int&& x)
{ 
    // 在函数内, x 为左值. 不在乎外面传入是否为右值
    printf(" x : %d\n", x);
    x+=10;
}

int main()
{
    // // r 是左值(有名字), 但它是右值引用
    // int&& r = 20;

    // foo(10);
    // //foo(r); // error : need lvalue
    // foo(std::move(r));

    // printf(" x: %d\n", r);

    // // int* p = NULL;   // 本质是 p = 0;


    // // printf("  p : %d, \n", p);
    // // printf("  p : %p, \n", p);
    // // printf(" *p : %d \n", *p);

    // int a = -1;
    // unsigned int u = static_cast<unsigned int>(a);


    // printf("\au : %u \n", u);

    // int  a = 4;
    // int* b = &a;
    // int* c = NULL;
    // printf(" b %p %p, %d, %d",&b, b, *b, c);

    double dx = 1.0 / 9;

    printf(" b %.5f",dx);

    return 0;
}
