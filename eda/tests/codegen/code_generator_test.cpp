#include <gtest/gtest.h>

#include <cstdlib>
#include <fstream>
#include <string>
#include <unordered_map>

#include "mnf/codegen/code_generator.h"
#include "mnf/lexer/lexer.h"
#include "mnf/parser/parser.h"
#include "mnf/semantic/semantic_checker.h"
#include "mnf/semantic/symbol_table.h"
#include "mnf/elaboration/elaborator.h"
#include "mnf/sim/combinational_evaluator.h"

namespace {

mnf::ElaboratedDesign BuildDesign(const std::string& input) {
  mnf::Lexer lexer(input, "codegen_test.nl");
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

// 运行编译型仿真：生成C++ → g++编译 → 执行 → 解析输出
// 输出格式：每行 "net_<id>=<value>"
std::unordered_map<int, int> RunCompiledSim(const mnf::ElaboratedDesign& design,
                                            const std::unordered_map<std::string, int>& input_values) {
  mnf::CodeGenerator gen;
  mnf::CodeGenProgramOptions options;
  options.input_values = input_values;
  options.print_all_nets = true;

  auto gen_result = gen.GenerateProgram(design, options);
  EXPECT_TRUE(gen_result.Ok());

  const std::string tmp_dir = "/tmp/mnf_codegen_test";
  system(("mkdir -p " + tmp_dir).c_str());

  // 写入生成代码，替换main部分加入输入设置和输出打印
  const std::string src_path = tmp_dir + "/sim.cpp";
  const std::string exe_path = tmp_dir + "/sim";
  const std::string out_path = tmp_dir + "/output.txt";

  std::ofstream(src_path) << gen_result.source;
  EXPECT_EQ(system(("g++ -std=c++17 -O2 " + src_path + " -o " + exe_path + " 2>" + tmp_dir + "/compile_err.txt").c_str()), 0)
      << "compiled simulation failed to compile";

  EXPECT_EQ(system((exe_path + " > " + out_path).c_str()), 0);

  std::unordered_map<int, int> results;
  std::ifstream out(out_path);
  std::string line;
  while (std::getline(out, line)) {
    auto eq = line.find('=');
    if (eq != std::string::npos) {
      int id = std::stoi(line.substr(4, eq - 4));  // "net_<id>=<value>"
      int val = std::stoi(line.substr(eq + 1));
      results[id] = val;
    }
  }
  return results;
}

void ExpectCompiledSimMatchesInterpreter(
    const mnf::ElaboratedDesign& design,
    const std::unordered_map<std::string, int>& input_values) {
  mnf::CombinationalEvaluator evaluator;
  const auto interp_result = evaluator.Evaluate(design.top_graph, input_values);
  ASSERT_TRUE(interp_result.Ok());

  const auto compiled_results = RunCompiledSim(design, input_values);

  for (const auto& net : design.top_graph.nets) {
    const int interp_val = interp_result.net_values[static_cast<std::size_t>(net.id)];
    const auto it = compiled_results.find(net.id);
    ASSERT_NE(it, compiled_results.end()) << "net_" << net.id << " (" << net.qualified_name
                                          << ") not found in compiled output";
    EXPECT_EQ(interp_val, it->second) << "mismatch on " << net.qualified_name;
  }
}

}  // namespace

TEST(CodeGeneratorTest, GeneratesCombinationalAssigns) {
  const std::string input = R"(module top(in1, in2, out1);
  input in1;
  input in2;
  output out1;
  assign out1 = in1 & in2;
endmodule
)";

  const auto design = BuildDesign(input);
  mnf::CodeGenerator gen;
  const auto result = gen.Generate(design);

  ASSERT_TRUE(result.Ok()) << "code generation failed";
  EXPECT_NE(result.source.find("struct top"), std::string::npos);
  EXPECT_NE(result.source.find("void top::evaluate()"), std::string::npos);
  EXPECT_NE(result.source.find("&"), std::string::npos);
  EXPECT_NE(result.source.find("int main()"), std::string::npos);
}

TEST(CodeGeneratorTest, GeneratesConstantAssign) {
  const std::string input = R"(module top(out1);
  output out1;
  assign out1 = 1;
endmodule
)";

  const auto design = BuildDesign(input);
  mnf::CodeGenerator gen;
  const auto result = gen.Generate(design);

  ASSERT_TRUE(result.Ok());
  EXPECT_NE(result.source.find("= 1;"), std::string::npos);
}

TEST(CodeGeneratorTest, GeneratesMixedOperators) {
  const std::string input = R"(module top(in1, in2, in3, out1);
  input in1;
  input in2;
  input in3;
  output out1;
  assign out1 = ~in1 | (in2 & in3);
endmodule
)";

  const auto design = BuildDesign(input);
  mnf::CodeGenerator gen;
  const auto result = gen.Generate(design);

  ASSERT_TRUE(result.Ok());
  EXPECT_NE(result.source.find("~("), std::string::npos);
  EXPECT_NE(result.source.find("|"), std::string::npos);
  EXPECT_NE(result.source.find("&"), std::string::npos);
}

// Step 1.3: 端到端集成测试 — 编译型 vs 解释型对比
TEST(CodeGeneratorTest, CompiledSimMatchesInterpreter) {
  const std::string input = R"(module top(in1, in2, out1);
  input in1;
  input in2;
  output out1;
  assign out1 = in1 & in2;
endmodule
)";

  const auto design = BuildDesign(input);

  // 解释型结果
  mnf::CombinationalEvaluator evaluator;
  const auto interp_result = evaluator.Evaluate(design.top_graph, {{"in1", 1}, {"in2", 1}});
  ASSERT_TRUE(interp_result.Ok());

  // 编译型结果：设置输入 in1=1, in2=1
  // 需要知道net_id — 从IR中查找
  auto compiled_results = RunCompiledSim(design, {{"in1", 1}, {"in2", 1}});

  // 对比每个net的值
  for (const auto& net : design.top_graph.nets) {
    int interp_val = interp_result.net_values[static_cast<std::size_t>(net.id)];
    auto it = compiled_results.find(net.id);
    ASSERT_NE(it, compiled_results.end()) << "net_" << net.id << " (" << net.qualified_name << ") not found in compiled output";
    EXPECT_EQ(interp_val, it->second) << "mismatch on " << net.qualified_name;
  }
}

TEST(CodeGeneratorTest, CompiledSimUsesDependencyOrderInsteadOfSourceOrder) {
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

  const auto design = BuildDesign(input);
  ExpectCompiledSimMatchesInterpreter(design, {{"in1", 1}, {"in2", 0}, {"in3", 1}});
}

TEST(CodeGeneratorTest, CompiledSimMatchesInterpreterForHierarchy) {
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

  const auto design = BuildDesign(input);
  ExpectCompiledSimMatchesInterpreter(design, {{"in1", 1}, {"in2", 1}});
  ExpectCompiledSimMatchesInterpreter(design, {{"in1", 1}, {"in2", 0}});
}
