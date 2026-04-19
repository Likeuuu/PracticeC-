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
  wire in1;
endmodule
)";

  mnf::Lexer lexer(input, "semantic_checker_wire_port_conflict_test.nl");
  mnf::Parser parser(lexer);
  auto parse_result = parser.ParseProgram();
  assert(parse_result.Ok());

  mnf::SymbolTable symbols;
  mnf::SemanticChecker checker;
  const auto diagnostics = checker.Check(*parse_result.value, symbols);
  assert(!diagnostics.empty());

  bool saw_wire_port_conflict = false;
  for (const auto& diagnostic : diagnostics) {
    if (diagnostic.message.find("Wire name conflicts with module port") != std::string::npos) {
      saw_wire_port_conflict = true;
    }
  }
  assert(saw_wire_port_conflict);

  return 0;
}
