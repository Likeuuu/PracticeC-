#### 1. rule of zero/three/five
#### 2. noexcept 
        (because of && and stl(vector))
#### 3. string a -- printf("a %s", a.c_str()) 
#### 4. SingleTon ton2 = SingleTon::getInstance(); || SingleTon& ton2 = SingleTon::getInstance();
    
#### 5. #define NDEBUG  disable assert



<!-- 单例模式
	保证全局唯一, 靠
		禁止拷贝
		私有化构造
		构造变量为static(c++ 11及之后, 确保了多线程调用getInstance, 变量也只会构造一次)
		单例的getInstance函数可以为static,以供直接调用

工厂模式 (解耦靠多态)
	1. 简单工厂 :  产品解耦, 但工厂未解耦, 导致扩展时, 工厂变动麻烦--一个车间生产多种方向盘
	2. 工厂方法 : 工厂也跟着解耦(顶层工厂规定标准) --- 保时捷车间生产关于保时捷的轮胎, 劳斯莱斯车间生产劳斯莱斯的轮胎, 
	3. 抽象工厂 : 工厂方法升级版---保时捷车间生产关于保时捷的轮胎, 车架, 方向盘.  劳斯莱斯车间生产劳斯莱斯的轮胎, 车架, 方向盘
	注: 抽象就是顶层规定一些列标准(纯虚函数),  然后派生类进行自定义操作(复写).

模板方法:
	顶层中, 规定流程, 流程部分规定标准(纯虚函数), 派生类在这个流程框架下, 进行部分自定义操作.

策略模式:
	类strategy中(最好作为顶层) , 存放许多抽象的方法 
	类context, 存放顶层strategy的变量 , 具体使用时初始化该strategy变量,然后调用.

观察者模式:
	分为observer 与 subject---例子:楚门的世界
	1. observer 里, 定义如何去观察的标准(纯虚函数)
	2. subject 里, 定义
		关于observer (保存observer的vector, 注册,  删除, 通知)
		关于subject(自己做的事情|变量, 并通知) -->
