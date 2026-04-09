#pragma once

#include <string>

#include "mnf/lexer/token.h"

namespace mnf {

class Lexer {
public:
  Lexer(std::string input, std::string file_name = "");

  Token NextToken();
  Token PeekToken();

private:
  Token LexToken();
  void SkipWhitespace();
  void SkipComment();
  char CurrentChar() const;
  char Advance();
  bool IsAtEnd() const;

  std::string input_;
  std::string file_name_;
  std::size_t pos_ = 0;
  std::size_t line_ = 1;
  std::size_t column_ = 1;
  bool has_peeked_ = false;
  Token peeked_;
};

}  // namespace mnf
