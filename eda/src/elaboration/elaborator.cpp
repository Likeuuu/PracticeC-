#include "mnf/elaboration/elaborator.h"

#include <algorithm>
#include <queue>
#include <unordered_map>
#include <utility>
#include <vector>

#include "mnf/common/diagnostic.h"

namespace mnf {

namespace {

const ModuleDecl* FindTopModule(const Program& program, const std::string& top_name) {
  for (const auto& module : program.modules) {
    if (module->name == top_name) {
      return module.get();
    }
  }
  return nullptr;
}

std::vector<ModuleDependencyIR> CollectDependencies(const Program& program) {
  std::vector<ModuleDependencyIR> dependencies;
  for (const auto& module : program.modules) {
    for (const auto& instance : module->instances) {
      dependencies.push_back(ModuleDependencyIR{module->name, instance.module_name});
    }
  }
  return dependencies;
}

std::vector<std::string> ComputeModuleOrder(const Program& program,
                                            const std::vector<ModuleDependencyIR>& dependencies,
                                            bool* has_cycle) {
  std::unordered_map<std::string, int> indegree;
  std::unordered_map<std::string, std::vector<std::string>> graph;

  for (const auto& module : program.modules) {
    indegree.emplace(module->name, 0);
    graph.emplace(module->name, std::vector<std::string>{});
  }

  for (const auto& dependency : dependencies) {
    graph[dependency.from_module].push_back(dependency.to_module);
    ++indegree[dependency.to_module];
  }

  std::queue<std::string> ready;
  for (const auto& [name, degree] : indegree) {
    if (degree == 0) {
      ready.push(name);
    }
  }

  std::vector<std::string> order;
  while (!ready.empty()) {
    std::string name = ready.front();
    ready.pop();
    order.push_back(name);

    for (const auto& next : graph[name]) {
      --indegree[next];
      if (indegree[next] == 0) {
        ready.push(next);
      }
    }
  }

  *has_cycle = order.size() != program.modules.size();
  if (*has_cycle) {
    order.clear();
  }
  return order;
}

void CollectExpressionSourceNets(const Expression& expr,
                                 const std::unordered_map<std::string, int>& net_ids,
                                 std::vector<int>& source_net_ids) {
  if (expr.kind == Expression::Kind::Identifier) {
    const auto it = net_ids.find(expr.text);
    if (it != net_ids.end()) {
      source_net_ids.push_back(it->second);
    }
    return;
  }

  if (expr.kind == Expression::Kind::Binary) {
    if (expr.lhs != nullptr) {
      CollectExpressionSourceNets(*expr.lhs, net_ids, source_net_ids);
    }
    if (expr.rhs != nullptr) {
      CollectExpressionSourceNets(*expr.rhs, net_ids, source_net_ids);
    }
  }
}

ResolvedNetGraphIR BuildTopGraph(const ModuleDecl& top_module) {
  ResolvedNetGraphIR graph;
  std::unordered_map<std::string, int> net_ids;

  int next_id = 0;
  for (const auto& port_name : top_module.ports) {
    graph.nets.push_back(ResolvedNetIR{next_id, port_name, ResolvedNetIR::Kind::Port});
    net_ids.emplace(port_name, next_id);
    ++next_id;
  }

  for (const auto& wire_decl : top_module.wire_decls) {
    for (const auto& wire_name : wire_decl.names) {
      if (net_ids.find(wire_name) != net_ids.end()) {
        continue;
      }
      graph.nets.push_back(ResolvedNetIR{next_id, wire_name, ResolvedNetIR::Kind::Wire});
      net_ids.emplace(wire_name, next_id);
      ++next_id;
    }
  }

  for (const auto& assign_stmt : top_module.assign_stmts) {
    ResolvedAssignIR resolved_assign;
    const auto lhs_it = net_ids.find(assign_stmt.lhs);
    if (lhs_it != net_ids.end()) {
      resolved_assign.target_net_id = lhs_it->second;
    }
    resolved_assign.expr_op = assign_stmt.rhs.kind == Expression::Kind::Binary ? assign_stmt.rhs.text : "";
    CollectExpressionSourceNets(assign_stmt.rhs, net_ids, resolved_assign.source_net_ids);
    graph.assigns.push_back(std::move(resolved_assign));
  }

  for (const auto& instance : top_module.instances) {
    for (const auto& connection : instance.connections) {
      ResolvedInstanceBindingIR binding;
      binding.instance_name = instance.instance_name;
      binding.module_name = instance.module_name;
      binding.port_name = connection.port_name;
      const auto signal_it = net_ids.find(connection.signal_name);
      if (signal_it != net_ids.end()) {
        binding.signal_net_id = signal_it->second;
      }
      graph.instance_bindings.push_back(std::move(binding));
    }
  }

  return graph;
}

}  // namespace

Result<ElaboratedDesign> Elaborator::Elaborate(const Program& program,
                                               const SymbolTable& symbols,
                                               const std::string& top_name) const {
  ElaboratedDesign design;
  design.top_name = top_name;

  for (const auto& module : program.modules) {
    design.modules.push_back(ModuleIR{module->name, module->ports});
  }

  design.module_dependencies = CollectDependencies(program);

  bool has_cycle = false;
  design.module_order = ComputeModuleOrder(program, design.module_dependencies, &has_cycle);
  if (has_cycle) {
    return Result<ElaboratedDesign>{std::nullopt,
                                    {Diagnostic{DiagnosticLevel::Error,
                                                "Module dependency cycle detected during elaboration",
                                                {"", 1, 1}}}};
  }

  const ModuleDecl* top_module = FindTopModule(program, top_name);
  if (top_module == nullptr) {
    return Result<ElaboratedDesign>{std::nullopt,
                                    {Diagnostic{DiagnosticLevel::Error,
                                                "Top module not found: " + top_name,
                                                {"", 1, 1}}}};
  }

  design.top_graph = BuildTopGraph(*top_module);

  std::vector<Diagnostic> diagnostics;
  for (const auto& instance : top_module->instances) {
    auto instance_result = ElaborateInstance(instance, symbols);
    if (!instance_result.Ok()) {
      diagnostics.insert(diagnostics.end(), instance_result.diagnostics.begin(), instance_result.diagnostics.end());
      continue;
    }
    design.top_instances.push_back(std::move(*instance_result.value));
  }

  if (!diagnostics.empty()) {
    return Result<ElaboratedDesign>{std::nullopt, diagnostics};
  }

  return Result<ElaboratedDesign>{std::move(design), {}};
}

Result<InstanceIR> Elaborator::ElaborateInstance(const InstanceDecl& instance,
                                                 const SymbolTable& symbols) const {
  InstanceIR ir;
  ir.module_name = instance.module_name;
  ir.instance_name = instance.instance_name;
  for (const auto& connection : instance.connections) {
    ir.connections.push_back(ConnectionIR{connection.port_name, connection.signal_name});
  }

  const ModuleDecl* referenced_module = symbols.FindModule(instance.module_name);
  if (referenced_module == nullptr) {
    return Result<InstanceIR>{std::nullopt,
                              {Diagnostic{DiagnosticLevel::Error,
                                          "Cannot elaborate undefined module: " + instance.module_name,
                                          instance.location}}};
  }

  std::vector<Diagnostic> diagnostics;
  for (const auto& child_instance : referenced_module->instances) {
    auto child_result = ElaborateInstance(child_instance, symbols);
    if (!child_result.Ok()) {
      diagnostics.insert(diagnostics.end(), child_result.diagnostics.begin(), child_result.diagnostics.end());
      continue;
    }
    ir.children.push_back(std::move(*child_result.value));
  }

  if (!diagnostics.empty()) {
    return Result<InstanceIR>{std::nullopt, diagnostics};
  }

  return Result<InstanceIR>{std::move(ir), {}};
}

}  // namespace mnf
