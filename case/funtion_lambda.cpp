#include <iostream>
#include <functional>
#include <unordered_map>
#include <string>

class EventTrigger
{
public:
    void registerEvent(const std::string& name, std::function<void()> func)
    {
        unorder_func[name].emplace_back(func);
    }

    void triger(const std::string& name)
    {
        auto it = unorder_func.find(name);
        if (it != unorder_func.end())
        {
            for (auto& a : it->second)
            {
                a();
            }
        }
        else
        {
            std::cout << "name is error" << std::endl;
        }

    }

private:
    std::unordered_map<std::string, std::vector<std::function<void()>>> unorder_func;
};

int main()
{
    EventTrigger eventTrig;
    eventTrig.registerEvent("set up", [](){std::cout << "set 1" << std::endl;});

    eventTrig.registerEvent("set up", [](){std::cout << "set 2" << std::endl;});

    eventTrig.registerEvent("set up", [](){std::cout << "set 3" << std::endl;});

    eventTrig.registerEvent("run", [](){std::cout << "run 1" << std::endl;});

    eventTrig.registerEvent("run", [](){std::cout << "run 2" << std::endl;});

    eventTrig.registerEvent("run", [](){std::cout << "run 3" << std::endl;});

    eventTrig.registerEvent("end", [](){std::cout << "end 1" << std::endl;});

    eventTrig.registerEvent("end", [](){std::cout << "end 2" << std::endl;});

    eventTrig.registerEvent("end", [](){std::cout << "end 3" << std::endl;});

    eventTrig.triger("set up");
    eventTrig.triger("run");
    eventTrig.triger("running");
    eventTrig.triger("end");

    return 0;
}
