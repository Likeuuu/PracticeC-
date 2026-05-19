#include "mnf/output/json_writer.h"

#include <sstream>

namespace mnf {

namespace {

void WriteInstance(std::ostringstream& oss, const InstanceIR& instance) {
  oss << "{\"module\": \"" << instance.module_name << "\", ";
  oss << "\"instance\": \"" << instance.instance_name << "\", ";
  oss << "\"connections\": [";
  for (std::size_t i = 0; i < instance.connections.size(); ++i) {
    if (i != 0) {
      oss << ", ";
    }
    oss << "{\"port\": \"" << instance.connections[i].port_name << "\", ";
    oss << "\"signal\": \"" << instance.connections[i].signal_name << "\"}";
  }
  oss << "]";
  if (!instance.children.empty()) {
    oss << ", \"children\": [";
    for (std::size_t i = 0; i < instance.children.size(); ++i) {
      if (i != 0) {
        oss << ", ";
      }
      WriteInstance(oss, instance.children[i]);
    }
    oss << "]";
  }
  oss << "}";
}

void WriteResolvedExpr(std::ostringstream& oss, const ResolvedExprIR& expr) {
  switch (expr.kind) {
    case ResolvedExprIR::Kind::Net:
      oss << "{\"kind\": \"net\", \"net\": " << expr.net_id << "}";
      return;
    case ResolvedExprIR::Kind::Constant:
      oss << "{\"kind\": \"constant\", \"value\": " << expr.constant_value << "}";
      return;
    case ResolvedExprIR::Kind::Unary:
      oss << "{\"kind\": \"unary\", \"op\": \"" << expr.op << "\", \"rhs\": ";
      if (expr.rhs != nullptr) {
        WriteResolvedExpr(oss, *expr.rhs);
      } else {
        oss << "null";
      }
      oss << "}";
      return;
    case ResolvedExprIR::Kind::Binary:
      oss << "{\"kind\": \"binary\", \"op\": \"" << expr.op << "\", \"lhs\": ";
      if (expr.lhs != nullptr) {
        WriteResolvedExpr(oss, *expr.lhs);
      } else {
        oss << "null";
      }
      oss << ", \"rhs\": ";
      if (expr.rhs != nullptr) {
        WriteResolvedExpr(oss, *expr.rhs);
      } else {
        oss << "null";
      }
      oss << "}";
      return;
  }
}

const char* ScopeKindToString(ResolvedScopeSymbolIR::Kind kind) {
  switch (kind) {
    case ResolvedScopeSymbolIR::Kind::Port:
      return "port";
    case ResolvedScopeSymbolIR::Kind::Reg:
      return "reg";
    case ResolvedScopeSymbolIR::Kind::Wire:
    default:
      return "wire";
  }
}

const char* NetKindToString(ResolvedNetIR::Kind kind) {
  switch (kind) {
    case ResolvedNetIR::Kind::Port:
      return "port";
    case ResolvedNetIR::Kind::Reg:
      return "reg";
    case ResolvedNetIR::Kind::Wire:
    default:
      return "wire";
  }
}

const char* AlwaysSensitivityKindToString(ResolvedAlwaysIR::SensitivityKind kind) {
  switch (kind) {
    case ResolvedAlwaysIR::SensitivityKind::CombinationalStar:
      return "combinational_star";
    case ResolvedAlwaysIR::SensitivityKind::ExplicitList:
      return "explicit_list";
    case ResolvedAlwaysIR::SensitivityKind::Implicit:
    default:
      return "implicit";
  }
}

const char* ProceduralAssignKindToString(ResolvedProceduralAssignIR::AssignmentKind kind) {
  switch (kind) {
    case ResolvedProceduralAssignIR::AssignmentKind::NonBlocking:
      return "nonblocking";
    case ResolvedProceduralAssignIR::AssignmentKind::Blocking:
    default:
      return "blocking";
  }
}

void WriteResolvedProcessStmt(std::ostringstream& oss, const ResolvedProcessStmtIR& stmt) {
  if (stmt.kind == ResolvedProcessStmtIR::Kind::Assignment) {
    oss << "{\"kind\": \"assignment\"";
    if (stmt.assign_stmt != nullptr) {
      oss << ", \"assignment_kind\": \"" << ProceduralAssignKindToString(stmt.assign_stmt->assignment_kind) << "\"";
      oss << ", \"target\": " << stmt.assign_stmt->target_net_id;
      oss << ", \"target_name\": \"" << stmt.assign_stmt->target_name_view << "\"";
      oss << ", \"expr\": ";
      WriteResolvedExpr(oss, stmt.assign_stmt->rhs_expr);
    }
    oss << "}";
    return;
  }

  if (stmt.kind == ResolvedProcessStmtIR::Kind::If) {
    oss << "{\"kind\": \"if\"";
    if (stmt.if_stmt != nullptr) {
      oss << ", \"condition\": ";
      WriteResolvedExpr(oss, stmt.if_stmt->condition_expr);
      oss << ", \"then\": ";
      if (stmt.if_stmt->then_stmt != nullptr) {
        WriteResolvedProcessStmt(oss, *stmt.if_stmt->then_stmt);
      } else {
        oss << "null";
      }
    }
    oss << "}";
    return;
  }

  oss << "{\"kind\": \"block\", \"statements\": [";
  for (std::size_t i = 0; i < stmt.statements.size(); ++i) {
    if (i != 0) {
      oss << ", ";
    }
    if (stmt.statements[i] != nullptr) {
      WriteResolvedProcessStmt(oss, *stmt.statements[i]);
    } else {
      oss << "null";
    }
  }
  oss << "]}";
}

}  // namespace

std::string JsonWriter::Write(const ElaboratedDesign& design) const {
  std::ostringstream oss;
  oss << "{\n";
  oss << "  \"top\": \"" << design.top_name << "\",\n";
  oss << "  \"modules\": [";
  for (std::size_t i = 0; i < design.modules.size(); ++i) {
    if (i != 0) {
      oss << ", ";
    }
    oss << "{\"name\": \"" << design.modules[i].name << "\", \"ports\": [";
    for (std::size_t j = 0; j < design.modules[i].ports.size(); ++j) {
      if (j != 0) {
        oss << ", ";
      }
      oss << "\"" << design.modules[i].ports[j] << "\"";
    }
    oss << "]}";
  }
  oss << "],\n";
  oss << "  \"module_dependencies\": [";
  for (std::size_t i = 0; i < design.module_dependencies.size(); ++i) {
    if (i != 0) {
      oss << ", ";
    }
    oss << "{\"from\": \"" << design.module_dependencies[i].from_module
        << "\", \"to\": \"" << design.module_dependencies[i].to_module << "\"}";
  }
  oss << "],\n";
  oss << "  \"module_order\": [";
  for (std::size_t i = 0; i < design.module_order.size(); ++i) {
    if (i != 0) {
      oss << ", ";
    }
    oss << "\"" << design.module_order[i] << "\"";
  }
  oss << "],\n";
  oss << "  \"top_graph\": {";
  oss << "\"nets\": [";
  for (std::size_t i = 0; i < design.top_graph.nets.size(); ++i) {
    if (i != 0) {
      oss << ", ";
    }
    oss << "{\"id\": " << design.top_graph.nets[i].id
        << ", \"name\": \"" << design.top_graph.nets[i].name
        << "\", \"qualified_name\": \"" << design.top_graph.nets[i].qualified_name
        << "\", \"kind\": \"" << NetKindToString(design.top_graph.nets[i].kind)
        << "\"}";
  }
  oss << "], ";
  oss << "\"scope_frames\": [";
  for (std::size_t i = 0; i < design.top_graph.scope_frames.size(); ++i) {
    if (i != 0) {
      oss << ", ";
    }
    const auto& frame = design.top_graph.scope_frames[i];
    oss << "{\"instance_path\": \"" << frame.instance_path
        << "\", \"module\": \"" << frame.module_name << "\", \"symbols\": [";
    for (std::size_t j = 0; j < frame.symbols.size(); ++j) {
      if (j != 0) {
        oss << ", ";
      }
      const auto& symbol = frame.symbols[j];
      oss << "{\"name\": \"" << symbol.name
          << "\", \"decl_name\": \"" << symbol.decl_name
          << "\", \"qualified_name\": \"" << symbol.qualified_name
          << "\", \"kind\": \"" << ScopeKindToString(symbol.kind)
          << "\", \"net\": " << symbol.net_id << "}";
    }
    oss << "]}";
  }
  oss << "], ";
  oss << "\"assigns\": [";
  for (std::size_t i = 0; i < design.top_graph.assigns.size(); ++i) {
    if (i != 0) {
      oss << ", ";
    }
    oss << "{\"instance_path\": \"" << design.top_graph.assigns[i].instance_path
        << "\", \"target\": " << design.top_graph.assigns[i].target_net_id << ", \"expr\": ";
    WriteResolvedExpr(oss, design.top_graph.assigns[i].rhs_expr);
    oss << "}";
  }
  oss << "], ";
  oss << "\"always_blocks\": [";
  for (std::size_t i = 0; i < design.top_graph.always_blocks.size(); ++i) {
    if (i != 0) {
      oss << ", ";
    }
    oss << "{\"instance_path\": \"" << design.top_graph.always_blocks[i].instance_path << "\", \"body\": ";
    if (design.top_graph.always_blocks[i].body != nullptr) {
      WriteResolvedProcessStmt(oss, *design.top_graph.always_blocks[i].body);
    } else {
      oss << "null";
    }
    oss << "}";
  }
  oss << "], ";
  oss << "\"instance_bindings\": [";
  for (std::size_t i = 0; i < design.top_graph.instance_bindings.size(); ++i) {
    if (i != 0) {
      oss << ", ";
    }
    oss << "{\"instance_path\": \"" << design.top_graph.instance_bindings[i].instance_path
        << "\", \"module\": \"" << design.top_graph.instance_bindings[i].module_name
        << "\", \"port\": \"" << design.top_graph.instance_bindings[i].port_name
        << "\", \"signal\": " << design.top_graph.instance_bindings[i].signal_net_id << "}";
  }
  oss << "]},\n";
  oss << "  \"top_instances\": [";
  for (std::size_t i = 0; i < design.top_instances.size(); ++i) {
    if (i != 0) {
      oss << ", ";
    }
    WriteInstance(oss, design.top_instances[i]);
  }
  oss << "]\n";
  oss << "}\n";
  return oss.str();
}

}  // namespace mnf
