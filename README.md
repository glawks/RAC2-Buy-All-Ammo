# Vendor Buy All

**Development test build: 0.2.2-dev.3 — SDK 1.31 (not a stable release).** The public v0.2.1 release remains unchanged. [Latest development builds](https://github.com/glawks/Vendor-Buy-All/actions).

Buy ammunition for all your owned weapons in one action at the Gadgetron vendor in **Ratchet & Clank (2002)** or the Megacorp vendor in **Ratchet & Clank 2: Going Commando**. No need to refill each weapon separately.

**[Download the latest release](https://github.com/glawks/Vendor-Buy-All/releases/latest)** · **[Report a problem](https://github.com/glawks/Vendor-Buy-All/issues)**

## Features

- **Buy all ammo:** press the control mapped to **Square** (PlayStation) / **X** (Xbox) while browsing the vendor's main weapon list.
- **Normal shop prices:** refill each eligible, owned weapon using the vendor's current ammo prices. If you're short on bolts, the mod buys as much as you can afford in vendor-list order.
- **Safe purchases:** skip unowned weapons, full ammo, weapon/upgrade purchases and stock the vendor does not sell. Normal individual purchases remain available; Buy All is disabled inside purchase-confirmation dialogs.
- **Purchase feedback:** the vendor's own success or rejection sound, plus a HUD quote, full-ammo notice, insufficient-bolts status or brief confirmation.
- **Android support:** contextual Square action using OpenRAC's touch controls.

## Supported games and platforms

| Game | Supported disc versions |
| --- | --- |
| Ratchet & Clank | PAL, NTSC-U |
| Ratchet & Clank 2: Going Commando | NTSC-U v1.01, PAL, NTSC-U v2.00 |

The release includes native builds for **Windows x64, Linux x64/ARM64, macOS Intel/Apple Silicon and Android ARM64**. Requires OpenRAC with **mod API 1.20 or later**.

## Installation

1. If you installed the older **RAC2 Buy All Ammo** mod (`rac2-buy-all-ammo`), remove or disable it first. This release has a new mod ID, `vendor-buy-all`; running both can cause duplicate controls.
2. In the **OpenRAC Launcher → Mods**, choose **Add from a link** and enter:

   `https://github.com/glawks/Vendor-Buy-All`

3. Enable **Vendor Buy All** and start your supported game. Mod settings take effect on the next game start.

You can also [download `vendor-buy-all-0.2.1.zip`](https://github.com/glawks/Vendor-Buy-All/releases/tag/v0.2.1), the combined release archive, and install it manually through the launcher. Individual platform ZIPs and `SHA256SUMS` are also available. The SDK-validated release packages include this README and the MIT licence.

## HUD and controls — SDK 1.31 development build

**Ratchet & Clank (RAC1):** the Buy All prompt now uses OpenRAC's menu-overlay API, drawing a dark translucent panel with a compact live quote between the Buy and Exit monitors, above the weapon carousel. The `OPENRAC_UI_SQUARE` token displays the **native Square icon on PlayStation**, **X on Xbox**, or **the currently bound keyboard key**, automatically, without a manual controller-label setting.

The panel shows the purchase cost, partial-refill quote, ammo-full status or insufficient bolts. After purchasing, it displays `AMMO BOUGHT FOR 200 BOLTS` (with the actual price) without a button icon. It disappears when you leave the vendor or enter a normal purchase dialog.

**Going Commando (RAC2):** the existing vendor prompt and purchase logic are unchanged. **RAC2 vendor HUD position** offers three locations. **Show vendor ammo HUD** controls prompt visibility in both games.

**Compatibility:** this test build requires an OpenRAC development port supporting **mod API 1.31**, as introduced by [PR #691](https://github.com/OpenRAC/OpenRAC/pull/691). It will not load in the older API 1.20 port. The SDK is pinned to the merged OpenRAC commit in `sdk.lock` until a stable SDK 1.31 archive is released.

### Downloading this test build

Open [GitHub Actions](https://github.com/glawks/Vendor-Buy-All/actions), choose the latest successful `dev` workflow run, download **Artifacts → mod-release**, extract the outer Actions ZIP, and install the inner `vendor-buy-all-0.2.2-dev.3.zip` with OpenRAC.

**Do not install it beside the upstream `ammo-max` mod**, which already includes Buy All. Disable the upstream copy when testing this standalone build, to avoid duplicated input handlers.

## Development release notes — 0.2.2-dev.3

- Lowered the RAC1 Buy All panel by six pixels (native Y 268 → 274) to give the rotating shop weapon more clearance.

- Added RAC1 SDK 1.31 menu overlay with translucent background and native device-aware Square token; the confirmation is `AMMO BOUGHT FOR X BOLTS` without an action icon.
- Removed the old manual RAC1 controller-label setting from this dev build.
- Left RAC1 purchase/sound logic and RAC2 vendor integration unchanged.
- Added PAL/NTSC mock runtime tests of the menu-overlay callback and native button token.
- **Requires in-game visual verification**; the stable v0.2.1 release is unaffected.

## Stable release notes — 0.2.1

- First combined release line for RAC1 and RAC2 (`0.2.0` introduced RAC1 support).
- Updated release documentation and added the complete **MIT licence** to the packaged archives.
- Added CI checks for README/licence inclusion in both platform builds and the combined release archive.
- No changes to ammo-purchase mechanics, sounds or HUD placement since 0.2.0.

### Verification

All six platform builds are checked by GitHub Actions. Automated tests cover the purchase planner, PAL/NTSC RAC1 adapters, one purchase per press, prices and partial purchases, vendor/dialog exclusions, success/rejection sound IDs, HUD cleanup and keyboard/controller text changes.

The main RAC1 and RAC2 purchase paths and vendor sounds were tested in-game during development. The latest compact RAC1 HUD text/outline and the PDA pricing path have automated coverage but have **not yet had a final in-game visual/behaviour confirmation**. OpenRAC Mods maintainers may request additional testing.

## Development

The `main` branch is the released version. The `dev` branch is for test builds; its successful Actions runs provide downloadable artifacts without publishing stable GitHub releases.

To build locally, install Python 3, CMake 3.20+, Ninja and a C++17 compiler:

```bash
python tools/fetch_sdk.py
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The `dev` branch pins SDK 1.31 to an exact merged OpenRAC commit in `sdk.lock` and uses a sparse Git checkout until its stable SDK ZIP is published. Use `-DOPENRAC_SDK=/path/to/sdk` to build against a local SDK. The common purchase planner lives in `src/purchase_plan.hpp`; the RAC1/RAC2 adapters use SDK memory/functions and native sound callbacks. RAC1 uses `on_overlay_phases` to draw over the vendor menu; RAC2 continues using its existing vendor draw hook.

## Licence

MIT — see [LICENSE](LICENSE). OpenRAC and its mod SDK have their own licences. No game-disc assets are distributed.
