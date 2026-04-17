#include "mnf/semantic/semantic_checker.h"

#include <algorithm>
#include <unordered_set>
#include <vector>

namespace mnf {

namespace {

std::vector<std::string> CollectDeclaredSignals(const ModuleDecl& module) {
  std::vector<std::string> signals = module.ports;
  for (const auto& wire_decl : module.wire_decls) {
    signals.insert(signals.end(), wire_decl.names.begin(), wire_decl.names.end());
  }
  return signals;
}

bool Contains(const std::vector<std::string>& names, const std::string& name) {
  return std::find(names.begin(), names.end(), name) != names.end();
}

}  // namespace

std::vector<Diagnostic> SemanticChecker::Check(const Program& program, SymbolTable& symbols) const {
  std::vector<Diagnostic> diagnostics;

  for (const auto& module : program.modules) {
    if (!symbols.AddModule(module.get())) {
      diagnostics.push_back(Diagnostic{DiagnosticLevel::Error, "Duplicate module definition: " + module->name, module->location});
      continue;
    }

    std::unordered_set<std::string> check_dump;
    for(const auto& iter : module->ports)
    {
      if (!check_dump.insert(iter).second){
        diagnostics.push_back(Diagnostic{DiagnosticLevel::Error, "Duplicate port definition: " + module->name, module->location});
        continue;
      }
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
  const auto declared_signals = CollectDeclaredSignals(module);

  for (const auto& instance : module.instances) {
    const ModuleDecl* referenced_module = symbols.FindModule(instance.module_name);
    if (referenced_module == nullptr) {
      diagnostics.push_back(Diagnostic{DiagnosticLevel::Error,
                                       "Undefined module referenced by instance '" + instance.instance_name + "': " + instance.module_name,
                                       instance.location});
      continue;
    }

    if (instance.connections.size() != referenced_module->ports.size()) {
      diagnostics.push_back(Diagnostic{DiagnosticLevel::Error,
                                       "Port connection count mismatch for instance '" + instance.instance_name + "'",
                                       instance.location});
    }

    std::unordered_set<std::string> connected_ports;
    for (const auto& connection : instance.connections) {
      if (!Contains(referenced_module->ports, connection.port_name)) {
        diagnostics.push_back(Diagnostic{DiagnosticLevel::Error,
                                         "Unknown port '" + connection.port_name + "' on module '" + referenced_module->name + "'",
                                         connection.location});
      }
      if (!connected_ports.insert(connection.port_name).second) {
        diagnostics.push_back(Diagnostic{DiagnosticLevel::Error,
                                         "Duplicate connection for port '" + connection.port_name + "' on instance '" + instance.instance_name + "'",
                                         connection.location});
      }
      if (!Contains(declared_signals, connection.signal_name)) {
        diagnostics.push_back(Diagnostic{DiagnosticLevel::Error,
                                         "Signal '" + connection.signal_name + "' is not declared in module '" + module.name + "'",
                                         connection.location});
      }
    }
  }
}

}  // namespace mnf
