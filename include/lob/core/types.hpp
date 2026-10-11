#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <cmath>
#include <limits>
#include <optional>

namespace lob {

using OrderId = uint64_t;
using TradeId = uint64_t;
using SequenceNumber = uint64_t;
using Timestamp = uint64_t; // Nanoseconds since epoch or simulation start
using Price = int64_t;      // Integer tick units
using Quantity = int64_t;   // Integer lot units

constexpr uint32_t kInvalidIndex = std::numeric_limits<uint32_t>::max();
constexpr Price kInvalidPrice = std::numeric_limits<Price>::min();
constexpr Quantity kInvalidQuantity = -1;

enum class Side : uint8_t {
    Buy = 0,
    Sell = 1
};

[[nodiscard]] constexpr Side opposite_side(Side side) noexcept {
    return side == Side::Buy ? Side::Sell : Side::Buy;
}

[[nodiscard]] constexpr std::string_view side_to_string(Side side) noexcept {
    switch (side) {
        case Side::Buy:  return "BUY";
        case Side::Sell: return "SELL";
    }
    return "UNKNOWN";
}

[[nodiscard]] inline std::optional<Side> string_to_side(std::string_view s) noexcept {
    if (s == "BUY" || s == "Buy" || s == "B" || s == "buy") return Side::Buy;
    if (s == "SELL" || s == "Sell" || s == "S" || s == "sell") return Side::Sell;
    return std::nullopt;
}

enum class OrderType : uint8_t {
    Limit = 0,
    Market = 1
};

[[nodiscard]] constexpr std::string_view order_type_to_string(OrderType type) noexcept {
    switch (type) {
        case OrderType::Limit:  return "LIMIT";
        case OrderType::Market: return "MARKET";
    }
    return "UNKNOWN";
}

[[nodiscard]] inline std::optional<OrderType> string_to_order_type(std::string_view s) noexcept {
    if (s == "LIMIT" || s == "Limit" || s == "limit" || s == "L") return OrderType::Limit;
    if (s == "MARKET" || s == "Market" || s == "market" || s == "M") return OrderType::Market;
    return std::nullopt;
}

enum class TimeInForce : uint8_t {
    GTC = 0, // Good-Til-Cancelled
    IOC = 1, // Immediate-Or-Cancel
    FOK = 2  // Fill-Or-Kill
};

[[nodiscard]] constexpr std::string_view tif_to_string(TimeInForce tif) noexcept {
    switch (tif) {
        case TimeInForce::GTC: return "GTC";
        case TimeInForce::IOC: return "IOC";
        case TimeInForce::FOK: return "FOK";
    }
    return "UNKNOWN";
}

[[nodiscard]] inline std::optional<TimeInForce> string_to_tif(std::string_view s) noexcept {
    if (s == "GTC" || s == "gtc") return TimeInForce::GTC;
    if (s == "IOC" || s == "ioc") return TimeInForce::IOC;
    if (s == "FOK" || s == "fok") return TimeInForce::FOK;
    return std::nullopt;
}

enum class OrderStatus : uint8_t {
    New = 0,
    Accepted = 1,
    PartiallyFilled = 2,
    Filled = 3,
    Canceled = 4,
    Rejected = 5,
    Expired = 6
};

[[nodiscard]] constexpr std::string_view order_status_to_string(OrderStatus status) noexcept {
    switch (status) {
        case OrderStatus::New:             return "NEW";
        case OrderStatus::Accepted:        return "ACCEPTED";
        case OrderStatus::PartiallyFilled: return "PARTIALLY_FILLED";
        case OrderStatus::Filled:          return "FILLED";
        case OrderStatus::Canceled:        return "CANCELED";
        case OrderStatus::Rejected:        return "REJECTED";
        case OrderStatus::Expired:         return "EXPIRED";
    }
    return "UNKNOWN";
}

enum class ExecutionType : uint8_t {
    New = 0,
    Trade = 1,
    Canceled = 2,
    Rejected = 3,
    Expired = 4,
    Modified = 5
};

[[nodiscard]] constexpr std::string_view exec_type_to_string(ExecutionType type) noexcept {
    switch (type) {
        case ExecutionType::New:      return "NEW";
        case ExecutionType::Trade:    return "TRADE";
        case ExecutionType::Canceled: return "CANCELED";
        case ExecutionType::Rejected: return "REJECTED";
        case ExecutionType::Expired:  return "EXPIRED";
        case ExecutionType::Modified: return "MODIFIED";
    }
    return "UNKNOWN";
}

enum class RejectReason : uint8_t {
    None = 0,
    InvalidPrice = 1,
    InvalidQuantity = 2,
    DuplicateOrderId = 3,
    OrderNotFound = 4,
    InsufficientLiquidity = 5,
    SelfCrossNotAllowed = 6,
    RiskLimitExceeded = 7,
    MarketOrderCannotRest = 8,
    TickSizeMismatch = 9,
    LotSizeMismatch = 10,
    Unknown = 99
};

[[nodiscard]] constexpr std::string_view reject_reason_to_string(RejectReason r) noexcept {
    switch (r) {
        case RejectReason::None:                  return "NONE";
        case RejectReason::InvalidPrice:          return "INVALID_PRICE";
        case RejectReason::InvalidQuantity:       return "INVALID_QUANTITY";
        case RejectReason::DuplicateOrderId:      return "DUPLICATE_ORDER_ID";
        case RejectReason::OrderNotFound:         return "ORDER_NOT_FOUND";
        case RejectReason::InsufficientLiquidity: return "INSUFFICIENT_LIQUIDITY";
        case RejectReason::SelfCrossNotAllowed:   return "SELF_CROSS_NOT_ALLOWED";
        case RejectReason::RiskLimitExceeded:     return "RISK_LIMIT_EXCEEDED";
        case RejectReason::MarketOrderCannotRest: return "MARKET_ORDER_CANNOT_REST";
        case RejectReason::TickSizeMismatch:      return "TICK_SIZE_MISMATCH";
        case RejectReason::LotSizeMismatch:       return "LOT_SIZE_MISMATCH";
        case RejectReason::Unknown:               return "UNKNOWN";
    }
    return "UNKNOWN";
}

struct OrderHandle {
    uint32_t index{kInvalidIndex};
    uint32_t generation{0};

