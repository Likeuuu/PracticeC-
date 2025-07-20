#include <iostream>
#include <vector>
#include <algorithm>

#include <memory>

class Observer
{
public:
    virtual void update(int data) = 0;
    virtual ~Observer() = default;
};

class Subject
{
public:
    virtual void attach(std::shared_ptr<Observer>&)  = 0;
    virtual void dettach(std::shared_ptr<Observer>&) = 0;
    virtual void notify()           = 0;
    virtual ~Subject()              = default;
};

class TV : public Observer
{
public:
    void update(int data) override
    {
        std::cout << "TV  data is " << data << std::endl;
    };
};

class IPhone : public Observer
{
public:
    void update(int data) override
    {
        std::cout << "IPhone  data is " << data << std::endl;
    };
};

class Chumen : public Subject
{
private:
    int tempratrure  = 0;
    std::vector<std::shared_ptr<Observer>> vec_Observer;

public:
    void attach(std::shared_ptr<Observer> &ob) override
    {
        vec_Observer.emplace_back(ob);
    };

    void dettach(std::shared_ptr<Observer> &ob) override
    {
        vec_Observer.erase(remove(vec_Observer.begin(), vec_Observer.end(), ob), vec_Observer.end());
    };

    void notify() override
    {
        for (auto& iter : vec_Observer)
        {
            iter->update(tempratrure);
        }
    };

    void setTemprature(int data)
    {
        tempratrure = data;
        notify();
    }
} ;

int main()
{
    std::shared_ptr<Observer> p_tv      = std::make_shared<TV>();
    std::shared_ptr<Observer> p_iphone  = std::make_shared<IPhone>();

    std::shared_ptr<Chumen> p_subject  = std::make_shared<Chumen>();

    p_subject->attach(p_tv);
    p_subject->attach(p_iphone);

    p_subject->setTemprature(10);

    p_subject->dettach(p_tv);
    p_subject->setTemprature(20);

    return 0;
}
