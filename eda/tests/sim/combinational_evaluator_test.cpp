#include <gtest/gtest.h>

#include <string>
#include <unordered_map>

#include "mnf/elaboration/elaborator.h"
#include "mnf/lexer/lexer.h"
#include "mnf/parser/parser.h"
#include "mnf/semantic/semantic_checker.h"
#include "mnf/semantic/symbol_table.h"
#include "mnf/sim/combinational_evaluator.h"

namespace {

mnf::ElaboratedDesign BuildDesign(const std::string& input) {
  mnf::Lexer lexer(input, "combinational_evaluator_test.nl");
  mnf::Parser parser(lexer);
  auto parse_result = parser.ParseProgram();
  EXPECT_TRUE(parse_result.Ok());

  mnf::SymbolTable symbols;
  mnf::SemanticChecker checker;
  const auto diagnostics = checker.Check(*parse_result.value, symbols);
  EXPECT_TRUE(diagnostics.empty());

  mnf::Elaborator elaborator;
  auto design_result = elaborator.Elaborate(*parse_result.value, symbols, "top");
  EXPECT_TRUE(design_result.Ok());
  return *design_result.value;
}

int NetValueByName(const mnf::CombinationalEvalResult& result,
                   const mnf::ResolvedNetGraphIR& graph,
                   const std::string& qualified_name) {
  for (const auto& net : graph.nets) {
    if (net.qualified_name == qualified_name) {
      return result.net_values[static_cast<std::size_t>(net.id)];
    }
  }
  return -1;
}

bool HasDiagnosticSubstring(const mnf::CombinationalEvalResult& result, const std::string& needle) {
  for (const auto& diagnostic : result.diagnostics) {
    if (diagnostic.message.find(needle) != std::string::npos) {
      return true;
    }
  }
  return false;
}

}  // namespace

TEST(CombinationalEvaluatorTest, EvaluatesTopLevelAndHierarchicalAssigns) {
  const std::string input = R"(module leaf(a, y);
  input a;
  output y;
  wire leaf_wire;
  assign leaf_wire = a;
  assign y = leaf_wire;
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
  assign out1 = mid_out;
endmodule
)";

  const mnf::ElaboratedDesign design = BuildDesign(input);
  mnf::CombinationalEvaluator evaluator;

  const auto result_zero = evaluator.Evaluate(design.top_graph, {{"in1", 1}, {"in2", 0}});
  ASSERT_TRUE(result_zero.Ok());
  EXPECT_EQ(NetValueByName(result_zero, design.top_graph, "and_out"), 0);
  EXPECT_EQ(NetValueByName(result_zero, design.top_graph, "u_mid.mid_wire"), 0);
  EXPECT_EQ(NetValueByName(result_zero, design.top_graph, "u_mid.u_leaf.leaf_wire"), 0);
  EXPECT_EQ(NetValueByName(result_zero, design.top_graph, "out1"), 0);

  const auto result_one = evaluator.Evaluate(design.top_graph, {{"in1", 1}, {"in2", 1}});
  ASSERT_TRUE(result_one.Ok());
  EXPECT_EQ(NetValueByName(result_one, design.top_graph, "and_out"), 1);
  EXPECT_EQ(NetValueByName(result_one, design.top_graph, "u_mid.mid_wire"), 1);
  EXPECT_EQ(NetValueByName(result_one, design.top_graph, "u_mid.u_leaf.leaf_wire"), 1);
  EXPECT_EQ(NetValueByName(result_one, design.top_graph, "out1"), 1);
}

TEST(CombinationalEvaluatorTest, EvaluatesConstantAssigns) {
  const std::string input = R"(module top(out1);
  output out1;
  assign out1 = 1;
endmodule
)";

  const mnf::ElaboratedDesign design = BuildDesign(input);
  mnf::CombinationalEvaluator evaluator;
  const auto result = evaluator.Evaluate(design.top_graph, {});

  ASSERT_TRUE(result.Ok());
  EXPECT_EQ(NetValueByName(result, design.top_graph, "out1"), 1);
}

