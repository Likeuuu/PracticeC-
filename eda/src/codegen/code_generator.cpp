#include "mnf/codegen/code_generator.h"

#include <algorithm>
#include <queue>
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
    *code += "  int net_" + std::to_string(net.id) + " = 0;  // " + net.qualified_name + "\n";
  }

  *code += "\n  void evaluate();\n";
  *code += "};\n\n";
}

void CodeGenerator::EmitEvaluateFunction(const ResolvedNetGraphIR& graph,
                                         const std::string& top_name,
                                         std::string* code,
                                         std::vector<Diagnostic>* diagnostics) const {
  *code += "void " + top_name + "::evaluate() {\n";

  const std::vector<int> evaluation_order = BuildAssignEvaluationOrder(graph, diagnostics);
  if (!diagnostics->empty()) {
    *code += "  // code generation failed before assign emission\n";
    *code += "}\n\n";
    return;
  }

  for (const int assign_index : evaluation_order) {
    const auto& assign = graph.assigns[static_cast<std::size_t>(assign_index)];
    *code += "  net_" + std::to_string(assign.target_net_id) + " = ";
    EmitExprCode(assign.rhs_expr, code, diagnostics);
    *code += ";\n";
  }

  *code += "}\n\n";
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
      *code += "~(";
      EmitExprCode(*expr.rhs, code, diagnostics);
      *code += ")";
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

void CodeGenerator::EmitMainFunction(const ElaboratedDesign& design,
                                     const CodeGenProgramOptions& options,
                                     std::string* code,
                                     std::vector<Diagnostic>* diagnostics) const {
  *code += "\nint main() {\n";
  *code += "  " + design.top_name + " sim;\n";

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
