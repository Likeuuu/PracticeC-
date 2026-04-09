#include "mnf/parser/parser.h"

#include <utility>

namespace mnf {

Parser::Parser(Lexer& lexer) : lexer_(lexer), current_(lexer_.NextToken()) {}

Result<Program> Parser::ParseProgram() {
  Program program;

  while (current_.kind != TokenKind::EndOfFile) {
    auto module = ParseModule();
    diagnostics_.insert(diagnostics_.end(), module.diagnostics.begin(), module.diagnostics.end());
    if (!module.Ok()) {
      return Result<Program>{std::nullopt, diagnostics_};
    }
    program.modules.push_back(std::move(*module.value));
  }

  return Result<Program>{std::move(program), diagnostics_};
}

Result<std::unique_ptr<ModuleDecl>> Parser::ParseModule() {
  diagnostics_.push_back(Diagnostic{DiagnosticLevel::Error, "ParseModule is not implemented yet", current_.location});
  return Result<std::unique_ptr<ModuleDecl>>{std::nullopt, diagnostics_};
}

Result<PortDecl> Parser::ParsePortDecl() {
  diagnostics_.push_back(Diagnostic{DiagnosticLevel::Error, "ParsePortDecl is not implemented yet", current_.location});
  return Result<PortDecl>{std::nullopt, diagnostics_};
}

Result<WireDecl> Parser::ParseWireDecl() {
  diagnostics_.push_back(Diagnostic{DiagnosticLevel::Error, "ParseWireDecl is not implemented yet", current_.location});
  return Result<WireDecl>{std::nullopt, diagnostics_};
}

Result<InstanceDecl> Parser::ParseInstanceDecl() {
  diagnostics_.push_back(Diagnostic{DiagnosticLevel::Error, "ParseInstanceDecl is not implemented yet", current_.location});
  return Result<InstanceDecl>{std::nullopt, diagnostics_};
}

bool Parser::Match(TokenKind kind) {
  if (current_.kind != kind) {
    return false;
  }
  current_ = lexer_.NextToken();
  return true;
}

bool Parser::Expect(TokenKind kind, const char* message) {
  if (Match(kind)) {
    return true;
  }
  diagnostics_.push_back(Diagnostic{DiagnosticLevel::Error, message, current_.location});
  return false;
}

Token Parser::Consume() {
  Token consumed = current_;
  current_ = lexer_.NextToken();
  return consumed;
}

}  // namespace mnf
