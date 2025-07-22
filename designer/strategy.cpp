#include <iostream>
#include <memory>

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
    std::shared_ptr<Strategy> up_stragegy_a = std::make_shared<AStrategy>();
    std::shared_ptr<Strategy> up_stragegy_b = std::make_shared<BStrategy>();
    std::unique_ptr<Context>  up_context    = std::make_unique<Context>();

    up_context->setStrategy(up_stragegy_a);
    up_context->doSomething();


    return 0;
}
