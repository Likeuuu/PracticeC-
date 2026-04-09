#pragma once

#include <string>

#include "mnf/common/result.h"
#include "mnf/elaboration/ir.h"
#include "mnf/parser/ast.h"
#include "mnf/semantic/symbol_table.h"

namespace mnf {

class Elaborator {
public:
  Result<ElaboratedDesign> Elaborate(const Program& program,
                                     const SymbolTable& symbols,
                                     const std::string& top_name) const;

private:
  Result<InstanceIR> ElaborateInstance(const InstanceDecl& instance,
                                       const SymbolTable& symbols) const;
};

}  // namespace mnf
