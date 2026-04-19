#include <gtest/gtest.h>

#include <algorithm>
#include <string>

#include "mnf/elaboration/elaborator.h"
#include "mnf/lexer/lexer.h"
#include "mnf/parser/parser.h"
#include "mnf/semantic/semantic_checker.h"
#include "mnf/semantic/symbol_table.h"

TEST(ElaboratorTest, BuildsHierarchyAndModuleDependencyOrder) {
  const std::string input = R"(module leaf(a, y);
  input a;
  output y;
endmodule

module mid(in1, out1);
  input in1;
  output out1;
  leaf u_leaf(.a(in1), .y(out1));
endmodule

module top(in1, out1);
  input in1;
  output out1;
  wire mid_out;
  mid u_mid(.in1(in1), .out1(mid_out));
endmodule
)";

  mnf::Lexer lexer(input, "elaborator_test.nl");
  mnf::Parser parser(lexer);
  auto parse_result = parser.ParseProgram();
  ASSERT_TRUE(parse_result.Ok());

  mnf::SymbolTable symbols;
  mnf::SemanticChecker checker;
  const auto diagnostics = checker.Check(*parse_result.value, symbols);
  ASSERT_TRUE(diagnostics.empty());

  mnf::Elaborator elaborator;
  auto design_result = elaborator.Elaborate(*parse_result.value, symbols, "top");
  ASSERT_TRUE(design_result.Ok());
  EXPECT_EQ(design_result.value->top_name, "top");
  EXPECT_EQ(design_result.value->modules.size(), 3u);
  EXPECT_EQ(design_result.value->module_dependencies.size(), 2u);
  EXPECT_EQ(design_result.value->module_order.size(), 3u);
  EXPECT_NE(std::find(design_result.value->module_order.begin(), design_result.value->module_order.end(), "top"), design_result.value->module_order.end());
  EXPECT_NE(std::find(design_result.value->module_order.begin(), design_result.value->module_order.end(), "mid"), design_result.value->module_order.end());
  EXPECT_NE(std::find(design_result.value->module_order.begin(), design_result.value->module_order.end(), "leaf"), design_result.value->module_order.end());
  ASSERT_EQ(design_result.value->top_instances.size(), 1u);
  EXPECT_EQ(design_result.value->top_instances[0].module_name, "mid");
  ASSERT_EQ(design_result.value->top_instances[0].children.size(), 1u);
  EXPECT_EQ(design_result.value->top_instances[0].children[0].module_name, "leaf");
  EXPECT_EQ(design_result.value->top_instances[0].children[0].instance_name, "u_leaf");
}
