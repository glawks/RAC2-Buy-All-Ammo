#!/usr/bin/env python3
"""Preflight checks that do not require downloading a toolchain."""
from pathlib import Path
import json
import re

root = Path(__file__).resolve().parent.parent
manifest = json.loads((root / "openrac-mod.json").read_text(encoding="utf-8"))
source = (root / "src/mod.cpp").read_text(encoding="utf-8")

assert manifest["id"] == "rac2-buy-all-ammo"
assert manifest["games"] == {"rac2": ["ntsc", "pal", "ntsc2"]}
assert re.fullmatch(r"[0-9]+\.[0-9]+\.[0-9]+(?:-[0-9A-Za-z.-]+)?", manifest["version"])
assert f'OPENRAC_MOD_CPP("{manifest["id"]}", "{manifest["version"]}", load)' in source
assert manifest["api"] == "1.20"
assert manifest["hooks"] == []
assert (root / "src/purchase_plan.hpp").is_file()
print(f'Preflight passed: {manifest["id"]} {manifest["version"]}')
