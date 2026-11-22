// SPDX-License-Identifier: MIT
#include "openrac/mod.hpp"
#include "rac1_vendor.hpp"
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

namespace {
using namespace buy_all_ammo;
using Disc = vendor_buy_all::rac1::Disc;
using Sound = int (*)(int, int, openrac::gaddr);

Sound s_sound = nullptr;
bool s_down = false;
bool s_pressed = false;
bool s_show = true;
bool s_touch = false;
Hint s_hint = Hint::Hidden;
int s_cost = 0;
int s_notice = 0;

// The game thread publishes a compact immutable snapshot; the overlay draws
// on the window thread without touching guest RAM or non-atomic game state.
std::atomic<std::uint64_t> s_prompt{0};
constexpr std::uint64_t kHintMask = 0xFFu;
constexpr unsigned kCostShift = 8u;
void publish_prompt() {
    const auto hint = s_show ? s_hint : Hint::Hidden;
    const std::uint64_t bits =
        (static_cast<std::uint64_t>(static_cast<std::uint32_t>(s_cost)) << kCostShift)
        | static_cast<std::uint64_t>(hint);
    s_prompt.store(bits, std::memory_order_release);
}

// The native 512 x 448 vendor picture: between Buy and Exit monitors,
// above the weapon carousel. Adapted from SDK 1.31's vendor-prompt example.
constexpr float kPanelX = 176.0f;
constexpr float kPanelY = 268.0f;
constexpr float kPanelW = 160.0f;
constexpr float kPanelH = 28.0f;
void draw_prompt(void*, const openrac_overlay* o) {
    if (!o || o->size < offsetof(openrac_overlay, phase) + sizeof(o->phase) ||
        o->phase != OPENRAC_OVERLAY_MENU || !o->rect || !o->text || !o->text_width)
        return;
    const auto bits = s_prompt.load(std::memory_order_acquire);
    const auto hint = static_cast<Hint>(bits & kHintMask);
    if (hint == Hint::Hidden) return;
    const auto cost = static_cast<std::uint32_t>(bits >> kCostShift);
    char prompt[96] = {};
    switch (hint) {
        case Hint::Buy:
            std::snprintf(prompt, sizeof(prompt), OPENRAC_UI_SQUARE " Buy all - %u bolts", cost);
            break;
        case Hint::Partial:
            std::snprintf(prompt, sizeof(prompt), OPENRAC_UI_SQUARE " Buy all - %u bolts (partial)", cost);
            break;
        case Hint::Full:
            std::snprintf(prompt, sizeof(prompt), OPENRAC_UI_SQUARE " Buy all - Ammo full");
            break;
        case Hint::Insufficient:
            std::snprintf(prompt, sizeof(prompt), OPENRAC_UI_SQUARE " Buy all - Not enough bolts");
            break;
        case Hint::Purchased:
            std::snprintf(prompt, sizeof(prompt), OPENRAC_UI_SQUARE " Buy all - Bought: %u bolts", cost);
            break;
        default: return;
    }

    // RRGGBBAA: translucent charcoal and a subtle accent along its top.
    // OPENRAC_UI_SQUARE is rendered by the port as the currently active input.
    o->rect(o->context, kPanelX, kPanelY, kPanelW, kPanelH, 0x080B10B8u);
    o->rect(o->context, kPanelX, kPanelY, kPanelW, 1.6f, 0x6192B8D0u);
    float height = 15.0f;
    float width = o->text_width(o->context, height, prompt);
    while (width > kPanelW - 12.0f && height > 10.0f) {
        height -= 1.0f;
        width = o->text_width(o->context, height, prompt);
    }
    o->text(o->context, kPanelX + (kPanelW - width) * 0.5f,
            kPanelY + (kPanelH - height) * 0.5f, height, 0xF4F6FFFFu, prompt);
}
openrac::gaddr menu(const openrac::Mod& mod, const Disc& disc) {
    return mod.relocate_level(0, disc.vendor);
}
void sound(openrac::Mod& mod, const Disc& disc, int id) {
    const auto vendor = menu(mod, disc);
    const auto owner = vendor ? mod.memory().read<openrac::gaddr>(vendor + 0x1Cu) : 0;
    if (owner) s_sound(id, 0, owner);
}
void touch(openrac::Mod& mod, bool visible) {
    if (visible != s_touch) {
        mod.touch_hint("buy_all_ammo", visible);
        s_touch = visible;
    }
}
int collect(openrac::Mod& mod, const Disc& disc,
            std::array<Candidate, kItems>& out, openrac::gaddr& ammo) {
    if (mod.loaded_level() < 0) return 0;
    const auto mode = mod.relocate_level(0, disc.mode);
    if (!mode || mod.memory().read<std::int32_t>(mode) != 5) return 0;
    ammo = mod.relocate(disc.ammo);
    return vendor_buy_all::rac1::candidates(mod.memory(), menu(mod, disc),
        mod.relocate_level(0, disc.table), ammo, mod.relocate(disc.owned), out);
}
void tick(openrac::Mod& mod, const Disc& disc) {
    const bool pressed = s_pressed;
    s_pressed = false;
    std::array<Candidate, kItems> entries{};
    openrac::gaddr ammo = 0;
    const int count = collect(mod, disc, entries, ammo);
    if (!count) {
        s_hint = Hint::Hidden;
        s_notice = 0;
        touch(mod, false);
        publish_prompt();
        return;
    }
    touch(mod, true);
    const auto quote = make_quote(entries, count, mod.bolts());
    if (pressed) {
        const auto& plan = quote.affordable;
        if (plan.count && plan.cost > 0) {
            mod.bolts(-plan.cost);
            for (int i = 0; i < plan.count; ++i) {
                const auto& entry = plan.entries[static_cast<std::size_t>(i)];
                const auto address = ammo + static_cast<std::uint32_t>(entry.item) * 4u;
                mod.memory().write<std::int32_t>(address,
                    mod.memory().read<std::int32_t>(address) + entry.amount);
            }
            sound(mod, disc, 7);
            s_cost = plan.cost;
            s_notice = mod.version() == "pal" ? 100 : 120;
            mod.info("Vendor Buy All: RAC1 bought " + std::to_string(plan.count) +
                     " ammo types for " + std::to_string(plan.cost) + " bolts");
        } else {
            sound(mod, disc, 2);
            s_notice = 0;
        }
    }
    if (s_notice > 0) {
        --s_notice;
        s_hint = Hint::Purchased;
    } else {
        s_hint = hint_for(quote);
        s_cost = quote.affordable.cost;
    }
    publish_prompt();
}
} // namespace

