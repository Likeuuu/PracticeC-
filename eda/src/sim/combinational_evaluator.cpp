#include "mnf/sim/combinational_evaluator.h"

#include <algorithm>
#include <queue>
#include <unordered_map>
#include <unordered_set>

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

    case ResolvedExprIR::Kind::Unary:
      if (expr.rhs == nullptr) {
        diagnostics->push_back(MakeEvalError("Unary expression is incomplete"));
        return kUnknown;
      }
      if (expr.op != "~") {
        diagnostics->push_back(MakeEvalError("Unsupported unary operator: " + expr.op));
        return kUnknown;
      }
      {
        const int operand_value = EvaluateExpr(*expr.rhs, net_values, diagnostics);
        if (operand_value == kUnknown) {
          return kUnknown;
        }
        return operand_value == 0 ? 1 : 0;
      }

    case ResolvedExprIR::Kind::Binary:
      if (expr.lhs == nullptr || expr.rhs == nullptr) {
        diagnostics->push_back(MakeEvalError("Binary expression is incomplete"));
        return kUnknown;
      }
      {
        const int lhs_value = EvaluateExpr(*expr.lhs, net_values, diagnostics);
        const int rhs_value = EvaluateExpr(*expr.rhs, net_values, diagnostics);
        if (lhs_value == kUnknown || rhs_value == kUnknown) {
          return kUnknown;
        }

        if (expr.op == "&") {
          return lhs_value & rhs_value;
        }
        if (expr.op == "^") {
          return lhs_value ^ rhs_value;
        }
        if (expr.op == "|") {
          return lhs_value | rhs_value;
        }

        diagnostics->push_back(MakeEvalError("Unsupported binary operator: " + expr.op));
        return kUnknown;
      }
  }

  diagnostics->push_back(MakeEvalError("Unsupported resolved expression kind"));
  return kUnknown;
}

std::vector<int> BuildEvaluationOrder(const ResolvedNetGraphIR& graph,
                                      std::vector<Diagnostic>* diagnostics) {
  std::unordered_map<int, int> target_to_assign;
  std::vector<std::vector<int>> assign_edges(graph.assigns.size());
  std::vector<int> indegree(graph.assigns.size(), 0);

  for (std::size_t i = 0; i < graph.assigns.size(); ++i) {
    const int target_net_id = graph.assigns[i].target_net_id;
    if (target_net_id < 0) {
      diagnostics->push_back(MakeEvalError("Assign target net id is invalid"));
      continue;
    }

    if (!target_to_assign.emplace(target_net_id, static_cast<int>(i)).second) {
      diagnostics->push_back(MakeEvalError("Multiple combinational drivers detected on the same net"));
    }
  }

  for (std::size_t i = 0; i < graph.assigns.size(); ++i) {
    std::unordered_set<int> seen_dependencies;
    for (const int net_id : graph.assigns[i].source_net_ids) {
      const auto it = target_to_assign.find(net_id);
      if (it == target_to_assign.end()) {
        continue;
      }

      const int producer_index = it->second;
      if (producer_index == static_cast<int>(i)) {
        continue;
      }
      if (!seen_dependencies.insert(producer_index).second) {
        continue;
      }

      assign_edges[static_cast<std::size_t>(producer_index)].push_back(static_cast<int>(i));
      ++indegree[i];
    }
  }

  std::queue<int> ready;
  for (std::size_t i = 0; i < indegree.size(); ++i) {
    if (indegree[i] == 0) {
      ready.push(static_cast<int>(i));
    }
  }

  std::vector<int> order;
  while (!ready.empty()) {
    const int index = ready.front();
    ready.pop();
    order.push_back(index);

    for (const int next_index : assign_edges[static_cast<std::size_t>(index)]) {
      --indegree[static_cast<std::size_t>(next_index)];
      if (indegree[static_cast<std::size_t>(next_index)] == 0) {
        ready.push(next_index);
      }
    }
  }

  if (order.size() != graph.assigns.size()) {
    diagnostics->push_back(MakeEvalError("Combinational assign cycle detected"));
  }

  return order;
}

void EvaluateAssigns(const ResolvedNetGraphIR& graph,
                     const std::vector<int>& evaluation_order,
                     std::vector<int>* net_values,
                     std::vector<Diagnostic>* diagnostics) {
  for (const int assign_index : evaluation_order) {
    const auto& assign = graph.assigns[static_cast<std::size_t>(assign_index)];
    if (assign.target_net_id < 0 || static_cast<std::size_t>(assign.target_net_id) >= net_values->size()) {
      diagnostics->push_back(MakeEvalError("Assign target net id is invalid"));
      continue;
    }

    const int value = EvaluateExpr(assign.rhs_expr, *net_values, diagnostics);
    if (value == kUnknown) {
      continue;
    }

    (*net_values)[static_cast<std::size_t>(assign.target_net_id)] = value;
  }
}

