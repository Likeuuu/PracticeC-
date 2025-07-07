#pragma once

#include <cstring>
#include <iostream>

#define DEBUG
#include <cassert>


namespace lxm
{

// rule of zero
class string_zero
{
public:
    std::string   info;

void print()
{
    printf("zero is %s.\n", info.c_str());
    return;
}
};

// rule of three
class string_three
{
private: 
        char *data = nullptr;

public:
        string_three(const char* str = "")
        {
            data = new char[strlen(str) + 1];
            std::strcpy(data, str);
        };

        ~string_three()
        {
            delete[] data;
        };

        string_three(const string_three& other)
        {
            data = new char[strlen(other.data) + 1];
            std::strcpy(data, other.data);
        };

        string_three& operator=(const string_three& other)
        {
            if (this != &other)
            {
                delete[] data;
                data = new char[strlen(other.data) + 1];
                std::strcpy(data, other.data);
            }

            return *this;
        };

        void print()
        {
            printf("three is %s.\n", data);
            return;
        }
};

// rule of five
class string_five
{
private:
        char *data = nullptr;
public:
        string_five(const char *str = "")
        {
            data = new char[strlen(str) + 1];
            strcpy(data, str);
        };

        ~string_five()
        {
            delete[] data;
        };

        string_five(const string_five &other)
        {
            data = new char[strlen(other.data) + 1];
            strcpy(data, other.data);
        };

        string_five& operator=(const string_five &other)
        {
            if (this != &other)
            {
                delete[] data;
                data = new char[strlen(other.data) + 1];
                strcpy(data, other.data);
            }

            return *this;
        };

        string_five(string_five &&other) noexcept
        {
            data = other.data;
            other.data = nullptr;
        };

        string_five& operator=(string_five &&other) noexcept
        {
            if (this != &other)
            {
                delete[] data;
                data = other.data;
                other.data = nullptr;
            }

            return *this;
        };

        void print()
        {
            printf("five is %s.\n", data);
            return;
        }
};

int rule_of_xx()
{
    // rule of zero/three/five
    string_zero zero;
    zero.info  = "hallo zero";
    zero.print();

    string_three three("hallo three");
    three.print();

    return 0;  
};
} // lxm
