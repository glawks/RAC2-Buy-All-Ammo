// SPDX-License-Identifier: MIT
#pragma once

#include <array>
#include <cstdint>

namespace buy_all_ammo {
constexpr int kItems = 56;

struct Candidate {
    int item = -1;
    int current = 0;
    int capacity = 0;
    int unit_price = 0;
};

struct Purchase {
    int item = -1;
    int amount = 0;
    int unit_price = 0;
};

struct Plan {
    std::array<Purchase, kItems> entries{};
    int count = 0;
    int cost = 0;
};

// Buy as many rounds as available bolts allow, in vendor-list order.
// This does no I/O, so tests can prove the accounting before game memory is touched.
inline Plan make_plan(const std::array<Candidate, kItems>& candidates,
                      int count, int available_bolts) {
    Plan plan;
    if (count < 0 || count > kItems || available_bolts <= 0) return plan;
    std::array<bool, kItems> seen{};
    int left = available_bolts;
    for (int i = 0; i < count; ++i) {
        const Candidate& c = candidates[static_cast<std::size_t>(i)];
        if (c.item < 0 || c.item >= kItems || seen[static_cast<std::size_t>(c.item)]) continue;
        seen[static_cast<std::size_t>(c.item)] = true;
        if (c.unit_price <= 0 || c.capacity <= 0 || c.current < 0 || c.current >= c.capacity) continue;
        const int needed = c.capacity - c.current;
        const int affordable = left / c.unit_price;
        const int rounds = needed < affordable ? needed : affordable;
        if (rounds <= 0) continue;
        plan.entries[static_cast<std::size_t>(plan.count++)] = {c.item, rounds, c.unit_price};
        const int cost = rounds * c.unit_price; // <= available_bolts; cannot overflow a positive int
        plan.cost += cost;
        left -= cost;
    }
    return plan;
}
} // namespace buy_all_ammo
