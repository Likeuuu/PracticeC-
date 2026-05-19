#pragma once

#include <memory>
#include <string>
#include <vector>

namespace mnf {

struct ConnectionIR {
  std::string port_name;
  std::string signal_name;
};

struct InstanceIR {
  std::string module_name;
  std::string instance_name;
  std::vector<ConnectionIR> connections;
  std::vector<InstanceIR> children;
};

struct ModuleIR {
  std::string name;
  std::vector<std::string> ports;
};

struct ModuleDependencyIR {
  std::string from_module;
  std::string to_module;
};

struct ResolvedNetIR {
  enum class Kind {
    Port,
    Wire,
    Reg
  };

  int id = -1;
  std::string name;
  std::string qualified_name;
  Kind kind = Kind::Wire;
};

struct ResolvedExprIR {
  enum class Kind {
    Net,
    Constant,
    Unary,
    Binary
  };

  Kind kind = Kind::Net;
  int net_id = -1;
  int constant_value = 0;
  std::string op;
  std::unique_ptr<ResolvedExprIR> lhs;
  std::unique_ptr<ResolvedExprIR> rhs;

  ResolvedExprIR() = default;

  ResolvedExprIR(const ResolvedExprIR& other)
      : kind(other.kind), net_id(other.net_id), constant_value(other.constant_value), op(other.op) {
    if (other.lhs != nullptr) {
      lhs = std::make_unique<ResolvedExprIR>(*other.lhs);
    }
    if (other.rhs != nullptr) {
      rhs = std::make_unique<ResolvedExprIR>(*other.rhs);
    }
  }

  ResolvedExprIR& operator=(const ResolvedExprIR& other) {
    if (this == &other) {
      return *this;
    }

    kind = other.kind;
    net_id = other.net_id;
    constant_value = other.constant_value;
    op = other.op;
    lhs = other.lhs != nullptr ? std::make_unique<ResolvedExprIR>(*other.lhs) : nullptr;
    rhs = other.rhs != nullptr ? std::make_unique<ResolvedExprIR>(*other.rhs) : nullptr;
    return *this;
  }

  ResolvedExprIR(ResolvedExprIR&&) noexcept = default;
  ResolvedExprIR& operator=(ResolvedExprIR&&) noexcept = default;
};

struct ResolvedAssignIR {
  std::string instance_path;
  int target_net_id = -1;
  std::string target_name_view;
  ResolvedExprIR rhs_expr;
  std::vector<int> source_net_ids;
};

struct ResolvedProceduralAssignIR {
  enum class AssignmentKind {
    Blocking,
    NonBlocking
  };

  AssignmentKind assignment_kind = AssignmentKind::Blocking;
  int target_net_id = -1;
  std::string target_name_view;
  ResolvedExprIR rhs_expr;
  std::vector<int> source_net_ids;
};

struct ResolvedIfIR;

struct ResolvedProcessStmtIR {
  enum class Kind {
    Assignment,
    Block,
    If
  };

  Kind kind = Kind::Assignment;
  std::unique_ptr<ResolvedProceduralAssignIR> assign_stmt;
  std::unique_ptr<ResolvedIfIR> if_stmt;
  std::vector<std::unique_ptr<ResolvedProcessStmtIR>> statements;

  ResolvedProcessStmtIR() = default;

  ResolvedProcessStmtIR(const ResolvedProcessStmtIR& other) : kind(other.kind) {
    if (other.assign_stmt != nullptr) {
      assign_stmt = std::make_unique<ResolvedProceduralAssignIR>(*other.assign_stmt);
    }
    if (other.if_stmt != nullptr) {
      if_stmt = std::make_unique<ResolvedIfIR>(*other.if_stmt);
    }
    for (const auto& stmt : other.statements) {
      statements.push_back(stmt != nullptr ? std::make_unique<ResolvedProcessStmtIR>(*stmt) : nullptr);
    }
  }

  ResolvedProcessStmtIR& operator=(const ResolvedProcessStmtIR& other) {
    if (this == &other) {
      return *this;
    }

    kind = other.kind;
    assign_stmt = other.assign_stmt != nullptr ? std::make_unique<ResolvedProceduralAssignIR>(*other.assign_stmt) : nullptr;
    if_stmt = other.if_stmt != nullptr ? std::make_unique<ResolvedIfIR>(*other.if_stmt) : nullptr;
    statements.clear();
    for (const auto& stmt : other.statements) {
      statements.push_back(stmt != nullptr ? std::make_unique<ResolvedProcessStmtIR>(*stmt) : nullptr);
    }
    return *this;
  }

