// SPDX-License-Identifier: MIT
// Ratchet & Clank 2 / OpenRAC: press Square (Xbox X) in the Megacorp vendor to refill owned ammo.
// Addresses: OpenRAC/openrac-mods ng-plus-plus/src/discs.h and difficulty-mod/src/rac2/discs.h.
// Vendor record: OpenRAC/rac2-decomp src/game/002F5220-purchase.c and 002F5220-menu.c.
//
// The gameplay tick owns all guest-memory reads and writes. The port's overlay can run on a
// different (render) thread: its callback reads only ONE atomically published HUD snapshot.
#include "openrac/mod.hpp"
#include "purchase_plan.hpp"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

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
// On an Xbox controller, OpenRAC maps its X button to this same button.
constexpr std::uint16_t kSquare = 0x8000u;
constexpr std::uint64_t kHudKindMask = 0xFFu;
constexpr unsigned kHudCostShift = 8;

// Published atomically so the renderer never accesses the game memory or torn HUD data.
std::atomic<std::uint64_t> s_hud{0};
std::atomic<bool> s_top_right{false};
bool s_square_was_down = false;
bool s_square_just_pressed = false;
bool s_show_hud = true;
bool s_touch_buy_visible = false;
int s_notice_ticks = 0;
int s_notice_cost = 0;

void publish(buy_all_ammo::Hint hint, int cost = 0) {
    const auto bits = (static_cast<std::uint64_t>(static_cast<std::uint32_t>(cost)) << kHudCostShift) |
                      static_cast<std::uint64_t>(hint);
    s_hud.store(bits, std::memory_order_release);
}

// The overlay is drawn AFTER the game's own menus, unlike hud_text_set (which is hidden by them).
// Screen dimensions are 512x448, and the only non-constant value read here is the atomic snapshot.
void draw_hud(void*, const openrac_overlay* overlay) {
    if (!overlay || overlay->size < sizeof(openrac_overlay) ||
        !overlay->rect || !overlay->text) return;

    const std::uint64_t bits = s_hud.load(std::memory_order_acquire);
    const auto hint = static_cast<buy_all_ammo::Hint>(bits & kHudKindMask);
    if (hint == buy_all_ammo::Hint::Hidden) return;

    const auto cost = static_cast<std::uint32_t>(bits >> kHudCostShift);
    const bool purchased = hint == buy_all_ammo::Hint::Purchased;
    const bool unavailable = hint == buy_all_ammo::Hint::Full ||
                             hint == buy_all_ammo::Hint::Insufficient;
    const float x = 248.0f;
    const float y = s_top_right.load(std::memory_order_relaxed) ? 30.0f : 351.0f;
    constexpr float width = 251.0f;
    constexpr float height = 66.0f;

    const auto* ctx = overlay->context;
    const std::uint32_t accent = purchased ? 0x72E0ACFFu
                               : unavailable ? 0x91A1B6FFu : 0x6AC8EFFFu;
    const std::uint32_t bright = purchased ? 0xD6FFEAFFu
                               : unavailable ? 0xC2CBD8FFu : 0xF6FAFFFFu;
    overlay->rect(const_cast<void*>(ctx), x, y, width, height, 0x0B1525E8u);
    overlay->rect(const_cast<void*>(ctx), x, y, 3.0f, height, accent);
    overlay->rect(const_cast<void*>(ctx), x, y, width, 1.0f, 0xFFFFFF30u);

    // Draw a controller-agnostic Square outline next to "/ X": Square on PlayStation, X on Xbox.
    const float bx = x + 14.0f;
    const float by = y + 13.0f;
    overlay->rect(const_cast<void*>(ctx), bx, by, 17.0f, 2.0f, accent);
    overlay->rect(const_cast<void*>(ctx), bx, by + 15.0f, 17.0f, 2.0f, accent);
    overlay->rect(const_cast<void*>(ctx), bx, by, 2.0f, 17.0f, accent);
    overlay->rect(const_cast<void*>(ctx), bx + 15.0f, by, 2.0f, 17.0f, accent);
    overlay->text(const_cast<void*>(ctx), x + 39.0f, y + 11.0f, 15.0f, bright, "/ X");
    overlay->text(const_cast<void*>(ctx), x + 85.0f, y + 10.0f, 16.0f, bright, "BUY ALL AMMO");

    char description[96];
    switch (hint) {
        case buy_all_ammo::Hint::Buy:
            std::snprintf(description, sizeof description, "REFILL: %u BOLTS", cost);
            break;
        case buy_all_ammo::Hint::Partial:
            std::snprintf(description, sizeof description, "SPEND: %u BOLTS (PARTIAL)", cost);
            break;
        case buy_all_ammo::Hint::Full:
            std::snprintf(description, sizeof description, "ALL AMMO FULL");
            break;
        case buy_all_ammo::Hint::Insufficient:
            std::snprintf(description, sizeof description, "NOT ENOUGH BOLTS");
            break;
        case buy_all_ammo::Hint::Purchased:
            std::snprintf(description, sizeof description, "PURCHASED FOR %u BOLTS", cost);
            break;
        default:
            return;
    }
    overlay->text(const_cast<void*>(ctx), x + 15.0f, y + 42.0f, 13.0f, bright, description);
}

