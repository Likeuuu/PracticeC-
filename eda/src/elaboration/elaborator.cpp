#include "mnf/elaboration/elaborator.h"

#include <algorithm>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "mnf/common/diagnostic.h"

namespace mnf {

namespace {

struct ScopeSymbol {
  enum class Kind {
    Port,
    Wire
  };

  Kind kind = Kind::Wire;
  int net_id = -1;
  std::string decl_name;
  std::string qualified_name;
};

struct ScopeFrame {
  std::string instance_path;
  const ModuleDecl* module = nullptr;
  const ScopeFrame* parent = nullptr;
  std::unordered_map<std::string, ScopeSymbol> local_symbols;

  bool BindLocal(const std::string& name,
                 ScopeSymbol::Kind kind,
                 int net_id,
                 const std::string& decl_name,
                 const std::string& qualified_name) {
    return local_symbols.emplace(name, ScopeSymbol{kind, net_id, decl_name, qualified_name}).second;
  }

  bool ContainsLocal(const std::string& name) const {
    return local_symbols.find(name) != local_symbols.end();
  }

  bool LookupSymbol(const std::string& name, ScopeSymbol* symbol) const {
    const auto it = local_symbols.find(name);
    if (it != local_symbols.end()) {
      *symbol = it->second;
      return true;
    }
    if (parent != nullptr) {
      return parent->LookupSymbol(name, symbol);
    }
    return false;
  }

