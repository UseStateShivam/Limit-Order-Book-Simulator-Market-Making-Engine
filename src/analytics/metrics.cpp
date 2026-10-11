#include "lob/analytics/metrics.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <algorithm>
#include <ctime>

namespace lob {

MetricsTracker::MetricsTracker(const Config& config)
    : config_(config) {}

void MetricsTracker::record_fill(const ExecutionReport& report, double current_ref_price) {
    if (report.order_status == OrderStatus::Filled) completed_fills_++;
    else if (report.order_status == OrderStatus::PartiallyFilled) partial_fills_++;

    executed_volume_ += report.last_qty;

    FillRecord fill;
    fill.timestamp = report.timestamp;
    fill.order_id = report.order_id;
    fill.side = report.side;
    fill.price = report.last_price;
    fill.quantity = report.last_qty;
    fill.reference_price = current_ref_price;
    fills_.push_back(fill);

    // Spread capture
    double exec_price = ticks_to_price(report.last_price, config_.instrument.tick_size);
    double spread_captured = 0.0;
    if (report.side == Side::Buy) {
        spread_captured = current_ref_price - exec_price;
    } else {
        spread_captured = exec_price - current_ref_price;
    }
    total_spread_captured_ += spread_captured * report.last_qty;
}

void MetricsTracker::record_mid_price(Timestamp ts, double price) {
    mid_price_history_.emplace_back(ts, price);
}

PerformanceSummary MetricsTracker::compute_summary(
    const PortfolioAccounting& accounting,
    double wall_clock_seconds) const {

    PerformanceSummary s;
    s.run_id = "RUN_" + std::to_string(config_.simulation.random_seed);
    s.strategy_type = config_.strategy.type;
    s.scenario = config_.simulation.scenario;
    s.random_seed = config_.simulation.random_seed;

    s.initial_capital = 100000.0;
    s.ending_equity = accounting.current_equity();
    s.net_pnl = accounting.net_pnl();
    s.gross_pnl = accounting.gross_pnl();
    s.realized_pnl = accounting.realized_pnl();
    s.unrealized_pnl = accounting.unrealized_pnl();
    s.total_fees = accounting.cumulative_fees();
    s.return_pct = (s.initial_capital > 0) ? (s.net_pnl / s.initial_capital) * 100.0 : 0.0;

    s.orders_submitted = orders_submitted_;
    s.orders_canceled = orders_canceled_;
    s.orders_rejected = orders_rejected_;
    s.completed_fills = completed_fills_;
    s.partial_fills = partial_fills_;
    s.executed_volume = executed_volume_;

    size_t total_fill_events = completed_fills_ + partial_fills_;
    s.fill_rate_pct = (orders_submitted_ > 0) ? (static_cast<double>(total_fill_events) / orders_submitted_) * 100.0 : 0.0;
    s.quote_to_fill_ratio = (total_fill_events > 0) ? static_cast<double>(orders_submitted_) / total_fill_events : 0.0;
    s.avg_spread_captured = (executed_volume_ > 0) ? total_spread_captured_ / executed_volume_ : 0.0;

    // Inventory metrics
    const auto& eq_curve = accounting.equity_curve();
    double sum_abs_inv = 0.0;
    Quantity max_abs = 0;
    for (const auto& pt : eq_curve) {
        Quantity abs_q = std::abs(pt.inventory);
        sum_abs_inv += static_cast<double>(abs_q);
        if (abs_q > max_abs) max_abs = abs_q;
    }
    s.mean_abs_inventory = (!eq_curve.empty()) ? sum_abs_inv / eq_curve.size() : 0.0;
    s.max_abs_inventory = max_abs;
    s.max_drawdown_pct = accounting.max_drawdown_pct() * 100.0;
    s.max_drawdown_abs = accounting.max_drawdown_abs();

    // Markout horizons: 1ms, 10ms, 100ms, 1s
    std::vector<std::pair<uint64_t, std::string>> horizons = {
        {1'000'000ULL, "1ms"},
        {10'000'000ULL, "10ms"},
        {100'000'000ULL, "100ms"},
        {1'000'000'000ULL, "1s"}
    };

    for (const auto& [h_ns, label] : horizons) {
        MarkoutMetric m;
        m.horizon_ns = h_ns;
        m.horizon_label = label;
        double sum_markout = 0.0;
        size_t count = 0;

        for (const auto& fill : fills_) {
            Timestamp target_ts = fill.timestamp + h_ns;
            auto it = std::lower_bound(
                mid_price_history_.begin(), mid_price_history_.end(), target_ts,
                [](const auto& pair, Timestamp val) { return pair.first < val; });

            if (it != mid_price_history_.end()) {
                double future_price = it->second;
                double fill_price = ticks_to_price(fill.price, config_.instrument.tick_size);
                double markout = (fill.side == Side::Buy) ? (future_price - fill_price) : (fill_price - future_price);
                sum_markout += markout;
                count++;
            }
        }

        m.sample_count = count;
        m.mean_markout = (count > 0) ? sum_markout / count : 0.0;
        s.markouts.push_back(m);
    }

    s.total_events_processed = mid_price_history_.size();
    s.elapsed_simulation_sec = wall_clock_seconds;
    s.throughput_events_per_sec = (wall_clock_seconds > 0.0) ? static_cast<double>(s.total_events_processed) / wall_clock_seconds : 0.0;

    return s;
}

std::string PerformanceSummary::to_json() const {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(4);
    ss << "{\n"
       << "  \"run_id\": \"" << run_id << "\",\n"
       << "  \"strategy_type\": \"" << strategy_type << "\",\n"
       << "  \"scenario\": \"" << scenario << "\",\n"
       << "  \"random_seed\": " << random_seed << ",\n"
       << "  \"financials\": {\n"
       << "    \"initial_capital\": " << initial_capital << ",\n"
       << "    \"ending_equity\": " << ending_equity << ",\n"
       << "    \"net_pnl\": " << net_pnl << ",\n"
       << "    \"gross_pnl\": " << gross_pnl << ",\n"
       << "    \"realized_pnl\": " << realized_pnl << ",\n"
       << "    \"unrealized_pnl\": " << unrealized_pnl << ",\n"
       << "    \"total_fees\": " << total_fees << ",\n"
       << "    \"return_pct\": " << return_pct << "\n"
       << "  },\n"
       << "  \"execution\": {\n"
       << "    \"orders_submitted\": " << orders_submitted << ",\n"
       << "    \"orders_canceled\": " << orders_canceled << ",\n"
       << "    \"orders_rejected\": " << orders_rejected << ",\n"
       << "    \"completed_fills\": " << completed_fills << ",\n"
       << "    \"partial_fills\": " << partial_fills << ",\n"
       << "    \"executed_volume\": " << executed_volume << ",\n"
       << "    \"fill_rate_pct\": " << fill_rate_pct << ",\n"
       << "    \"quote_to_fill_ratio\": " << quote_to_fill_ratio << ",\n"
       << "    \"avg_spread_captured\": " << avg_spread_captured << "\n"
       << "  },\n"
       << "  \"risk\": {\n"
       << "    \"mean_abs_inventory\": " << mean_abs_inventory << ",\n"
       << "    \"max_abs_inventory\": " << max_abs_inventory << ",\n"
       << "    \"max_drawdown_pct\": " << max_drawdown_pct << ",\n"
       << "    \"max_drawdown_abs\": " << max_drawdown_abs << "\n"
       << "  },\n"
       << "  \"adverse_selection_markouts\": [\n";

    for (size_t i = 0; i < markouts.size(); ++i) {
        const auto& m = markouts[i];
        ss << "    {\"horizon\": \"" << m.horizon_label << "\", \"mean_markout\": "
           << m.mean_markout << ", \"samples\": " << m.sample_count << "}"
           << (i + 1 < markouts.size() ? ",\n" : "\n");
    }

    ss << "  ],\n"
       << "  \"performance\": {\n"
       << "    \"total_events\": " << total_events_processed << ",\n"
       << "    \"runtime_sec\": " << elapsed_simulation_sec << ",\n"
       << "    \"throughput_events_per_sec\": " << throughput_events_per_sec << "\n"
       << "  }\n"
       << "}\n";
    return ss.str();
}

std::string PerformanceSummary::to_markdown() const {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(4);
    ss << "# Simulation Report: " << strategy_type << " (" << scenario << ")\n\n"
       << "## 1. Executive Summary\n"
       << "- **Run ID**: `" << run_id << "`\n"
       << "- **Strategy**: `" << strategy_type << "`\n"
       << "- **Scenario**: `" << scenario << "`\n"
       << "- **Net P&L**: `" << (net_pnl >= 0 ? "+" : "") << net_pnl << " USD`\n"
       << "- **Return on Capital**: `" << return_pct << "%`\n"
       << "- **Max Drawdown**: `" << max_drawdown_pct << "% (" << max_drawdown_abs << " USD)`\n\n"
       << "## 2. Trading and Financial Metrics\n"
       << "| Metric | Value |\n"
       << "|---|---|\n"
       << "| Initial Capital | $" << initial_capital << " |\n"
       << "| Ending Equity | $" << ending_equity << " |\n"
       << "| Gross P&L | $" << gross_pnl << " |\n"
       << "| Net P&L | $" << net_pnl << " |\n"
       << "| Realized P&L | $" << realized_pnl << " |\n"
       << "| Unrealized P&L | $" << unrealized_pnl << " |\n"
       << "| Fees & Rebates | $" << total_fees << " |\n\n"
       << "## 3. Order Execution & Microstructure\n"
       << "| Metric | Value |\n"
       << "|---|---|\n"
       << "| Orders Submitted | " << orders_submitted << " |\n"
       << "| Orders Canceled | " << orders_canceled << " |\n"
       << "| Fills (Full / Partial) | " << completed_fills << " / " << partial_fills << " |\n"
       << "| Executed Volume | " << executed_volume << " lots |\n"
       << "| Fill Rate | " << fill_rate_pct << "% |\n"
       << "| Quote-to-Fill Ratio | " << quote_to_fill_ratio << " |\n"
       << "| Avg Spread Captured | $" << avg_spread_captured << " / lot |\n\n"
       << "## 4. Adverse Selection Markouts (Post-Fill)\n"
       << "| Horizon | Mean Markout | Sample Count |\n"
       << "|---|---|---|\n";
    for (const auto& m : markouts) {
        ss << "| " << m.horizon_label << " | $" << m.mean_markout << " | " << m.sample_count << " |\n";
    }
    ss << "\n## 5. Risk & Inventory Exposure\n"
       << "| Metric | Value |\n"
       << "|---|---|\n"
       << "| Mean Absolute Inventory | " << mean_abs_inventory << " lots |\n"
       << "| Max Absolute Inventory | " << max_abs_inventory << " lots |\n"
       << "| Max Drawdown % | " << max_drawdown_pct << "% |\n"
       << "| Max Drawdown Abs | $" << max_drawdown_abs << " |\n";

    return ss.str();
}

void MetricsTracker::export_results(
    const std::string& output_dir,
    const PerformanceSummary& summary,
    const Config& config,
    const std::vector<Trade>& trades,
    const std::vector<ExecutionReport>& reports,
    const std::vector<EquityPoint>& equity_curve,
    const std::vector<StrategyDecision>& decisions) {

    std::filesystem::create_directories(output_dir);

    // 1. summary.json
    {
        std::ofstream out(output_dir + "/summary.json");
        out << summary.to_json();
    }

    // 2. report.md
    {
        std::ofstream out(output_dir + "/report.md");
        out << summary.to_markdown();
    }

    // 3. run_metadata.json
    {
        std::ofstream out(output_dir + "/run_metadata.json");
        out << "{\n"
            << "  \"run_id\": \"" << summary.run_id << "\",\n"
            << "  \"symbol\": \"" << config.instrument.symbol << "\",\n"
            << "  \"strategy\": \"" << config.strategy.type << "\",\n"
            << "  \"scenario\": \"" << config.simulation.scenario << "\",\n"
            << "  \"seed\": " << config.simulation.random_seed << ",\n"
            << "  \"tick_size\": " << config.instrument.tick_size << ",\n"
            << "  \"lot_size\": " << config.instrument.lot_size << ",\n"
            << "  \"gamma\": " << config.strategy.gamma << ",\n"
            << "  \"k\": " << config.strategy.k << ",\n"
            << "  \"max_inventory\": " << config.strategy.max_inventory << "\n"
            << "}\n";
    }

    // 4. trades.csv
    {
        std::ofstream out(output_dir + "/trades.csv");
        out << "trade_id,sequence,timestamp_ns,maker_order_id,taker_order_id,taker_side,price,quantity\n";
        for (const auto& t : trades) {
            out << t.trade_id << "," << t.sequence << "," << t.timestamp << ","
                << t.maker_order_id << "," << t.taker_order_id << ","
                << side_to_string(t.taker_side) << ","
                << ticks_to_price(t.price, config.instrument.tick_size) << ","
                << lots_to_qty(t.quantity, config.instrument.lot_size) << "\n";
        }
    }

    // 5. execution_reports.csv
    {
        std::ofstream out(output_dir + "/execution_reports.csv");
        out << "sequence,timestamp_ns,order_id,side,order_type,exec_type,order_status,price,last_qty,leaves_qty,cum_qty\n";
        for (const auto& r : reports) {
            out << r.sequence << "," << r.timestamp << "," << r.order_id << ","
                << side_to_string(r.side) << ","
                << order_type_to_string(r.order_type) << ","
                << exec_type_to_string(r.exec_type) << ","
                << order_status_to_string(r.order_status) << ","
                << ticks_to_price(r.price, config.instrument.tick_size) << ","
                << lots_to_qty(r.last_qty, config.instrument.lot_size) << ","
                << lots_to_qty(r.leaves_qty, config.instrument.lot_size) << ","
                << lots_to_qty(r.cum_qty, config.instrument.lot_size) << "\n";
        }
    }

    // 6. equity_curve.csv
    {
        std::ofstream out(output_dir + "/equity_curve.csv");
        out << "timestamp_ns,cash,inventory,reference_price,equity,realized_pnl,unrealized_pnl,cumulative_fees\n";
        for (const auto& pt : equity_curve) {
            out << pt.timestamp << "," << pt.cash << "," << pt.inventory << ","
                << pt.reference_price << "," << pt.equity << ","
                << pt.realized_pnl << "," << pt.unrealized_pnl << ","
                << pt.cumulative_fees << "\n";
        }
    }

    // 7. strategy_decisions.csv
    {
        std::ofstream out(output_dir + "/strategy_decisions.csv");
        out << "timestamp_ns,reference_price,volatility,inventory,reservation_price,half_spread,target_bid,target_ask,imbalance,action,reason\n";
        for (const auto& d : decisions) {
            out << d.timestamp << "," << d.reference_price << "," << d.volatility << ","
                << d.inventory << "," << d.reservation_price << "," << d.half_spread << ","
                << ticks_to_price(d.target_bid, config.instrument.tick_size) << ","
                << ticks_to_price(d.target_ask, config.instrument.tick_size) << ","
                << d.imbalance << ",\"" << d.action << "\",\"" << d.reason << "\"\n";
        }
    }
}

void MetricsTracker::reset() {
    orders_submitted_ = 0;
    orders_canceled_ = 0;
    orders_rejected_ = 0;
    completed_fills_ = 0;
    partial_fills_ = 0;
    executed_volume_ = 0;
    total_spread_captured_ = 0.0;
    fills_.clear();
    mid_price_history_.clear();
}

} // namespace lob