void touch_buy_button(openrac::Mod& mod, bool visible) {
    if (s_touch_buy_visible == visible) return;
    s_touch_buy_visible = visible;
    // The "vendor" touch layout normally has no Square button. The mod adds it while buying
    // ammunition is possible, rather than replacing the game's normal vendor controls.
    mod.touch_hint("buy_all_ammo", visible);
}

// Read only the vendor's own ammo entries. Return zero outside its main list, including dialogs.
int candidates_at_vendor(const openrac::Mod& mod,
                         const Disc& disc,
                         std::array<buy_all_ammo::Candidate, buy_all_ammo::kItems>& out,
                         openrac::gaddr& ammo_addr) {
    const auto& memory = mod.memory();
    const auto mode = mod.relocate(disc.game_state);
    if (!mode || memory.read<std::int32_t>(mode) != 5) return 0;

    const auto vendor = mod.relocate(disc.vendor);
    if (!vendor || memory.read<std::int32_t>(vendor + kVendorDialogState) != 0) return 0;
    const int count = memory.read<std::int32_t>(vendor + kVendorCount);
    const int cursor = memory.read<std::int32_t>(vendor + kVendorCursor);
    if (count < 1 || count > kVendorMaxEntries || cursor < 0 || cursor >= count) return 0;

    const auto owned = mod.relocate(disc.owned);
    const auto slots = mod.relocate(disc.slot);
    const auto weapon_table = mod.relocate(disc.table);
    ammo_addr = mod.relocate(disc.ammo);
    if (!owned || !slots || !weapon_table || !ammo_addr) return 0;

    int out_count = 0;
    std::array<bool, buy_all_ammo::kItems> seen{};
    for (int row = 0; row < count && out_count < buy_all_ammo::kItems; ++row) {
        const auto entry = vendor + kVendorEntries + static_cast<std::uint32_t>(row) * kVendorEntrySize;
        const int item = memory.read<std::int32_t>(entry);
        const int kind = memory.read<std::int32_t>(entry + 4u);
        if (kind != 1 || item < 0 || item >= buy_all_ammo::kItems) continue;
        const auto index = static_cast<std::size_t>(item);
        if (seen[index]) continue;
        if (!memory.read<std::uint8_t>(owned + static_cast<std::uint32_t>(item))) continue;
        const int slot = memory.read<std::uint8_t>(slots + static_cast<std::uint32_t>(item));
        if (slot < 0 || slot >= kMaxRows) continue;
        const auto record = weapon_table + static_cast<std::uint32_t>(slot) * kRowSize;
        const int unit_price = memory.read<std::uint16_t>(record + kRowUnitPrice);
        const int capacity = memory.read<std::uint16_t>(record + kRowCapacity);
        const int current = memory.read<std::int32_t>(ammo_addr + static_cast<std::uint32_t>(item) * 4u);
        seen[index] = true;
        out[static_cast<std::size_t>(out_count++)] = {item, current, capacity, unit_price};
    }
    return out_count;
}

