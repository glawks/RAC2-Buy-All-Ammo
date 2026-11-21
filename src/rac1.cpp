// SPDX-License-Identifier: MIT
#include "openrac/mod.hpp"
#include "openrac/games/rac1.hpp"
#include "rac1_vendor.hpp"
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
std::string s_last_hud;
int s_hud_level = -1;
bool s_prompt_logged = false;

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
        publish_hud(mod);
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
    publish_hud(mod);
}

// The vendor renders its six displays as captured textures, then calls
// update_hud(). The released SDK supports independent native HUD text after
// those captures. Its ASCII-only text excludes the game's 0x13 Square glyph,
// so label the correct Square / Xbox X input explicitly for this layout test.
void publish_hud(openrac::Mod& mod) {
    char status[64] = {};
    if (s_show) {
        switch (s_hint) {
            case Hint::Buy:
                std::snprintf(status, sizeof status, "Cost: %d bolts", s_cost);
                break;
            case Hint::Partial:
                std::snprintf(status, sizeof status, "Partial: %d bolts", s_cost);
                break;
            case Hint::Full:
                std::snprintf(status, sizeof status, "Ammo full");
                break;
            case Hint::Insufficient:
                std::snprintf(status, sizeof status, "Not enough bolts");
                break;
            case Hint::Purchased:
                std::snprintf(status, sizeof status, "Bought");
                break;
            default:
                break;
        }
    }
    if (status[0] == '\0') {
        if (!s_last_hud.empty())
            mod.hud_text_set({});
        s_last_hud.clear();
        s_hud_level = -1;
        return;
    }
    if (s_last_hud == status && s_hud_level == mod.loaded_level())
        return;
    openrac_hud_text title{};
    std::snprintf(title.text, sizeof title.text, "Square / Xbox X: Buy all");
    title.x = 0.50f;
    title.y = 0.695f;
    title.scale = 0.85f;
    title.rgba[0] = 0xF0;
    title.rgba[1] = 0xF0;
    title.rgba[2] = 0xF0;
    title.rgba[3] = 0xFF;
    title.flags = OPENRAC_HUD_SMALL | OPENRAC_HUD_SHADOW;
    openrac_hud_text detail = title;
    std::snprintf(detail.text, sizeof detail.text, "%s", status);
    detail.y = 0.732f;
    if (mod.hud_text_set({title, detail})) {
        s_last_hud = status;
        s_hud_level = mod.loaded_level();
        if (!s_prompt_logged) {
            mod.info("Vendor Buy All: RAC1 HUD text published (independent of vendor captures)");
            s_prompt_logged = true;
        }
    } else if (!s_prompt_logged) {
        mod.warn("Vendor Buy All: RAC1 HUD text API declined the shop prompt");
        s_prompt_logged = true;
    }
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
    s_prompt_logged = false;
    s_last_hud.clear();
    s_hud_level = -1;
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

    mod.info(std::string("Vendor Buy All 0.2.0-dev.6: RAC1 independent HUD text, Square / Xbox X; HUD ") + (s_show ? "enabled" : "disabled"));
}
