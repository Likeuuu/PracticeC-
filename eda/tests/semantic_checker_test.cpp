#include <cassert>
#include <string>

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
  leaf u1(.a(in1), .missing(out1));
endmodule
)";

  mnf::Lexer lexer(input, "semantic_checker_test.nl");
  mnf::Parser parser(lexer);
  auto parse_result = parser.ParseProgram();
  assert(parse_result.Ok());

  mnf::SymbolTable symbols;
  mnf::SemanticChecker checker;
  const auto diagnostics = checker.Check(*parse_result.value, symbols);
  assert(!diagnostics.empty());

  bool saw_unknown_port = false;
  for (const auto& diagnostic : diagnostics) {
    if (diagnostic.message.find("Unknown port") != std::string::npos) {
      saw_unknown_port = true;
    }
  }
  assert(saw_unknown_port);

  return 0;
}
