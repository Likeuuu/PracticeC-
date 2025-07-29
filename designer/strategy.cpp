#include <iostream>
#include <memory>
#include <utility>

class Strategy
{
public:
    virtual void say() = 0;
    virtual ~Strategy() = default;
};

class AStrategy : public Strategy
{
    void say() override
    {
        std::cout << "This is A" << std::endl;
    };
};

class BStrategy : public Strategy
{
    void say() override
    {
        std::cout << "This is B" << std::endl;
    };
};

class Context
{
private:
    std::shared_ptr<Strategy> sp_strategy_;
public:
    void doSomething()
    {
        if (sp_strategy_)
        sp_strategy_->say();
    };

    void setStrategy(std::shared_ptr<Strategy> &sp_strategy)
    {
        sp_strategy_ = sp_strategy;
    };
};

int main()
{
    // 1. common use
    std::shared_ptr<Strategy> up_stragegy_a = std::make_shared<AStrategy>();
    std::shared_ptr<Strategy> up_stragegy_b = std::make_shared<BStrategy>();
    std::unique_ptr<Context>  up_context    = std::make_unique<Context>();

    up_context->setStrategy(up_stragegy_a);
    up_context->doSomething();

    // 2. init way
    std::shared_ptr<Strategy> up_stragegy_b2 = std::make_shared<BStrategy>();
    std::shared_ptr<Strategy> up_stragegy_b3();

    auto deleter = [](Strategy* p){std::cout << "delete" << std::endl;};
    std::shared_ptr<Strategy> up_stragegy_b4(new BStrategy(), deleter);
    std::shared_ptr<Strategy> up_stragegy_b5(nullptr, deleter);
    up_stragegy_b5.reset(new AStrategy());


    return 0;
}
