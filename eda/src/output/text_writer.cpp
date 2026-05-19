#include "mnf/output/text_writer.h"

#include <sstream>

namespace mnf {

namespace {

void WriteInstanceSummary(std::ostringstream& oss, const InstanceIR& instance, int depth) {
  oss << std::string(static_cast<std::size_t>(depth) * 2, ' ')
      << "- " << instance.instance_name << " : " << instance.module_name << "\n";
  for (const auto& child : instance.children) {
    WriteInstanceSummary(oss, child, depth + 1);
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

const char* AlwaysSensitivityKindToString(ResolvedAlwaysIR::SensitivityKind kind) {
  switch (kind) {
    case ResolvedAlwaysIR::SensitivityKind::CombinationalStar:
      return "@(*)";
    case ResolvedAlwaysIR::SensitivityKind::ExplicitList:
      return "@(list)";
    case ResolvedAlwaysIR::SensitivityKind::Implicit:
    default:
      return "<implicit>";
  }
}

const char* ProceduralAssignKindToString(ResolvedProceduralAssignIR::AssignmentKind kind) {
  switch (kind) {
    case ResolvedProceduralAssignIR::AssignmentKind::NonBlocking:
      return "<=";
    case ResolvedProceduralAssignIR::AssignmentKind::Blocking:
    default:
      return "=";
  }
}

std::string FormatResolvedExpr(const ResolvedExprIR& expr) {
  switch (expr.kind) {
    case ResolvedExprIR::Kind::Net:
      return "net[" + std::to_string(expr.net_id) + "]";
    case ResolvedExprIR::Kind::Constant:
      return std::to_string(expr.constant_value);
    case ResolvedExprIR::Kind::Unary: {
      const std::string rhs = expr.rhs != nullptr ? FormatResolvedExpr(*expr.rhs) : "<null>";
      return "(" + expr.op + rhs + ")";
    }
    case ResolvedExprIR::Kind::Binary: {
      const std::string lhs = expr.lhs != nullptr ? FormatResolvedExpr(*expr.lhs) : "<null>";
      const std::string rhs = expr.rhs != nullptr ? FormatResolvedExpr(*expr.rhs) : "<null>";
      return "(" + lhs + " " + expr.op + " " + rhs + ")";
    }
  }

  return "<expr?>";
}

void WriteResolvedProcessStmt(std::ostringstream& oss,
                              const ResolvedProcessStmtIR& stmt,
                              int depth) {
  const std::string indent(static_cast<std::size_t>(depth) * 2, ' ');
  if (stmt.kind == ResolvedProcessStmtIR::Kind::Assignment) {
    if (stmt.assign_stmt != nullptr) {
      oss << indent
          << "assign " << stmt.assign_stmt->target_name_view
          << " " << ProceduralAssignKindToString(stmt.assign_stmt->assignment_kind)
          << " " << FormatResolvedExpr(stmt.assign_stmt->rhs_expr)
          << "\n";
    }
    return;
  }

  if (stmt.kind == ResolvedProcessStmtIR::Kind::If) {
    if (stmt.if_stmt != nullptr) {
      oss << indent << "if " << FormatResolvedExpr(stmt.if_stmt->condition_expr) << "\n";
      if (stmt.if_stmt->then_stmt != nullptr) {
        WriteResolvedProcessStmt(oss, *stmt.if_stmt->then_stmt, depth + 1);
      }
    }
    return;
  }

  oss << indent << "begin\n";
  for (const auto& nested : stmt.statements) {
    if (nested != nullptr) {
      WriteResolvedProcessStmt(oss, *nested, depth + 1);
    }
  }
  oss << indent << "end\n";
}

}  // namespace

std::string TextWriter::WriteSummary(const ElaboratedDesign& design) const {
  std::ostringstream oss;
  oss << "Top: " << design.top_name << "\n";
  oss << "Module count: " << design.modules.size() << "\n";
  oss << "Module dependencies: " << design.module_dependencies.size() << "\n";
  if (!design.module_order.empty()) {
    oss << "Module order: ";
    for (std::size_t i = 0; i < design.module_order.size(); ++i) {
      if (i != 0) {
        oss << " -> ";
      }
      oss << design.module_order[i];
    }
    oss << "\n";
  }
  oss << "Resolved hierarchical nets: " << design.top_graph.nets.size() << "\n";
  for (const auto& net : design.top_graph.nets) {
    oss << "  net[" << net.id << "] " << net.qualified_name << " (" << NetKindToString(net.kind) << ")\n";
  }
  oss << "Resolved scope frames: " << design.top_graph.scope_frames.size() << "\n";
  for (const auto& frame : design.top_graph.scope_frames) {
    oss << "  scope@" << (frame.instance_path.empty() ? "<top>" : frame.instance_path)
        << " module=" << frame.module_name << "\n";
    for (const auto& symbol : frame.symbols) {
      oss << "    " << symbol.name
          << " kind=" << ScopeKindToString(symbol.kind)
          << " decl=" << symbol.decl_name
          << " view=" << symbol.qualified_name
          << " -> net[" << symbol.net_id << "]\n";
    }
  }
  oss << "Resolved hierarchical assigns: " << design.top_graph.assigns.size() << "\n";
  for (const auto& assign : design.top_graph.assigns) {
    oss << "  assign@" << (assign.instance_path.empty() ? "<top>" : assign.instance_path)
        << " target=" << assign.target_net_id
        << " expr=" << FormatResolvedExpr(assign.rhs_expr)
        << "\n";
  }
  oss << "Resolved hierarchical always blocks: " << design.top_graph.always_blocks.size() << "\n";
  for (const auto& always_block : design.top_graph.always_blocks) {
    oss << "  always@" << (always_block.instance_path.empty() ? "<top>" : always_block.instance_path) << "\n";
    if (always_block.body != nullptr) {
      WriteResolvedProcessStmt(oss, *always_block.body, 2);
    }
  }
  oss << "Resolved hierarchical instance bindings: " << design.top_graph.instance_bindings.size() << "\n";
  for (const auto& binding : design.top_graph.instance_bindings) {
    oss << "  bind@" << binding.instance_path << " " << binding.port_name << " -> net[" << binding.signal_net_id << "]\n";
  }
  oss << "Top instances: " << design.top_instances.size() << "\n";
  for (const auto& instance : design.top_instances) {
    WriteInstanceSummary(oss, instance, 1);
  }
  return oss.str();
}

}  // namespace mnf
