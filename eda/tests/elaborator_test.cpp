#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

#include "mnf/elaboration/elaborator.h"
#include "mnf/lexer/lexer.h"
#include "mnf/parser/parser.h"
#include "mnf/semantic/semantic_checker.h"
#include "mnf/semantic/symbol_table.h"

namespace {

void ExpectNetIds(const std::vector<int>& actual, const std::vector<int>& expected) {
  ASSERT_EQ(actual.size(), expected.size());
  for (std::size_t i = 0; i < expected.size(); ++i) {
    EXPECT_EQ(actual[i], expected[i]);
  }
}

const mnf::ResolvedScopeFrameIR* FindScopeFrame(const mnf::ResolvedNetGraphIR& graph, const std::string& instance_path) {
  for (const auto& frame : graph.scope_frames) {
    if (frame.instance_path == instance_path) {
      return &frame;
    }
  }
  return nullptr;
}

const mnf::ResolvedScopeSymbolIR* FindScopeSymbol(const mnf::ResolvedScopeFrameIR& frame, const std::string& name) {
  for (const auto& symbol : frame.symbols) {
    if (symbol.name == name) {
      return &symbol;
    }
  }
  return nullptr;
}

}  // namespace

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

  ASSERT_EQ(design_result.value->top_graph.scope_frames.size(), 3u);
  const auto* top_scope = FindScopeFrame(design_result.value->top_graph, "");
  ASSERT_NE(top_scope, nullptr);
  EXPECT_EQ(top_scope->module_name, "top");
  const auto* top_port = FindScopeSymbol(*top_scope, "in1");
  ASSERT_NE(top_port, nullptr);
  EXPECT_EQ(top_port->kind, mnf::ResolvedScopeSymbolIR::Kind::Port);
  EXPECT_EQ(top_port->qualified_name, "in1");
  EXPECT_EQ(top_port->net_id, 0);
  const auto* top_wire = FindScopeSymbol(*top_scope, "and_out");
  ASSERT_NE(top_wire, nullptr);
  EXPECT_EQ(top_wire->kind, mnf::ResolvedScopeSymbolIR::Kind::Wire);
  EXPECT_EQ(top_wire->qualified_name, "and_out");
  EXPECT_EQ(top_wire->net_id, 4);

  const auto* mid_scope = FindScopeFrame(design_result.value->top_graph, "u_mid");
  ASSERT_NE(mid_scope, nullptr);
  EXPECT_EQ(mid_scope->module_name, "mid");
  const auto* mid_port = FindScopeSymbol(*mid_scope, "in1");
  ASSERT_NE(mid_port, nullptr);
  EXPECT_EQ(mid_port->kind, mnf::ResolvedScopeSymbolIR::Kind::Port);
  EXPECT_EQ(mid_port->qualified_name, "u_mid.in1");
  EXPECT_EQ(mid_port->net_id, 4);
  const auto* mid_wire = FindScopeSymbol(*mid_scope, "mid_wire");
  ASSERT_NE(mid_wire, nullptr);
  EXPECT_EQ(mid_wire->kind, mnf::ResolvedScopeSymbolIR::Kind::Wire);
  EXPECT_EQ(mid_wire->qualified_name, "u_mid.mid_wire");
  EXPECT_EQ(mid_wire->net_id, 5);

  ASSERT_EQ(design_result.value->top_graph.assigns.size(), 3u);
  EXPECT_EQ(design_result.value->top_graph.assigns[0].instance_path, "");
  EXPECT_EQ(design_result.value->top_graph.assigns[0].target_net_id, 4);
  EXPECT_EQ(design_result.value->top_graph.assigns[0].target_name_view, "and_out");
  EXPECT_EQ(design_result.value->top_graph.assigns[0].rhs_expr.kind, mnf::ResolvedExprIR::Kind::Binary);
  EXPECT_EQ(design_result.value->top_graph.assigns[0].rhs_expr.op, "&");
  ASSERT_NE(design_result.value->top_graph.assigns[0].rhs_expr.lhs, nullptr);
  ASSERT_NE(design_result.value->top_graph.assigns[0].rhs_expr.rhs, nullptr);
  EXPECT_EQ(design_result.value->top_graph.assigns[0].rhs_expr.lhs->kind, mnf::ResolvedExprIR::Kind::Net);
  EXPECT_EQ(design_result.value->top_graph.assigns[0].rhs_expr.lhs->net_id, 0);
  EXPECT_EQ(design_result.value->top_graph.assigns[0].rhs_expr.rhs->kind, mnf::ResolvedExprIR::Kind::Net);
  EXPECT_EQ(design_result.value->top_graph.assigns[0].rhs_expr.rhs->net_id, 1);
  ExpectNetIds(design_result.value->top_graph.assigns[0].source_net_ids, {0, 1});

  EXPECT_EQ(design_result.value->top_graph.assigns[1].instance_path, "u_mid");
  EXPECT_EQ(design_result.value->top_graph.assigns[1].target_net_id, 5);
  EXPECT_EQ(design_result.value->top_graph.assigns[1].target_name_view, "u_mid.mid_wire");
  EXPECT_EQ(design_result.value->top_graph.assigns[1].rhs_expr.kind, mnf::ResolvedExprIR::Kind::Net);
  EXPECT_EQ(design_result.value->top_graph.assigns[1].rhs_expr.net_id, 4);
  ExpectNetIds(design_result.value->top_graph.assigns[1].source_net_ids, {4});

  EXPECT_EQ(design_result.value->top_graph.assigns[2].instance_path, "u_mid.u_leaf");
  EXPECT_EQ(design_result.value->top_graph.assigns[2].target_net_id, 6);
  EXPECT_EQ(design_result.value->top_graph.assigns[2].target_name_view, "u_mid.u_leaf.leaf_wire");
  EXPECT_EQ(design_result.value->top_graph.assigns[2].rhs_expr.kind, mnf::ResolvedExprIR::Kind::Net);
  EXPECT_EQ(design_result.value->top_graph.assigns[2].rhs_expr.net_id, 5);
  ExpectNetIds(design_result.value->top_graph.assigns[2].source_net_ids, {5});

  ASSERT_EQ(design_result.value->top_graph.instance_bindings.size(), 4u);
  EXPECT_EQ(design_result.value->top_graph.instance_bindings[0].instance_path, "u_mid");
  EXPECT_EQ(design_result.value->top_graph.instance_bindings[0].signal_net_id, 4);
  EXPECT_EQ(design_result.value->top_graph.instance_bindings[1].signal_net_id, 3);
  EXPECT_EQ(design_result.value->top_graph.instance_bindings[2].instance_path, "u_mid.u_leaf");
  EXPECT_EQ(design_result.value->top_graph.instance_bindings[2].signal_net_id, 5);
  EXPECT_EQ(design_result.value->top_graph.instance_bindings[3].signal_net_id, 3);
}

