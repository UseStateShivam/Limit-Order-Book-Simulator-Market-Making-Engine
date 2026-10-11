#include <gtest/gtest.h>
#include "lob/strategy/market_maker_strategy.hpp"

using namespace lob;

TEST(StrategyTest, ReservationPriceFormulas) {
    double mid = 100.0;
    double vol = 0.02;
    double tau = 300.0; // 300 seconds
    double gamma = 0.1;

    // 1. Zero inventory => r_t == S_t
    double r_zero = MarketMakerStrategy::calculate_reservation_price(mid, 0, vol, tau, gamma);
    EXPECT_DOUBLE_EQ(r_zero, mid);

    // 2. Positive inventory => r_t < S_t (desire to sell)
    double r_long = MarketMakerStrategy::calculate_reservation_price(mid, 5, vol, tau, gamma);
    EXPECT_LT(r_long, mid);
    // Exact: 100 - 5 * 0.1 * (0.02^2) * 300 = 100 - 5 * 0.1 * 0.0004 * 300 = 100 - 0.06 = 99.94
    EXPECT_NEAR(r_long, 99.94, 1e-9);

    // 3. Negative inventory => r_t > S_t (desire to buy)
    double r_short = MarketMakerStrategy::calculate_reservation_price(mid, -5, vol, tau, gamma);
    EXPECT_GT(r_short, mid);
    EXPECT_NEAR(r_short, 100.06, 1e-9);

    // 4. Higher gamma increases inventory adjustment
    double r_higher_gamma = MarketMakerStrategy::calculate_reservation_price(mid, 5, vol, tau, 0.2);
    EXPECT_LT(r_higher_gamma, r_long);

    // 5. Higher volatility increases inventory adjustment
    double r_higher_vol = MarketMakerStrategy::calculate_reservation_price(mid, 5, 0.04, tau, gamma);
    EXPECT_LT(r_higher_vol, r_long);

    // 6. Zero remaining horizon removes inventory adjustment
    double r_zero_tau = MarketMakerStrategy::calculate_reservation_price(mid, 5, vol, 0.0, gamma);
    EXPECT_DOUBLE_EQ(r_zero_tau, mid);
}

TEST(StrategyTest, OptimalHalfSpreadAndSmallGammaStability) {
    double vol = 0.02;
    double tau = 300.0;
    double k = 1.5;

    // Standard gamma = 0.1
    double delta_normal = MarketMakerStrategy::calculate_optimal_half_spread(vol, tau, 0.1, k);
    EXPECT_GT(delta_normal, 0.0);

    // Small gamma limit: gamma -> 0
    // (1/gamma) * ln(1 + gamma/k) -> 1/k = 1/1.5 = 0.666667
    double delta_small = MarketMakerStrategy::calculate_optimal_half_spread(vol, tau, 1e-7, k);
    EXPECT_TRUE(std::isfinite(delta_small));
    EXPECT_NEAR(delta_small, 1.0 / k, 1e-4);
}

TEST(StrategyTest, ImbalanceCalculation) {
    OrderBook book;
    book.add_resting_order({1, Side::Buy, 100, 30, OrderType::Limit, TimeInForce::GTC, 100}, 1);
    book.add_resting_order({2, Side::Sell, 102, 10, OrderType::Limit, TimeInForce::GTC, 101}, 2);

    // Bids = 30, Asks = 10 => Imbalance = (30 - 10) / (30 + 10) = 20 / 40 = 0.50
    double imb = MarketMakerStrategy::calculate_imbalance(book, 5);
    EXPECT_DOUBLE_EQ(imb, 0.50);
}

TEST(StrategyTest, InventoryRiskLimitsEnforcement) {
    Config cfg;
    cfg.strategy.max_inventory = 5;
    cfg.strategy.order_quantity = 1;

    MarketMakerStrategy strategy(cfg);
    OrderBook book;
    book.add_resting_order({1, Side::Buy, 100, 10, OrderType::Limit, TimeInForce::GTC, 100}, 1);
    book.add_resting_order({2, Side::Sell, 102, 10, OrderType::Limit, TimeInForce::GTC, 101}, 2);

    // Fill 5 buy orders to reach max inventory limit
    for (int i = 0; i < 5; ++i) {
        ExecutionReport rep;
        rep.order_id = 1'000'000 + i;
        rep.side = Side::Buy;
        rep.last_qty = 1;
        rep.exec_type = ExecutionType::Trade;
        rep.order_status = OrderStatus::Filled;
        strategy.on_execution(rep);
    }
    EXPECT_EQ(strategy.current_inventory(), 5);

    // Now at upper limit: should NOT quote on bid side, only offer ask
    auto actions = strategy.on_market_update(1'000'000'000ULL, book);
    bool has_bid_submit = false;
    for (const auto& act : actions) {
        if (act.type == QuoteAction::Type::Submit && act.order.side == Side::Buy) {
            has_bid_submit = true;
        }
    }
    EXPECT_FALSE(has_bid_submit);
}
