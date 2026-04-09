#include "mnf/app/cli.h"

#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

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

}  // namespace

int CliApp::Run(int argc, char** argv) const {
  if (argc < 2) {
    std::cout << "MiniNetlistFrontend bootstrap\n";
    std::cout << "Usage: mnf_cli <input-file> [top-module]\n";
    return 0;
  }

  const std::string input_path = argv[1];
  const std::string top_name = argc >= 3 ? argv[2] : "top";
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
  std::cout << text_writer.WriteSummary(*design_result.value);
  std::cout << json_writer.Write(*design_result.value);
  return 0;
}

}  // namespace mnf
