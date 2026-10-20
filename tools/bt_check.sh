#!/usr/bin/env bash
# Valida o backtrack sem abrir o jogo:
#   1. testes da logica pura
#   2. invariantes da tabela de cvars + paridade com o manifesto
#   3. cvars do manifesto presentes na tab do HUB
#   4. simbolos e cvars no client.so (se ja foi compilado)
set -eu

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

TMP="${TMPDIR:-/tmp}"

echo "== 1/4 logica pura =="
./tools/run_bt_tests.sh

echo "== 2/4 tabela de cvars =="
g++ -std=c++17 -O1 -Wall -Wextra -I cl_dll -DBT_ROOT="\"$ROOT\"" \
	tests/bt_config_test.cpp cl_dll/backtrack_cfg.cpp -o "$TMP/bt_cfg_test"
"$TMP/bt_cfg_test"

echo "== 3/4 bindings no HUB =="
bash tools/check_hub_bindings.sh

echo "== 4/4 binario =="
if [ -f build/cl_dll/client.so ]; then
	COUNT=$(strings build/cl_dll/client.so | grep '^cl_bt_' | sort -u | wc -l)
	echo "cvars no client.so: $COUNT"
	[ "$COUNT" -eq 37 ] || { echo "FALHOU: esperado 37 cvars no binario"; exit 1; }
	for sym in Backtrack_GetPoint Bt_FilterAccepts Bt_TrimOlder Bt_LagWindow Bt_PartAllowed; do
		nm -C build/cl_dll/client.so | grep -q " T $sym" || {
			echo "FALHOU: simbolo $sym ausente"; exit 1; }
	done
	echo "simbolos: ok"
else
	echo "client.so ainda nao compilado, pulando"
fi

echo "BT CHECK: TUDO OK"
