#include <gtest/gtest.h>

#include <algorithm>
#include <string>

#include "mnf/elaboration/elaborator.h"
#include "mnf/lexer/lexer.h"
#include "mnf/parser/parser.h"
#include "mnf/semantic/semantic_checker.h"
#include "mnf/semantic/symbol_table.h"

TEST(ElaboratorTest, BuildsHierarchyAndResolvedTopGraph) {
  const std::string input = R"(module leaf(a, y);
  input a;
  output y;
  wire leaf_wire;
  assign leaf_wire = a;
endmodule

module mid(in1, out1);
  input in1;
  output out1;
  wire mid_wire;
  assign mid_wire = in1;
  leaf u_leaf(.a(mid_wire), .y(out1));
endmodule

module top(in1, in2, out1);
  input in1;
  input in2;
  output out1;
  wire mid_out;
  wire and_out;
  assign and_out = in1 & in2;
  mid u_mid(.in1(and_out), .out1(mid_out));
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

  ASSERT_EQ(design_result.value->top_graph.nets.size(), 7u);
  EXPECT_EQ(design_result.value->top_graph.nets[0].qualified_name, "in1");
  EXPECT_EQ(design_result.value->top_graph.nets[4].qualified_name, "and_out");
  EXPECT_EQ(design_result.value->top_graph.nets[5].qualified_name, "u_mid.mid_wire");
  EXPECT_EQ(design_result.value->top_graph.nets[6].qualified_name, "u_mid.u_leaf.leaf_wire");

  ASSERT_EQ(design_result.value->top_graph.assigns.size(), 3u);
  EXPECT_EQ(design_result.value->top_graph.assigns[0].instance_path, "");
  EXPECT_EQ(design_result.value->top_graph.assigns[0].target_net_id, 4);
  EXPECT_EQ(design_result.value->top_graph.assigns[0].expr_op, "&");
  ASSERT_EQ(design_result.value->top_graph.assigns[0].source_net_ids.size(), 2u);
  EXPECT_EQ(design_result.value->top_graph.assigns[0].source_net_ids[0], 0);
  EXPECT_EQ(design_result.value->top_graph.assigns[0].source_net_ids[1], 1);

  EXPECT_EQ(design_result.value->top_graph.assigns[1].instance_path, "u_mid");
  EXPECT_EQ(design_result.value->top_graph.assigns[1].target_net_id, 5);
  ASSERT_EQ(design_result.value->top_graph.assigns[1].source_net_ids.size(), 1u);
  EXPECT_EQ(design_result.value->top_graph.assigns[1].source_net_ids[0], 4);

  EXPECT_EQ(design_result.value->top_graph.assigns[2].instance_path, "u_mid.u_leaf");
  EXPECT_EQ(design_result.value->top_graph.assigns[2].target_net_id, 6);
  ASSERT_EQ(design_result.value->top_graph.assigns[2].source_net_ids.size(), 1u);
  EXPECT_EQ(design_result.value->top_graph.assigns[2].source_net_ids[0], 5);

  ASSERT_EQ(design_result.value->top_graph.instance_bindings.size(), 4u);
  EXPECT_EQ(design_result.value->top_graph.instance_bindings[0].instance_path, "u_mid");
  EXPECT_EQ(design_result.value->top_graph.instance_bindings[0].signal_net_id, 4);
  EXPECT_EQ(design_result.value->top_graph.instance_bindings[1].signal_net_id, 3);
  EXPECT_EQ(design_result.value->top_graph.instance_bindings[2].instance_path, "u_mid.u_leaf");
  EXPECT_EQ(design_result.value->top_graph.instance_bindings[2].signal_net_id, 5);
  EXPECT_EQ(design_result.value->top_graph.instance_bindings[3].signal_net_id, 3);
}
