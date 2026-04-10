#include <cassert>
#include <string>

#include "mnf/elaboration/elaborator.h"
#include "mnf/lexer/lexer.h"
#include "mnf/parser/parser.h"
#include "mnf/semantic/semantic_checker.h"
#include "mnf/semantic/symbol_table.h"

int main() {
  const std::string input = R"(module leaf(a, y);
  input a;
  output y;
endmodule

module top(in1, out1);
  input in1;
  output out1;
  leaf u1(.a(in1), .y(out1));
endmodule
)";

  mnf::Lexer lexer(input, "elaborator_test.nl");
  mnf::Parser parser(lexer);
  auto parse_result = parser.ParseProgram();
  assert(parse_result.Ok());

  mnf::SymbolTable symbols;
  mnf::SemanticChecker checker;
  const auto diagnostics = checker.Check(*parse_result.value, symbols);
  assert(diagnostics.empty());

  mnf::Elaborator elaborator;
  auto design_result = elaborator.Elaborate(*parse_result.value, symbols, "top");
  assert(design_result.Ok());
  assert(design_result.value->top_name == "top");
  assert(design_result.value->modules.size() == 2);

  return 0;
}
