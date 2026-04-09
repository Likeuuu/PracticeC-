#pragma once

#include <optional>
#include <vector>

#include "mnf/common/diagnostic.h"

namespace mnf {

template <typename T>
struct Result {
  std::optional<T> value;
  std::vector<Diagnostic> diagnostics;

  bool Ok() const {
    return value.has_value();
  }
};

}  // namespace mnf
