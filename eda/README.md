# Mini Netlist Frontend

A C++17 training project for a simplified Verilog / digital simulation frontend.

This project is intentionally built around the capability set that appears in
digital simulation / Verilog simulator development roles:

- lexer / parser / AST construction
- semantic analysis and symbol handling
- elaboration and hierarchical name resolution
- resolved net graph construction
- combinational scheduling and evaluation
- readable IR / debug output

The goal is not to build a full commercial simulator. The goal is to grow the
engineering habits and mental model needed to explain and implement core parts
of a simulator frontend in interviews.

## Build

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## CLI

```bash
./build/mnf_cli examples/hierarchy.nl
./build/mnf_cli examples/hierarchy.nl top --format=text
./build/mnf_cli examples/combinational_demo.nl top --format=both
```

Formats:

- `text`: readable walkthrough output for interviews and learning
- `json`: machine-friendly IR dump
- `both`: text section followed by JSON section

## Recommended Reading Order

1. `docs/architecture.md`
2. `docs/walkthrough.md`
3. `docs/grammar.md`
4. `src/parser/parser.cpp`
5. `src/semantic/semantic_checker.cpp`
6. `src/elaboration/elaborator.cpp`
7. `src/sim/combinational_evaluator.cpp`
