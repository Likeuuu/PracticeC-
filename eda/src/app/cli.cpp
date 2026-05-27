#include "mnf/app/cli.h"

#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <unordered_map>
#include <vector>

#include "mnf/codegen/code_generator.h"
#include "mnf/common/diagnostic.h"
#include "mnf/elaboration/elaborator.h"
#include "mnf/lexer/lexer.h"
#include "mnf/output/json_writer.h"
#include "mnf/output/text_writer.h"
#include "mnf/parser/parser.h"
#include "mnf/semantic/semantic_checker.h"
#include "mnf/semantic/symbol_table.h"

namespace mnf {

namespace {

enum class OutputFormat {
  Text,
  Json,
  Both,
  Cpp
};

std::string ReadFile(const std::string& path) {
  std::ifstream ifs(path);
  return std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
}

void PrintDiagnostics(const std::vector<Diagnostic>& diagnostics) {
  for (const auto& diagnostic : diagnostics) {
    std::cerr << diagnostic.location.file << ':' << diagnostic.location.line << ':' << diagnostic.location.column
              << ' ' << diagnostic.message << '\n';
  }
}

OutputFormat ParseOutputFormat(const std::string& arg) {
  if (arg == "--format=text") {
    return OutputFormat::Text;
  }
  if (arg == "--format=json") {
    return OutputFormat::Json;
  }
  if (arg == "--format=cpp") {
    return OutputFormat::Cpp;
  }
  return OutputFormat::Both;
}

bool ParseSignalValueArg(const std::string& value,
                         const std::string& prefix,
                         std::unordered_map<std::string, int>* values) {
  if (value.rfind(prefix, 0) != 0) {
    return false;
  }

  const std::string binding = value.substr(prefix.size());
  const auto eq = binding.find('=');
  if (eq == std::string::npos) {
    return false;
  }

  const std::string name = binding.substr(0, eq);
  const std::string raw_value = binding.substr(eq + 1);
  if (name.empty() || (raw_value != "0" && raw_value != "1")) {
    return false;
  }

  (*values)[name] = raw_value == "1" ? 1 : 0;
  return true;
}

void PrintUsage() {
  std::cout << "MiniNetlistFrontend bootstrap\n";
  std::cout << "Usage: mnf_cli <input-file> [top-module] [--format=text|json|both|cpp]\n";
  std::cout << "       [--input=name=0|1] [--prev=name=0|1] [--print-nets]\n";
}

}  // namespace

int CliApp::Run(int argc, char** argv) const {
  if (argc < 2) {
    PrintUsage();
    return 0;
  }

  const std::string input_path = argv[1];
  std::string top_name = "top";
  OutputFormat format = OutputFormat::Both;
  CodeGenProgramOptions codegen_options;

  for (int i = 2; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg.rfind("--format=", 0) == 0) {
      format = ParseOutputFormat(arg);
      continue;
    }
    if (arg == "--print-nets") {
      codegen_options.print_all_nets = true;
      continue;
    }
    if (ParseSignalValueArg(arg, "--input=", &codegen_options.input_values)) {
      continue;
    }
    if (ParseSignalValueArg(arg, "--prev=", &codegen_options.previous_input_values)) {
      continue;
    }
    top_name = arg;
  }

  const std::string input = ReadFile(input_path);

  Lexer lexer(input, input_path);
  Parser parser(lexer);
  auto parse_result = parser.ParseProgram();
  if (!parse_result.Ok()) {
    PrintDiagnostics(parse_result.diagnostics);
    return 1;
  }

  SymbolTable symbols;
  SemanticChecker checker;
  auto diagnostics = checker.Check(*parse_result.value, symbols);
  if (!diagnostics.empty()) {
    PrintDiagnostics(diagnostics);
    return 1;
  }

  Elaborator elaborator;
  auto design_result = elaborator.Elaborate(*parse_result.value, symbols, top_name);
  if (!design_result.Ok()) {
    PrintDiagnostics(design_result.diagnostics);
    return 1;
  }

  TextWriter text_writer;
  JsonWriter json_writer;
  CodeGenerator code_generator;

  if (format == OutputFormat::Cpp) {
    const auto codegen_result = code_generator.GenerateProgram(*design_result.value, codegen_options);
    if (!codegen_result.Ok()) {
      PrintDiagnostics(codegen_result.diagnostics);
      return 1;
    }
    std::cout << codegen_result.source;
    return 0;
  }

  if (format == OutputFormat::Text || format == OutputFormat::Both) {
    std::cout << "===== TEXT SUMMARY =====\n";
    std::cout << text_writer.WriteSummary(*design_result.value);
  }

  if (format == OutputFormat::Json || format == OutputFormat::Both) {
    if (format == OutputFormat::Both) {
      std::cout << "===== JSON IR =====\n";
    }
    std::cout << json_writer.Write(*design_result.value);
  }

  return 0;
}

}  // namespace mnf
