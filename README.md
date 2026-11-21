# RAC2 Buy All Ammo

An OpenRAC native quality-of-life mod for **Ratchet & Clank 2: Going Commando**.

At the **Megacorp weapon vendor**, press **Square** (PlayStation controller) or **X** (Xbox controller) to refill the ammunition sold by that vendor for your owned weapons. Bolts are deducted at the game's normal per-round prices. When you cannot afford everything, the mod buys what you can afford in the vendor's list order.

- Skips unowned weapons, full ammo, items not sold by that vendor, and upgrades.
- Runs only in the vendor list, not the buy/confirm dialog; standard vendor buttons remain unchanged.
- Supports RAC2 NTSC-U v1.01 (`ntsc`), PAL (`pal`), and NTSC-U v2.00 (`ntsc2`).
- No in-game button hint or purchase message yet; the purchase is logged.

## Status

**Development prototype** (`0.1.0-dev.1`). Calculation unit tests pass, but the native game mod has **not yet been tested in RAC2**. Back up your save before testing. Important in-game checks: controller input, shop detection, displayed ammunition, purchase feedback, saving/loading, and multiplayer/other-mod compatibility.

## Downloads: `dev` versus `main`

The same `.github/workflows/build.yml` runs on both branches and produces six platforms:

| Platform | Archive suffix |
|---|---|
| Windows x64 | `windows-x86_64` |
| Linux x64 | `linux-x86_64` |
| Linux ARM64 | `linux-aarch64` |
| macOS Apple Silicon | `macos-arm64` |
| macOS Intel | `macos-x86_64` |
| Android ARM64 | `android-arm64` |

**`dev`:** Each push builds/tests/packages the mod and adds downloadable **Actions → build → latest run → Artifacts**. `mod-release` contains the single combined all-platform ZIP, six individual ZIPs and `SHA256SUMS`. The six `mod-<platform>` artifacts are provided separately too. Nothing on `dev` publishes a GitHub Release.

**`main`:** The workflow makes the same downloadable artifacts. After all six builds pass, a push with a *new stable* version in `openrac-mod.json` (for example `0.1.0`, rather than `0.1.0-dev.1`) automatically creates the corresponding GitHub Release (`v0.1.0`), containing the combined ZIP, six platform ZIPs and checksums. Re-pushing the same version preserves its existing release. Pull requests and manual workflow runs do not publish releases.

To test, download the `mod-release` Actions artifact ZIP, extract the **`rac2-buy-all-ammo-<version>.zip`** inside it, and install that combined ZIP in OpenRAC's launcher. **Do not install** the outer GitHub Actions artifact ZIP.

## Building locally

Requires Python 3, CMake 3.20+, Ninja and a C++17 compiler:

```bash
python tools/fetch_sdk.py
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The SDK version and SHA256 are pinned in `sdk.lock`. To use a local SDK instead, set `-DOPENRAC_SDK=/path/to/sdk` when configuring CMake. The SDK's `tools/package_mod.py` produces the platform ZIP from `build/package/rac2_buy_all_ammo`.

## Implementation notes

The mod reads a PS2-style pad report (Square is the Xbox X mapping), checks the live vendor game state, iterates that vendor's ammo entries, reads each weapon's owned flag, active record, current ammo, capacity and per-round price, calculates a bounded purchase, and uses the OpenRAC SDK's bolts interface to deduct the cost. Source for the game's structures: OpenRAC's `ng-plus-plus/src/discs.h`, `difficulty-mod/src/rac2/discs.h`, and `rac2-decomp/src/game/002F5220-purchase.c`.

## License

MIT. OpenRAC's SDK is separately licensed.
