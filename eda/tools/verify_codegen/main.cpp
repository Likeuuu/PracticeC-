#include <clang-c/Index.h>

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

std::string ToString(CXString text) {
  const char* cstr = clang_getCString(text);
  std::string value = cstr != nullptr ? cstr : "";
  clang_disposeString(text);
  return value;
}

bool IsFromMainFile(CXCursor cursor) {
  CXSourceLocation location = clang_getCursorLocation(cursor);
  return clang_Location_isFromMainFile(location) != 0;
}

bool StartsWith(const std::string& value, const std::string& prefix) {
  return value.rfind(prefix, 0) == 0;
}

struct VerifyContext {
  std::string expected_struct_name;
  int net_field_count = 0;
  int prev_net_field_count = 0;
  int evaluate_binary_operator_count = 0;
  int evaluate_net_reference_count = 0;
  bool found_struct = false;
  bool in_target_struct = false;
  bool in_evaluate_method = false;
  bool found_evaluate_method = false;
  bool found_main_function = false;
};

enum CXChildVisitResult VisitCursor(CXCursor cursor, CXCursor parent, CXClientData client_data) {
  auto* context = static_cast<VerifyContext*>(client_data);
  const CXCursorKind kind = clang_getCursorKind(cursor);

  if (!IsFromMainFile(cursor)) {
    return CXChildVisit_Continue;
  }

  if (kind == CXCursor_StructDecl) {
    const bool previous_in_target_struct = context->in_target_struct;
    const std::string name = ToString(clang_getCursorSpelling(cursor));
    if (name == context->expected_struct_name) {
      context->found_struct = true;
      context->in_target_struct = true;
      clang_visitChildren(cursor, VisitCursor, context);
      context->in_target_struct = previous_in_target_struct;
      return CXChildVisit_Continue;
    }
  }

  if (context->in_target_struct && kind == CXCursor_FieldDecl) {
    const std::string field_name = ToString(clang_getCursorSpelling(cursor));
    if (StartsWith(field_name, "net_")) {
      ++context->net_field_count;
    } else if (StartsWith(field_name, "prev_net_")) {
      ++context->prev_net_field_count;
    }
    return CXChildVisit_Continue;
  }

  if (context->in_evaluate_method) {
    if (kind == CXCursor_BinaryOperator || kind == CXCursor_CompoundAssignOperator) {
      ++context->evaluate_binary_operator_count;
    }

    if (kind == CXCursor_DeclRefExpr || kind == CXCursor_MemberRefExpr) {
      const std::string referenced_name = ToString(clang_getCursorSpelling(cursor));
      if (StartsWith(referenced_name, "net_") || StartsWith(referenced_name, "prev_net_")) {
        ++context->evaluate_net_reference_count;
      }
    }
  }

  if (kind == CXCursor_CXXMethod) {
    const std::string method_name = ToString(clang_getCursorSpelling(cursor));
    const std::string semantic_parent_name =
        ToString(clang_getCursorSpelling(clang_getCursorSemanticParent(cursor)));
    if (semantic_parent_name == context->expected_struct_name && method_name == "evaluate" &&
        clang_Cursor_getNumArguments(cursor) == 0 &&
        ToString(clang_getTypeSpelling(clang_getCursorResultType(cursor))) == "void") {
      context->found_evaluate_method = true;
      if (clang_isCursorDefinition(cursor) != 0) {
        const bool previous_in_evaluate_method = context->in_evaluate_method;
        context->in_evaluate_method = true;
        clang_visitChildren(cursor, VisitCursor, context);
        context->in_evaluate_method = previous_in_evaluate_method;
        return CXChildVisit_Continue;
      }
    }
    return CXChildVisit_Recurse;
  }

  if (kind == CXCursor_FunctionDecl) {
    const std::string function_name = ToString(clang_getCursorSpelling(cursor));
    if (function_name == "main" && clang_Cursor_getNumArguments(cursor) == 0) {
      context->found_main_function = true;
    }
    return CXChildVisit_Recurse;
  }

