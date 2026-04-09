#include "mnf/elaboration/elaborator.h"

namespace mnf {

Result<ElaboratedDesign> Elaborator::Elaborate(const Program& program,
                                               const SymbolTable& symbols,
                                               const std::string& top_name) const {
  (void)symbols;

  ElaboratedDesign design;
  design.top_name = top_name;

  for (const auto& module : program.modules) {
    design.modules.push_back(ModuleIR{module->name});
  }

  return Result<ElaboratedDesign>{design, {}};
}

Result<InstanceIR> Elaborator::ElaborateInstance(const InstanceDecl& instance,
                                                 const SymbolTable& symbols) const {
  (void)symbols;

  InstanceIR ir;
  ir.module_name = instance.module_name;
  ir.instance_name = instance.instance_name;
  for (const auto& connection : instance.connections) {
    ir.connections.push_back(ConnectionIR{connection.port_name, connection.signal_name});
  }

  return Result<InstanceIR>{ir, {}};
}

}  // namespace mnf
