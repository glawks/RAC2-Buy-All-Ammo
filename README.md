# Vendor Buy All

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

## HUD and controls

**Ratchet & Clank (RAC1):** a compact, single-line Buy All prompt sits between the Buy and Exit monitors, above the weapon carousel. For example, `X Buy All - 150 bolts`. The current quote and status update automatically; a dark text outline keeps the label legible.

- Recent **keyboard input** switches the label to `F` (the default keyboard Square binding). **Controller button activity** restores the controller label.
- The default controller label is **X**. PlayStation users can select **Square** in the mod's **RAC1 controller button label** setting.
- The mod does not identify the make of a controller automatically. With a custom keyboard layout, the text `F` may differ from your Square binding; pressing your mapped Square button still triggers the purchase.

**Going Commando (RAC2):** a separate framed prompt appears near the Buy control, with the game's native button icons. **RAC2 vendor HUD position** can move it below the vendor controls, bottom-right or top-right.

The **Show vendor ammo HUD** setting controls prompt visibility for both games; Buy All remains functional when the prompt is hidden.

### Current HUD limitations

RAC1's SDK 1.20 independent HUD-text API does not render the game's special button glyphs, so it uses **F**, **X** or **Square** as text instead of a graphical icon. The SDK's rectangle overlay is unavailable in vendor menus, so RAC1 uses a soft dark text outline rather than a filled background panel. These are UI limitations, not purchase-function limitations.

## Release notes — 0.2.1

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

SDK 1.20 is pinned by version and SHA-256 in `sdk.lock`. Use `-DOPENRAC_SDK=/path/to/sdk` to build against a local SDK. The common purchase planner lives in `src/purchase_plan.hpp`; the RAC1/RAC2 adapters use SDK memory/functions and native sound callbacks. RAC1 presents its HUD through `hud_text_set`; RAC2 uses the vendor draw hook.

## Licence

MIT — see [LICENSE](LICENSE). OpenRAC and its mod SDK have their own licences. No game-disc assets are distributed.
