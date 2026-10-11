#pragma once

#include "lob/core/types.hpp"
#include "lob/core/config.hpp"
#include "lob/matching/matching_engine.hpp"
#include "lob/strategy/market_maker_strategy.hpp"
#include "lob/analytics/accounting.hpp"
#include "lob/analytics/metrics.hpp"
#include <vector>
#include <string>

namespace lob {

class Simulator {
public:
    explicit Simulator(const Config& config);

    // Runs a complete deterministic simulation using configured scenario and seed
    PerformanceSummary run();

    // Runs simulation with externally supplied stream of events
    PerformanceSummary run_with_events(const std::vector<OrderRequest>& events);

    // Runs historical replay from file
    PerformanceSummary replay_file(const std::string& filepath);

    [[nodiscard]] const Config& config() const noexcept { return config_; }
    [[nodiscard]] const MatchingEngine& engine() const noexcept { return engine_; }
    [[nodiscard]] const PortfolioAccounting& accounting() const noexcept { return accounting_; }
    [[nodiscard]] const MarketMakerStrategy& strategy() const noexcept { return strategy_; }
    [[nodiscard]] const std::vector<Trade>& trades() const noexcept { return trades_; }
    [[nodiscard]] const std::vector<ExecutionReport>& execution_reports() const noexcept { return reports_; }

    void reset();

private:
    void handle_trade(const Trade& trade);
    void handle_execution(const ExecutionReport& report);
    void execute_quote_actions(const std::vector<QuoteAction>& actions, Timestamp ts);

    Config config_;
    MatchingEngine engine_;
    MarketMakerStrategy strategy_;
    PortfolioAccounting accounting_;
    MetricsTracker metrics_;

    std::vector<Trade> trades_;
    std::vector<ExecutionReport> reports_;
    Timestamp current_time_{0};
    double current_ref_price_{100.0};
};

} // namespace lob
