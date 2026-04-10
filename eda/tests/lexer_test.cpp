#include <cassert>
#include <string>
#include <vector>

#include "mnf/lexer/lexer.h"
#include "mnf/lexer/token.h"

int main() {
  const std::string input = "module top(a, b); // comment\ninput a, b;\nendmodule\n";
  mnf::Lexer lexer(input, "lexer_test.nl");

  const std::vector<mnf::TokenKind> expected = {
      mnf::TokenKind::Module,
      mnf::TokenKind::Identifier,
      mnf::TokenKind::LParen,
      mnf::TokenKind::Identifier,
      mnf::TokenKind::Comma,
      mnf::TokenKind::Identifier,
      mnf::TokenKind::RParen,
      mnf::TokenKind::Semicolon,
      mnf::TokenKind::Input,
      mnf::TokenKind::Identifier,
      mnf::TokenKind::Comma,
      mnf::TokenKind::Identifier,
      mnf::TokenKind::Semicolon,
      mnf::TokenKind::EndModule,
      mnf::TokenKind::EndOfFile,
  };

  for (mnf::TokenKind kind : expected) {
    const mnf::Token token = lexer.NextToken();
    assert(token.kind == kind);
  }

  return 0;
}
