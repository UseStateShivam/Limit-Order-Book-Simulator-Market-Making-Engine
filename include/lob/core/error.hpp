#pragma once

#include <string>
#include <string_view>
#include <system_error>

namespace lob {

enum class ErrorCode {
    Success = 0,
    InvalidPrice,
    InvalidQuantity,
    DuplicateOrderId,
    OrderNotFound,
    InsufficientLiquidity,
    CrossedBookDetected,
    SelfCrossingError,
    RiskLimitBreached,
    CorruptArenaState,
    InvariantViolation,
    DataParseError,
    ConfigError,
    ReplayError
};

class LobErrorCategory : public std::error_category {
public:
    [[nodiscard]] const char* name() const noexcept override {
        return "LimitOrderBook";
    }

    [[nodiscard]] std::string message(int ev) const override {
        switch (static_cast<ErrorCode>(ev)) {
            case ErrorCode::Success:              return "Success";
            case ErrorCode::InvalidPrice:         return "Invalid order price";
            case ErrorCode::InvalidQuantity:      return "Invalid order quantity";
            case ErrorCode::DuplicateOrderId:     return "Duplicate order identifier";
            case ErrorCode::OrderNotFound:        return "Order identifier not found";
            case ErrorCode::InsufficientLiquidity: return "Insufficient liquidity for execution";
            case ErrorCode::CrossedBookDetected:  return "Crossed book detected";
            case ErrorCode::SelfCrossingError:    return "Self-crossing order rejected";
            case ErrorCode::RiskLimitBreached:    return "Risk constraint breached";
            case ErrorCode::CorruptArenaState:    return "Order arena memory state corrupted";
            case ErrorCode::InvariantViolation:   return "Order book invariant violation";
            case ErrorCode::DataParseError:       return "Market data parse error";
            case ErrorCode::ConfigError:          return "Configuration error";
            case ErrorCode::ReplayError:          return "Historical replay error";
        }
        return "Unknown error";
    }
};

inline const LobErrorCategory& lob_category() noexcept {
    static const LobErrorCategory cat;
    return cat;
}

inline std::error_code make_error_code(ErrorCode e) noexcept {
    return {static_cast<int>(e), lob_category()};
}

class LobException : public std::runtime_error {
public:
    explicit LobException(ErrorCode code, std::string_view msg = "")
        : std::runtime_error(std::string(make_error_code(code).message()) + 
                             (msg.empty() ? "" : ": ") + std::string(msg)),
          code_(code) {}

    [[nodiscard]] ErrorCode code() const noexcept { return code_; }

private:
    ErrorCode code_;
};

} // namespace lob

namespace std {
template <>
struct is_error_code_enum<lob::ErrorCode> : true_type {};
} // namespace std
