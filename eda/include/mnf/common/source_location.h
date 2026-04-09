#pragma once

#include <cstddef>
#include <string>

namespace mnf {

struct SourceLocation {
  std::string file;
  std::size_t line = 1;
  std::size_t column = 1;
};

}  // namespace mnf
