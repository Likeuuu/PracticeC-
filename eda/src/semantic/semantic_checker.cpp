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
  for (const auto& reg_decl : module.reg_decls) {
    signals.insert(signals.end(), reg_decl.names.begin(), reg_decl.names.end());
  }
  return signals;
}

std::vector<std::string> CollectDeclaredWires(const ModuleDecl& module) {
  std::vector<std::string> wire_names;
  for (const auto& wire_decl : module.wire_decls) {
    wire_names.insert(wire_names.end(), wire_decl.names.begin(), wire_decl.names.end());
  }

  return wire_names;
}

std::vector<std::string> CollectDeclaredRegs(const ModuleDecl& module) {
  std::vector<std::string> reg_names;
  for (const auto& reg_decl : module.reg_decls) {
    reg_names.insert(reg_names.end(), reg_decl.names.begin(), reg_decl.names.end());
  }

  return reg_names;
}

std::vector<std::string> CollectDeclaredPorts(const ModuleDecl& module) {
  std::vector<std::string> ports;
  for (const auto& decl : module.port_decls) {
    ports.insert(ports.end(), decl.names.begin(), decl.names.end());
  }
  return ports;
}

bool Contains(const std::vector<std::string>& names, const std::string& name) {
  return std::find(names.begin(), names.end(), name) != names.end();
}

void ValidateExpression(const Expression& expr,
                        const std::vector<std::string>& declared_signals,
                        std::vector<Diagnostic>& diagnostics,
                        const char* undeclared_message_prefix) {
  if (expr.kind == Expression::Kind::Identifier) {
    if (!Contains(declared_signals, expr.text)) {
      diagnostics.push_back(Diagnostic{DiagnosticLevel::Error,
                                       std::string(undeclared_message_prefix) + expr.text,
                                       expr.location});
    }
    return;
  }

  if (expr.kind == Expression::Kind::Unary) {
    if (expr.rhs != nullptr) {
      ValidateExpression(*expr.rhs, declared_signals, diagnostics, undeclared_message_prefix);
    }
    return;
  }

  if (expr.kind == Expression::Kind::Binary) {
    if (expr.lhs != nullptr) {
      ValidateExpression(*expr.lhs, declared_signals, diagnostics, undeclared_message_prefix);
    }
    if (expr.rhs != nullptr) {
      ValidateExpression(*expr.rhs, declared_signals, diagnostics, undeclared_message_prefix);
    }
  }
}

void ValidateProceduralStmt(const ProceduralStmt& stmt,
                            const std::vector<std::string>& declared_signals,
                            std::vector<Diagnostic>& diagnostics) {
  if (stmt.kind == ProceduralStmt::Kind::Assignment) {
    if (stmt.assign_stmt == nullptr) {
      return;
    }

    if (!Contains(declared_signals, stmt.assign_stmt->lhs)) {
      diagnostics.push_back(Diagnostic{DiagnosticLevel::Error,
                                       "Procedural assignment target is not declared: " + stmt.assign_stmt->lhs,
                                       stmt.assign_stmt->location});
    }

    ValidateExpression(stmt.assign_stmt->rhs,
                       declared_signals,
                       diagnostics,
                       "Procedural assignment source is not declared: ");
    return;
  }

  if (stmt.kind == ProceduralStmt::Kind::If) {
    if (stmt.if_stmt == nullptr) {
      return;
    }

    ValidateExpression(stmt.if_stmt->condition,
                       declared_signals,
                       diagnostics,
                       "Procedural if condition signal is not declared: ");

    if (stmt.if_stmt->then_stmt != nullptr) {
      ValidateProceduralStmt(*stmt.if_stmt->then_stmt, declared_signals, diagnostics);
    }
    return;
  }

  if (stmt.kind == ProceduralStmt::Kind::Block) {
    for (const auto& nested_stmt : stmt.statements) {
      if (nested_stmt != nullptr) {
        ValidateProceduralStmt(*nested_stmt, declared_signals, diagnostics);
      }
    }
  }
}

}  // namespace

