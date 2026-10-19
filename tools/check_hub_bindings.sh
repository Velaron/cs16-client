#!/usr/bin/env bash
# Falha se alguma cvar do manifesto nao estiver ligada a um widget do HUB.
#
# As cvars de hitbox por modo (cl_silent_* / cl_psilent_*) sao montadas em
# runtime na tab Hitboxes, com snprintf(prefixo + sufixo). Para elas a
# auditoria confere o prefixo e cada sufixo separadamente.
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

  case "$name" in
    cl_silent_*|cl_psilent_*)
      suffix="${name#cl_}"
      suffix="${suffix#silent_}"
      suffix="${suffix#psilent_}"
      if ! grep -q "\"$suffix\"" "$SRC"; then
        echo "MISSING: $name (sufixo \"$suffix\" na tab Hitboxes)"
        missing=$((missing+1))
      fi
      continue
      ;;
  esac

  if ! grep -q "\"$name\"" "$SRC"; then
    echo "MISSING: $name"
    missing=$((missing+1))
  fi
done < "$MANIFEST"

# os tres prefixos de perfil tambem precisam existir no codigo
for p in cl_aim_ cl_silent_ cl_psilent_; do
  if ! grep -q "\"$p\"" "$SRC"; then
    echo "MISSING: prefixo $p"
    missing=$((missing+1))
  fi
done

echo "total=$total missing=$missing"
[ "$missing" -eq 0 ] || exit 1
echo "OK: todas as cvars do manifesto estao no HUB"
