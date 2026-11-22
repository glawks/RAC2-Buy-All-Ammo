#!/usr/bin/env python3
"""Stage the pinned OpenRAC SDK (release ZIP, or development source commit)."""
import hashlib
import json
import shutil
import subprocess
import urllib.request
import zipfile
from io import BytesIO
from pathlib import Path

root = Path(__file__).resolve().parent.parent
lock = json.loads((root / "sdk.lock").read_text(encoding="utf-8"))
dest = root / ".sdk"
folder = dest / f"openrac-mod-sdk-{lock['version']}"

if (folder / "cmake" / "OpenRACModConfig.cmake").is_file():
    print(folder)
elif "source_commit" in lock:
    # No public SDK 1.31 ZIP yet. Fetch only the sdk/ subtree from the exact
    # reviewed OpenRAC commit. Never follow a moving branch for CI builds.
    tmp = dest / ".openrac-sdk-source"
    if tmp.exists():
        shutil.rmtree(tmp)
    dest.mkdir(parents=True, exist_ok=True)
    try:
        subprocess.run(["git", "init", str(tmp)], check=True, stdout=subprocess.DEVNULL)
        def git(*args):
            subprocess.run(["git", "-C", str(tmp), *args], check=True,
                           stdout=subprocess.DEVNULL)
        git("remote", "add", "origin", lock["source_repository"])
        git("sparse-checkout", "init", "--cone")
        git("sparse-checkout", "set", lock.get("source_path", "sdk"))
        git("-c", "protocol.version=2", "fetch", "--depth=1", "--filter=blob:none",
            "--no-tags", "origin", lock["source_commit"])
        git("checkout", "--detach", "FETCH_HEAD")
        actual = subprocess.check_output(
            ["git", "-C", str(tmp), "rev-parse", "HEAD"], text=True).strip()
        if actual != lock["source_commit"]:
            raise ValueError(f"SDK commit mismatch: {actual}")
        shutil.copytree(tmp / lock.get("source_path", "sdk"), folder)
        if not (folder / "cmake" / "OpenRACModConfig.cmake").is_file():
            raise ValueError("Pinned commit is missing the SDK CMake package")
    finally:
        if tmp.exists():
            try:
                shutil.rmtree(tmp)
            except PermissionError:
                # Windows runners can briefly keep a Git pack file locked after
                # checkout. The source checkout is ignored in .sdk/; a verified,
                # staged SDK is sufficient, so cleanup need not fail the build.
                if not (folder / "cmake" / "OpenRACModConfig.cmake").is_file():
                    raise
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
