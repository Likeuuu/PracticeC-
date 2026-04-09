#include <string>
#include <vector>
#include <iostream>


struct A {
	int x;
	int y;
};

struct B {
    int x;
    int y;
};

int main()
{
    // 初始化方式
	// 1. 拷贝
	// 2. 直接
	// 3. 列表拷贝
	// 4. 列表直接
	// 5. 默认			危险, 最好还是列表默认
	// 6. 列表默认   
	
	// int a_1 = 10;       std::string s_1 = "hallo";
	// int a_2(10);        std::string s_2("hallo");
	// int a_3 = {10};     std::string s_3 = {"hallo"};
	// int a_4{10};        std::string s_4{"hallo"};
	// int a_5; int a_6{}; std::string s_5; std::string s_6{};

	// //容器初始化方式--- 都用列表进行拷贝和直接
	// 	std::vector<int>  vec_a = {1, 2, 3};
	// 	std::vector<int>  vec_b{1, 2, 3};

	// int value = 42;

	// // 1. 安全地将具体指针转为 void*（用于通用接口，如pthread参数）
	// void* void_ptr = static_cast<void*>(&value); // 正确做法

	// // 2. 安全地将 void* 转回原始类型
	// int* int_ptr = static_cast<int*>(void_ptr); // 正确做法
	// std::cout << *int_ptr << std::endl; // 安全地输出 42

	// void* p = malloc(sizeof(int));
	// bool* ip = static_cast<bool*>(p);  // ❌ 不允许
	// *ip = true;


	A a{1, 2};
	B* b = reinterpret_cast<B*>(&a);
	int v = b->x;
	
	printf("v is %d\n", v);

    return 0;
}
