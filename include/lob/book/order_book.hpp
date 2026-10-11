#pragma once

#include "lob/core/types.hpp"
#include "lob/core/error.hpp"
#include "lob/book/order_arena.hpp"
#include "lob/book/price_level.hpp"
#include <map>
#include <unordered_map>
#include <vector>
#include <optional>

namespace lob {

struct Level2Entry {
    Price price{0};
    Quantity quantity{0};
    uint32_t order_count{0};
};

struct L2Snapshot {
    Timestamp timestamp{0};
    SequenceNumber sequence{0};
    std::vector<Level2Entry> bids;
    std::vector<Level2Entry> asks;
};

struct Level3Entry {
    OrderId order_id{0};
    Price price{0};
    Quantity quantity{0};
    uint32_t queue_position{0};
    Timestamp timestamp{0};
};

struct L3Snapshot {
    Timestamp timestamp{0};
    SequenceNumber sequence{0};
    std::vector<Level3Entry> bids;
    std::vector<Level3Entry> asks;
};

class OrderBook {
public:
    explicit OrderBook(size_t arena_capacity = 65536);

    // Queries
    [[nodiscard]] std::optional<Price> best_bid() const noexcept;
    [[nodiscard]] std::optional<Price> best_ask() const noexcept;
    [[nodiscard]] Quantity best_bid_qty() const noexcept;
    [[nodiscard]] Quantity best_ask_qty() const noexcept;
    [[nodiscard]] std::optional<double> mid_price() const noexcept;
    [[nodiscard]] std::optional<Price> spread_ticks() const noexcept;

    [[nodiscard]] Quantity depth_at_price(Side side, Price price) const noexcept;
    [[nodiscard]] Quantity total_bid_quantity(size_t max_depth = 0) const noexcept;
    [[nodiscard]] Quantity total_ask_quantity(size_t max_depth = 0) const noexcept;

    [[nodiscard]] L2Snapshot get_l2_snapshot(size_t max_depth = 10, Timestamp ts = 0, SequenceNumber seq = 0) const;
    [[nodiscard]] L3Snapshot get_l3_snapshot(size_t max_depth = 10, Timestamp ts = 0, SequenceNumber seq = 0) const;

    [[nodiscard]] bool contains_order(OrderId order_id) const noexcept;
    [[nodiscard]] const OrderRecord* find_order(OrderId order_id) const noexcept;
    [[nodiscard]] size_t total_orders() const noexcept;
    [[nodiscard]] size_t bid_level_count() const noexcept;
    [[nodiscard]] size_t ask_level_count() const noexcept;

    // Direct book mutations (used by matching engine and resting liquidity)
    OrderHandle add_resting_order(const OrderRequest& req, SequenceNumber seq);
    OrderRecord remove_order(OrderId order_id);
    bool modify_order_quantity(OrderId order_id, Quantity new_qty);
    void reduce_order_quantity(OrderId order_id, Quantity executed_qty);

    // Engine access to best price levels for matching
    [[nodiscard]] std::map<Price, PriceLevel, std::greater<Price>>& bids() noexcept { return bids_; }
    [[nodiscard]] const std::map<Price, PriceLevel, std::greater<Price>>& bids() const noexcept { return bids_; }
    [[nodiscard]] std::map<Price, PriceLevel, std::less<Price>>& asks() noexcept { return asks_; }
    [[nodiscard]] const std::map<Price, PriceLevel, std::less<Price>>& asks() const noexcept { return asks_; }

    [[nodiscard]] OrderArena& arena() noexcept { return arena_; }
    [[nodiscard]] const OrderArena& arena() const noexcept { return arena_; }

    [[nodiscard]] std::unordered_map<OrderId, OrderHandle>& order_index() noexcept { return order_index_; }
    [[nodiscard]] const std::unordered_map<OrderId, OrderHandle>& order_index() const noexcept { return order_index_; }

    // Invariant check - verifies full structural and financial consistency
    void verify_invariants() const;

    void clear();

private:
    OrderArena arena_;
    std::map<Price, PriceLevel, std::greater<Price>> bids_; // Best bid first
    std::map<Price, PriceLevel, std::less<Price>> asks_;    // Best ask first
    std::unordered_map<OrderId, OrderHandle> order_index_;
};

} // namespace lob
