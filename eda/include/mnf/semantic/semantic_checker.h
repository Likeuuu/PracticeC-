#pragma once

#include <vector>

#include "mnf/common/diagnostic.h"
#include "mnf/parser/ast.h"
#include "mnf/semantic/symbol_table.h"

namespace mnf {

class SemanticChecker {
public:
  std::vector<Diagnostic> Check(const Program& program, SymbolTable& symbols) const;

private:
  void CheckModule(const ModuleDecl& module,
                   const SymbolTable& symbols,
                   std::vector<Diagnostic>& diagnostics) const;
};

}  // namespace mnf
