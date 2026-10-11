#pragma once

#include "lob/core/types.hpp"
#include <string>

namespace lob {

enum class MarketEventType : uint8_t {
    Snapshot = 0,
    BBO = 1,
    Trade = 2,
    Depth = 3,
    OrderAdd = 4,
    OrderCancel = 5,
    OrderModify = 6
};

[[nodiscard]] constexpr std::string_view market_event_type_to_string(MarketEventType type) noexcept {
    switch (type) {
        case MarketEventType::Snapshot:    return "SNAPSHOT";
        case MarketEventType::BBO:         return "BBO";
        case MarketEventType::Trade:       return "TRADE";
        case MarketEventType::Depth:       return "DEPTH";
        case MarketEventType::OrderAdd:    return "ADD";
        case MarketEventType::OrderCancel: return "CANCEL";
        case MarketEventType::OrderModify: return "MODIFY";
    }
    return "UNKNOWN";
}

struct MarketDataEvent {
    Timestamp timestamp{0};
    SequenceNumber sequence{0};
    MarketEventType type{MarketEventType::BBO};
    std::string symbol{"BTC-USDT"};

    // Top-of-book / quote fields
    Price bid_price{0};
    Quantity bid_quantity{0};
    Price ask_price{0};
    Quantity ask_quantity{0};

    // Trade fields
    Price trade_price{0};
    Quantity trade_quantity{0};
    Side aggressor_side{Side::Buy};

    // Order-level fields (when available)
    OrderId order_id{0};
    Side order_side{Side::Buy};
};

} // namespace lob
