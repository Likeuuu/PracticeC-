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
  oss << "Top instances: " << design.top_instances.size() << "\n";
  for (const auto& instance : design.top_instances) {
    WriteInstanceSummary(oss, instance, 1);
  }
  return oss.str();
}

}  // namespace mnf
