#include <gtest/gtest.h>

#include <string>

#include "mnf/elaboration/elaborator.h"
#include "mnf/lexer/lexer.h"
#include "mnf/parser/parser.h"
#include "mnf/semantic/semantic_checker.h"
#include "mnf/semantic/symbol_table.h"

TEST(ElaboratorTest, RejectsModuleDependencyCycles) {
  const std::string input = R"(module a(in1, out1);
  input in1;
  output out1;
  b u_b(.in1(in1), .out1(out1));
endmodule

module b(in1, out1);
  input in1;
  output out1;
  a u_a(.in1(in1), .out1(out1));
endmodule
)";

  mnf::Lexer lexer(input, "elaboration_cycle_test.nl");
  mnf::Parser parser(lexer);
  auto parse_result = parser.ParseProgram();
  ASSERT_TRUE(parse_result.Ok());

  mnf::SymbolTable symbols;
  mnf::SemanticChecker checker;
  const auto diagnostics = checker.Check(*parse_result.value, symbols);
  ASSERT_TRUE(diagnostics.empty());

  mnf::Elaborator elaborator;
  auto design_result = elaborator.Elaborate(*parse_result.value, symbols, "a");
  ASSERT_FALSE(design_result.Ok());
  ASSERT_FALSE(design_result.diagnostics.empty());

  bool saw_cycle = false;
  for (const auto& diagnostic : design_result.diagnostics) {
    if (diagnostic.message.find("cycle") != std::string::npos) {
      saw_cycle = true;
    }
  }
  EXPECT_TRUE(saw_cycle);
}