    [[nodiscard]] bool is_valid() const noexcept {
        return index != kInvalidIndex;
    }
};

struct OrderRecord {
    OrderId order_id{0};
    Side side{Side::Buy};
    Price price{0};
    Quantity original_qty{0};
    Quantity remaining_qty{0};
    SequenceNumber sequence{0};
    Timestamp timestamp{0};
    OrderType type{OrderType::Limit};
    TimeInForce tif{TimeInForce::GTC};
    OrderStatus status{OrderStatus::New};

    uint32_t prev_idx{kInvalidIndex};
    uint32_t next_idx{kInvalidIndex};
    uint32_t generation{0};
    bool active{false};
};

struct Trade {
    TradeId trade_id{0};
    SequenceNumber sequence{0};
    Timestamp timestamp{0};
    OrderId maker_order_id{0};
    OrderId taker_order_id{0};
    Side taker_side{Side::Buy};
    Price price{0};
    Quantity quantity{0};
};

struct ExecutionReport {
    SequenceNumber sequence{0};
    Timestamp timestamp{0};
    OrderId order_id{0};
    Side side{Side::Buy};
    OrderType order_type{OrderType::Limit};
    ExecutionType exec_type{ExecutionType::New};
    OrderStatus order_status{OrderStatus::Accepted};
    RejectReason reject_reason{RejectReason::None};
    Price price{0};
    Quantity last_qty{0};
    Price last_price{0};
    Quantity leaves_qty{0};
    Quantity cum_qty{0};
    TradeId trade_id{0};
};

struct OrderRequest {
    OrderId order_id{0};
    Side side{Side::Buy};
    Price price{0};
    Quantity quantity{0};
    OrderType type{OrderType::Limit};
    TimeInForce tif{TimeInForce::GTC};
    Timestamp timestamp{0};
};

struct CancelRequest {
    OrderId order_id{0};
    Timestamp timestamp{0};
};

struct ModifyRequest {
    OrderId order_id{0};
    Quantity new_quantity{0};
    Timestamp timestamp{0};
};

struct ReplaceRequest {
    OrderId old_order_id{0};
    OrderId new_order_id{0};
    Side side{Side::Buy};
    Price new_price{0};
    Quantity new_quantity{0};
    OrderType type{OrderType::Limit};
    TimeInForce tif{TimeInForce::GTC};
    Timestamp timestamp{0};
};

// Tick conversion helpers
[[nodiscard]] inline Price price_to_ticks(double price, double tick_size) noexcept {
    if (tick_size <= 0.0) return 0;
    return static_cast<Price>(std::round(price / tick_size));
}

[[nodiscard]] inline double ticks_to_price(Price ticks, double tick_size) noexcept {
    return static_cast<double>(ticks) * tick_size;
}

[[nodiscard]] inline Quantity qty_to_lots(double qty, double lot_size) noexcept {
    if (lot_size <= 0.0) return 0;
    return static_cast<Quantity>(std::round(qty / lot_size));
}

[[nodiscard]] inline double lots_to_qty(Quantity lots, double lot_size) noexcept {
    return static_cast<double>(lots) * lot_size;
}

} // namespace lob
