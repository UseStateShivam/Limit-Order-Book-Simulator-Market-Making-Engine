#pragma once

#include "lob/core/types.hpp"
#include <vector>
#include <list>
#include <optional>

namespace lob {

// Structurally simple, unoptimized reference implementation
// used as a differential correctness oracle in automated tests.
class ReferenceMatchingEngine {
public:
    struct SimpleOrder {
        OrderId order_id{0};
        Side side{Side::Buy};
        Price price{0};
        Quantity remaining_qty{0};
        Timestamp timestamp{0};
    };

    ReferenceMatchingEngine() = default;

    std::vector<Trade> process_order(const OrderRequest& req);
    bool process_cancel(OrderId order_id);

    [[nodiscard]] std::optional<Price> best_bid() const noexcept;
    [[nodiscard]] std::optional<Price> best_ask() const noexcept;
    [[nodiscard]] Quantity best_bid_qty() const noexcept;
    [[nodiscard]] Quantity best_ask_qty() const noexcept;
    [[nodiscard]] size_t total_orders() const noexcept;

    [[nodiscard]] const std::list<SimpleOrder>& bids() const noexcept { return bids_; }
    [[nodiscard]] const std::list<SimpleOrder>& asks() const noexcept { return asks_; }

    void reset();

private:
    std::list<SimpleOrder> bids_; // Kept sorted: highest price first, FIFO on ties
    std::list<SimpleOrder> asks_; // Kept sorted: lowest price first, FIFO on ties
    TradeId next_trade_id_{1};
    SequenceNumber seq_{0};
};

} // namespace lob
