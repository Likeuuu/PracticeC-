# Mini Netlist Frontend (MNF)

训练项目：小型 Verilog 前端 → 展平 → 解释器/编译仿真后端。

## 一句话概括

> 解析 Verilog 子集 → 语义检查 → 层次展平为 ResolvedNetGraphIR → 解释执行 或 生成 C++ 仿真代码。

## 支持的语法

| 类别 | 构造 |
|------|------|
| 模块 | `module/endmodule`、端口、`input/output/wire/reg` |
| 连续赋值 | `assign` |
| 实例化 | 命名端口连接 `.a(signal)` |
| 表达式 | 常量 0/1、标识符、`~`、`&`、`|`、`^`、括号 |
| 过程块 | `always begin/end`、`@(*)`、`@(a or b)`、`@(posedge clk)` |
| 过程赋值 | 阻塞 `=`、非阻塞 `<=`、`if`（无 else） |

**未支持**：位宽、四值逻辑、延迟、参数、完整事件调度等

## 流水线

```
源码 → Lexer → Parser → AST → 语义检查 → 符号表 → 展平器 → ResolvedNetGraphIR → 输出(text/json/cpp) → 解释器 或 C++代码生成
```

## 核心 IR：ResolvedNetGraphIR

展平后的长寿命 IR，包含：
- **nets** — 展平后的信号，带稳定 `net_id`
- **assigns** — 解析后的连续赋值
- **always_blocks** — 解析后的过程块
- **instance_bindings** — 层次展开后的端口绑定
- **scope_frames** — 层次命名快照

每条赋值同时保留 `rhs_expr`（语义树）和 `source_net_ids`（依赖缓存），前者用于解释/代码生成，后者用于调度。

## 构建 & 测试

```bash
cmake -S . -B build && cmake --build build
ctest --test-dir build --output-on-failure
```

## CLI

```bash
./build/mnf_cli <输入文件> [顶层模块] [--format=text|json|both|cpp]
```

| 格式 | 说明 |
|------|------|
| `text` | 人类可读的展平结果 |
| `json` | 机器可读的 IR |
| `both` | text + json |
| `cpp` | 生成 C++ 仿真源码 |

编译仿真选项：`--input=name=0|1`、`--prev=name=0|1`、`--print-nets`

## assign/always 如何降级为 C++

| Verilog | 生成 C++ |
|---------|----------|
| `assign out = a & b;` | `net_4 = net_0 & net_1;` |
| `always begin state = d; end` | `net_state = net_d;` |
| `always @(*)` | delta 循环内执行，直到稳定 |
| `always @(posedge clk)` | `if (prev_net_clk==0 && net_clk==1) {...}` |
| `state <= d;`（非阻塞） | `nb_net_state=net_d;` → 循环后提交 |

## 代码验证

若 libclang 可用，构建 `verify_codegen`，用 Clang AST 验证生成代码结构（非文本匹配）。

## 示例文件

| 文件 | 用途 |
|------|------|
| `simple.nl` | 最小模块 |
| `hierarchy.nl` | 层次展平 |
| `combinational_demo.nl` | 表达式+层次 |
| `codegen_hierarchy.nl` | 编译仿真 |
| `codegen_always.nl` | 过程块仿真 |
| `codegen_posedge.nl` | 上升沿+非阻塞 |

## 例子
```bash
./build/mnf_cli examples/hierarchy.nl
./build/mnf_cli examples/hierarchy.nl top --format=text
./build/mnf_cli examples/combinational_demo.nl top --format=json
./build/mnf_cli examples/combinational_demo.nl top --format=both
```
