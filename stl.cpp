#include <iostream>
#include <unordered_map>
#include <vector>

int main()
{
    // map
    std::unordered_map<int, int> unmap_a;
    std::pair temp{1,2};
    unmap_a.insert({1,1});
    //unmap_a.insert(temp);
    unmap_a.emplace(1,3);
    unmap_a.try_emplace(1, std::move(temp.second));

    std::cout << unmap_a[1] << std::endl;
    std::cout << temp.first << std::endl;


    // vector
    std::vector<int>  vec_a{};
    int a = 2;
    vec_a.emplace_back(std::move(a));
    vec_a.emplace_back(1);

    return 0;
}


// stl的常用方法的坑(自己的)
// 	map结构的 map_a
// 		1. 直接使用下标 map_a[i] 可能有问题, 可能创建一个默认值,  因此需要find / count 判断是否存在
// 		2. insert, emplace,  try_emplace 中, emplace基本完全代替insert
// 			2.1 用 try_emplace 就好了 
// 	vector
// 		 push_back()  emplace_back()  
// 			就用 emplace_back()  , 效果一样的, 也能传左右值
// 			而且针对传入参数, 有原地构造的优势