void ExecuteProcessStmt(const ResolvedProcessStmtIR& stmt,
                        std::vector<int>* net_values,
                        std::unordered_map<int, int>* pending_nonblocking,
                        std::vector<Diagnostic>* diagnostics) {
  if (stmt.kind == ResolvedProcessStmtIR::Kind::Assignment) {
    if (stmt.assign_stmt == nullptr) {
      return;
    }

    if (stmt.assign_stmt->target_net_id < 0 ||
        static_cast<std::size_t>(stmt.assign_stmt->target_net_id) >= net_values->size()) {
      diagnostics->push_back(MakeEvalError("Procedural assignment target net id is invalid"));
      return;
    }

    const int value = EvaluateExpr(stmt.assign_stmt->rhs_expr, *net_values, diagnostics);
    if (value == kUnknown) {
      return;
    }

    if (stmt.assign_stmt->assignment_kind == ResolvedProceduralAssignIR::AssignmentKind::Blocking) {
      (*net_values)[static_cast<std::size_t>(stmt.assign_stmt->target_net_id)] = value;
    } else {
      (*pending_nonblocking)[stmt.assign_stmt->target_net_id] = value;
    }
    return;
  }

  if (stmt.kind == ResolvedProcessStmtIR::Kind::If) {
    if (stmt.if_stmt == nullptr) {
      return;
    }

    const int condition_value = EvaluateExpr(stmt.if_stmt->condition_expr, *net_values, diagnostics);
    if (condition_value == 1 && stmt.if_stmt->then_stmt != nullptr) {
      ExecuteProcessStmt(*stmt.if_stmt->then_stmt, net_values, pending_nonblocking, diagnostics);
    }
    return;
  }

  for (const auto& nested_stmt : stmt.statements) {
    if (nested_stmt != nullptr) {
      ExecuteProcessStmt(*nested_stmt, net_values, pending_nonblocking, diagnostics);
    }
  }
}

bool DidAnySensitiveNetChange(const ResolvedAlwaysIR& always_block,
                              const std::vector<int>& before_values,
                              const std::vector<int>& after_values,
                              const std::unordered_set<int>& driven_input_net_ids) {
  if (always_block.sensitivity_kind == ResolvedAlwaysIR::SensitivityKind::Implicit) {
    return true;
  }

  for (const int net_id : always_block.sensitivity_net_ids) {
    if (driven_input_net_ids.find(net_id) != driven_input_net_ids.end()) {
      return true;
    }
    if (net_id < 0 || static_cast<std::size_t>(net_id) >= before_values.size() ||
        static_cast<std::size_t>(net_id) >= after_values.size()) {
      continue;
    }
    if (before_values[static_cast<std::size_t>(net_id)] != after_values[static_cast<std::size_t>(net_id)]) {
      return true;
    }
  }
  return false;
}

void ExecuteAlwaysBlocks(const ResolvedNetGraphIR& graph,
                         const std::vector<int>& before_always_values,
                         const std::unordered_set<int>& driven_input_net_ids,
                         std::vector<int>* net_values,
                         std::vector<Diagnostic>* diagnostics) {
  std::unordered_map<int, int> pending_nonblocking;

  for (const auto& always_block : graph.always_blocks) {
    if (always_block.body == nullptr) {
      continue;
    }
    if (!DidAnySensitiveNetChange(always_block, before_always_values, *net_values, driven_input_net_ids)) {
      continue;
    }
    ExecuteProcessStmt(*always_block.body, net_values, &pending_nonblocking, diagnostics);
  }

  for (const auto& [net_id, value] : pending_nonblocking) {
    if (net_id < 0 || static_cast<std::size_t>(net_id) >= net_values->size()) {
      diagnostics->push_back(MakeEvalError("Nonblocking assignment target net id is invalid"));
      continue;
    }
    (*net_values)[static_cast<std::size_t>(net_id)] = value;
  }
}

}  // namespace

CombinationalEvalResult CombinationalEvaluator::Evaluate(
    const ResolvedNetGraphIR& graph,
    const std::unordered_map<std::string, int>& input_values) const {
  CombinationalEvalResult result;
  result.net_values.assign(graph.nets.size(), kUnknown);

  std::unordered_set<int> driven_input_net_ids;

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
    driven_input_net_ids.insert(it->id);
  }

  const std::vector<int> evaluation_order = BuildEvaluationOrder(graph, &result.diagnostics);
  if (!result.Ok()) {
    return result;
  }

  const std::vector<int> before_assign_values = result.net_values;
  EvaluateAssigns(graph, evaluation_order, &result.net_values, &result.diagnostics);
  ExecuteAlwaysBlocks(graph, before_assign_values, driven_input_net_ids, &result.net_values, &result.diagnostics);
  EvaluateAssigns(graph, evaluation_order, &result.net_values, &result.diagnostics);

  return result;
}

}  // namespace mnf
