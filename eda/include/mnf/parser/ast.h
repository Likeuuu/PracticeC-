#pragma once

#include <memory>
#include <string>
#include <vector>

#include "mnf/common/source_location.h"

namespace mnf {

struct AstNode {
  SourceLocation location;
  virtual ~AstNode() = default;
};

struct PortDecl : AstNode {
  enum class Direction {
    Input,
    Output
  };

  Direction direction = Direction::Input;
  std::vector<std::string> names;
};

struct WireDecl : AstNode {
  std::vector<std::string> names;
};

struct Expression : AstNode {
  enum class Kind {
    Identifier,
    Number,
    Binary
  };

  Kind kind = Kind::Identifier;
  std::string text;
  std::unique_ptr<Expression> lhs;
  std::unique_ptr<Expression> rhs;
};

struct AssignStmt : AstNode {
  std::string lhs;
  Expression rhs;
};

struct NamedConnection : AstNode {
  std::string port_name;
  std::string signal_name;
};

struct InstanceDecl : AstNode {
  std::string module_name;
  std::string instance_name;
  std::vector<NamedConnection> connections;
};

struct ModuleDecl : AstNode {
  std::string name; // module name
  std::vector<std::string> ports;
  std::vector<PortDecl> port_decls;
  std::vector<WireDecl> wire_decls;
  std::vector<AssignStmt> assign_stmts;
  std::vector<InstanceDecl> instances;
};

struct Program : AstNode {
  std::vector<std::unique_ptr<ModuleDecl>> modules;
};

}  // namespace mnf
