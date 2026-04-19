#include <gtest/gtest.h>

#include <string>

#include "mnf/lexer/lexer.h"
#include "mnf/parser/parser.h"
#include "mnf/semantic/semantic_checker.h"
#include "mnf/semantic/symbol_table.h"

TEST(SemanticCheckerTest, ReportsDuplicatePortsInModuleHeader) {
  const std::string input = R"(module leaf(a, a, y);
  input a;
  output y;
endmodule

module top(in1, in1, out1);
  input in1;
  output out1;
  leaf u1(.a(in1), .y(out1));
endmodule
)";

  mnf::Lexer lexer(input, "semantic_checker_repeat_ports.nl");
  mnf::Parser parser(lexer);
  auto parse_result = parser.ParseProgram();
  ASSERT_TRUE(parse_result.Ok());

  mnf::SymbolTable symbols;
  mnf::SemanticChecker checker;
  const auto diagnostics = checker.Check(*parse_result.value, symbols);
  ASSERT_FALSE(diagnostics.empty());

  bool saw_duplicate_header_port = false;
  for (const auto& diagnostic : diagnostics) {
    if (diagnostic.message.find("Duplicate port definition in module header") != std::string::npos) {
      saw_duplicate_header_port = true;
    }
  }
  EXPECT_TRUE(saw_duplicate_header_port);
}
