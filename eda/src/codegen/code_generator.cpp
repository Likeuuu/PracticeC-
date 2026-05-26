#include "mnf/codegen/code_generator.h"

#include <algorithm>
#include <queue>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mnf {

namespace {

Diagnostic MakeCodeGenError(const std::string& message) {
  return Diagnostic{DiagnosticLevel::Error, message, {"", 1, 1}};
}

bool IsBitValue(int value) {
  return value == 0 || value == 1;
}

void CollectNonBlockingTargets(const ResolvedProcessStmtIR& stmt, std::set<int>* targets) {
  if (stmt.kind == ResolvedProcessStmtIR::Kind::Assignment) {
    if (stmt.assign_stmt != nullptr &&
        stmt.assign_stmt->assignment_kind == ResolvedProceduralAssignIR::AssignmentKind::NonBlocking) {
      targets->insert(stmt.assign_stmt->target_net_id);
    }
    return;
  }

  if (stmt.kind == ResolvedProcessStmtIR::Kind::If) {
    if (stmt.if_stmt != nullptr && stmt.if_stmt->then_stmt != nullptr) {
      CollectNonBlockingTargets(*stmt.if_stmt->then_stmt, targets);
    }
    return;
  }

  for (const auto& nested_stmt : stmt.statements) {
    if (nested_stmt != nullptr) {
      CollectNonBlockingTargets(*nested_stmt, targets);
    }
  }
}

std::set<int> CollectNonBlockingTargets(const ResolvedNetGraphIR& graph) {
  std::set<int> targets;
  for (const auto& always_block : graph.always_blocks) {
    if (always_block.body != nullptr) {
      CollectNonBlockingTargets(*always_block.body, &targets);
    }
  }
  return targets;
}

int ComputeMaxDeltaCycles(const ResolvedNetGraphIR& graph) {
  const std::size_t base = graph.nets.size() + graph.assigns.size() + graph.always_blocks.size();
  return static_cast<int>(std::max<std::size_t>(8, base * 4));
}

std::vector<int> BuildAssignEvaluationOrder(const ResolvedNetGraphIR& graph,
                                            std::vector<Diagnostic>* diagnostics) {
  std::unordered_map<int, int> target_to_assign;
  std::vector<std::vector<int>> assign_edges(graph.assigns.size());
  std::vector<int> indegree(graph.assigns.size(), 0);

  for (std::size_t i = 0; i < graph.assigns.size(); ++i) {
    const int target_net_id = graph.assigns[i].target_net_id;
    if (target_net_id < 0) {
      diagnostics->push_back(MakeCodeGenError("code generation: assign target net id is invalid"));
      continue;
    }

    if (!target_to_assign.emplace(target_net_id, static_cast<int>(i)).second) {
      diagnostics->push_back(MakeCodeGenError("code generation: multiple combinational drivers detected"));
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
    diagnostics->push_back(MakeCodeGenError("code generation: combinational assign cycle detected"));
  }

  return order;
}

}  // namespace

CodeGenResult CodeGenerator::Generate(const ElaboratedDesign& design) const {
  return GenerateProgram(design, CodeGenProgramOptions{});
}

CodeGenResult CodeGenerator::GenerateProgram(const ElaboratedDesign& design,
                                             const CodeGenProgramOptions& options) const {
  CodeGenResult result;
  auto& code = result.source;
  auto& diags = result.diagnostics;

  if (options.print_all_nets) {
    code += "#include <iostream>\n\n";
  }
  EmitModuleStruct(design.top_graph, design.top_name, &code);
  EmitEvaluateFunction(design.top_graph, design.top_name, &code, &diags);
  EmitMainFunction(design, options, &code, &diags);
  return result;
}

void CodeGenerator::EmitModuleStruct(const ResolvedNetGraphIR& graph,
                                     const std::string& top_name,
                                     std::string* code) const {
  *code += "struct " + top_name + " {\n";

  for (const auto& net : graph.nets) {
    *code += "  int net_" + std::to_string(net.id) + " = -1;  // " + net.qualified_name + "\n";
    *code += "  int prev_net_" + std::to_string(net.id) + " = -1;\n";
  }

  *code += "\n  void evaluate();\n";
  *code += "};\n\n";
}

void CodeGenerator::EmitEvaluateFunction(const ResolvedNetGraphIR& graph,
                                         const std::string& top_name,
                                         std::string* code,
                                         std::vector<Diagnostic>* diagnostics) const {
  *code += "void " + top_name + "::evaluate() {\n";
  const std::set<int> nonblocking_targets = CollectNonBlockingTargets(graph);
  const int max_delta_cycles = ComputeMaxDeltaCycles(graph);

  if (graph.always_blocks.empty()) {
    EmitAssignStatements(graph, code, diagnostics);
  } else {
    *code += "  for (int delta = 0; delta < " + std::to_string(max_delta_cycles) + "; ++delta) {\n";
    for (const auto& net : graph.nets) {
      *code += "    const int delta_prev_net_" + std::to_string(net.id) + " = net_" + std::to_string(net.id) + ";\n";
    }
    for (const int target_net_id : nonblocking_targets) {
      *code += "    int nb_net_" + std::to_string(target_net_id) + " = 0;\n";
      *code += "    bool has_nb_net_" + std::to_string(target_net_id) + " = false;\n";
    }

    EmitAssignStatements(graph, code, diagnostics);
    for (const auto& always_block : graph.always_blocks) {
      EmitAlwaysBlock(always_block, code, diagnostics);
    }
    for (const int target_net_id : nonblocking_targets) {
      *code += "    if (has_nb_net_" + std::to_string(target_net_id) + ") {\n";
      *code += "      net_" + std::to_string(target_net_id) + " = nb_net_" + std::to_string(target_net_id) + ";\n";
      *code += "    }\n";
    }
    EmitAssignStatements(graph, code, diagnostics);

    *code += "    bool changed = false;\n";
    for (const auto& net : graph.nets) {
      *code += "    changed = changed || delta_prev_net_" + std::to_string(net.id)
            + " != net_" + std::to_string(net.id) + ";\n";
    }
    *code += "    if (!changed) {\n";
    *code += "      break;\n";
    *code += "    }\n";
    *code += "  }\n";
  }

  for (const auto& net : graph.nets) {
    *code += "  prev_net_" + std::to_string(net.id) + " = net_" + std::to_string(net.id) + ";\n";
  }

  *code += "}\n\n";
}

void CodeGenerator::EmitAssignStatements(const ResolvedNetGraphIR& graph,
                                         std::string* code,
                                         std::vector<Diagnostic>* diagnostics) const {
  const std::vector<int> evaluation_order = BuildAssignEvaluationOrder(graph, diagnostics);
  if (!diagnostics->empty()) {
    *code += "  // code generation failed before assign emission\n";
    return;
  }

  for (const int assign_index : evaluation_order) {
    const auto& assign = graph.assigns[static_cast<std::size_t>(assign_index)];
    *code += "    net_" + std::to_string(assign.target_net_id) + " = ";
    EmitExprCode(assign.rhs_expr, code, diagnostics);
    *code += ";\n";
  }
}

void CodeGenerator::EmitExprCode(const ResolvedExprIR& expr,
                                 std::string* code,
                                 std::vector<Diagnostic>* diagnostics) const {
  switch (expr.kind) {
    case ResolvedExprIR::Kind::Net:
      *code += "net_" + std::to_string(expr.net_id);
      break;

    case ResolvedExprIR::Kind::Constant:
      *code += std::to_string(expr.constant_value);
      break;

    case ResolvedExprIR::Kind::Unary:
      if (expr.rhs == nullptr) {
        diagnostics->push_back(MakeCodeGenError("code generation: unary expression has null rhs"));
        *code += "/* ERROR: null rhs */";
        return;
      }
      *code += "((";
      EmitExprCode(*expr.rhs, code, diagnostics);
      *code += ") == 0 ? 1 : 0)";
      break;

    case ResolvedExprIR::Kind::Binary:
      if (expr.lhs == nullptr || expr.rhs == nullptr) {
        diagnostics->push_back(MakeCodeGenError("code generation: binary expression has null operand"));
        *code += "/* ERROR: null operand */";
        return;
      }
      *code += "(";
      EmitExprCode(*expr.lhs, code, diagnostics);
      *code += ") " + expr.op + " (";
      EmitExprCode(*expr.rhs, code, diagnostics);
      *code += ")";
      break;
  }
}

void CodeGenerator::EmitAlwaysBlock(const ResolvedAlwaysIR& always_block,
                                    std::string* code,
                                    std::vector<Diagnostic>* diagnostics) const {
  if (always_block.sensitivity_kind == ResolvedAlwaysIR::SensitivityKind::Posedge) {
    if (always_block.sensitivity_net_ids.empty()) {
      diagnostics->push_back(MakeCodeGenError("code generation: posedge always block has no sensitivity net"));
      return;
    }
    *code += "    if (";
    for (std::size_t i = 0; i < always_block.sensitivity_net_ids.size(); ++i) {
      if (i != 0) {
        *code += " || ";
      }
      const int net_id = always_block.sensitivity_net_ids[i];
      *code += "(prev_net_" + std::to_string(net_id) + " == 0 && net_" + std::to_string(net_id) + " == 1)";
    }
    *code += ") {\n";
    if (always_block.body != nullptr) {
      EmitProcessStmt(*always_block.body, code, diagnostics);
    }
    *code += "    }\n";
    return;
  }

  if (always_block.body == nullptr) {
    return;
  }

  EmitProcessStmt(*always_block.body, code, diagnostics);
}

void CodeGenerator::EmitProcessStmt(const ResolvedProcessStmtIR& stmt,
                                    std::string* code,
                                    std::vector<Diagnostic>* diagnostics) const {
  if (stmt.kind == ResolvedProcessStmtIR::Kind::Assignment) {
    if (stmt.assign_stmt == nullptr) {
      diagnostics->push_back(MakeCodeGenError("code generation: procedural assignment is missing"));
      return;
    }

    const int target_net_id = stmt.assign_stmt->target_net_id;
    if (stmt.assign_stmt->assignment_kind == ResolvedProceduralAssignIR::AssignmentKind::Blocking) {
      *code += "    net_" + std::to_string(target_net_id) + " = ";
      EmitExprCode(stmt.assign_stmt->rhs_expr, code, diagnostics);
      *code += ";\n";
    } else {
      *code += "    nb_net_" + std::to_string(target_net_id) + " = ";
      EmitExprCode(stmt.assign_stmt->rhs_expr, code, diagnostics);
      *code += ";\n";
      *code += "    has_nb_net_" + std::to_string(target_net_id) + " = true;\n";
    }
    return;
  }

  if (stmt.kind == ResolvedProcessStmtIR::Kind::If) {
    if (stmt.if_stmt == nullptr) {
      diagnostics->push_back(MakeCodeGenError("code generation: if statement is missing"));
      return;
    }

    *code += "    if (";
    EmitExprCode(stmt.if_stmt->condition_expr, code, diagnostics);
    *code += ") {\n";
    if (stmt.if_stmt->then_stmt != nullptr) {
      EmitProcessStmt(*stmt.if_stmt->then_stmt, code, diagnostics);
    }
    *code += "    }\n";
    return;
  }

  for (const auto& nested_stmt : stmt.statements) {
    if (nested_stmt != nullptr) {
      EmitProcessStmt(*nested_stmt, code, diagnostics);
    }
  }
}

void CodeGenerator::EmitMainFunction(const ElaboratedDesign& design,
                                     const CodeGenProgramOptions& options,
                                     std::string* code,
                                     std::vector<Diagnostic>* diagnostics) const {
  *code += "\nint main() {\n";
  *code += "  " + design.top_name + " sim;\n";

  for (const auto& [name, value] : options.previous_input_values) {
    if (!IsBitValue(value)) {
      diagnostics->push_back(MakeCodeGenError("code generation: previous input value must be 0 or 1: " + name));
      continue;
    }

    const auto it = std::find_if(
        design.top_graph.nets.begin(), design.top_graph.nets.end(), [&](const ResolvedNetIR& net) {
          return net.qualified_name == name;
        });
    if (it == design.top_graph.nets.end()) {
      diagnostics->push_back(MakeCodeGenError("code generation: previous input net not found: " + name));
      continue;
    }

    *code += "  sim.prev_net_" + std::to_string(it->id) + " = " + std::to_string(value) + ";\n";
  }

  for (const auto& [name, value] : options.input_values) {
    if (!IsBitValue(value)) {
      diagnostics->push_back(MakeCodeGenError("code generation: input value must be 0 or 1: " + name));
      continue;
    }

    const auto it = std::find_if(
        design.top_graph.nets.begin(), design.top_graph.nets.end(), [&](const ResolvedNetIR& net) {
          return net.qualified_name == name;
        });
    if (it == design.top_graph.nets.end()) {
      diagnostics->push_back(MakeCodeGenError("code generation: input net not found: " + name));
      continue;
    }

    *code += "  sim.net_" + std::to_string(it->id) + " = " + std::to_string(value) + ";\n";
  }

  *code += "  sim.evaluate();\n";

  if (options.print_all_nets) {
    for (const auto& net : design.top_graph.nets) {
      *code += "  std::cout << \"net_" + std::to_string(net.id) + "=\" << sim.net_"
            + std::to_string(net.id) + " << \"\\n\";\n";
    }
  }

  *code += "  return 0;\n";
  *code += "}\n";
}

}  // namespace mnf
