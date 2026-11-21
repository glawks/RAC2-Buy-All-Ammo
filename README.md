# Vendor Buy All

An OpenRAC mod for **Ratchet & Clank** and **Ratchet & Clank 2: Going Commando**.

Press **Square** (PlayStation) or **X** (Xbox) in the vendor's main list to buy ammunition for all owned weapons whose ammo that vendor sells. Prices come from the game's live item records. If you cannot afford a full refill, the mod buys what you can afford in vendor-list order.

- Skips unowned weapons, weapon purchases, upgrades, full ammo and ammo with no selling price.
- Leaves normal single-item purchases available; buy-all is disabled inside their confirmation dialog.
- Shows the native button glyph, affordable price, partial-purchase status, full-ammo status and a brief purchase confirmation.
- Plays the game's normal purchase sound after a transaction, or its rejection sound if nothing can be bought.
- Adds a contextual Square touch action on Android.

## Development build

**0.2.0-dev.2** introduces RAC1 support and renames the mod ID from `rac2-buy-all-ammo` to **`vendor-buy-all`**. RAC2's purchase behaviour, native prompt and sounds were tested in-game during development. RAC1 purchases and both success/rejection sounds have also been confirmed in-game. **This build fixes the RAC1 prompt placement; its revised monitor layout still needs in-game testing**, along with PDA prices and save/load behaviour.

Remove the old **RAC2 Buy All Ammo** installation before enabling **Vendor Buy All**. The new ID is a separate mod to the launcher; leaving both active can cause duplicate input handlers and prompts. Settings under the old ID are not migrated.

| Game | Supported discs |
|---|---|
| Ratchet & Clank | PAL (`pal`), NTSC-U (`ntsc`) |
| Going Commando | NTSC-U v1.01 (`ntsc`), PAL (`pal`), NTSC-U v2.00 (`ntsc2`) |

## Native prompts

**RAC1:** The normal Buy control, buy-all control and status occupy separate native text lines inside the Buy monitor. The prior placement below the carousel icons was cropped out by the monitor capture. It uses RAC1's own small font and controller glyph. The Gadgetron PDA's alternate per-round price is used when its vendor price selector is active.

**RAC2:** A separate bordered box below the regular Buy control contains the buy-all prompt and status. Under Mod settings, **RAC2 vendor HUD position** can move it to the top or bottom right. **Show vendor ammo HUD** applies to both games.

The native Square glyph follows the shared font artwork, including the separate [Xbox Icons mod](https://github.com/OpenRAC/openrac-mods/tree/master/xbox-icons). Keyboard prompt changes use OpenRAC's shared font handling; no Xbox artwork is bundled or painted by this mod. RAC1's new capture placement and input prompt transitions require in-game verification.

## Test downloads and releases

Each push to **dev** builds six platforms and uploads downloadable Actions artifacts. Development builds do not publish GitHub Releases.

| Platform | Archive suffix |
|---|---|
| Windows x64 | `windows-x86_64` |
| Linux x64 | `linux-x86_64` |
| Linux ARM64 | `linux-aarch64` |
| macOS Apple Silicon | `macos-arm64` |
| macOS Intel | `macos-x86_64` |
| Android ARM64 | `android-arm64` |

Open **Actions → build → latest successful dev run → Artifacts → mod-release**. Extract the **`vendor-buy-all-0.2.0-dev.2.zip`** inside that artifact, then install that inner combined ZIP in OpenRAC's launcher. The outer Actions ZIP is not an installable mod. Separate platform ZIPs and `SHA256SUMS` are also included.

The workflow uses the current repository name automatically, so renaming the GitHub repository to `vendor-buy-all` requires no additional workflow change. On **main**, a push with a new stable manifest version publishes a release after all six platform builds and packaging checks succeed. A development version, pull request or manual workflow run does not publish a release. Existing releases are preserved.

## Building locally

Requires Python 3, CMake 3.20+, Ninja and a C++17 compiler:

```bash
python tools/fetch_sdk.py
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

SDK 1.20 is pinned by version and SHA256 in `sdk.lock`. To use a local SDK, pass `-DOPENRAC_SDK=/path/to/sdk`. Package `build/package/vendor_buy_all` with the SDK's `tools/package_mod.py`.

Tests cover bounded and partial purchases, invalid entries, full-ammo and insufficient-bolts quotes, duplicate entries, and RAC1 vendor-state guards, stock ownership, manual-dialog exclusion and PDA alternate prices. Runtime tests also exercise each RAC1 disc adapter through SDK callbacks with simulated game memory, checking payment, ammo changes, one purchase per press, native sound IDs, dialog exclusion and the native prompt draw. They cannot establish in-game visual placement or audio playback.

## Implementation

Each game has its own vendor adapter and native drawing path. Both share the purchase planner in `src/purchase_plan.hpp`. The gameplay tick reads the active vendor's ammo rows, computes a quote from the current balance, deducts its bounded total through the SDK's bolts interface and adds the purchased rounds on the game thread.

RAC1 data and sound behaviour are based on `OpenRAC/rac1-decomp`'s `vendor_menu.c`, `game/ui/vendor.c`, and the per-disc addresses documented by OpenRAC's ammo-max and cheats mods. A scoped message lookup hook draws the Buy monitor's localized original Buy label, native buy-all control and status, then returns the original string's existing terminator to prevent the normal windowed print drawing on top. It does not alter the game's message data or purchase handlers; outside the main vendor list it passes through unchanged.

RAC2 uses its existing vendor list adapter and `Vendor_DrawList` hook. Sound IDs 7 (purchase) and 2 (rejection) use each game's own vendor owner and sound mode. The manifest names hooks by stable, game-qualified IDs so the launcher can load the correct adapter.

## License

MIT. OpenRAC's SDK is separately licensed.
