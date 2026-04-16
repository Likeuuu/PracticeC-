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
                                    {Diagnostic{DiagnosticLevel::Error, "Top module not found: " + top_name, {"", 1, 1}}}};
  }

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
