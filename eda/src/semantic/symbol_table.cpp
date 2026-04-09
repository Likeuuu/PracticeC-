#include "mnf/semantic/symbol_table.h"

namespace mnf {

bool SymbolTable::AddModule(const ModuleDecl* module) {
  return modules_.emplace(module->name, module).second;
}

const ModuleDecl* SymbolTable::FindModule(const std::string& name) const {
  const auto it = modules_.find(name);
  if (it == modules_.end()) {
    return nullptr;
  }
  return it->second;
}

}  // namespace mnf
