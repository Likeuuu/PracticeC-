#pragma once

#include <string>

#include "mnf/common/source_location.h"

namespace mnf {

enum class TokenKind {
  Module,
  EndModule,
  Input,
  Output,
  Wire,
  Assign,
  Identifier,
  Number,
  LParen,
  RParen,
  Comma,
  Semicolon,
  Dot,
  Equal,
  Ampersand,
  EndOfFile,
  Invalid
};

struct Token {
  TokenKind kind = TokenKind::Invalid;
  std::string lexeme;
  SourceLocation location;
};

const char* ToString(TokenKind kind);

}  // namespace mnf
