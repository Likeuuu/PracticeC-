#include "mnf/output/text_writer.h"

#include <sstream>

namespace mnf {

std::string TextWriter::WriteSummary(const ElaboratedDesign& design) const {
  std::ostringstream oss;
  oss << "Top: " << design.top_name << "\n";
  oss << "Module count: " << design.modules.size() << "\n";
  return oss.str();
}

}  // namespace mnf
