#include <iostream>
#include <memory>

class MakeDrink
{
public:
    void make()
    {
        boilWater();
        brew();
        pourIncup();
        addCondiments();
    }

    void brew()
    {
        std::cout << "common brew()" << std::endl;
    }
    void pourIncup()
    {
        std::cout<< "common pourIncup()" << std::endl;
    }

    virtual void boilWater()     = 0;
    virtual void addCondiments() = 0;
};

class LemonDrink : public MakeDrink
{
public:
    void boilWater()      override
    {
        std::cout<< "lemon boilWater()" << std::endl;
    };

    void addCondiments()  override
    {
        std::cout<< "lemon addCondiments()" << std::endl;
    };
};


class AppleDrink : public MakeDrink
{
public:
    void boilWater()      override
    {
        std::cout<< "apple boilWater()" << std::endl;
    };

    void addCondiments()  override
    {
        std::cout<< "apple addCondiments()" << std::endl;
    };
};

int main()
{
    std::shared_ptr<MakeDrink>   sp_lemon = std::make_shared<LemonDrink>();
    std::shared_ptr<AppleDrink>  sp_apple = std::make_shared<AppleDrink>();

    sp_lemon->make();
    sp_apple->make();   

    return 0;
}
