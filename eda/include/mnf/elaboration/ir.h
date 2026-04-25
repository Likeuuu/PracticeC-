#pragma once

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

struct ResolvedAssignIR {
  std::string instance_path;
  int target_net_id = -1;
  std::vector<int> source_net_ids;
  std::string expr_op;
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
