#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "mnf/common/diagnostic.h"
#include "mnf/elaboration/ir.h"

namespace mnf {

struct CombinationalEvalResult {
  std::vector<int> net_values; // id : signal value
  std::vector<Diagnostic> diagnostics;

  bool Ok() const {
    return diagnostics.empty();
  }
};

class CombinationalEvaluator {
public:
  CombinationalEvalResult Evaluate(const ResolvedNetGraphIR& graph,
                                   const std::unordered_map<std::string, int>& input_values) const;
};

}  // namespace mnf
