#include "mnf/parser/parser.h"

#include <utility>
#include <vector>

namespace mnf {

Parser::Parser(Lexer& lexer) : lexer_(lexer), current_(lexer_.NextToken()) {}

Result<Program> Parser::ParseProgram() {
  Program program;

  while (current_.kind != TokenKind::EndOfFile) {
    if (current_.kind != TokenKind::Module) {
      diagnostics_.push_back(Diagnostic{DiagnosticLevel::Error,
                                        "Expected 'module' declaration",
                                        current_.location});
      return Result<Program>{std::nullopt, diagnostics_};
    }

    auto module = ParseModule();
    if (!module.Ok()) {
      return Result<Program>{std::nullopt, diagnostics_};
    }
    program.modules.push_back(std::move(*module.value));
  }

  return Result<Program>{std::move(program), diagnostics_};
}

Result<std::unique_ptr<ModuleDecl>> Parser::ParseModule() {
  if (!Match(TokenKind::Module)) {
    diagnostics_.push_back(Diagnostic{DiagnosticLevel::Error,
                                      "Expected 'module' keyword",
                                      current_.location});
    return Result<std::unique_ptr<ModuleDecl>>{std::nullopt, {}};
  }

  Token name_token;
  if (!ExpectIdentifier("Expected module name", &name_token)) {
    return Result<std::unique_ptr<ModuleDecl>>{std::nullopt, {}};
  }

  auto module = std::make_unique<ModuleDecl>();
  module->location = name_token.location;
  module->name = name_token.lexeme;

  if (!Expect(TokenKind::LParen, "Expected '(' after module name")) {
    return Result<std::unique_ptr<ModuleDecl>>{std::nullopt, {}};
  }

  if (current_.kind != TokenKind::RParen) {
    auto ports = ParseIdentifierList();
    if (!ports.Ok()) {
      return Result<std::unique_ptr<ModuleDecl>>{std::nullopt, {}};
    }
    module->ports = std::move(*ports.value);
  }

  if (!Expect(TokenKind::RParen, "Expected ')' after module port list") ||
      !Expect(TokenKind::Semicolon, "Expected ';' after module header")) {
    return Result<std::unique_ptr<ModuleDecl>>{std::nullopt, {}};
  }

  while (current_.kind != TokenKind::EndModule && current_.kind != TokenKind::EndOfFile) {
    if (current_.kind == TokenKind::Input || current_.kind == TokenKind::Output) {
      auto port_decl = ParsePortDecl();
      if (!port_decl.Ok()) {
        return Result<std::unique_ptr<ModuleDecl>>{std::nullopt, {}};
      }
      module->port_decls.push_back(std::move(*port_decl.value));
      continue;
    }

    if (current_.kind == TokenKind::Wire) {
      auto wire_decl = ParseWireDecl();
      if (!wire_decl.Ok()) {
        return Result<std::unique_ptr<ModuleDecl>>{std::nullopt, {}};
      }
      module->wire_decls.push_back(std::move(*wire_decl.value));
      continue;
    }

    if (current_.kind == TokenKind::Assign) {
      auto assign_stmt = ParseAssignStmt();
      if (!assign_stmt.Ok()) {
        return Result<std::unique_ptr<ModuleDecl>>{std::nullopt, {}};
      }
      module->assign_stmts.push_back(std::move(*assign_stmt.value));
      continue;
    }

    if (current_.kind == TokenKind::Identifier) {
      auto instance_decl = ParseInstanceDecl();
      if (!instance_decl.Ok()) {
        return Result<std::unique_ptr<ModuleDecl>>{std::nullopt, {}};
      }
      module->instances.push_back(std::move(*instance_decl.value));
      continue;
    }

    diagnostics_.push_back(Diagnostic{DiagnosticLevel::Error,
                                      "Unexpected token inside module body: " + std::string(ToString(current_.kind)),
                                      current_.location});
    return Result<std::unique_ptr<ModuleDecl>>{std::nullopt, {}};
  }

  if (!Expect(TokenKind::EndModule, "Expected 'endmodule' to close module")) {
    return Result<std::unique_ptr<ModuleDecl>>{std::nullopt, {}};
  }

  return Result<std::unique_ptr<ModuleDecl>>{std::move(module), {}};
}

Result<PortDecl> Parser::ParsePortDecl() {
  PortDecl decl;
  decl.location = current_.location;

  if (Match(TokenKind::Input)) {
    decl.direction = PortDecl::Direction::Input;
  } else if (Match(TokenKind::Output)) {
    decl.direction = PortDecl::Direction::Output;
  } else {
    diagnostics_.push_back(Diagnostic{DiagnosticLevel::Error,
                                      "Expected 'input' or 'output' declaration",
                                      current_.location});
    return Result<PortDecl>{std::nullopt, {}};
  }

  auto names = ParseIdentifierList();
  if (!names.Ok()) {
    return Result<PortDecl>{std::nullopt, {}};
  }
  decl.names = std::move(*names.value);

  if (!Expect(TokenKind::Semicolon, "Expected ';' after port declaration")) {
    return Result<PortDecl>{std::nullopt, {}};
  }

  return Result<PortDecl>{std::move(decl), {}};
}

Result<WireDecl> Parser::ParseWireDecl() {
  WireDecl decl;
  decl.location = current_.location;

  if (!Match(TokenKind::Wire)) {
    diagnostics_.push_back(Diagnostic{DiagnosticLevel::Error,
                                      "Expected 'wire' declaration",
                                      current_.location});
    return Result<WireDecl>{std::nullopt, {}};
  }

  auto names = ParseIdentifierList();
  if (!names.Ok()) {
    return Result<WireDecl>{std::nullopt, {}};
  }
  decl.names = std::move(*names.value);

  if (!Expect(TokenKind::Semicolon, "Expected ';' after wire declaration")) {
    return Result<WireDecl>{std::nullopt, {}};
  }

  return Result<WireDecl>{std::move(decl), {}};
}

Result<AssignStmt> Parser::ParseAssignStmt() {
  AssignStmt stmt;
  stmt.location = current_.location;

  if (!Match(TokenKind::Assign)) {
    diagnostics_.push_back(Diagnostic{DiagnosticLevel::Error,
                                      "Expected 'assign' statement",
                                      current_.location});
    return Result<AssignStmt>{std::nullopt, {}};
  }

  Token lhs_token;
  if (!ExpectIdentifier("Expected identifier on left-hand side of assign", &lhs_token)) {
    return Result<AssignStmt>{std::nullopt, {}};
  }
  stmt.lhs = lhs_token.lexeme;

  if (!Expect(TokenKind::Equal, "Expected '=' in assign statement")) {
    return Result<AssignStmt>{std::nullopt, {}};
  }

  auto rhs = ParseExpression();
  if (!rhs.Ok()) {
    return Result<AssignStmt>{std::nullopt, {}};
  }
  stmt.rhs = std::move(*rhs.value);

  if (!Expect(TokenKind::Semicolon, "Expected ';' after assign statement")) {
    return Result<AssignStmt>{std::nullopt, {}};
  }

  return Result<AssignStmt>{std::move(stmt), {}};
}

Result<Expression> Parser::ParseExpression() {
  auto lhs = ParsePrimaryExpression();
  if (!lhs.Ok()) {
    return Result<Expression>{std::nullopt, {}};
  }

  while (current_.kind == TokenKind::Ampersand) {
    Token op_token = Consume();
    auto rhs = ParsePrimaryExpression();
    if (!rhs.Ok()) {
      return Result<Expression>{std::nullopt, {}};
    }

    Expression combined;
    combined.location = op_token.location;
    combined.kind = Expression::Kind::Binary;
    combined.text = op_token.lexeme;
    combined.lhs = std::make_unique<Expression>(std::move(*lhs.value));
    combined.rhs = std::make_unique<Expression>(std::move(*rhs.value));
    lhs = Result<Expression>{std::move(combined), {}};
  }

  return lhs;
}

Result<Expression> Parser::ParsePrimaryExpression() {
  Expression expr;
  expr.location = current_.location;

  if (current_.kind == TokenKind::Identifier) {
    expr.kind = Expression::Kind::Identifier;
    expr.text = Consume().lexeme;
    return Result<Expression>{std::move(expr), {}};
  }

  if (current_.kind == TokenKind::Number) {
    expr.kind = Expression::Kind::Number;
    expr.text = Consume().lexeme;
    return Result<Expression>{std::move(expr), {}};
  }

  diagnostics_.push_back(Diagnostic{DiagnosticLevel::Error,
                                    "Expected identifier or number in expression",
                                    current_.location});
  return Result<Expression>{std::nullopt, {}};
}

Result<InstanceDecl> Parser::ParseInstanceDecl() {
  InstanceDecl decl;
  decl.location = current_.location;

  Token module_name;
  if (!ExpectIdentifier("Expected instance module name", &module_name)) {
    return Result<InstanceDecl>{std::nullopt, {}};
  }
  decl.module_name = module_name.lexeme;

  Token instance_name;
  if (!ExpectIdentifier("Expected instance name", &instance_name)) {
    return Result<InstanceDecl>{std::nullopt, {}};
  }
  decl.instance_name = instance_name.lexeme;

  if (!Expect(TokenKind::LParen, "Expected '(' after instance name")) {
    return Result<InstanceDecl>{std::nullopt, {}};
  }

  if (current_.kind != TokenKind::RParen) {
    while (true) {
      auto connection = ParseNamedConnection();
      if (!connection.Ok()) {
        return Result<InstanceDecl>{std::nullopt, {}};
      }
      decl.connections.push_back(std::move(*connection.value));

      if (!Match(TokenKind::Comma)) {
        break;
      }
    }
  }

  if (!Expect(TokenKind::RParen, "Expected ')' after instance connections") ||
      !Expect(TokenKind::Semicolon, "Expected ';' after instance declaration")) {
    return Result<InstanceDecl>{std::nullopt, {}};
  }

  return Result<InstanceDecl>{std::move(decl), {}};
}

Result<NamedConnection> Parser::ParseNamedConnection() {
  NamedConnection connection;
  connection.location = current_.location;

  if (!Expect(TokenKind::Dot, "Expected '.' to start named connection")) {
    return Result<NamedConnection>{std::nullopt, {}};
  }

  Token port_name;
  if (!ExpectIdentifier("Expected port name after '.'", &port_name)) {
    return Result<NamedConnection>{std::nullopt, {}};
  }
  connection.port_name = port_name.lexeme;

  if (!Expect(TokenKind::LParen, "Expected '(' after connection port name")) {
    return Result<NamedConnection>{std::nullopt, {}};
  }

  Token signal_name;
  if (!ExpectIdentifier("Expected signal name in named connection", &signal_name)) {
    return Result<NamedConnection>{std::nullopt, {}};
  }
  connection.signal_name = signal_name.lexeme;

  if (!Expect(TokenKind::RParen, "Expected ')' after connection signal")) {
    return Result<NamedConnection>{std::nullopt, {}};
  }

  return Result<NamedConnection>{std::move(connection), {}};
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

bool Parser::ExpectIdentifier(const char* message, Token* token) {
  if (current_.kind != TokenKind::Identifier) {
    diagnostics_.push_back(Diagnostic{DiagnosticLevel::Error, message, current_.location});
    return false;
  }
  *token = Consume();
  return true;
}

Result<std::vector<std::string>> Parser::ParseIdentifierList() {
  std::vector<std::string> identifiers;

  Token token;
  if (!ExpectIdentifier("Expected identifier", &token)) {
    return Result<std::vector<std::string>>{std::nullopt, {}};
  }
  identifiers.push_back(token.lexeme);

  while (Match(TokenKind::Comma)) {
    if (!ExpectIdentifier("Expected identifier after ','", &token)) {
      return Result<std::vector<std::string>>{std::nullopt, {}};
    }
    identifiers.push_back(token.lexeme);
  }

  return Result<std::vector<std::string>>{std::move(identifiers), {}};
}

}  // namespace mnf
