#include "lob/strategy/market_maker_strategy.hpp"
#include <algorithm>
#include <cmath>

namespace lob {

MarketMakerStrategy::MarketMakerStrategy(const Config& config)
    : config_(config),
      vol_estimator_(config.strategy.volatility_window, 0.01) {}

double MarketMakerStrategy::calculate_reservation_price(
    double mid_price, Quantity inventory, double vol, double tau, double gamma) noexcept {
    if (tau <= 0.0) tau = 0.0;
    // r_t = S_t - q_t * gamma * sigma^2 * tau
    return mid_price - static_cast<double>(inventory) * gamma * (vol * vol) * tau;
}

double MarketMakerStrategy::calculate_optimal_half_spread(
    double vol, double tau, double gamma, double k) noexcept {
    if (tau <= 0.0) tau = 0.001;
    if (k <= 0.0) k = 1.5;

    // First term: (gamma * sigma^2 * tau) / 2
    double term1 = (gamma * (vol * vol) * tau) / 2.0;

    // Second term: (1 / gamma) * ln(1 + gamma / k)
    // Numerically stable handling for small gamma
    double term2 = 0.0;
    if (gamma < 1e-5) {
        // Taylor expansion: 1/k - gamma / (2 * k^2)
        term2 = (1.0 / k) - (gamma / (2.0 * k * k));
    } else {
        term2 = (1.0 / gamma) * std::log(1.0 + (gamma / k));
    }

    return term1 + term2;
}

double MarketMakerStrategy::calculate_imbalance(const OrderBook& book, size_t depth) noexcept {
    Quantity bid_qty = book.total_bid_quantity(depth);
    Quantity ask_qty = book.total_ask_quantity(depth);
    Quantity total = bid_qty + ask_qty;
    if (total == 0) return 0.0;
    return static_cast<double>(bid_qty - ask_qty) / static_cast<double>(total);
}

std::vector<QuoteAction> MarketMakerStrategy::on_market_update(Timestamp ts, const OrderBook& book) {
    std::vector<QuoteAction> actions;
    auto mid_opt = book.mid_price();
    if (!mid_opt) return actions;

    if (start_time_ == 0) start_time_ = ts;

    double mid_ticks = *mid_opt;
    double mid_price = ticks_to_price(static_cast<Price>(std::round(mid_ticks)), config_.instrument.tick_size);
    vol_estimator_.add_price(mid_price);

    double vol = (config_.strategy.fixed_volatility > 0.0)
                 ? config_.strategy.fixed_volatility
                 : vol_estimator_.estimate();

    double elapsed_sec = (ts > start_time_) ? static_cast<double>(ts - start_time_) / 1e9 : 0.0;
    double tau = std::max(0.001, config_.strategy.horizon_seconds - elapsed_sec);

    double res_price = calculate_reservation_price(
        mid_price, inventory_, vol, tau, config_.strategy.gamma);

    double imbalance = calculate_imbalance(book, 5);

    if (config_.strategy.type == "as_liquidity_aware") {
        res_price += config_.strategy.liquidity_skew_coeff * imbalance * config_.instrument.tick_size;
    }

    Price target_bid_ticks = 0;
    Price target_ask_ticks = 0;
    double half_spread = 0.0;

    if (config_.strategy.type == "fixed_spread") {
        Price mid_int = static_cast<Price>(std::round(mid_ticks));
        target_bid_ticks = mid_int - config_.strategy.fixed_half_spread_ticks;
        target_ask_ticks = mid_int + config_.strategy.fixed_half_spread_ticks;
        half_spread = static_cast<double>(config_.strategy.fixed_half_spread_ticks) * config_.instrument.tick_size;
    } else {
        half_spread = calculate_optimal_half_spread(
            vol, tau, config_.strategy.gamma, config_.strategy.k);
        double target_bid_p = res_price - half_spread;
        double target_ask_p = res_price + half_spread;
        target_bid_ticks = price_to_ticks(target_bid_p, config_.instrument.tick_size);
        target_ask_ticks = price_to_ticks(target_ask_p, config_.instrument.tick_size);
    }

    if (target_bid_ticks >= target_ask_ticks) {
        target_ask_ticks = target_bid_ticks + 1;
    }

    // Inside market boundary enforcement: market maker places resting liquidity
    auto best_b = book.best_bid();
    auto best_a = book.best_ask();
    if (best_b && best_a) {
        if (target_bid_ticks < *best_b - 2) {
            target_bid_ticks = *best_b - 1;
        }
        if (target_ask_ticks > *best_a + 2) {
            target_ask_ticks = *best_a + 1;
        }
    }
    if (best_b && target_ask_ticks <= *best_b) {
        target_ask_ticks = *best_b + 1;
    }
    if (best_a && target_bid_ticks >= *best_a) {
        target_bid_ticks = *best_a - 1;
    }
    if (target_bid_ticks >= target_ask_ticks) {
        target_ask_ticks = target_bid_ticks + 1;
    }

    StrategyDecision decision;
    decision.timestamp = ts;
    decision.reference_price = mid_price;
    decision.volatility = vol;
    decision.inventory = inventory_;
    decision.reservation_price = res_price;
    decision.half_spread = half_spread;
    decision.target_bid = target_bid_ticks;
    decision.target_ask = target_ask_ticks;
    decision.actual_bid = target_bid_ticks;
    decision.actual_ask = target_ask_ticks;
    decision.imbalance = imbalance;

    // Inventory constraint check
    bool allow_bid = (inventory_ < config_.strategy.max_inventory);
    bool allow_ask = (inventory_ > -config_.strategy.max_inventory);

    // Bid quote management
    if (!allow_bid) {
        if (current_bid_id_ != 0) {
            QuoteAction act;
            act.type = QuoteAction::Type::Cancel;
            act.cancel_order_id = current_bid_id_;
            actions.push_back(act);
            current_bid_id_ = 0;
            current_bid_price_ = 0;
        }
        decision.action += "CANCEL_BID_RISK ";
    } else {
        bool need_update = false;
        if (current_bid_id_ == 0) {
            need_update = true;
        } else if (std::abs(target_bid_ticks - current_bid_price_) >= config_.strategy.quote_update_threshold_ticks) {
            need_update = true;
        }

        if (need_update) {
            if (current_bid_id_ != 0) {
                QuoteAction act;
                act.type = QuoteAction::Type::Cancel;
                act.cancel_order_id = current_bid_id_;
                actions.push_back(act);
            }
            OrderId new_id = ++next_strategy_order_id_;
            QuoteAction act;
            act.type = QuoteAction::Type::Submit;
            act.order.order_id = new_id;
            act.order.side = Side::Buy;
            act.order.price = target_bid_ticks;
            act.order.quantity = config_.strategy.order_quantity;
            act.order.type = OrderType::Limit;
            act.order.tif = TimeInForce::GTC;
            act.order.timestamp = ts;
            actions.push_back(act);

            current_bid_id_ = new_id;
            current_bid_price_ = target_bid_ticks;
            decision.action += "QUOTE_BID ";
        }
    }

    // Ask quote management
    if (!allow_ask) {
        if (current_ask_id_ != 0) {
            QuoteAction act;
            act.type = QuoteAction::Type::Cancel;
            act.cancel_order_id = current_ask_id_;
            actions.push_back(act);
            current_ask_id_ = 0;
            current_ask_price_ = 0;
        }
        decision.action += "CANCEL_ASK_RISK ";
    } else {
        bool need_update = false;
        if (current_ask_id_ == 0) {
            need_update = true;
        } else if (std::abs(target_ask_ticks - current_ask_price_) >= config_.strategy.quote_update_threshold_ticks) {
            need_update = true;
        }

        if (need_update) {
            if (current_ask_id_ != 0) {
                QuoteAction act;
                act.type = QuoteAction::Type::Cancel;
                act.cancel_order_id = current_ask_id_;
                actions.push_back(act);
            }
            OrderId new_id = ++next_strategy_order_id_;
            QuoteAction act;
            act.type = QuoteAction::Type::Submit;
            act.order.order_id = new_id;
            act.order.side = Side::Sell;
            act.order.price = target_ask_ticks;
            act.order.quantity = config_.strategy.order_quantity;
            act.order.type = OrderType::Limit;
            act.order.tif = TimeInForce::GTC;
            act.order.timestamp = ts;
            actions.push_back(act);

            current_ask_id_ = new_id;
            current_ask_price_ = target_ask_ticks;
            decision.action += "QUOTE_ASK ";
        }
    }

    if (decision.action.empty()) {
        decision.action = "HOLD";
        decision.reason = "Quotes within threshold";
    } else {
        decision.reason = "Target moved or risk updated";
    }

    decision_log_.push_back(decision);
    return actions;
}

void MarketMakerStrategy::on_execution(const ExecutionReport& report) {
    if (report.exec_type == ExecutionType::Trade) {
        if (report.side == Side::Buy) {
            inventory_ += report.last_qty;
        } else {
            inventory_ -= report.last_qty;
        }
    }

    if (report.order_id == current_bid_id_) {
        if (report.order_status == OrderStatus::Filled ||
            report.order_status == OrderStatus::Canceled ||
            report.order_status == OrderStatus::Rejected) {
            current_bid_id_ = 0;
            current_bid_price_ = 0;
        }
    }
    if (report.order_id == current_ask_id_) {
        if (report.order_status == OrderStatus::Filled ||
            report.order_status == OrderStatus::Canceled ||
            report.order_status == OrderStatus::Rejected) {
            current_ask_id_ = 0;
            current_ask_price_ = 0;
        }
    }
}

void MarketMakerStrategy::reset() {
    vol_estimator_.reset();
    inventory_ = 0;
    current_bid_id_ = 0;
    current_bid_price_ = 0;
    current_ask_id_ = 0;
    current_ask_price_ = 0;
    next_strategy_order_id_ = 1'000'000;
    start_time_ = 0;
    decision_log_.clear();
}

} // namespace lob
