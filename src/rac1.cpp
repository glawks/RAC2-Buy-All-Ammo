// SPDX-License-Identifier: MIT
#include "openrac/mod.hpp"
#include "openrac/games/rac1.hpp"
#include "rac1_vendor.hpp"
#include <cstdio>
#include <string>

namespace {
using namespace buy_all_ammo;
using Disc = vendor_buy_all::rac1::Disc;
using Print = void (*)(int, int, std::uint64_t, openrac::gaddr, int);
using Sound = int (*)(int, int, openrac::gaddr);
Print s_print = nullptr;
Sound s_sound = nullptr;
bool s_down = false;
bool s_pressed = false;
bool s_show = true;
bool s_touch = false;
Hint s_hint = Hint::Hidden;
int s_cost = 0;
int s_notice = 0;

openrac::gaddr menu(const openrac::Mod& mod, const Disc& disc) {
    return mod.relocate_level(0, disc.vendor);
}
void sound(openrac::Mod& mod, const Disc& disc, int id) {
    const auto vendor = menu(mod, disc);
    const auto owner = vendor ? mod.memory().read<openrac::gaddr>(vendor + 0x1Cu) : 0;
    // RAC1 uses mode 0, RAC2 uses 17; both use vendor sound 7 for buying, 2 for rejection.
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
}

// RAC1 renders vendor monitors into separate captured textures. The SDK exposes
// the item-details pass but not the Buy-label pass. Drawing after the native
// details places the prompt in a visible capture without altering Buy or Exit.
void draw(openrac::Mod& mod, const Disc& disc) {
    if (!s_show || s_hint == Hint::Hidden) return;
    std::array<Candidate, kItems> current{};
    openrac::gaddr ammo = 0;
    if (!collect(mod, disc, current, ammo)) return;
    char status[64];
    switch (s_hint) {
        case Hint::Buy: std::snprintf(status, sizeof status, "%d bolts", s_cost); break;
        case Hint::Partial: std::snprintf(status, sizeof status, "Partial %d", s_cost); break;
        case Hint::Full: std::snprintf(status, sizeof status, "Ammo full"); break;
        case Hint::Insufficient: std::snprintf(status, sizeof status, "No bolts"); break;
        case Hint::Purchased: std::snprintf(status, sizeof status, "Bought"); break;
        default: return;
    }
    openrac::StackFrame frame(mod, 96);
    const auto line = [&](const char* label, int y) {
        for (std::size_t i = 0; i <= std::char_traits<char>::length(label); ++i)
            mod.memory().write<char>(frame.address() + static_cast<openrac::gaddr>(i), label[i]);
        // Native small font; lower-left of the details monitor, away from
        // the item's right-aligned price and the selected weapon's name.
        s_print(6, y, 0x80F0F0F0u, frame.address(), -1);
    };
    line("\x13 Buy all", 86);
    line(status, 110);
}

} // namespace

void load_rac1(openrac::Mod& mod) {
    const Disc* disc = nullptr;
    for (const auto& candidate : vendor_buy_all::rac1::discs)
        if (mod.version() == candidate.version) disc = &candidate;
    if (!disc) throw openrac::Error("Unknown RAC1 disc version");
    s_down = s_pressed = s_touch = false;
    s_hint = Hint::Hidden;
    s_notice = s_cost = 0;
    s_show = mod.setting<bool>("show_hud");
    s_print = mod.required_function<Print>(disc->print);
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
    mod.hook<openrac::rac1::Vendor_DrawDetails>([&mod, disc](auto original) {
        original();
        draw(mod, *disc);
    });
    mod.info("Vendor Buy All 0.2.0-dev.3: RAC1 Square / Xbox X, details-monitor prompt");
}
