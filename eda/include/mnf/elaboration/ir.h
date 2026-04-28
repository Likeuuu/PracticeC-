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
    Wire
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
  ResolvedExprIR rhs_expr;
};

struct ResolvedInstanceBindingIR {
  std::string instance_path;
  std::string module_name;
  std::string port_name;
  int signal_net_id = -1;
};

struct ResolvedNetGraphIR {
  std::vector<ResolvedNetIR> nets;
  std::vector<ResolvedAssignIR> assigns;
  std::vector<ResolvedInstanceBindingIR> instance_bindings;
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
