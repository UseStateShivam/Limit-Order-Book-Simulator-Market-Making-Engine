#pragma once

#include "lob/core/types.hpp"
#include <vector>
#include <cmath>
#include <string>

namespace lob {

struct EquityPoint {
    Timestamp timestamp{0};
    double cash{0.0};
    Quantity inventory{0};
    double reference_price{0.0};
    double equity{0.0};
    double realized_pnl{0.0};
    double unrealized_pnl{0.0};
    double cumulative_fees{0.0};
};

class PortfolioAccounting {
public:
    explicit PortfolioAccounting(double initial_cash = 100000.0,
                                 double maker_fee_rate = -0.0001,
                                 double taker_fee_rate = 0.0002)
        : initial_cash_(initial_cash),
          cash_(initial_cash),
          maker_fee_rate_(maker_fee_rate),
          taker_fee_rate_(taker_fee_rate),
          peak_equity_(initial_cash) {}

    void on_trade(const Trade& trade, bool is_maker, double tick_size, double lot_size, double reference_price) {
        double exec_price = ticks_to_price(trade.price, tick_size);
        double exec_qty = lots_to_qty(trade.quantity, lot_size);
        double notional = exec_price * exec_qty;

        double fee_rate = is_maker ? maker_fee_rate_ : taker_fee_rate_;
        double fee = notional * fee_rate; // negative fee rate means rebate (cash increase)
        cumulative_fees_ += fee;
        cash_ -= fee;

        bool is_buy = (is_maker && trade.taker_side == Side::Sell) ||
                      (!is_maker && trade.taker_side == Side::Buy);

        if (is_buy) {
            cash_ -= notional;
            if (inventory_ >= 0) {
                // Expanding long position
                double old_cost = avg_cost_ * lots_to_qty(inventory_, lot_size);
                inventory_ += trade.quantity;
                double new_total_qty = lots_to_qty(inventory_, lot_size);
                avg_cost_ = (new_total_qty > 0) ? (old_cost + notional) / new_total_qty : 0.0;
            } else {
                // Covering short position
                Quantity closing_qty = std::min(trade.quantity, -inventory_);
                double closing_realized = (avg_cost_ - exec_price) * lots_to_qty(closing_qty, lot_size);
                realized_pnl_ += closing_realized;
                inventory_ += trade.quantity;
                if (inventory_ > 0) {
                    avg_cost_ = exec_price;
                } else if (inventory_ == 0) {
                    avg_cost_ = 0.0;
                }
            }
        } else {
            // Sell
            cash_ += notional;
            if (inventory_ <= 0) {
                // Expanding short position
                double old_cost = avg_cost_ * lots_to_qty(-inventory_, lot_size);
                inventory_ -= trade.quantity;
                double new_total_qty = lots_to_qty(-inventory_, lot_size);
                avg_cost_ = (new_total_qty > 0) ? (old_cost + notional) / new_total_qty : 0.0;
            } else {
                // Selling long position
                Quantity closing_qty = std::min(trade.quantity, inventory_);
                double closing_realized = (exec_price - avg_cost_) * lots_to_qty(closing_qty, lot_size);
                realized_pnl_ += closing_realized;
                inventory_ -= trade.quantity;
                if (inventory_ < 0) {
                    avg_cost_ = exec_price;
                } else if (inventory_ == 0) {
                    avg_cost_ = 0.0;
                }
            }
        }

        total_executed_lots_ += trade.quantity;
        trade_count_++;
        record_equity_point(trade.timestamp, reference_price, lot_size);
    }

    void record_equity_point(Timestamp ts, double reference_price, double lot_size) {
        double inv_qty = lots_to_qty(inventory_, lot_size);
        double pos_value = inv_qty * reference_price;
        double equity = cash_ + pos_value;

        if (inventory_ > 0) {
            unrealized_pnl_ = (reference_price - avg_cost_) * inv_qty;
        } else if (inventory_ < 0) {
            unrealized_pnl_ = (avg_cost_ - reference_price) * (-inv_qty);
        } else {
            unrealized_pnl_ = 0.0;
        }

        if (equity > peak_equity_) peak_equity_ = equity;
        double drawdown = (peak_equity_ > 0.0) ? (equity - peak_equity_) / peak_equity_ : 0.0;
        if (drawdown < max_drawdown_pct_) max_drawdown_pct_ = drawdown;
        double abs_dd = peak_equity_ - equity;
        if (abs_dd > max_drawdown_abs_) max_drawdown_abs_ = abs_dd;

        equity_curve_.push_back({
            ts, cash_, inventory_, reference_price, equity,
            realized_pnl_, unrealized_pnl_, cumulative_fees_
        });
    }

    [[nodiscard]] double cash() const noexcept { return cash_; }
    [[nodiscard]] Quantity inventory() const noexcept { return inventory_; }
    [[nodiscard]] double realized_pnl() const noexcept { return realized_pnl_; }
    [[nodiscard]] double unrealized_pnl() const noexcept { return unrealized_pnl_; }
    [[nodiscard]] double cumulative_fees() const noexcept { return cumulative_fees_; }
    [[nodiscard]] double current_equity() const noexcept {
        return equity_curve_.empty() ? initial_cash_ : equity_curve_.back().equity;
    }
    [[nodiscard]] double net_pnl() const noexcept {
        return current_equity() - initial_cash_;
    }
    [[nodiscard]] double gross_pnl() const noexcept {
        return net_pnl() + cumulative_fees_;
    }
    [[nodiscard]] double max_drawdown_pct() const noexcept { return max_drawdown_pct_; }
    [[nodiscard]] double max_drawdown_abs() const noexcept { return max_drawdown_abs_; }
    [[nodiscard]] Quantity total_executed_lots() const noexcept { return total_executed_lots_; }
    [[nodiscard]] size_t trade_count() const noexcept { return trade_count_; }
    [[nodiscard]] const std::vector<EquityPoint>& equity_curve() const noexcept { return equity_curve_; }

    void reset() {
        cash_ = initial_cash_;
        inventory_ = 0;
        avg_cost_ = 0.0;
        realized_pnl_ = 0.0;
        unrealized_pnl_ = 0.0;
        cumulative_fees_ = 0.0;
        peak_equity_ = initial_cash_;
        max_drawdown_pct_ = 0.0;
        max_drawdown_abs_ = 0.0;
        total_executed_lots_ = 0;
        trade_count_ = 0;
        equity_curve_.clear();
    }

private:
    double initial_cash_{100000.0};
    double cash_{100000.0};
    Quantity inventory_{0};
    double avg_cost_{0.0};
    double realized_pnl_{0.0};
    double unrealized_pnl_{0.0};
    double cumulative_fees_{0.0};
    double maker_fee_rate_{-0.0001};
    double taker_fee_rate_{0.0002};

    double peak_equity_{100000.0};
    double max_drawdown_pct_{0.0};
    double max_drawdown_abs_{0.0};

    Quantity total_executed_lots_{0};
    size_t trade_count_{0};
    std::vector<EquityPoint> equity_curve_;
};

} // namespace lob
