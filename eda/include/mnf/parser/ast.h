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

struct RegDecl : AstNode {
  std::vector<std::string> names;
};

struct Expression : AstNode {
  enum class Kind {
    Identifier,
    Number,
    Unary,
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

struct ProceduralAssignStmt : AstNode {
  enum class AssignmentKind {
    Blocking,
    NonBlocking
  };

  AssignmentKind assignment_kind = AssignmentKind::Blocking;
  std::string lhs;
  Expression rhs;
};

struct ProceduralIfStmt;

struct ProceduralStmt : AstNode {
  enum class Kind {
    Assignment,
    Block,
    If
  };

  Kind kind = Kind::Assignment;
  std::unique_ptr<ProceduralAssignStmt> assign_stmt;
  std::unique_ptr<ProceduralIfStmt> if_stmt;
  std::vector<std::unique_ptr<ProceduralStmt>> statements;
};

struct ProceduralIfStmt : AstNode {
  Expression condition;
  std::unique_ptr<ProceduralStmt> then_stmt;
};

struct AlwaysBlock : AstNode {
  enum class SensitivityKind {
    Implicit,
    CombinationalStar
  };

  SensitivityKind sensitivity_kind = SensitivityKind::Implicit;
  std::unique_ptr<ProceduralStmt> body;
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
  std::vector<RegDecl> reg_decls;
  std::vector<AssignStmt> assign_stmts;
  std::vector<AlwaysBlock> always_blocks;
  std::vector<InstanceDecl> instances;
};

struct Program : AstNode {
  std::vector<std::unique_ptr<ModuleDecl>> modules;
};

}  // namespace mnf
