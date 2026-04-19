#include <gtest/gtest.h>

#include <string>

#include "mnf/lexer/lexer.h"
#include "mnf/parser/parser.h"
#include "mnf/semantic/semantic_checker.h"
#include "mnf/semantic/symbol_table.h"

TEST(SemanticCheckerTest, ReportsPortHeaderAndDeclarationMismatch) {
  const std::string input = R"(module leaf(a, y);
  input a;
  output y;
endmodule

module top(in1, out1, missing_decl);
  input in1;
  output out1;
  input extra_decl;
  leaf u1(.a(in1), .y(out1));
endmodule
)";

  mnf::Lexer lexer(input, "semantic_checker_port_consistency_test.nl");
  mnf::Parser parser(lexer);
  auto parse_result = parser.ParseProgram();
  ASSERT_TRUE(parse_result.Ok());

  mnf::SymbolTable symbols;
  mnf::SemanticChecker checker;
  const auto diagnostics = checker.Check(*parse_result.value, symbols);
  ASSERT_FALSE(diagnostics.empty());

  bool saw_missing_decl = false;
  bool saw_extra_decl = false;
  for (const auto& diagnostic : diagnostics) {
    if (diagnostic.message.find("missing input/output declaration") != std::string::npos) {
      saw_missing_decl = true;
    }
    if (diagnostic.message.find("Declared port not listed in module header") != std::string::npos) {
      saw_extra_decl = true;
    }
  }

  EXPECT_TRUE(saw_missing_decl);
  EXPECT_TRUE(saw_extra_decl);
}
