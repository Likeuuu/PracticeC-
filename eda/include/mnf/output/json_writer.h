#pragma once

#include <string>

#include "mnf/elaboration/ir.h"

namespace mnf {

class JsonWriter {
public:
  std::string Write(const ElaboratedDesign& design) const;
};

}  // namespace mnf
