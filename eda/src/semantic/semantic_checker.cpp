#include "mnf/semantic/semantic_checker.h"

namespace mnf {

std::vector<Diagnostic> SemanticChecker::Check(const Program& program, SymbolTable& symbols) const {
  std::vector<Diagnostic> diagnostics;

  for (const auto& module : program.modules) {
    if (!symbols.AddModule(module.get())) {
      diagnostics.push_back(Diagnostic{DiagnosticLevel::Error, "Duplicate module definition: " + module->name, module->location});
      continue;
    }
  }

  for (const auto& module : program.modules) {
    CheckModule(*module, symbols, diagnostics);
  }

  return diagnostics;
}

void SemanticChecker::CheckModule(const ModuleDecl& module,
                                  const SymbolTable& symbols,
                                  std::vector<Diagnostic>& diagnostics) const {
  for (const auto& instance : module.instances) {
    if (symbols.FindModule(instance.module_name) == nullptr) {
      diagnostics.push_back(Diagnostic{DiagnosticLevel::Error,
                                       "Undefined module referenced by instance '" + instance.instance_name + "': " + instance.module_name,
                                       instance.location});
    }
  }
}

}  // namespace mnf
