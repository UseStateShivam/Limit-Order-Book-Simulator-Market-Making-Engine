#pragma once

#include "lob/core/types.hpp"
#include "lob/core/error.hpp"
#include "lob/book/order_arena.hpp"

namespace lob {

struct PriceLevel {
    Price price{0};
    Quantity aggregate_qty{0};
    uint32_t order_count{0};
    uint32_t head_order_idx{kInvalidIndex};
    uint32_t tail_order_idx{kInvalidIndex};

    explicit PriceLevel(Price p = 0) : price(p) {}

    [[nodiscard]] bool empty() const noexcept {
        return order_count == 0 || head_order_idx == kInvalidIndex;
    }

    void append_order(OrderArena& arena, uint32_t order_idx) {
        auto& order = arena.get_by_index(order_idx);
        order.prev_idx = tail_order_idx;
        order.next_idx = kInvalidIndex;

        if (tail_order_idx != kInvalidIndex) {
            arena.get_by_index(tail_order_idx).next_idx = order_idx;
        } else {
            head_order_idx = order_idx;
        }
        tail_order_idx = order_idx;

        aggregate_qty += order.remaining_qty;
        order_count++;
    }

    void remove_order(OrderArena& arena, uint32_t order_idx) {
        auto& order = arena.get_by_index(order_idx);
        const uint32_t prev_idx = order.prev_idx;
        const uint32_t next_idx = order.next_idx;

        if (prev_idx != kInvalidIndex) {
            arena.get_by_index(prev_idx).next_idx = next_idx;
        } else {
            head_order_idx = next_idx;
        }

        if (next_idx != kInvalidIndex) {
            arena.get_by_index(next_idx).prev_idx = prev_idx;
        } else {
            tail_order_idx = prev_idx;
        }

        aggregate_qty -= order.remaining_qty;
        if (order_count > 0) order_count--;

        order.prev_idx = kInvalidIndex;
        order.next_idx = kInvalidIndex;
    }

    void reduce_order_qty(OrderArena& arena, uint32_t order_idx, Quantity delta) {
        auto& order = arena.get_by_index(order_idx);
        if (delta > order.remaining_qty) {
            throw LobException(ErrorCode::InvalidQuantity, "Reduction delta exceeds remaining order quantity");
        }
        order.remaining_qty -= delta;
        aggregate_qty -= delta;
    }
};

} // namespace lob
