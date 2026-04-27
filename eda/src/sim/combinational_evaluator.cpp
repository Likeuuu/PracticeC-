#include "mnf/sim/combinational_evaluator.h"

#include <algorithm>

namespace mnf {

namespace {

constexpr int kUnknown = -1;

bool IsBitValue(int value) {
  return value == 0 || value == 1;
}

Diagnostic MakeEvalError(const std::string& message) {
  return Diagnostic{DiagnosticLevel::Error, message, {"", 1, 1}};
}

}  // namespace

CombinationalEvalResult CombinationalEvaluator::Evaluate(
    const ResolvedNetGraphIR& graph,
    const std::unordered_map<std::string, int>& input_values) const {
  CombinationalEvalResult result;
  result.net_values.assign(graph.nets.size(), kUnknown);

  for (const auto& [name, value] : input_values) {
    if (!IsBitValue(value)) {
      result.diagnostics.push_back(MakeEvalError("Input value must be 0 or 1: " + name));
      continue;
    }

    auto it = std::find_if(graph.nets.begin(), graph.nets.end(), [&](const ResolvedNetIR& net) {
      return net.qualified_name == name;
    });
    if (it == graph.nets.end()) {
      result.diagnostics.push_back(MakeEvalError("Input net not found: " + name));
      continue;
    }
    result.net_values[static_cast<std::size_t>(it->id)] = value;
  }

  bool changed = true;
  for (std::size_t pass = 0; pass < graph.assigns.size() && changed; ++pass) {
    changed = false;
    for (const auto& assign : graph.assigns) {
      if (assign.target_net_id < 0 || static_cast<std::size_t>(assign.target_net_id) >= result.net_values.size()) {
        result.diagnostics.push_back(MakeEvalError("Assign target net id is invalid"));
        continue;
      }

      int value = kUnknown;
      if (assign.has_constant_value) {
        value = assign.constant_value == 0 ? 0 : 1;
      } else if (assign.expr_op.empty()) {
        if (assign.source_net_ids.size() != 1) {
          continue;
        }
        const int source_id = assign.source_net_ids[0];
        if (source_id < 0 || static_cast<std::size_t>(source_id) >= result.net_values.size()) {
          result.diagnostics.push_back(MakeEvalError("Assign source net id is invalid"));
          continue;
        }
        value = result.net_values[static_cast<std::size_t>(source_id)];
      } else if (assign.expr_op == "&") {
        if (assign.source_net_ids.size() != 2) {
          continue;
        }
        const int lhs_id = assign.source_net_ids[0];
        const int rhs_id = assign.source_net_ids[1];
        if (lhs_id < 0 || rhs_id < 0 ||
            static_cast<std::size_t>(lhs_id) >= result.net_values.size() ||
            static_cast<std::size_t>(rhs_id) >= result.net_values.size()) {
          result.diagnostics.push_back(MakeEvalError("Binary assign source net id is invalid"));
          continue;
        }
        const int lhs = result.net_values[static_cast<std::size_t>(lhs_id)];
        const int rhs = result.net_values[static_cast<std::size_t>(rhs_id)];
        if (lhs == kUnknown || rhs == kUnknown) {
          continue;
        }
        value = lhs & rhs;
      } else {
        result.diagnostics.push_back(MakeEvalError("Unsupported assign operator: " + assign.expr_op));
        continue;
      }

      if (value == kUnknown) {
        continue;
      }

      auto& target_value = result.net_values[static_cast<std::size_t>(assign.target_net_id)];
      if (target_value != value) {
        target_value = value;
        changed = true;
      }
    }
  }

  return result;
}

}  // namespace mnf
