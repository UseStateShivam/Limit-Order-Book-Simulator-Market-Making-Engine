#pragma once

#include "lob/core/types.hpp"
#include "lob/core/config.hpp"
#include "lob/book/order_book.hpp"
#include "lob/strategy/volatility_estimator.hpp"
#include <vector>
#include <string>
#include <optional>

namespace lob {

struct StrategyDecision {
    Timestamp timestamp{0};
    double reference_price{0.0};
    double volatility{0.0};
    Quantity inventory{0};
    double reservation_price{0.0};
    double half_spread{0.0};
    Price target_bid{0};
    Price target_ask{0};
    Price actual_bid{0};
    Price actual_ask{0};
    double imbalance{0.0};
    std::string action{""};
    std::string reason{""};
};

struct QuoteAction {
    enum class Type { None, Submit, Cancel, Replace };
    Type type{Type::None};
    OrderRequest order;
    OrderId cancel_order_id{0};
};

class MarketMakerStrategy {
public:
    explicit MarketMakerStrategy(const Config& config);

    // Core quantitative formulas (exposed for unit testing and direct verification)
    static double calculate_reservation_price(
        double mid_price, Quantity inventory, double vol, double tau, double gamma) noexcept;

    static double calculate_optimal_half_spread(
        double vol, double tau, double gamma, double k) noexcept;

    static double calculate_imbalance(const OrderBook& book, size_t depth = 5) noexcept;

    // Strategy lifecycle
    std::vector<QuoteAction> on_market_update(Timestamp ts, const OrderBook& book);
    void on_execution(const ExecutionReport& report);

    [[nodiscard]] Quantity current_inventory() const noexcept { return inventory_; }
    [[nodiscard]] const std::vector<StrategyDecision>& decision_log() const noexcept { return decision_log_; }
    [[nodiscard]] const Config& config() const noexcept { return config_; }

    void reset();

private:
    Config config_;
    RollingVolatilityEstimator vol_estimator_;
    Quantity inventory_{0};

    OrderId current_bid_id_{0};
    Price current_bid_price_{0};
    OrderId current_ask_id_{0};
    Price current_ask_price_{0};

    OrderId next_strategy_order_id_{1'000'000};
    Timestamp start_time_{0};

    std::vector<StrategyDecision> decision_log_;
};

} // namespace lob
