#pragma once
#include "common.h"

namespace hdjt
{
class Parser
{
public:
    Circuit_t  parse(const std::string& filepath);
};
} // namespace hdjt
