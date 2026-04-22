#include "mnf/lexer/lexer.h"

#include <cctype>
#include <unordered_map>
#include <utility>

namespace mnf {

namespace {

Token MakeToken(TokenKind kind,
                std::string lexeme,
                const std::string& file,
                std::size_t line,
                std::size_t column) {
  return Token{kind, std::move(lexeme), SourceLocation{file, line, column}};
}

}  // namespace

const char* ToString(TokenKind kind) {
  switch (kind) {
    case TokenKind::Module:
      return "Module";
    case TokenKind::EndModule:
      return "EndModule";
    case TokenKind::Input:
      return "Input";
    case TokenKind::Output:
      return "Output";
    case TokenKind::Wire:
      return "Wire";
    case TokenKind::Assign:
      return "Assign";
    case TokenKind::Identifier:
      return "Identifier";
    case TokenKind::Number:
      return "Number";
    case TokenKind::LParen:
      return "LParen";
    case TokenKind::RParen:
      return "RParen";
    case TokenKind::Comma:
      return "Comma";
    case TokenKind::Semicolon:
      return "Semicolon";
    case TokenKind::Dot:
      return "Dot";
    case TokenKind::Equal:
      return "Equal";
    case TokenKind::Ampersand:
      return "Ampersand";
    case TokenKind::EndOfFile:
      return "EndOfFile";
    case TokenKind::Invalid:
    default:
      return "Invalid";
  }
}

Lexer::Lexer(std::string input, std::string file_name)
    : input_(std::move(input)), file_name_(std::move(file_name)) {}

Token Lexer::NextToken() {
  if (has_peeked_) {
    has_peeked_ = false;
    return peeked_;
  }
  return LexToken();
}

Token Lexer::PeekToken() {
  if (!has_peeked_) {
    peeked_ = LexToken();
    has_peeked_ = true;
  }
  return peeked_;
}

Token Lexer::LexToken() {
  SkipWhitespace();
  while (!IsAtEnd() && CurrentChar() == '/' && pos_ + 1 < input_.size() && input_[pos_ + 1] == '/') {
    SkipComment();
    SkipWhitespace();
  }

  const std::size_t start_line = line_;
  const std::size_t start_column = column_;

  if (IsAtEnd()) {
    return MakeToken(TokenKind::EndOfFile, "", file_name_, start_line, start_column);
  }

  const char ch = Advance();
  switch (ch) {
    case '(':
      return MakeToken(TokenKind::LParen, "(", file_name_, start_line, start_column);
    case ')':
      return MakeToken(TokenKind::RParen, ")", file_name_, start_line, start_column);
    case ',':
      return MakeToken(TokenKind::Comma, ",", file_name_, start_line, start_column);
    case ';':
      return MakeToken(TokenKind::Semicolon, ";", file_name_, start_line, start_column);
    case '.':
      return MakeToken(TokenKind::Dot, ".", file_name_, start_line, start_column);
    case '=':
      return MakeToken(TokenKind::Equal, "=", file_name_, start_line, start_column);
    case '&':
      return MakeToken(TokenKind::Ampersand, "&", file_name_, start_line, start_column);
    default:
      break;
  }

  if (std::isalpha(static_cast<unsigned char>(ch)) || ch == '_') {
    std::string lexeme(1, ch);
    while (!IsAtEnd()) {
      const char current = CurrentChar();
      if (!std::isalnum(static_cast<unsigned char>(current)) && current != '_') {
        break;
      }
      lexeme.push_back(Advance());
    }

    static const std::unordered_map<std::string, TokenKind> keywords = {
        {"module", TokenKind::Module},
        {"endmodule", TokenKind::EndModule},
        {"input", TokenKind::Input},
        {"output", TokenKind::Output},
        {"wire", TokenKind::Wire},
        {"assign", TokenKind::Assign},
    };

    const auto it = keywords.find(lexeme);
    if (it != keywords.end()) {
      return MakeToken(it->second, lexeme, file_name_, start_line, start_column);
    }
    return MakeToken(TokenKind::Identifier, lexeme, file_name_, start_line, start_column);
  }

  if (std::isdigit(static_cast<unsigned char>(ch))) {
    std::string lexeme(1, ch);
    while (!IsAtEnd() && std::isdigit(static_cast<unsigned char>(CurrentChar()))) {
      lexeme.push_back(Advance());
    }
    return MakeToken(TokenKind::Number, lexeme, file_name_, start_line, start_column);
  }

  return MakeToken(TokenKind::Invalid, std::string(1, ch), file_name_, start_line, start_column);
}

void Lexer::SkipWhitespace() {
  while (!IsAtEnd()) {
    const char ch = CurrentChar();
    if (ch == ' ' || ch == '\t' || ch == '\r') {
      Advance();
      continue;
    }
    if (ch == '\n') {
      Advance();
      continue;
    }
    break;
  }
}

void Lexer::SkipComment() {
  while (!IsAtEnd() && CurrentChar() != '\n') {
    Advance();
  }
}

char Lexer::CurrentChar() const {
  return input_[pos_];
}

char Lexer::Advance() {
  const char ch = input_[pos_++];
  if (ch == '\n') {
    ++line_;
    column_ = 1;
  } else {
    ++column_;
  }
  return ch;
}

bool Lexer::IsAtEnd() const {
  return pos_ >= input_.size();
}

}  // namespace mnf