TEST(CombinationalEvaluatorTest, EvaluatesMixedOperatorsAndUnaryExpressions) {
  const std::string input = R"(module top(in1, in2, in3, out1);
  input in1;
  input in2;
  input in3;
  output out1;
  assign out1 = ~in1 | (in2 & in3) ^ 1;
endmodule
)";

  const mnf::ElaboratedDesign design = BuildDesign(input);
  mnf::CombinationalEvaluator evaluator;

  const auto result_a = evaluator.Evaluate(design.top_graph, {{"in1", 1}, {"in2", 1}, {"in3", 1}});
  ASSERT_TRUE(result_a.Ok());
  EXPECT_EQ(NetValueByName(result_a, design.top_graph, "out1"), 0);

  const auto result_b = evaluator.Evaluate(design.top_graph, {{"in1", 0}, {"in2", 0}, {"in3", 1}});
  ASSERT_TRUE(result_b.Ok());
  EXPECT_EQ(NetValueByName(result_b, design.top_graph, "out1"), 1);
}

TEST(CombinationalEvaluatorTest, EvaluatesAssignsInDependencyOrderInsteadOfSourceOrder) {
  const std::string input = R"(module top(in1, in2, in3, out1);
  input in1;
  input in2;
  input in3;
  output out1;
  wire a1;
  wire a2;
  assign out1 = a2;
  assign a2 = a1 | in3;
  assign a1 = in1 & in2;
endmodule
)";

  const mnf::ElaboratedDesign design = BuildDesign(input);
  mnf::CombinationalEvaluator evaluator;
  const auto result = evaluator.Evaluate(design.top_graph, {{"in1", 1}, {"in2", 0}, {"in3", 1}});

  ASSERT_TRUE(result.Ok());
  EXPECT_EQ(NetValueByName(result, design.top_graph, "a1"), 0);
  EXPECT_EQ(NetValueByName(result, design.top_graph, "a2"), 1);
  EXPECT_EQ(NetValueByName(result, design.top_graph, "out1"), 1);
}

TEST(CombinationalEvaluatorTest, ReportsCombinationalAssignCycles) {
  const std::string input = R"(module top(in1, out1);
  input in1;
  output out1;
  wire a1;
  assign a1 = out1;
  assign out1 = a1 | in1;
endmodule
)";

  const mnf::ElaboratedDesign design = BuildDesign(input);
  mnf::CombinationalEvaluator evaluator;
  const auto result = evaluator.Evaluate(design.top_graph, {{"in1", 1}});

  ASSERT_FALSE(result.Ok());
  EXPECT_TRUE(HasDiagnosticSubstring(result, "Combinational assign cycle detected"));
}

TEST(CombinationalEvaluatorTest, BlockingAssignmentUpdatesImmediatelyInsideAlways) {
  const std::string input = R"(module top(in1, out1);
  input in1;
  output out1;
  reg state;
  assign out1 = state;
  always begin
    state = in1;
  end
endmodule
)";

  const mnf::ElaboratedDesign design = BuildDesign(input);
  mnf::CombinationalEvaluator evaluator;
  const auto result = evaluator.Evaluate(design.top_graph, {{"in1", 1}});

  ASSERT_TRUE(result.Ok());
  EXPECT_EQ(NetValueByName(result, design.top_graph, "state"), 1);
  EXPECT_EQ(NetValueByName(result, design.top_graph, "out1"), 1);
}

TEST(CombinationalEvaluatorTest, NonBlockingAssignmentCommitsAfterAlwaysBlock) {
  const std::string input = R"(module top(in1, out1, out2);
  input in1;
  output out1;
  output out2;
  reg state;
  assign out2 = state;
  always begin
    state <= in1;
    out1 = state;
  end
endmodule
)";

  const mnf::ElaboratedDesign design = BuildDesign(input);
  mnf::CombinationalEvaluator evaluator;
  const auto result = evaluator.Evaluate(design.top_graph, {{"in1", 1}});

  ASSERT_TRUE(result.Ok());
  EXPECT_GE(result.delta_cycles, 2);
  EXPECT_EQ(NetValueByName(result, design.top_graph, "state"), 1);
  EXPECT_EQ(NetValueByName(result, design.top_graph, "out1"), 1);
  EXPECT_EQ(NetValueByName(result, design.top_graph, "out2"), 1);
}