void buy(openrac::Mod& mod,
         openrac::gaddr ammo_addr,
         const buy_all_ammo::Plan& plan) {
    const auto& memory = mod.memory();
    // One transaction on the game thread; all costs come from the active weapon's vendor record.
    // The SDK's bolts method keeps the game's own balance state in sync.
    mod.bolts(-plan.cost);
    for (int i = 0; i < plan.count; ++i) {
        const auto& purchase = plan.entries[static_cast<std::size_t>(i)];
        const auto addr = ammo_addr + static_cast<std::uint32_t>(purchase.item) * 4u;
        memory.write<std::int32_t>(addr, memory.read<std::int32_t>(addr) + purchase.amount);
    }
    mod.info("Buy All Ammo: purchased " + std::to_string(plan.count) +
             " ammo types for " + std::to_string(plan.cost) + " bolts");
}

void tick(openrac::Mod& mod, const Disc& disc) {
    const bool square = s_square_just_pressed;
    s_square_just_pressed = false;

    std::array<buy_all_ammo::Candidate, buy_all_ammo::kItems> candidates{};
    openrac::gaddr ammo_addr = 0;
    const int count = candidates_at_vendor(mod, disc, candidates, ammo_addr);
    if (count <= 0) {
        s_notice_ticks = 0;
        publish(buy_all_ammo::Hint::Hidden);
        touch_buy_button(mod, false);
        return;
    }

    const auto quote = buy_all_ammo::make_quote(candidates, count, mod.bolts());
    const auto hint = buy_all_ammo::hint_for(quote);
    touch_buy_button(mod, hint == buy_all_ammo::Hint::Buy ||
                              hint == buy_all_ammo::Hint::Partial);

    if (square && quote.affordable.cost > 0 && quote.affordable.count > 0) {
        buy(mod, ammo_addr, quote.affordable);
        s_notice_ticks = (std::string(disc.version) == "pal") ? 100 : 120;
        s_notice_cost = quote.affordable.cost;
    }
    if (s_notice_ticks > 0) {
        --s_notice_ticks;
        publish(buy_all_ammo::Hint::Purchased, s_notice_cost);
    } else {
        publish(hint, quote.affordable.cost);
    }
}

void load(openrac::Mod& mod) {
    if (mod.game() != "rac2") throw openrac::Error("R&C2 only");
    const Disc* disc = nullptr;
    for (const Disc& d : kDiscs) {
        if (mod.version() == d.version) { disc = &d; break; }
    }
    if (!disc) throw openrac::Error("Unknown RAC2 disc version");

    s_square_was_down = false;
    s_square_just_pressed = false;
    s_notice_ticks = 0;
    s_notice_cost = 0;
    s_touch_buy_visible = false;
    publish(buy_all_ammo::Hint::Hidden);
    s_show_hud = mod.setting<bool>("show_hud");
    s_top_right.store(mod.setting<std::string>("hud_position") == "top_right",
                      std::memory_order_relaxed);

    mod.on_pad([](int port, std::uint8_t* report, int size) {
        if (port != 0 || report == nullptr || size < 2) return;
        const auto bits = static_cast<std::uint16_t>(report[0]) |
                          (static_cast<std::uint16_t>(report[1]) << 8);
        const bool down = (bits & kSquare) == 0;
        if (down && !s_square_was_down) s_square_just_pressed = true;
        s_square_was_down = down;
    });

    mod.on_frame([&mod, disc] { tick(mod, *disc); });
    if (s_show_hud && !mod.on_overlay(draw_hud)) {
        mod.info("Buy All Ammo: overlay unavailable; buying still works (no HUD).");
    }
    mod.info("RAC2 Buy All Ammo dev.2: Square / Xbox X, vendor HUD and Android touch button");
}
} // namespace

OPENRAC_MOD_CPP("rac2-buy-all-ammo", "0.1.0-dev.2", load)
