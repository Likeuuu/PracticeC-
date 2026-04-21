#include <gtest/gtest.h>

#include <string>

#include "mnf/lexer/lexer.h"
#include "mnf/parser/parser.h"

TEST(ParserTest, ParsesHierarchyIntoAst) {
  const std::string input = R"(module leaf(a, y);
  input a;
  output y;
endmodule

module top(in1, out1);
  input in1;
  output out1;
  wire w1;
  leaf u1(.a(in1), .y(out1));
endmodule
)";

  mnf::Lexer lexer(input, "parser_test.nl");
  mnf::Parser parser(lexer);
  auto result = parser.ParseProgram();

  ASSERT_TRUE(result.Ok());
  ASSERT_EQ(result.value->modules.size(), 2u);

  const mnf::ModuleDecl& leaf = *result.value->modules[0];
  EXPECT_EQ(leaf.name, "leaf");
  EXPECT_EQ(leaf.ports.size(), 2u);
  EXPECT_EQ(leaf.port_decls.size(), 2u);

  const mnf::ModuleDecl& top = *result.value->modules[1];
  EXPECT_EQ(top.name, "top");
  EXPECT_EQ(top.wire_decls.size(), 1u);
  ASSERT_EQ(top.instances.size(), 1u);
  EXPECT_EQ(top.instances[0].module_name, "leaf");
  EXPECT_EQ(top.instances[0].instance_name, "u1");
  ASSERT_EQ(top.instances[0].connections.size(), 2u);
  EXPECT_EQ(top.instances[0].connections[0].port_name, "a");
  EXPECT_EQ(top.instances[0].connections[0].signal_name, "in1");
}

TEST(ParserTest, ParsesAssignStatementsWithIdentifiersAndNumbers) {
  const std::string input = R"(module top(in1, out1);
  input in1;
  output out1;
  wire mid;
  assign mid = in1;
  assign out1 = 1;
endmodule
)";

  mnf::Lexer lexer(input, "parser_assign_test.nl");
  mnf::Parser parser(lexer);
  auto result = parser.ParseProgram();

  ASSERT_TRUE(result.Ok());
  ASSERT_EQ(result.value->modules.size(), 1u);
  const mnf::ModuleDecl& top = *result.value->modules[0];
  ASSERT_EQ(top.assign_stmts.size(), 2u);
  EXPECT_EQ(top.assign_stmts[0].lhs, "mid");
  EXPECT_EQ(top.assign_stmts[0].rhs.kind, mnf::Expression::Kind::Identifier);
  EXPECT_EQ(top.assign_stmts[0].rhs.text, "in1");
  EXPECT_EQ(top.assign_stmts[1].lhs, "out1");
  EXPECT_EQ(top.assign_stmts[1].rhs.kind, mnf::Expression::Kind::Number);
  EXPECT_EQ(top.assign_stmts[1].rhs.text, "1");
}
