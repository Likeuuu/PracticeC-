#include "mnf/output/json_writer.h"

#include <sstream>

namespace mnf {

std::string JsonWriter::Write(const ElaboratedDesign& design) const {
  std::ostringstream oss;
  oss << "{\n";
  oss << "  \"top\": \"" << design.top_name << "\",\n";
  oss << "  \"modules\": [";
  for (std::size_t i = 0; i < design.modules.size(); ++i) {
    if (i != 0) {
      oss << ", ";
    }
    oss << "\"" << design.modules[i].name << "\"";
  }
  oss << "]\n";
  oss << "}\n";
  return oss.str();
}

}  // namespace mnf
