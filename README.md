# RAC2 Buy All Ammo

An OpenRAC native quality-of-life mod for **Ratchet & Clank 2: Going Commando**.

At the **Megacorp weapon vendor**, press **Square** (PlayStation controller) or **X** (Xbox controller) to refill the ammunition sold by that vendor for your owned weapons. Bolts are deducted at the game's normal per-round prices. When you cannot afford everything, the mod buys what you can afford in the vendor's list order.

- Skips unowned weapons, full ammo, items not sold by that vendor, and upgrades.
- Runs only in the vendor list, not the buy/confirm dialog; standard vendor buttons remain unchanged.
- Supports RAC2 NTSC-U v1.01 (`ntsc`), PAL (`pal`), and NTSC-U v2.00 (`ntsc2`).
- The vendor now shows a HUD control hint (`Square / Xbox X`) and a live refill price, or a partial-purchase / full-ammo / insufficient-bolts status. After purchase, the HUD confirms the bolts spent for about two seconds. There is also an Android touch action (Square) on the vendor screen. Purchases continue to be logged.

## Status

**Development prototype** (`0.1.0-dev.5`). Purchase and HUD-quote unit tests pass, but the native game mod has **not yet been tested in RAC2 with this build**. Back up your save before testing. Important in-game checks: controller input, shop detection, displayed ammunition, purchase feedback, saving/loading, and multiplayer/other-mod compatibility.

## Changes in dev.5

- Moved the prompt into its own bordered box below the normal Buy control, avoiding overlap when manual ammo purchases are available.
- Kept the price, full-ammo and purchase-confirmation text within that box.
- Pressing buy-all when no ammo can be purchased plays the vendor's native rejection sound once, including on Android.

## Changes in dev.4

- Centred a larger, two-line prompt within the left vendor panel using native font metrics.
- Replaced the literal controller names with the native Square glyph, using OpenRAC's active-input prompt handling like the vendor Exit control.

## Changes in dev.3

- Fixed the missing vendor prompt by using native vendor drawing instead of the camera-dependent overlay.
- Default prompt placement is the empty left panel beside the ammo preview.
- Successful buy-all transactions play the normal vendor purchase sound once.

## Vendor HUD

The compact vendor prompt appears only in the weapon vendor's **main list**, not inside a purchase confirmation, cutscene, or other menu. It shows the action, exact affordable cost (or partial purchase when you cannot afford a full refill), and a brief purchase confirmation. Under **Mod settings**, `Show vendor ammo HUD` is on by default; `Vendor HUD position` defaults to the **Below vendor controls**, in a separate box below the normal Buy prompt, with **Bottom right** and **Top right** available if the vendor UI overlaps it.

The normal vendor controls remain unchanged. On Android the mod also adds a contextual Square action button in the vendor, because OpenRAC's default touch vendor layout has no Square button. The action stays available in the main vendor list so full-ammo and insufficient-bolts attempts also play the rejection sound. The prompt uses RAC2's own font drawing from the vendor draw hook, so it does not depend on a world camera being available. A successful buy-all plays the same vendor purchase sound as a normal ammo purchase, once per transaction. Visual placement and sound in this build still need in-game testing.

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

**`main`:** The workflow makes the same downloadable artifacts. After all six builds pass, a push with a *new stable* version in `openrac-mod.json` (for example `0.1.0`, rather than `0.1.0-dev.5`) automatically creates the corresponding GitHub Release (`v0.1.0`), containing the combined ZIP, six platform ZIPs and checksums. Re-pushing the same version preserves its existing release. Pull requests and manual workflow runs do not publish releases.

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

The HUD uses `Vendor_DrawList` with the native small font and guest stack text storage. It draws before the original list, which restores its own clipping rectangle. The gameplay tick publishes the quote; the vendor draw hook renders it on the game thread. The purchase sound calls `PlayMobySound(7, 17, vendor.moby)`, matching the normal vendor ammo transaction. The mod reads a PS2-style pad report (Square is the Xbox X mapping), checks the live vendor game state, iterates that vendor's ammo entries, reads each weapon's owned flag, active record, current ammo, capacity and per-round price, calculates a bounded purchase, and uses the OpenRAC SDK's bolts interface to deduct the cost. Source for the game's structures: OpenRAC's `ng-plus-plus/src/discs.h`, `difficulty-mod/src/rac2/discs.h`, and `rac2-decomp/src/game/002F5220-purchase.c`.

## License

MIT. OpenRAC's SDK is separately licensed.
