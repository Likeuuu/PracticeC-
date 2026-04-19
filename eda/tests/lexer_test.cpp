#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "mnf/lexer/lexer.h"
#include "mnf/lexer/token.h"

TEST(LexerTest, TokenizesModuleDeclarationAndSkipsComments) {
  const std::string input = R"(module top(a, b); // comment
input a, b;
endmodule
)";
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
    EXPECT_EQ(token.kind, kind);
  }
}