TEST(CombinationalEvaluatorTest, AlwaysStarRunsWhenSensitiveInputsChange) {
  const std::string input = R"(module top(in1, out1);
  input in1;
  output out1;
  reg state;
  assign out1 = state;
  always @(*) begin
    state = in1;
  end
endmodule
)";

  const mnf::ElaboratedDesign design = BuildDesign(input);
  mnf::CombinationalEvaluator evaluator;
  const auto result = evaluator.Evaluate(design.top_graph, {{"in1", 1}});

  ASSERT_TRUE(result.Ok());
  EXPECT_EQ(NetValueByName(result, design.top_graph, "state"), 1);
  EXPECT_EQ(NetValueByName(result, design.top_graph, "out1"), 1);
}


TEST(CombinationalEvaluatorTest, ExplicitSensitivityListTriggersOnListedInputs) {
  const std::string input = R"(module top(in1, in2, out1);
  input in1;
  input in2;
  output out1;
  reg state;
  assign out1 = state;
  always @(in1) begin
    state = in2;
  end
endmodule
)";

  const mnf::ElaboratedDesign design = BuildDesign(input);
  mnf::CombinationalEvaluator evaluator;
  const auto result = evaluator.Evaluate(design.top_graph, {{"in1", 1}, {"in2", 1}});

  ASSERT_TRUE(result.Ok());
  EXPECT_EQ(NetValueByName(result, design.top_graph, "state"), 1);
  EXPECT_EQ(NetValueByName(result, design.top_graph, "out1"), 1);
}


TEST(CombinationalEvaluatorTest, UsesMultipleDeltaCyclesForChainedAlwaysBlocks) {
  const std::string input = R"(module top(in1, out1);
  input in1;
  output out1;
  reg state;
  wire mid;
  assign mid = state;
  assign out1 = state;
  always @(in1) begin
    state <= in1;
  end
  always @(mid) begin
    out1 = mid;
  end
endmodule
)";

  const mnf::ElaboratedDesign design = BuildDesign(input);
  mnf::CombinationalEvaluator evaluator;
  const auto result = evaluator.Evaluate(design.top_graph, {{"in1", 1}});

  ASSERT_TRUE(result.Ok());
  EXPECT_GE(result.delta_cycles, 2);
  EXPECT_EQ(NetValueByName(result, design.top_graph, "state"), 1);
  EXPECT_EQ(NetValueByName(result, design.top_graph, "mid"), 1);
  EXPECT_EQ(NetValueByName(result, design.top_graph, "out1"), 1);
}


TEST(CombinationalEvaluatorTest, PosedgeAlwaysTriggersOnInputTransition) {
  const std::string input = R"(module top(clk, d, q);
  input clk;
  input d;
  output q;
  reg state;
  assign q = state;
  always @(posedge clk) begin
    state <= d;
  end
endmodule
)";

  const mnf::ElaboratedDesign design = BuildDesign(input);
  mnf::CombinationalEvaluator evaluator;

  const auto result_no_edge = evaluator.EvaluateTransition(design.top_graph,
                                                           {{"clk", 0}, {"d", 1}},
                                                           {{"clk", 0}, {"d", 1}});
  ASSERT_TRUE(result_no_edge.Ok());
  EXPECT_EQ(NetValueByName(result_no_edge, design.top_graph, "q"), -1);

  const auto result_edge = evaluator.EvaluateTransition(design.top_graph,
                                                        {{"clk", 0}, {"d", 1}},
                                                        {{"clk", 1}, {"d", 1}});
  ASSERT_TRUE(result_edge.Ok());
  EXPECT_EQ(NetValueByName(result_edge, design.top_graph, "state"), 1);
  EXPECT_EQ(NetValueByName(result_edge, design.top_graph, "q"), 1);
}
