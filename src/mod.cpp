// SPDX-License-Identifier: MIT
// Ratchet & Clank 2 / OpenRAC: press Square (Xbox X) in the Megacorp vendor to refill owned ammo.
// Addresses: OpenRAC/openrac-mods ng-plus-plus/src/discs.h and difficulty-mod/src/rac2/discs.h.
// Vendor record: OpenRAC/rac2-decomp src/game/002F5220-purchase.c and 002F5220-menu.c.
#include "openrac/mod.hpp"
#include "purchase_plan.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace {

struct Disc {
    const char* version;
    std::uint32_t game_state; // s32: 5 is the vendor
    std::uint32_t vendor;     // +0x84: purchase dialog state, +0x740: entry count
    std::uint32_t table;      // 0xE0-byte weapon records
    std::uint32_t slot;       // u8[56] active record per item
    std::uint32_t ammo;       // s32[56] current ammo
    std::uint32_t owned;      // u8[56] inventory flags
};

constexpr Disc kDiscs[] = {
    {"ntsc",  0x001A8BB0u, 0x0021F240u, 0x00239BA0u, 0x00139568u, 0x00139688u, 0x001A7AF8u},
    {"pal",   0x001A8C60u, 0x0021F1C0u, 0x00239BA0u, 0x001395E8u, 0x00139708u, 0x001A7B80u},
    {"ntsc2", 0x001A8BB0u, 0x0021F1C0u, 0x00239B20u, 0x00139568u, 0x00139688u, 0x001A7B00u},
};

constexpr std::uint32_t kVendorCursor = 0x80u;
constexpr std::uint32_t kVendorDialogState = 0x84u;
constexpr std::uint32_t kVendorEntries = 0x140u;
constexpr std::uint32_t kVendorCount = 0x740u;
constexpr std::uint32_t kVendorEntrySize = 0x18u;
constexpr std::uint32_t kRowSize = 0xE0u;
constexpr std::uint32_t kRowUnitPrice = 0x88u;
constexpr std::uint32_t kRowCapacity = 0x8Eu;
constexpr int kMaxRows = 119;
constexpr int kVendorMaxEntries = 64;

// The first pad's 16-bit PS2 report is little-endian, active LOW; bit 15 = Square.
// On an Xbox pad, the port maps X to this same button.
constexpr std::uint16_t kSquare = 0x8000u;

bool square_was_down = false;
bool square_just_pressed = false;

void refill(openrac::Mod& mod, const Disc& disc) {
    const auto& memory = mod.memory();
    // Ask the SDK to translate the executable's symbol addresses into the loaded level's addresses.
    // Below the shared-data threshold relocation leaves the address unchanged.
    const auto mode_addr = mod.relocate(disc.game_state);
    if (!mode_addr || memory.read<std::int32_t>(mode_addr) != 5) return;

    const auto vendor = mod.relocate(disc.vendor);
    if (!vendor || memory.read<std::int32_t>(vendor + kVendorDialogState) != 0) return;
    const int count = memory.read<std::int32_t>(vendor + kVendorCount);
    const int cursor = memory.read<std::int32_t>(vendor + kVendorCursor);
    if (count < 1 || count > kVendorMaxEntries || cursor < 0 || cursor >= count) return;

    const auto owned = mod.relocate(disc.owned);
    const auto slots = mod.relocate(disc.slot);
    const auto weapon_table = mod.relocate(disc.table);
    const auto ammo = mod.relocate(disc.ammo);
    if (!owned || !slots || !weapon_table || !ammo) return;

    // Iterate the vendor's ACTUAL ammo entries, so this never fills an item not on sale here.
    std::array<buy_all_ammo::Candidate, buy_all_ammo::kItems> candidates{};
    int candidates_count = 0;
    std::array<bool, buy_all_ammo::kItems> candidate_seen{};
    for (int row = 0; row < count && candidates_count < buy_all_ammo::kItems; ++row) {
        const auto entry = vendor + kVendorEntries + static_cast<std::uint32_t>(row) * kVendorEntrySize;
        const int item = memory.read<std::int32_t>(entry);
        const int kind = memory.read<std::int32_t>(entry + 4u);
        if (kind != 1 || item < 0 || item >= buy_all_ammo::kItems) continue;
        if (candidate_seen[static_cast<std::size_t>(item)]) continue;
        if (!memory.read<std::uint8_t>(owned + static_cast<std::uint32_t>(item))) continue;
        const int slot = memory.read<std::uint8_t>(slots + static_cast<std::uint32_t>(item));
        if (slot < 0 || slot >= kMaxRows) continue;
        const auto record = weapon_table + static_cast<std::uint32_t>(slot) * kRowSize;
        const int price = memory.read<std::uint16_t>(record + kRowUnitPrice);
        const int capacity = memory.read<std::uint16_t>(record + kRowCapacity);
        const int current = memory.read<std::int32_t>(ammo + static_cast<std::uint32_t>(item) * 4u);
        candidate_seen[static_cast<std::size_t>(item)] = true;
        candidates[static_cast<std::size_t>(candidates_count++)] = {item, current, capacity, price};
    }

    // Read the current balance once; do all arithmetic before touching any game state.
    const int balance = mod.bolts();
    const auto plan = buy_all_ammo::make_plan(candidates, candidates_count, balance);
    if (plan.count == 0 || plan.cost <= 0 || plan.cost > balance) return;

    // One transaction (on the game thread); matches the vendor's per-round pricing.
    // Use the SDK's bolts operation rather than modifying a raw balance address.
    mod.bolts(-plan.cost);
    for (int i = 0; i < plan.count; ++i) {
        const auto& bought = plan.entries[static_cast<std::size_t>(i)];
        const auto addr = ammo + static_cast<std::uint32_t>(bought.item) * 4u;
        const int current = memory.read<std::int32_t>(addr);
        memory.write<std::int32_t>(addr, current + bought.amount);
    }
    mod.info("Buy All Ammo: purchased " + std::to_string(plan.count) +
             " ammo types for " + std::to_string(plan.cost) + " bolts");
}

void load(openrac::Mod& mod) {
    if (mod.game() != "rac2") {
        throw openrac::Error("R&C2 only");
    }
    const Disc* disc = nullptr;
    for (const Disc& d : kDiscs) {
        if (mod.version() == d.version) { disc = &d; break; }
    }
    if (!disc) throw openrac::Error("Unknown RAC2 disc version");

    square_was_down = false;
    square_just_pressed = false;
    mod.on_pad([](int port, std::uint8_t* report, int size) {
        if (port != 0 || report == nullptr || size < 2) return;
        const auto bits = static_cast<std::uint16_t>(report[0]) |
                          (static_cast<std::uint16_t>(report[1]) << 8);
        const bool down = (bits & kSquare) == 0;
        if (down && !square_was_down) square_just_pressed = true;
        square_was_down = down;
    });
    mod.on_frame([&mod, disc] {
        if (!square_just_pressed) return;
        square_just_pressed = false;
        refill(mod, *disc);
    });
    mod.info("RAC2 Buy All Ammo loaded: Square / Xbox X in a weapon vendor");
}
} // namespace

OPENRAC_MOD_CPP("rac2-buy-all-ammo", "0.1.0-dev.1", load)
