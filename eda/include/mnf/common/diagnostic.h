#pragma once

#include <string>

#include "mnf/common/source_location.h"

namespace mnf {

enum class DiagnosticLevel {
  Info,
  Warning,
  Error
};

struct Diagnostic {
  DiagnosticLevel level = DiagnosticLevel::Error;
  std::string message;
  SourceLocation location;
};

}  // namespace mnf
