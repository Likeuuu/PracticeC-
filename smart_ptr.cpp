#include <memory>
#include <iostream>

class B;  // 前向声明

class A {
public:
    std::shared_ptr<B> b_ptr;
    ~A() { std::cout << "A destroyed\n"; }
};

class B {
public:
    std::shared_ptr<A> a_ptr;
    ~B() { std::cout << "B destroyed\n"; }
};

class C 
{
};

class D 
{
};

// 智能指针:
// 	目的: 自动管理内存, 防止内存泄露!!
// 	1. shared_ptr -- 多个文件访问同一内存, 最后一个使用完毕后, 系统自动释放
// 	2. unique_ptr-- 一个文件只能一个指针访问,  内存转移需要写 move()
// 	3. weak_ptr -- 针对 相互 shard , 导致内存不能释放.
// 			3.1 分主从, 主用shared , 从用weak
// 			3.2 分高低, 高用shared, 低用weak
// 			3.3 待补充

int main() {
    // 	1. 初始化方法:
    std::shared_ptr<C> sp_a = std::make_shared<C>();
    std::unique_ptr<D> sp_b = std::make_unique<D>();

// 	带删除器
// 	删除器shared与unique差异:
//  用shared, 可以用同一种删除器管理不同的内存释放(这些内存的删除行为是一致的)
    auto deleter_shared = [](C*){};
    auto deleter_unique = [](D*){};
    std::shared_ptr<C> p_c(new C(), deleter_shared);
    std::shared_ptr<C> p_d(nullptr, deleter_shared);  // 延迟传入变量内存地址
    p_d.reset(new C());
    

    std::unique_ptr<D, decltype(deleter_unique)>p_e(new D(), deleter_unique);

// 		删除器使用场景:
// 			1. 带malloc, delete这种c风格的
// 			2. 访问系统api (file这种)
    return 0;
}




// 智能指针:
// 	目的: 自动管理内存, 防止内存泄露!!
// 	1. shared_ptr -- 多个文件访问同一内存, 最后一个使用完毕后, 系统自动释放
// 	2. unique_ptr-- 一个文件只能一个指针访问,  内存转移需要写 move()
// 	3. weak_ptr -- 针对 相互 shard , 导致内存不能释放.
// 			3.1 分主从, 主用shared , 从用weak
// 			3.2 分高低, 高用shared, 低用weak
// 			3.3 待补充

// 	初始化方法:
// 		std::shared_ptr<A> sp_a = std::make_shared<C>();
//      std::unique_ptr<B> sp_b = std::make_unique<C>();
		
// 		带删除器
// 		删除器shared与unique差异:
// 		用shared, 可以用同一种删除器管理不同的内存释放(这些内存的删除行为是一致的)
// 			auto deleter_shared = [](C*){};
// 			auto deleter_unique = [](D*){};
// 			std::shared_ptr<C> p_c(new C(), deleter_shared);
// 			std::shared_ptr<C> p_d(nullptr, deleter_shared);  // 延迟传入变量内存地址
// 			p_d.reset(new C());

// 			std::unique_ptr<D, decltype(deleter_unique)>p_e(new D(), deleter_unique);


// 		删除器使用场景:
// 			1. 带malloc, delete这种c风格的
// 			2. 访问系统api (file这种)
