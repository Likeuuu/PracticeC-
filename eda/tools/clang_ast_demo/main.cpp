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

struct VisitorContext {
  std::vector<std::string> lines;
};

enum CXChildVisitResult VisitCursor(CXCursor cursor, CXCursor /*parent*/, CXClientData client_data) {
  auto* context = static_cast<VisitorContext*>(client_data);
  if (clang_getCursorKind(cursor) != CXCursor_FunctionDecl) {
    return CXChildVisit_Recurse;
  }

  const std::string function_name = ToString(clang_getCursorSpelling(cursor));
  const std::string return_type = ToString(clang_getTypeSpelling(clang_getCursorResultType(cursor)));
  const int arg_count = clang_Cursor_getNumArguments(cursor);

  std::string line = "function " + function_name + " returns " + return_type +
                     " args=" + std::to_string(arg_count) + " [";

  for (int i = 0; i < arg_count; ++i) {
    if (i != 0) {
      line += ", ";
    }
    const CXCursor arg_cursor = clang_Cursor_getArgument(cursor, i);
    const std::string arg_name = ToString(clang_getCursorSpelling(arg_cursor));
    const std::string arg_type = ToString(clang_getTypeSpelling(clang_getCursorType(arg_cursor)));
    line += arg_type + " " + arg_name;
  }
  line += "]";

  context->lines.push_back(std::move(line));
  return CXChildVisit_Recurse;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "usage: clang_ast_demo <source-file> [-- <clang-args...>]\n";
    return 1;
  }

  std::string source_file = argv[1];
  std::vector<const char*> clang_args;
  for (int i = 2; i < argc; ++i) {
    clang_args.push_back(argv[i]);
  }

  CXIndex index = clang_createIndex(0, 0);
  CXTranslationUnit tu = clang_parseTranslationUnit(index,
                                                    source_file.c_str(),
                                                    clang_args.empty() ? nullptr : clang_args.data(),
                                                    static_cast<int>(clang_args.size()),
                                                    nullptr,
                                                    0,
                                                    CXTranslationUnit_None);
  if (tu == nullptr) {
    std::cerr << "failed to parse translation unit: " << source_file << "\n";
    clang_disposeIndex(index);
    return 2;
  }

  const unsigned diagnostic_count = clang_getNumDiagnostics(tu);
  for (unsigned i = 0; i < diagnostic_count; ++i) {
    CXDiagnostic diagnostic = clang_getDiagnostic(tu, i);
    std::cerr << ToString(clang_formatDiagnostic(diagnostic, clang_defaultDiagnosticDisplayOptions())) << "\n";
    clang_disposeDiagnostic(diagnostic);
  }

  VisitorContext context;
  clang_visitChildren(clang_getTranslationUnitCursor(tu), VisitCursor, &context);

  for (const auto& line : context.lines) {
    std::cout << line << "\n";
  }

  clang_disposeTranslationUnit(tu);
  clang_disposeIndex(index);
  return 0;
}
