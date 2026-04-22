#pragma once

#include <memory>
#include <string>
#include <vector>

#include "mnf/common/result.h"
#include "mnf/lexer/lexer.h"
#include "mnf/parser/ast.h"

namespace mnf {

class Parser {
public:
  explicit Parser(Lexer& lexer);

  Result<Program> ParseProgram();

private:
  Result<std::unique_ptr<ModuleDecl>> ParseModule();
  Result<PortDecl> ParsePortDecl();
  Result<WireDecl> ParseWireDecl();
  Result<AssignStmt> ParseAssignStmt();
  Result<Expression> ParseExpression();
  Result<Expression> ParsePrimaryExpression();
  Result<InstanceDecl> ParseInstanceDecl();
  Result<NamedConnection> ParseNamedConnection();

  bool Match(TokenKind kind);
  bool Expect(TokenKind kind, const char* message);
  Token Consume();
  bool ExpectIdentifier(const char* message, Token* token);
  Result<std::vector<std::string>> ParseIdentifierList();

  Lexer& lexer_;
  Token current_;
  std::vector<Diagnostic> diagnostics_;
};

}  // namespace mnf
