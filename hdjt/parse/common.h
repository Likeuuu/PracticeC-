#pragma once
#include <vector>
#include <string>


namespace hdjt
{

struct Gate_t
{
    std::string               type; // AND OR  NOT
    std::string               name; 
    std::vector<std::string>  inputs;
    std::string               output;
};

struct Circuit_t
{
    std::vector<std::string>  inputs;
    std::vector<std::string>  outputs;
    std::vector<Gate_t>       gates;
};
}
