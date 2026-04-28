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
    case ResolvedNetIR::Kind::Wire:
    default:
      return "wire";
  }
}

std::string FormatResolvedExpr(const ResolvedExprIR& expr) {
  switch (expr.kind) {
    case ResolvedExprIR::Kind::Net:
      return "net[" + std::to_string(expr.net_id) + "]";
    case ResolvedExprIR::Kind::Constant:
      return std::to_string(expr.constant_value);
    case ResolvedExprIR::Kind::Binary: {
      const std::string lhs = expr.lhs != nullptr ? FormatResolvedExpr(*expr.lhs) : "<null>";
      const std::string rhs = expr.rhs != nullptr ? FormatResolvedExpr(*expr.rhs) : "<null>";
      return "(" + lhs + " " + expr.op + " " + rhs + ")";
    }
  }

  return "<expr?>";
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
  oss << "Resolved hierarchical assigns: " << design.top_graph.assigns.size() << "\n";
  for (const auto& assign : design.top_graph.assigns) {
    oss << "  assign@" << (assign.instance_path.empty() ? "<top>" : assign.instance_path)
        << " target=" << assign.target_net_id
        << " expr=" << FormatResolvedExpr(assign.rhs_expr)
        << "\n";
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
