#!/usr/bin/env python3
"""Gera cl_dll/ui/hub_cvars_list.h a partir de tools/hub_cvars.txt.

Rode depois de editar o manifesto:
    python3 tools/gen_hub_cvars.py
"""

import pathlib

root = pathlib.Path(__file__).resolve().parent.parent
manifest = root / "tools" / "hub_cvars.txt"
out = root / "cl_dll" / "ui" / "hub_cvars_list.h"

names = [
    line.strip()
    for line in manifest.read_text(encoding="utf-8").splitlines()
    if line.strip() and not line.strip().startswith("#")
]

lines = [
    "/* GERADO por tools/gen_hub_cvars.py - nao edite a mao. */",
    "#pragma once",
    "",
    "static const char *kHubCvars[] =",
    "{",
]
lines += ['\t"%s",' % n for n in names]
lines += ["};", "", "static const int kHubCvarCount = (int)( sizeof( kHubCvars ) / sizeof( kHubCvars[0] ));", ""]

out.write_text("\n".join(lines), encoding="utf-8")
print("gerado %s com %d cvars" % (out, len(names)))
