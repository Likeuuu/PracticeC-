#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 ]]; then
  echo "Usage: tools/run_compiled_sim <input.nl> [top] [--input=name=0|1] [--prev=name=0|1]" >&2
  exit 2
fi

input_file="$1"
shift
top_name="top"
if [[ $# -gt 0 && "$1" != --* ]]; then
  top_name="$1"
  shift
fi

repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="$repo_dir/build"
tmp_dir="${TMPDIR:-/tmp}/mnf_compiled_sim"
mkdir -p "$tmp_dir"

cmake --build "$build_dir" >/dev/null

cpp_path="$tmp_dir/sim.cpp"
exe_path="$tmp_dir/sim"

"$build_dir/mnf_cli" "$input_file" "$top_name" --format=cpp --print-nets "$@" > "$cpp_path"
g++ -std=c++17 -O2 "$cpp_path" -o "$exe_path"
"$exe_path"