  ResolvedProcessStmtIR(ResolvedProcessStmtIR&&) noexcept = default;
  ResolvedProcessStmtIR& operator=(ResolvedProcessStmtIR&&) noexcept = default;
};

struct ResolvedIfIR {
  ResolvedExprIR condition_expr;
  std::vector<int> condition_source_net_ids;
  std::unique_ptr<ResolvedProcessStmtIR> then_stmt;

  ResolvedIfIR() = default;

  ResolvedIfIR(const ResolvedIfIR& other)
      : condition_expr(other.condition_expr), condition_source_net_ids(other.condition_source_net_ids) {
    if (other.then_stmt != nullptr) {
      then_stmt = std::make_unique<ResolvedProcessStmtIR>(*other.then_stmt);
    }
  }

  ResolvedIfIR& operator=(const ResolvedIfIR& other) {
    if (this == &other) {
      return *this;
    }

    condition_expr = other.condition_expr;
    condition_source_net_ids = other.condition_source_net_ids;
    then_stmt = other.then_stmt != nullptr ? std::make_unique<ResolvedProcessStmtIR>(*other.then_stmt) : nullptr;
    return *this;
  }

  ResolvedIfIR(ResolvedIfIR&&) noexcept = default;
  ResolvedIfIR& operator=(ResolvedIfIR&&) noexcept = default;
};

struct ResolvedAlwaysIR {
  enum class SensitivityKind {
    Implicit,
    CombinationalStar,
    ExplicitList
  };

  std::string instance_path;
  SensitivityKind sensitivity_kind = SensitivityKind::Implicit;
  std::vector<int> sensitivity_net_ids;
  std::vector<std::string> sensitivity_name_views;
  std::unique_ptr<ResolvedProcessStmtIR> body;

  ResolvedAlwaysIR() = default;

  ResolvedAlwaysIR(const ResolvedAlwaysIR& other)
      : instance_path(other.instance_path),
        sensitivity_kind(other.sensitivity_kind),
        sensitivity_net_ids(other.sensitivity_net_ids),
        sensitivity_name_views(other.sensitivity_name_views) {
    if (other.body != nullptr) {
      body = std::make_unique<ResolvedProcessStmtIR>(*other.body);
    }
  }

  ResolvedAlwaysIR& operator=(const ResolvedAlwaysIR& other) {
    if (this == &other) {
      return *this;
    }

    instance_path = other.instance_path;
    sensitivity_kind = other.sensitivity_kind;
    sensitivity_net_ids = other.sensitivity_net_ids;
    sensitivity_name_views = other.sensitivity_name_views;
    body = other.body != nullptr ? std::make_unique<ResolvedProcessStmtIR>(*other.body) : nullptr;
    return *this;
  }

  ResolvedAlwaysIR(ResolvedAlwaysIR&&) noexcept = default;
  ResolvedAlwaysIR& operator=(ResolvedAlwaysIR&&) noexcept = default;
};

struct ResolvedInstanceBindingIR {
  std::string instance_path;
  std::string module_name;
  std::string port_name;
  int signal_net_id = -1;
};

struct ResolvedScopeSymbolIR {
  enum class Kind {
    Port,
    Wire,
    Reg
  };

  std::string name;
  std::string decl_name;
  std::string qualified_name;
  Kind kind = Kind::Wire;
  int net_id = -1;
};

struct ResolvedScopeFrameIR {
  std::string instance_path;
  std::string module_name;
  std::vector<ResolvedScopeSymbolIR> symbols;
};

struct ResolvedNetGraphIR {
  std::vector<ResolvedNetIR> nets;
  std::vector<ResolvedAssignIR> assigns;
  std::vector<ResolvedAlwaysIR> always_blocks;
  std::vector<ResolvedInstanceBindingIR> instance_bindings;
  std::vector<ResolvedScopeFrameIR> scope_frames;
};

struct ElaboratedDesign {
  std::string top_name;
  std::vector<ModuleIR> modules;
  std::vector<ModuleDependencyIR> module_dependencies;
  std::vector<std::string> module_order;
  std::vector<InstanceIR> top_instances;
  ResolvedNetGraphIR top_graph;
};

}  // namespace mnf