  bool LookupNetId(const std::string& name, int* net_id) const {
    ScopeSymbol symbol;
    if (!LookupSymbol(name, &symbol)) {
      return false;
    }
    *net_id = symbol.net_id;
    return true;
  }
};

ResolvedScopeSymbolIR::Kind ToResolvedScopeKind(ScopeSymbol::Kind kind) {
  switch (kind) {
    case ScopeSymbol::Kind::Port:
      return ResolvedScopeSymbolIR::Kind::Port;
    case ScopeSymbol::Kind::Wire:
    default:
      return ResolvedScopeSymbolIR::Kind::Wire;
  }
}

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

void CollectSourceNetIdsFromExpr(const ResolvedExprIR& expr, std::vector<int>* source_net_ids) {
  switch (expr.kind) {
    case ResolvedExprIR::Kind::Net:
      if (expr.net_id >= 0) {
        source_net_ids->push_back(expr.net_id);
      }
      return;
    case ResolvedExprIR::Kind::Constant:
      return;
    case ResolvedExprIR::Kind::Unary:
      if (expr.rhs != nullptr) {
        CollectSourceNetIdsFromExpr(*expr.rhs, source_net_ids);
      }
      return;
    case ResolvedExprIR::Kind::Binary:
      if (expr.lhs != nullptr) {
        CollectSourceNetIdsFromExpr(*expr.lhs, source_net_ids);
      }
      if (expr.rhs != nullptr) {
        CollectSourceNetIdsFromExpr(*expr.rhs, source_net_ids);
      }
      return;
  }
}

std::vector<int> BuildSourceNetIds(const ResolvedExprIR& expr) {
  std::vector<int> source_net_ids;
  CollectSourceNetIdsFromExpr(expr, &source_net_ids);

  std::unordered_set<int> seen;
  std::vector<int> unique_ids;
  unique_ids.reserve(source_net_ids.size());
  for (const int net_id : source_net_ids) {
    if (seen.insert(net_id).second) {
      unique_ids.push_back(net_id);
    }
  }
  return unique_ids;
}

ResolvedExprIR ResolveExpression(const Expression& expr, const ScopeFrame& scope) {
  ResolvedExprIR resolved_expr;

  if (expr.kind == Expression::Kind::Identifier) {
    resolved_expr.kind = ResolvedExprIR::Kind::Net;
    ScopeSymbol symbol;
    if (scope.LookupSymbol(expr.text, &symbol)) {
      resolved_expr.net_id = symbol.net_id;
    }
    return resolved_expr;
  }

  if (expr.kind == Expression::Kind::Number) {
    resolved_expr.kind = ResolvedExprIR::Kind::Constant;
    resolved_expr.constant_value = std::stoi(expr.text);
    return resolved_expr;
  }

  if (expr.kind == Expression::Kind::Unary) {
    resolved_expr.kind = ResolvedExprIR::Kind::Unary;
    resolved_expr.op = expr.text;
    if (expr.rhs != nullptr) {
      resolved_expr.rhs = std::make_unique<ResolvedExprIR>(ResolveExpression(*expr.rhs, scope));
    }
    return resolved_expr;
  }

  resolved_expr.kind = ResolvedExprIR::Kind::Binary;
  resolved_expr.op = expr.text;
  if (expr.lhs != nullptr) {
    resolved_expr.lhs = std::make_unique<ResolvedExprIR>(ResolveExpression(*expr.lhs, scope));
  }
  if (expr.rhs != nullptr) {
    resolved_expr.rhs = std::make_unique<ResolvedExprIR>(ResolveExpression(*expr.rhs, scope));
  }
  return resolved_expr;
}

std::string JoinPath(const std::string& instance_path, const std::string& local_name) {
  return instance_path.empty() ? local_name : instance_path + "." + local_name;
}

void CaptureScopeFrame(const ScopeFrame& scope, ResolvedNetGraphIR* graph) {
  ResolvedScopeFrameIR frame_ir;
  frame_ir.instance_path = scope.instance_path;
  frame_ir.module_name = scope.module != nullptr ? scope.module->name : "";

  std::vector<std::pair<std::string, ScopeSymbol>> ordered_symbols(scope.local_symbols.begin(), scope.local_symbols.end());
  std::sort(ordered_symbols.begin(), ordered_symbols.end(), [](const auto& lhs, const auto& rhs) {
    return lhs.first < rhs.first;
  });

  for (const auto& [name, symbol] : ordered_symbols) {
    frame_ir.symbols.push_back(ResolvedScopeSymbolIR{
        name,
        symbol.decl_name,
        symbol.qualified_name,
        ToResolvedScopeKind(symbol.kind),
        symbol.net_id});
  }

  graph->scope_frames.push_back(std::move(frame_ir));
}

void BindTopPorts(const ModuleDecl& top_module,
                  ScopeFrame* top_scope,
                  int* next_net_id,
                  ResolvedNetGraphIR* graph) {
  for (const auto& port_name : top_module.ports) {
    top_scope->BindLocal(port_name, ScopeSymbol::Kind::Port, *next_net_id, port_name, port_name);
    graph->nets.push_back(ResolvedNetIR{*next_net_id, port_name, port_name, ResolvedNetIR::Kind::Port});
    ++(*next_net_id);
  }
}

void AddLocalWiresToScope(const ModuleDecl& module,
                          const std::string& instance_path,
                          int* next_net_id,
                          ScopeFrame* scope,
                          ResolvedNetGraphIR* graph) {
  for (const auto& wire_decl : module.wire_decls) {
    for (const auto& wire_name : wire_decl.names) {
      if (scope->ContainsLocal(wire_name)) {
        continue;
      }
      const int net_id = (*next_net_id)++;
      const std::string qualified_name = JoinPath(instance_path, wire_name);
      scope->BindLocal(wire_name, ScopeSymbol::Kind::Wire, net_id, wire_name, qualified_name);
      graph->nets.push_back(ResolvedNetIR{net_id, wire_name, qualified_name, ResolvedNetIR::Kind::Wire});
    }
  }
}

void BuildResolvedGraphRecursive(const SymbolTable& symbols,
                                 ScopeFrame* scope,
                                 int* next_net_id,
                                 ResolvedNetGraphIR* graph) {
  AddLocalWiresToScope(*scope->module, scope->instance_path, next_net_id, scope, graph);
  CaptureScopeFrame(*scope, graph);

  for (const auto& assign_stmt : scope->module->assign_stmts) {
    ResolvedAssignIR resolved_assign;
    resolved_assign.instance_path = scope->instance_path;
    ScopeSymbol target_symbol;
    if (scope->LookupSymbol(assign_stmt.lhs, &target_symbol)) {
      resolved_assign.target_net_id = target_symbol.net_id;
      resolved_assign.target_name_view = target_symbol.qualified_name;
    }
    resolved_assign.rhs_expr = ResolveExpression(assign_stmt.rhs, *scope);
    resolved_assign.source_net_ids = BuildSourceNetIds(resolved_assign.rhs_expr);
    graph->assigns.push_back(std::move(resolved_assign));
  }

  for (const auto& instance : scope->module->instances) {
    const ModuleDecl* referenced_module = symbols.FindModule(instance.module_name);
    if (referenced_module == nullptr) {
      continue;
    }

    ScopeFrame child_scope;
    child_scope.instance_path = JoinPath(scope->instance_path, instance.instance_name);
    child_scope.module = referenced_module;
    child_scope.parent = scope;

    for (const auto& connection : instance.connections) {
      int signal_net_id = -1;
      if (!scope->LookupNetId(connection.signal_name, &signal_net_id)) {
        continue;
      }

      child_scope.BindLocal(connection.port_name,
                            ScopeSymbol::Kind::Port,
                            signal_net_id,
                            connection.port_name,
                            JoinPath(child_scope.instance_path, connection.port_name));
      graph->instance_bindings.push_back(ResolvedInstanceBindingIR{
          child_scope.instance_path,
          instance.module_name,
          connection.port_name,
          signal_net_id});
    }

    BuildResolvedGraphRecursive(symbols, &child_scope, next_net_id, graph);
  }
}

ResolvedNetGraphIR BuildResolvedGraph(const SymbolTable& symbols, const ModuleDecl& top_module) {
  ResolvedNetGraphIR graph;
  ScopeFrame top_scope;
  top_scope.module = &top_module;

  int next_net_id = 0;
  BindTopPorts(top_module, &top_scope, &next_net_id, &graph);
  BuildResolvedGraphRecursive(symbols, &top_scope, &next_net_id, &graph);
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

  design.top_graph = BuildResolvedGraph(symbols, *top_module);

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
