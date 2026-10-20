#!/usr/bin/env bash
# Compila e roda o teste host-side da logica pura do backtrack.
set -eu
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${TMPDIR:-/tmp}/bt_core_test"

g++ -std=c++17 -O1 -Wall -Wextra -I "$ROOT/cl_dll" \
	"$ROOT/tests/backtrack_core_test.cpp" "$ROOT/cl_dll/backtrack_core.cpp" \
	-o "$BIN"
"$BIN"
