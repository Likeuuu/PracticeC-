#include <iostream>
#include <cstring>
#include <memory>

class Shape
{
public:
    virtual void draw() = 0;
    virtual ~Shape() = default;
};

class Circle : public Shape
{
public:
    void draw() override
    {
        std::cout << "this is Circle" << std::endl;
    };
};

class Jungle : public Shape
{
public:
    void draw() override
    {
        std::cout << "this is Jungle" << std::endl;
    };
};





// 1. SimpleFactory
class SimpleFactory
{
public :
    static std::unique_ptr<Shape> createShape(const std::string &product)
    {
        if (product == "Circle")
        {
            return std::make_unique<Circle>();
        }
        else if (product == "Jungle")
        {
            return std::make_unique<Jungle>();
        }
        else
        {
            return nullptr;
        }
    };
};





// 2. FactoryWay
class FactoryWay
{
public:
    virtual std::unique_ptr<Shape> createShape() = 0;
    virtual ~FactoryWay() = default;
};

class CircleFactor : public FactoryWay
{
public:
    std::unique_ptr<Shape> createShape() override
    {
        return std::make_unique<Circle>();
    };
};

class JungleFactor : public FactoryWay
{
public:
    std::unique_ptr<Shape> createShape() override
    {
        return std::make_unique<Jungle>();
    };
};





// 3. abstract factory 
class Button
{
public:
    virtual void click() = 0;
    virtual ~ Button() = default;
};

class WindowsButton : public Button
{
public:
    void click() override
    {
        std::cout << "this is windowsbutton" << std::endl;
    };
};

class MacButton : public Button
{
public:
    void click() override
    {
        std::cout << "this is macbutton" << std::endl;
    };
};




class CheckBox
{
public:
    virtual void check() = 0;
    virtual ~ CheckBox() = default;
};

class WindowsCheckBox : public CheckBox
{
public:
    void check() override
    {
        std::cout << "this is windowsCheckBox" << std::endl;
    };
};

class MacCheckBox : public CheckBox
{
public:
    void check() override
    {
        std::cout << "this is macbCheckBox" << std::endl;
    };
};




class AbstractFactory
{
public:
    virtual std::unique_ptr<Button> createButton() = 0;
    virtual std::unique_ptr<CheckBox> createCheckBox() = 0;

    virtual ~AbstractFactory() = default;
};

class WindowFactory : public AbstractFactory
{
public:
    std::unique_ptr<Button> createButton() override
    {
        return std::make_unique<WindowsButton>();
    };

    std::unique_ptr<CheckBox> createCheckBox() override
    {
        return std::make_unique<WindowsCheckBox>();
    };
};


int main()
{
    // 1. SimpleFactory
    auto p_circle = SimpleFactory::createShape("Circle");
    p_circle->draw();

    // 2. FactoryWay
    std::unique_ptr<FactoryWay> p_factor  = std::make_unique<JungleFactor>(); //  use for poly
    auto p_product = p_factor->createShape();
    p_product->draw();


    // 3. AbstractFactory
    std::unique_ptr<AbstractFactory> p_abs_factory = std::make_unique<WindowFactory>();

    auto p_button = p_abs_factory->createButton();
    auto p_check  = p_abs_factory->createCheckBox();

    p_button->click();
    p_check->check();

    return 0;
}