std::vector<Diagnostic> SemanticChecker::Check(const Program& program, SymbolTable& symbols) const {
  std::vector<Diagnostic> diagnostics;

  for (const auto& module : program.modules) {
    if (!symbols.AddModule(module.get())) {
      diagnostics.push_back(Diagnostic{DiagnosticLevel::Error,
                                       "Duplicate module definition: " + module->name,
                                       module->location});
      continue;
    }

    std::unordered_set<std::string> seen_ports;
    for (const auto& port : module->ports) {
      if (!seen_ports.insert(port).second) {
        diagnostics.push_back(Diagnostic{DiagnosticLevel::Error,
                                         "Duplicate port definition in module header: " + port,
                                         module->location});
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
  const auto declared_ports = CollectDeclaredPorts(module);
  const auto declared_wires = CollectDeclaredWires(module);
  const auto declared_regs = CollectDeclaredRegs(module);

  std::unordered_set<std::string> seen_declared_ports;
  for (const auto& port_name : declared_ports) {
    if (!seen_declared_ports.insert(port_name).second) {
      diagnostics.push_back(Diagnostic{DiagnosticLevel::Error,
                                       "Duplicate port declaration: " + port_name,
                                       module.location});
    }
    if (!Contains(module.ports, port_name)) {
      diagnostics.push_back(Diagnostic{DiagnosticLevel::Error,
                                       "Declared port not listed in module header: " + port_name,
                                       module.location});
    }
  }

  for (const auto& port_name : module.ports) {
    if (!Contains(declared_ports, port_name)) {
      diagnostics.push_back(Diagnostic{DiagnosticLevel::Error,
                                       "Module header port missing input/output declaration: " + port_name,
                                       module.location});
    }
  }

  std::unordered_set<std::string> seen_declared_wires;
  for (const auto& wire_name : declared_wires) {
    if (!seen_declared_wires.insert(wire_name).second) {
      diagnostics.push_back(Diagnostic{DiagnosticLevel::Error,
                                       "Duplicate wire declaration: " + wire_name,
                                       module.location});
    }

    if (Contains(module.ports, wire_name)) {
      diagnostics.push_back(Diagnostic{DiagnosticLevel::Error,
                                       "Wire name conflicts with module port: " + wire_name,
                                       module.location});
    }

    if (Contains(declared_regs, wire_name)) {
      diagnostics.push_back(Diagnostic{DiagnosticLevel::Error,
                                       "Wire name conflicts with reg declaration: " + wire_name,
                                       module.location});
    }
  }

  std::unordered_set<std::string> seen_declared_regs;
  for (const auto& reg_name : declared_regs) {
    if (!seen_declared_regs.insert(reg_name).second) {
      diagnostics.push_back(Diagnostic{DiagnosticLevel::Error,
                                       "Duplicate reg declaration: " + reg_name,
                                       module.location});
    }

    if (Contains(module.ports, reg_name)) {
      diagnostics.push_back(Diagnostic{DiagnosticLevel::Error,
                                       "Reg name conflicts with module port: " + reg_name,
                                       module.location});
    }
  }

  for (const auto& assign_stmt : module.assign_stmts) {
    if (!Contains(declared_signals, assign_stmt.lhs)) {
      diagnostics.push_back(Diagnostic{DiagnosticLevel::Error,
                                       "Assign target is not declared: " + assign_stmt.lhs,
                                       assign_stmt.location});
    }

    ValidateExpression(assign_stmt.rhs,
                       declared_signals,
                       diagnostics,
                       "Assign source is not declared: ");
  }

  for (const auto& always_block : module.always_blocks) {
    if (always_block.sensitivity_kind == AlwaysBlock::SensitivityKind::ExplicitList) {
      std::unordered_set<std::string> seen_sensitivity_names;
      for (const auto& signal_name : always_block.sensitivity_signals) {
        if (!Contains(declared_signals, signal_name)) {
          diagnostics.push_back(Diagnostic{DiagnosticLevel::Error,
                                           "Always sensitivity signal is not declared: " + signal_name,
                                           always_block.location});
        }
        if (!seen_sensitivity_names.insert(signal_name).second) {
          diagnostics.push_back(Diagnostic{DiagnosticLevel::Error,
                                           "Duplicate signal in always sensitivity list: " + signal_name,
                                           always_block.location});
        }
      }
    }
    if (always_block.body != nullptr) {
      ValidateProceduralStmt(*always_block.body, declared_signals, diagnostics);
    }
  }

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
