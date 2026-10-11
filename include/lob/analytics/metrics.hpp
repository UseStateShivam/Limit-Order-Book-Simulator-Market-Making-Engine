#pragma once

#include "lob/core/types.hpp"
#include "lob/core/config.hpp"
#include "lob/analytics/accounting.hpp"
#include "lob/strategy/market_maker_strategy.hpp"
#include <vector>
#include <string>
#include <map>

namespace lob {

struct MarkoutMetric {
    uint64_t horizon_ns{0};
    std::string horizon_label{""};
    double mean_markout{0.0};
    size_t sample_count{0};
};

struct PerformanceSummary {
    std::string run_id{""};
    std::string strategy_type{""};
    std::string scenario{""};
    uint64_t random_seed{0};

    // Trading performance
    double initial_capital{0.0};
    double ending_equity{0.0};
    double net_pnl{0.0};
    double gross_pnl{0.0};
    double realized_pnl{0.0};
    double unrealized_pnl{0.0};
    double total_fees{0.0};
    double return_pct{0.0};

    // Execution performance
    size_t orders_submitted{0};
    size_t orders_canceled{0};
    size_t orders_rejected{0};
    size_t completed_fills{0};
    size_t partial_fills{0};
    Quantity executed_volume{0};
    double fill_rate_pct{0.0};
    double quote_to_fill_ratio{0.0};
    double avg_spread_captured{0.0};

    // Inventory & Risk
    double mean_abs_inventory{0.0};
    Quantity max_abs_inventory{0};
    double max_drawdown_pct{0.0};
    double max_drawdown_abs{0.0};

    // Adverse selection
    std::vector<MarkoutMetric> markouts;

    // Operational
    size_t total_events_processed{0};
    double elapsed_simulation_sec{0.0};
    double throughput_events_per_sec{0.0};

    [[nodiscard]] std::string to_json() const;
    [[nodiscard]] std::string to_markdown() const;
};

class MetricsTracker {
public:
    struct FillRecord {
        Timestamp timestamp{0};
        OrderId order_id{0};
        Side side{Side::Buy};
        Price price{0};
        Quantity quantity{0};
        double reference_price{0.0};
    };

    explicit MetricsTracker(const Config& config);

    void record_order_submitted() { orders_submitted_++; }
    void record_order_canceled() { orders_canceled_++; }
    void record_order_rejected() { orders_rejected_++; }
    void record_fill(const ExecutionReport& report, double current_ref_price);
    void record_mid_price(Timestamp ts, double price);

    [[nodiscard]] PerformanceSummary compute_summary(
        const PortfolioAccounting& accounting,
        double wall_clock_seconds = 0.0) const;

    static void export_results(
        const std::string& output_dir,
        const PerformanceSummary& summary,
        const Config& config,
        const std::vector<Trade>& trades,
        const std::vector<ExecutionReport>& reports,
        const std::vector<EquityPoint>& equity_curve,
        const std::vector<StrategyDecision>& decisions);

    void reset();

private:
    Config config_;
    size_t orders_submitted_{0};
    size_t orders_canceled_{0};
    size_t orders_rejected_{0};
    size_t completed_fills_{0};
    size_t partial_fills_{0};
    Quantity executed_volume_{0};
    double total_spread_captured_{0.0};

    std::vector<FillRecord> fills_;
    std::vector<std::pair<Timestamp, double>> mid_price_history_;
};

} // namespace lob