  return CXChildVisit_Recurse;
}

void PrintUsage() {
  std::cerr << "usage: verify_codegen <generated.cpp> <top-struct> <expected-net-count>\n";
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 4) {
    PrintUsage();
    return 2;
  }

  const std::string source_file = argv[1];
  const std::string top_struct = argv[2];
  const int expected_net_count = std::atoi(argv[3]);
  if (expected_net_count < 0) {
    std::cerr << "expected-net-count must be non-negative\n";
    return 2;
  }

  const char* clang_args[] = {"-std=c++17"};
  CXIndex index = clang_createIndex(0, 0);
  CXTranslationUnit tu = clang_parseTranslationUnit(index,
                                                    source_file.c_str(),
                                                    clang_args,
                                                    1,
                                                    nullptr,
                                                    0,
                                                    CXTranslationUnit_None);
  if (tu == nullptr) {
    std::cerr << "FAIL: failed to parse translation unit: " << source_file << "\n";
    clang_disposeIndex(index);
    return 1;
  }

  bool has_errors = false;
  const unsigned diagnostic_count = clang_getNumDiagnostics(tu);
  for (unsigned i = 0; i < diagnostic_count; ++i) {
    CXDiagnostic diagnostic = clang_getDiagnostic(tu, i);
    const CXDiagnosticSeverity severity = clang_getDiagnosticSeverity(diagnostic);
    if (severity >= CXDiagnostic_Error) {
      has_errors = true;
      std::cerr << ToString(clang_formatDiagnostic(diagnostic, clang_defaultDiagnosticDisplayOptions())) << "\n";
    }
    clang_disposeDiagnostic(diagnostic);
  }

  VerifyContext context;
  context.expected_struct_name = top_struct;
  clang_visitChildren(clang_getTranslationUnitCursor(tu), VisitCursor, &context);

  if (context.found_struct) {
    std::cout << "PASS: struct " << top_struct << " found\n";
  } else {
    has_errors = true;
    std::cout << "FAIL: struct " << top_struct << " not found\n";
  }

  if (context.net_field_count == expected_net_count) {
    std::cout << "PASS: net field count is " << context.net_field_count << "\n";
  } else {
    has_errors = true;
    std::cout << "FAIL: net field count is " << context.net_field_count
              << ", expected " << expected_net_count << "\n";
  }

  if (context.prev_net_field_count == expected_net_count) {
    std::cout << "PASS: previous-net field count is " << context.prev_net_field_count << "\n";
  } else {
    has_errors = true;
    std::cout << "FAIL: previous-net field count is " << context.prev_net_field_count
              << ", expected " << expected_net_count << "\n";
  }

  if (context.found_evaluate_method) {
    std::cout << "PASS: void " << top_struct << "::evaluate() found\n";
  } else {
    has_errors = true;
    std::cout << "FAIL: void " << top_struct << "::evaluate() not found\n";
  }

  if (context.evaluate_binary_operator_count > 0) {
    std::cout << "PASS: evaluate() contains " << context.evaluate_binary_operator_count
              << " binary/assignment operator AST nodes\n";
  } else {
    has_errors = true;
    std::cout << "FAIL: evaluate() contains no binary/assignment operator AST nodes\n";
  }

  if (context.evaluate_net_reference_count > 0) {
    std::cout << "PASS: evaluate() references generated net fields "
              << context.evaluate_net_reference_count << " times\n";
  } else {
    has_errors = true;
    std::cout << "FAIL: evaluate() does not reference generated net fields\n";
  }

  if (context.found_main_function) {
    std::cout << "PASS: main() found\n";
  } else {
    has_errors = true;
    std::cout << "FAIL: main() not found\n";
  }

  clang_disposeTranslationUnit(tu);
  clang_disposeIndex(index);
  return has_errors ? 1 : 0;
}
