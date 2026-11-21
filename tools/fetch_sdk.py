#!/usr/bin/env python3
"""Download and verify the pinned OpenRAC SDK, then extract it to .sdk/."""
import hashlib
import json
import urllib.request
import zipfile
from io import BytesIO
from pathlib import Path

root = Path(__file__).resolve().parent.parent
lock = json.loads((root / "sdk.lock").read_text())
dest = root / ".sdk"
folder = dest / f"openrac-mod-sdk-{lock['version']}"
if (folder / "cmake" / "OpenRACModConfig.cmake").is_file():
    print(folder)
else:
    data = urllib.request.urlopen(lock["url"], timeout=60).read()
    got = hashlib.sha256(data).hexdigest()
    if got != lock["sha256"]:
        raise ValueError(f"SDK SHA256 mismatch: got {got}")
    dest.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(BytesIO(data)) as archive:
        for info in archive.infolist():
            path = Path(info.filename)
            if path.is_absolute() or ".." in path.parts or not info.filename.startswith(folder.name + "/"):
                raise ValueError("Unexpected path in SDK zip")
        archive.extractall(dest)
    print(folder)
