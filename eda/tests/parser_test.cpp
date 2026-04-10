#include <cassert>
#include <string>

#include "mnf/lexer/lexer.h"
#include "mnf/parser/parser.h"

int main() {
  const std::string input = R"(module leaf(a, y);
  input a;
  output y;
endmodule

module top(in1, out1);
  input in1;
  output out1;
  wire w1;
  leaf u1(.a(in1), .y(out1));
endmodule
)";

  mnf::Lexer lexer(input, "parser_test.nl");
  mnf::Parser parser(lexer);
  auto result = parser.ParseProgram();

  assert(result.Ok());
  assert(result.value->modules.size() == 2);

  const mnf::ModuleDecl& leaf = *result.value->modules[0];
  assert(leaf.name == "leaf");
  assert(leaf.ports.size() == 2);
  assert(leaf.port_decls.size() == 2);

  const mnf::ModuleDecl& top = *result.value->modules[1];
  assert(top.name == "top");
  assert(top.wire_decls.size() == 1);
  assert(top.instances.size() == 1);
  assert(top.instances[0].module_name == "leaf");
  assert(top.instances[0].instance_name == "u1");
  assert(top.instances[0].connections.size() == 2);
  assert(top.instances[0].connections[0].port_name == "a");
  assert(top.instances[0].connections[0].signal_name == "in1");

  return 0;
}
