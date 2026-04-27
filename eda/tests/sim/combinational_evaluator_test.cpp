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
