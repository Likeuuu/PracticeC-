#include <string>
#include <vector>
int main()
{
    // 初始化方式
	// 1. 拷贝
	// 2. 直接
	// 3. 列表拷贝
	// 4. 列表直接
	// 5. 默认			危险, 最好还是列表默认
	// 6. 列表默认   
	
	int a_1 = 10;       std::string s_1 = "hallo";
	int a_2(10);        std::string s_2("hallo");
	int a_3 = {10};     std::string s_3 = {"hallo"};
	int a_4{10};        std::string s_4{"hallo"};
	int a_5; int a_6{}; std::string s_5; std::string s_6{};

	//容器初始化方式--- 都用列表进行拷贝和直接
		std::vector<int>  vec_a = {1, 2, 3};
		std::vector<int>  vec_b{1, 2, 3};

    return 0;
}
