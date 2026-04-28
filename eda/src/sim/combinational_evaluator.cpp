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

int EvaluateExpr(const ResolvedExprIR& expr,
                 const std::vector<int>& net_values,
                 std::vector<Diagnostic>* diagnostics) {
  switch (expr.kind) {
    case ResolvedExprIR::Kind::Net:
      if (expr.net_id < 0 || static_cast<std::size_t>(expr.net_id) >= net_values.size()) {
        diagnostics->push_back(MakeEvalError("Resolved net id is invalid in expression"));
        return kUnknown;
      }
      return net_values[static_cast<std::size_t>(expr.net_id)];

    case ResolvedExprIR::Kind::Constant:
      if (!IsBitValue(expr.constant_value)) {
        diagnostics->push_back(MakeEvalError("Only 0/1 constants are supported in combinational evaluation"));
        return kUnknown;
      }
      return expr.constant_value;

    case ResolvedExprIR::Kind::Binary:
      if (expr.lhs == nullptr || expr.rhs == nullptr) {
        diagnostics->push_back(MakeEvalError("Binary expression is incomplete"));
        return kUnknown;
      }
      if (expr.op != "&") {
        diagnostics->push_back(MakeEvalError("Unsupported binary operator: " + expr.op));
        return kUnknown;
      }

      {
        const int lhs_value = EvaluateExpr(*expr.lhs, net_values, diagnostics);
        const int rhs_value = EvaluateExpr(*expr.rhs, net_values, diagnostics);
        if (lhs_value == kUnknown || rhs_value == kUnknown) {
          return kUnknown;
        }
        return lhs_value & rhs_value;
      }
  }

  diagnostics->push_back(MakeEvalError("Unsupported resolved expression kind"));
  return kUnknown;
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

      const int value = EvaluateExpr(assign.rhs_expr, result.net_values, &result.diagnostics);
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
