#pragma once

#include <string>
#include <unordered_map>

#include "mnf/parser/ast.h"

namespace mnf {

class SymbolTable {
public:
  bool AddModule(const ModuleDecl* module);
  const ModuleDecl* FindModule(const std::string& name) const;

private:
  std::unordered_map<std::string, const ModuleDecl*> modules_;
};

}  // namespace mnf
