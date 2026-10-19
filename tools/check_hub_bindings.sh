#!/usr/bin/env bash
# Falha se alguma cvar do manifesto nao estiver ligada a um widget do HUB.
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MANIFEST="$ROOT/tools/hub_cvars.txt"
SRC="$ROOT/cl_dll/ui/hub_menu.cpp"
missing=0; total=0

if [ ! -f "$SRC" ]; then
  echo "FAIL: $SRC nao existe"
  exit 1
fi

while read -r name; do
  case "$name" in ''|'#'*) continue;; esac
  total=$((total+1))
  if ! grep -q "\"$name\"" "$SRC"; then
    echo "MISSING: $name"
    missing=$((missing+1))
  fi
done < "$MANIFEST"

echo "total=$total missing=$missing"
[ "$missing" -eq 0 ] || exit 1
echo "OK: todas as cvars do manifesto estao no HUB"
