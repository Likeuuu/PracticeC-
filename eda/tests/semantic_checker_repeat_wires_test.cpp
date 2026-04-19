#include <cassert>
#include <string>

#include "mnf/lexer/lexer.h"
#include "mnf/parser/parser.h"
#include "mnf/semantic/semantic_checker.h"
#include "mnf/semantic/symbol_table.h"

int main() {
  const std::string input = R"(module top(in1, out1);
  input in1;
  output out1;
  wire mid, mid;
endmodule
)";

  mnf::Lexer lexer(input, "semantic_checker_repeat_wires_test.nl");
  mnf::Parser parser(lexer);
  auto parse_result = parser.ParseProgram();
  assert(parse_result.Ok());

  mnf::SymbolTable symbols;
  mnf::SemanticChecker checker;
  const auto diagnostics = checker.Check(*parse_result.value, symbols);
  assert(!diagnostics.empty());

  bool saw_duplicate_wire = false;
  for (const auto& diagnostic : diagnostics) {
    if (diagnostic.message.find("Duplicate wire declaration") != std::string::npos) {
      saw_duplicate_wire = true;
    }
  }
  assert(saw_duplicate_wire);

  return 0;
}
