# Walkthrough

This walkthrough is designed for interview rehearsal and project explanation.

Use this input:

```bash
./build/mnf_cli examples/combinational_demo.nl top --format=text
```

## Demo Source

`examples/combinational_demo.nl` contains three modules:

- `leaf`
- `mid`
- `top`

The important signals are:

- `and_out`
- `mid_out`
- `u_mid.mid_wire`
- `u_mid.u_leaf.leaf_wire`

## What To Explain

### 1. Parsing

The parser reads:

- module declarations
- port declarations
- wire declarations
- instance declarations
- assign statements
- expression trees with precedence

For example:

```verilog
assign and_out = (in1 & in2) ^ ~in3;
```

This becomes an AST expression tree rather than a flat token list.

### 2. Semantic Analysis

Semantic checking answers:

- is every referenced module declared
- is every referenced signal declared
- are there duplicate ports / wires

At this stage the tool still does not know which concrete hierarchical object a
name refers to. It only checks whether the design is structurally meaningful.

### 3. Elaboration

Elaboration walks hierarchy and resolves names into concrete net ids.

Important artifacts:

- `nets`
- `assigns`
- `instance_bindings`
- `scope_frames`

`ScopeFrame` is used during elaboration to answer:

"inside this instance path, what does this identifier mean right now?"

The resolved graph keeps readable scope snapshots so we can inspect the result.

### 4. Scheduling

Each `assign` keeps:

- `rhs_expr`
- `source_net_ids`

`rhs_expr` preserves meaning.
`source_net_ids` is used to build the assign dependency DAG.

That DAG is topologically sorted to create a valid combinational evaluation
order.

### 5. Evaluation

Evaluation:

- seeds input values
- walks assigns in dependency order
- computes output net values

This is still a combinational evaluator, not a full event-driven simulator.

## How To Read The Text Output

The text summary is meant to be read top to bottom:

1. top module / module order
2. resolved nets
3. scope frames
4. resolved assigns
5. instance bindings
6. hierarchy summary

When explaining the project, focus on these transitions:

- source text -> AST
- AST -> validated names
- validated names -> resolved hierarchy
- resolved hierarchy -> dependency DAG
- dependency DAG -> combinational evaluation

## Good Interview Summary

A short project summary that fits this codebase:

> I built a small Verilog-style frontend in C++ that parses modules and
> expressions, performs semantic checks, elaborates hierarchy into resolved net
> ids, builds an assign dependency DAG, and evaluates combinational logic in
> topological order. I also kept both resolved expression trees and flattened
> source dependency caches because they serve different purposes in scheduling
> and debugging.
