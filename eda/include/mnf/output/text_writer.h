#pragma once

#include <string>

#include "mnf/elaboration/ir.h"

namespace mnf {

class TextWriter {
public:
  std::string WriteSummary(const ElaboratedDesign& design) const;
};

}  // namespace mnf
