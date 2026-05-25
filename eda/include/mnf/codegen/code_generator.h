#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "mnf/common/diagnostic.h"
#include "mnf/elaboration/ir.h"

namespace mnf {

struct CodeGenResult {
  std::string source;
  std::vector<Diagnostic> diagnostics;

  bool Ok() const {
    return diagnostics.empty();
  }
};

struct CodeGenProgramOptions {
  std::unordered_map<std::string, int> input_values;
  bool print_all_nets = false;
};

class CodeGenerator {
public:
  CodeGenResult Generate(const ElaboratedDesign& design) const;
  CodeGenResult GenerateProgram(const ElaboratedDesign& design,
                                const CodeGenProgramOptions& options) const;

private:
  void EmitExprCode(const ResolvedExprIR& expr,
                    std::string* code,
                    std::vector<Diagnostic>* diagnostics) const;
  void EmitModuleStruct(const ResolvedNetGraphIR& graph,
                        const std::string& top_name,
                        std::string* code) const;
  void EmitEvaluateFunction(const ResolvedNetGraphIR& graph,
                            const std::string& top_name,
                            std::string* code,
                            std::vector<Diagnostic>* diagnostics) const;
  void EmitMainFunction(const ElaboratedDesign& design,
                        const CodeGenProgramOptions& options,
                        std::string* code,
                        std::vector<Diagnostic>* diagnostics) const;
};

}  // namespace mnf
