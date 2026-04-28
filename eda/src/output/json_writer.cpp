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
        << "\", \"kind\": \"" << (design.top_graph.nets[i].kind == ResolvedNetIR::Kind::Port ? "port" : "wire")
        << "\"}";
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
