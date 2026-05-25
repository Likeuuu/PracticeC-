#include "mnf/codegen/code_generator.h"

#include <algorithm>
#include <string>

namespace mnf {

namespace {

Diagnostic MakeCodeGenError(const std::string& message) {
  return Diagnostic{DiagnosticLevel::Error, message, {"", 1, 1}};
}

bool IsBitValue(int value) {
  return value == 0 || value == 1;
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

  for (const auto& assign : graph.assigns) {
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
