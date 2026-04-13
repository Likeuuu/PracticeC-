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

struct ElaboratedDesign {
  std::string top_name;
  std::vector<ModuleIR> modules;
  std::vector<InstanceIR> top_instances;
};

}  // namespace mnf
