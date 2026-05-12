# Architecture Notes

## Project Goal

This project trains the core ideas behind a digital simulation frontend:

- parse HDL-like source text
- validate names and structure
- elaborate hierarchy into resolved objects
- build dependency information
- schedule combinational assigns
- evaluate signal propagation

It is intentionally small, but each layer is designed to reflect the shape of a
real simulator / compiler-style pipeline.

## Pipeline

### 1. Lexer

Input source text becomes a token stream.

Responsibilities:

- split characters into tokens
- detect lexical errors
- preserve source locations

### 2. Parser

Token stream becomes AST.

Responsibilities:

- build module / port / wire / instance / assign structure
- build expression trees with precedence
- detect syntax errors

### 3. Semantic Analysis

AST is checked for basic meaning.

Responsibilities:

- build module symbol table
- detect duplicate ports / wires
- validate instance references
- validate declared signal usage inside expressions

Current scope:

- global module lookup is implemented
- full typed symbol infrastructure is still intentionally minimal

### 4. Elaboration

AST names are resolved into concrete hierarchical circuit objects.

Responsibilities:

- walk module hierarchy
- bind instance connections
- resolve identifiers into concrete `net_id`
- build resolved net graph
- snapshot readable scope views

Key idea:

- `ScopeFrame` is a process-time name resolution environment
- `scope_frames` inside resolved IR are human-readable snapshots for debugging

### 5. Scheduling

Combinational assigns are turned into an execution order.

Responsibilities:

- use `source_net_ids` cache to build assign dependency graph
- topologically order combinational assigns
- detect combinational cycles
- detect multiple combinational drivers on the same net

### 6. Evaluation

Resolved graph is evaluated for a combinational input assignment.

Responsibilities:

- set input values
- evaluate resolved expression trees
- propagate values in dependency order
- return diagnostics when evaluation assumptions are violated

## Important IR Design Choices

### `rhs_expr` and `source_net_ids` are both kept

Why keep both?

- `rhs_expr` keeps the true expression semantics
- `source_net_ids` is a flattened dependency cache for scheduling / graph work

This mirrors a common compiler / EDA pattern:

- keep the rich tree for meaning
- keep a cheap index for repeated analysis

### `ScopeFrame` is not the final circuit

`ScopeFrame` exists to answer: "what does this name mean right now in this
hierarchical context?"

It is an elaboration-time object, not the final netlist object model.

The final long-lived data is stored in:

- `nets`
- `assigns`
- `instance_bindings`
- `scope_frames` snapshots

## What This Project Already Demonstrates Well

- recursive descent parsing
- expression tree construction
- semantic vs elaboration separation
- hierarchical instance expansion
- resolved expression tree vs flattened dependency cache
- DAG-based combinational scheduling

## What Is Still Deliberately Simplified

- no full Verilog grammar
- no `always` / sequential logic
- no event queue / delta cycle simulator
- no bit-width / 4-value logic model
- no full industrial symbol/type system

## Interview Value

This project is strong when explaining:

- the difference between AST, semantic checking, elaboration, and scheduling
- why hierarchy resolution needs scope
- why simulators often mix tree IR and flat dependency caches
- how combinational logic can be scheduled as a DAG
