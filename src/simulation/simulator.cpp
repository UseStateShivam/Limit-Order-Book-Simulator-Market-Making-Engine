#include "lob/simulation/simulator.hpp"
#include "lob/market_data/synthetic_generator.hpp"
#include "lob/market_data/csv_parser.hpp"
#include <chrono>

namespace lob {

Simulator::Simulator(const Config& config)
    : config_(config),
      engine_(config.instrument, config.matching),
      strategy_(config),
      accounting_(config.simulation.initial_cash,
                  config.simulation.maker_fee_rate,
                  config.simulation.taker_fee_rate),
      metrics_(config) {

    engine_.set_trade_callback([this](const Trade& trade) {
        handle_trade(trade);
    });

    engine_.set_execution_callback([this](const ExecutionReport& rep) {
        handle_execution(rep);
    });
}

void Simulator::handle_trade(const Trade& trade) {
    trades_.push_back(trade);

    bool is_strategy_maker = (trade.maker_order_id >= 1'000'000);
    bool is_strategy_taker = (trade.taker_order_id >= 1'000'000);

    if (is_strategy_maker) {
        accounting_.on_trade(trade, true, config_.instrument.tick_size,
                             config_.instrument.lot_size, current_ref_price_);
    } else if (is_strategy_taker) {
        accounting_.on_trade(trade, false, config_.instrument.tick_size,
                             config_.instrument.lot_size, current_ref_price_);
    }
}

void Simulator::handle_execution(const ExecutionReport& rep) {
    reports_.push_back(rep);

    if (rep.order_id >= 1'000'000) {
        strategy_.on_execution(rep);
        if (rep.exec_type == ExecutionType::Trade) {
            metrics_.record_fill(rep, current_ref_price_);
        } else if (rep.exec_type == ExecutionType::Canceled) {
            metrics_.record_order_canceled();
        } else if (rep.exec_type == ExecutionType::Rejected) {
            metrics_.record_order_rejected();
        }
    }
}

void Simulator::execute_quote_actions(const std::vector<QuoteAction>& actions, Timestamp ts) {
    for (const auto& act : actions) {
        if (act.type == QuoteAction::Type::Submit) {
            metrics_.record_order_submitted();
            engine_.process_order(act.order);
        } else if (act.type == QuoteAction::Type::Cancel) {
            CancelRequest cancel_req{act.cancel_order_id, ts};
            engine_.process_cancel(cancel_req);
        }
    }
}

PerformanceSummary Simulator::run() {
    SyntheticGenerator generator(config_);
    auto events = generator.generate_events(config_.simulation.event_count);
    return run_with_events(events);
}

PerformanceSummary Simulator::run_with_events(const std::vector<OrderRequest>& events) {
    reset();

    auto start_clock = std::chrono::steady_clock::now();

    for (const auto& ev : events) {
        current_time_ = ev.timestamp;

        // 1. Process market participant event
        engine_.process_order(ev);

        // 2. Update reference mid-price
        auto mid_opt = engine_.book().mid_price();
        if (mid_opt) {
            current_ref_price_ = ticks_to_price(
                static_cast<Price>(std::round(*mid_opt)), config_.instrument.tick_size);
        }
        metrics_.record_mid_price(current_time_, current_ref_price_);

        // 3. Strategy generates quotes
        auto actions = strategy_.on_market_update(current_time_, engine_.book());

        // 4. Submit quotes to matching engine
        execute_quote_actions(actions, current_time_);

        // 5. Update equity curve point
        accounting_.record_equity_point(current_time_, current_ref_price_, config_.instrument.lot_size);
    }

    auto end_clock = std::chrono::steady_clock::now();
    double wall_clock_sec = std::chrono::duration<double>(end_clock - start_clock).count();

    auto summary = metrics_.compute_summary(accounting_, wall_clock_sec);

    if (!config_.simulation.output_dir.empty()) {
        MetricsTracker::export_results(
            config_.simulation.output_dir,
            summary,
            config_,
            trades_,
            reports_,
            accounting_.equity_curve(),
            strategy_.decision_log());
    }

    return summary;
}

PerformanceSummary Simulator::replay_file(const std::string& filepath) {
    DataQualityReport dq_report;
    auto order_events = CsvParser::parse_order_events(
        filepath, config_.instrument.tick_size, config_.instrument.lot_size, &dq_report);

    if (order_events.empty()) {
        // Try parsing as market_events
        auto mkt_events = CsvParser::parse_market_events(
            filepath, config_.instrument.tick_size, config_.instrument.lot_size, &dq_report);

        std::vector<OrderRequest> synthetic_orders;
        synthetic_orders.reserve(mkt_events.size() * 2);
        OrderId oid = 1;
        for (const auto& mev : mkt_events) {
            if (mev.bid_price > 0 && mev.bid_quantity > 0) {
                OrderRequest b;
                b.order_id = ++oid;
                b.side = Side::Buy;
                b.price = mev.bid_price;
                b.quantity = mev.bid_quantity;
                b.timestamp = mev.timestamp;
                b.type = OrderType::Limit;
                b.tif = TimeInForce::GTC;
                synthetic_orders.push_back(b);
            }
            if (mev.ask_price > 0 && mev.ask_quantity > 0) {
                OrderRequest a;
                a.order_id = ++oid;
                a.side = Side::Sell;
                a.price = mev.ask_price;
                a.quantity = mev.ask_quantity;
                a.timestamp = mev.timestamp;
                a.type = OrderType::Limit;
                a.tif = TimeInForce::GTC;
                synthetic_orders.push_back(a);
            }
        }
        return run_with_events(synthetic_orders);
    }

    return run_with_events(order_events);
}

void Simulator::reset() {
    engine_.reset();
    strategy_.reset();
    accounting_.reset();
    metrics_.reset();
    trades_.clear();
    reports_.clear();
    current_time_ = 0;
    current_ref_price_ = 100.0;
}

} // namespace lob