void load_rac1(openrac::Mod& mod) {
    const Disc* disc = nullptr;
    for (const auto& candidate : vendor_buy_all::rac1::discs)
        if (mod.version() == candidate.version) disc = &candidate;
    if (!disc) throw openrac::Error("Unknown RAC1 disc version");
    if (!mod.has_overlay_phases())
        throw openrac::Error("RAC1 menu overlay requires OpenRAC API 1.31");
    s_down = s_pressed = s_touch = false;
    s_hint = Hint::Hidden;
    s_cost = s_notice = 0;
    s_prompt.store(0, std::memory_order_relaxed);
    s_show = mod.setting<bool>("show_hud");
    s_sound = mod.required_function<Sound>(disc->sound);
    mod.on_pad([](int port, std::uint8_t* report, int size) {
        if (port != 0 || !report || size < 2) return;
        const auto buttons = static_cast<std::uint16_t>(report[0]) |
                             (static_cast<std::uint16_t>(report[1]) << 8);
        const bool down = (buttons & 0x8000u) == 0;
        if (down && !s_down) s_pressed = true;
        s_down = down;
    });
    mod.on_frame([&mod, disc] { tick(mod, *disc); });
    if (!mod.on_overlay_phases(draw_prompt, nullptr, OPENRAC_OVERLAY_MENU))
        throw openrac::Error("OpenRAC refused RAC1's vendor menu overlay");
    mod.info(std::string("Vendor Buy All 0.2.2-dev.1: RAC1 SDK 1.31 native panel; HUD ")
             + (s_show ? "enabled" : "disabled"));
}
