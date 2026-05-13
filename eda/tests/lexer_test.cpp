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

TEST(LexerTest, TokenizesAlwaysRegBeginEndKeywords) {
  const std::string input = R"(module top(a, y);
  input a;
  output y;
  reg state;
  always begin
    state = a;
  end
endmodule
)";
  mnf::Lexer lexer(input, "lexer_keywords_test.nl");

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
      mnf::TokenKind::Semicolon,
      mnf::TokenKind::Output,
      mnf::TokenKind::Identifier,
      mnf::TokenKind::Semicolon,
      mnf::TokenKind::Reg,
      mnf::TokenKind::Identifier,
      mnf::TokenKind::Semicolon,
      mnf::TokenKind::Always,
      mnf::TokenKind::Begin,
      mnf::TokenKind::Identifier,
      mnf::TokenKind::Equal,
      mnf::TokenKind::Identifier,
      mnf::TokenKind::Semicolon,
      mnf::TokenKind::End,
      mnf::TokenKind::EndModule,
      mnf::TokenKind::EndOfFile,
  };

  for (mnf::TokenKind kind : expected) {
    const mnf::Token token = lexer.NextToken();
    EXPECT_EQ(token.kind, kind);
  }
}
