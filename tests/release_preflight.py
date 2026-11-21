#!/usr/bin/env python3
"""Preflight checks that do not require downloading a toolchain."""
from pathlib import Path
import json
import re

root = Path(__file__).resolve().parent.parent
manifest = json.loads((root / "openrac-mod.json").read_text(encoding="utf-8"))
source = (root / "src/mod.cpp").read_text(encoding="utf-8")

assert manifest["id"] == "vendor-buy-all"
assert manifest["games"] == {"rac1": ["pal", "ntsc"], "rac2": ["ntsc", "pal", "ntsc2"]}
assert re.fullmatch(r"[0-9]+\.[0-9]+\.[0-9]+(?:-[0-9A-Za-z.-]+)?", manifest["version"])
assert f'OPENRAC_MOD_CPP("{manifest["id"]}", "{manifest["version"]}", load)' in source
assert manifest["api"] == "1.20"
assert manifest["hooks"] == ["rac2.vendor.draw_list"]
assert (root / "src/purchase_plan.hpp").is_file()
assert manifest["license"] == "MIT"
assert (root / "LICENSE").is_file(), "Ship the declared MIT license with all releases"
print(f'Preflight passed: {manifest["id"]} {manifest["version"]}')
