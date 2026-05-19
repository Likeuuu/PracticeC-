#include <gtest/gtest.h>

#include <string>

#include "mnf/lexer/lexer.h"
#include "mnf/parser/parser.h"
#include "mnf/semantic/semantic_checker.h"
#include "mnf/semantic/symbol_table.h"

TEST(SemanticCheckerTest, ReportsUnknownPortsOnInstances) {
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
  ASSERT_TRUE(parse_result.Ok());

  mnf::SymbolTable symbols;
  mnf::SemanticChecker checker;
  const auto diagnostics = checker.Check(*parse_result.value, symbols);
  ASSERT_FALSE(diagnostics.empty());

  bool saw_unknown_port = false;
  for (const auto& diagnostic : diagnostics) {
    if (diagnostic.message.find("Unknown port") != std::string::npos) {
      saw_unknown_port = true;
    }
  }
  EXPECT_TRUE(saw_unknown_port);
}

TEST(SemanticCheckerTest, ReportsUndeclaredSignalsInAssignStatements) {
  const std::string input = R"(module top(in1, in2, out1);
  input in1;
  input in2;
  output out1;
  assign missing_lhs = in1;
  assign out1 = in1 & missing_rhs;
endmodule
)";

  mnf::Lexer lexer(input, "semantic_assign_test.nl");
  mnf::Parser parser(lexer);
  auto parse_result = parser.ParseProgram();
  ASSERT_TRUE(parse_result.Ok());

  mnf::SymbolTable symbols;
  mnf::SemanticChecker checker;
  const auto diagnostics = checker.Check(*parse_result.value, symbols);
  ASSERT_FALSE(diagnostics.empty());

  bool saw_missing_lhs = false;
  bool saw_missing_rhs = false;
  for (const auto& diagnostic : diagnostics) {
    if (diagnostic.message.find("Assign target is not declared") != std::string::npos) {
      saw_missing_lhs = true;
    }
    if (diagnostic.message.find("Assign source is not declared") != std::string::npos) {
      saw_missing_rhs = true;
    }
  }
  EXPECT_TRUE(saw_missing_lhs);
  EXPECT_TRUE(saw_missing_rhs);
}

TEST(SemanticCheckerTest, ReportsUndeclaredSignalsInProceduralStatements) {
  const std::string input = R"(module top(in1, out1);
  input in1;
  output out1;
  reg state;
  always begin
    if (missing_cond) state <= in1;
    missing_lhs = state;
    out1 = missing_rhs;
  end
endmodule
)";

  mnf::Lexer lexer(input, "semantic_procedural_test.nl");
  mnf::Parser parser(lexer);
  auto parse_result = parser.ParseProgram();
  ASSERT_TRUE(parse_result.Ok());

  mnf::SymbolTable symbols;
  mnf::SemanticChecker checker;
  const auto diagnostics = checker.Check(*parse_result.value, symbols);
  ASSERT_FALSE(diagnostics.empty());

  bool saw_missing_condition = false;
  bool saw_missing_lhs = false;
  bool saw_missing_rhs = false;
  for (const auto& diagnostic : diagnostics) {
    if (diagnostic.message.find("Procedural if condition signal is not declared") != std::string::npos) {
      saw_missing_condition = true;
    }
    if (diagnostic.message.find("Procedural assignment target is not declared") != std::string::npos) {
      saw_missing_lhs = true;
    }
    if (diagnostic.message.find("Procedural assignment source is not declared") != std::string::npos) {
      saw_missing_rhs = true;
    }
  }
  EXPECT_TRUE(saw_missing_condition);
  EXPECT_TRUE(saw_missing_lhs);
  EXPECT_TRUE(saw_missing_rhs);
}


TEST(SemanticCheckerTest, ReportsExplicitAlwaysSensitivityErrors) {
  const std::string input = R"(module top(in1, out1);
  input in1;
  output out1;
  reg state;
  always @(in1, in1, missing_sig) begin
    state = in1;
  end
endmodule
)";

  mnf::Lexer lexer(input, "semantic_always_sensitivity_test.nl");
  mnf::Parser parser(lexer);
  auto parse_result = parser.ParseProgram();
  ASSERT_TRUE(parse_result.Ok());

  mnf::SymbolTable symbols;
  mnf::SemanticChecker checker;
  const auto diagnostics = checker.Check(*parse_result.value, symbols);
  ASSERT_FALSE(diagnostics.empty());

  bool saw_duplicate = false;
  bool saw_missing = false;
  for (const auto& diagnostic : diagnostics) {
    if (diagnostic.message.find("Duplicate signal in always sensitivity list") != std::string::npos) {
      saw_duplicate = true;
    }
    if (diagnostic.message.find("Always sensitivity signal is not declared") != std::string::npos) {
      saw_missing = true;
    }
  }
  EXPECT_TRUE(saw_duplicate);
  EXPECT_TRUE(saw_missing);
}