TEST(ElaboratorTest, ResolvesAlwaysBlocksIntoProcessIr) {
  const std::string input = R"(module top(in1, in2, out1);
  input in1;
  input in2;
  output out1;
  reg state;
  always begin
    if (in1) state <= in2;
    out1 = state;
  end
endmodule
)";

  mnf::Lexer lexer(input, "elaborator_process_test.nl");
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

  ASSERT_EQ(design_result.value->top_graph.nets.size(), 4u);
  EXPECT_EQ(design_result.value->top_graph.nets[3].qualified_name, "state");
  EXPECT_EQ(design_result.value->top_graph.nets[3].kind, mnf::ResolvedNetIR::Kind::Reg);

  const auto* top_scope = FindScopeFrame(design_result.value->top_graph, "");
  ASSERT_NE(top_scope, nullptr);
  const auto* state_symbol = FindScopeSymbol(*top_scope, "state");
  ASSERT_NE(state_symbol, nullptr);
  EXPECT_EQ(state_symbol->kind, mnf::ResolvedScopeSymbolIR::Kind::Reg);
  EXPECT_EQ(state_symbol->net_id, 3);
  EXPECT_EQ(state_symbol->qualified_name, "state");

  ASSERT_EQ(design_result.value->top_graph.always_blocks.size(), 1u);
  const auto& always_block = design_result.value->top_graph.always_blocks[0];
  EXPECT_EQ(always_block.instance_path, "");
  ASSERT_NE(always_block.body, nullptr);
  EXPECT_EQ(always_block.body->kind, mnf::ResolvedProcessStmtIR::Kind::Block);
  ASSERT_EQ(always_block.body->statements.size(), 2u);

  const auto& if_stmt = always_block.body->statements[0];
  ASSERT_NE(if_stmt, nullptr);
  EXPECT_EQ(if_stmt->kind, mnf::ResolvedProcessStmtIR::Kind::If);
  ASSERT_NE(if_stmt->if_stmt, nullptr);
  EXPECT_EQ(if_stmt->if_stmt->condition_expr.kind, mnf::ResolvedExprIR::Kind::Net);
  EXPECT_EQ(if_stmt->if_stmt->condition_expr.net_id, 0);
  ExpectNetIds(if_stmt->if_stmt->condition_source_net_ids, {0});
  ASSERT_NE(if_stmt->if_stmt->then_stmt, nullptr);
  ASSERT_NE(if_stmt->if_stmt->then_stmt->assign_stmt, nullptr);
  EXPECT_EQ(if_stmt->if_stmt->then_stmt->assign_stmt->assignment_kind,
            mnf::ResolvedProceduralAssignIR::AssignmentKind::NonBlocking);
  EXPECT_EQ(if_stmt->if_stmt->then_stmt->assign_stmt->target_net_id, 3);
  EXPECT_EQ(if_stmt->if_stmt->then_stmt->assign_stmt->target_name_view, "state");
  ExpectNetIds(if_stmt->if_stmt->then_stmt->assign_stmt->source_net_ids, {1});

  const auto& assign_stmt = always_block.body->statements[1];
  ASSERT_NE(assign_stmt, nullptr);
  EXPECT_EQ(assign_stmt->kind, mnf::ResolvedProcessStmtIR::Kind::Assignment);
  ASSERT_NE(assign_stmt->assign_stmt, nullptr);
  EXPECT_EQ(assign_stmt->assign_stmt->assignment_kind,
            mnf::ResolvedProceduralAssignIR::AssignmentKind::Blocking);
  EXPECT_EQ(assign_stmt->assign_stmt->target_net_id, 2);
  EXPECT_EQ(assign_stmt->assign_stmt->target_name_view, "out1");
  ExpectNetIds(assign_stmt->assign_stmt->source_net_ids, {3});
}
