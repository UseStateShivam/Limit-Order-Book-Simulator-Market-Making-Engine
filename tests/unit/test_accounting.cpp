#include <gtest/gtest.h>
#include "lob/analytics/accounting.hpp"

using namespace lob;

TEST(AccountingTest, BuyAndSellPositionLifecycle) {
    PortfolioAccounting acc(100'000.0, 0.0, 0.0); // No fees for pure trade check

    // 1. Buy 10 lots at 100.00
    Trade t1{1, 1, 1000, 99, 100, Side::Buy, 10000, 10}; // Price 100.00, qty 10
    acc.on_trade(t1, false, 0.01, 1.0, 100.00);

    EXPECT_DOUBLE_EQ(acc.cash(), 99'000.0); // 100000 - 1000
    EXPECT_EQ(acc.inventory(), 10);
    EXPECT_DOUBLE_EQ(acc.current_equity(), 100'000.0);
    EXPECT_DOUBLE_EQ(acc.realized_pnl(), 0.0);
    EXPECT_DOUBLE_EQ(acc.unrealized_pnl(), 0.0);

    // 2. Market price rises to 105.00
    acc.record_equity_point(2000, 105.00, 1.0);
    EXPECT_DOUBLE_EQ(acc.current_equity(), 100'050.0);
    EXPECT_DOUBLE_EQ(acc.unrealized_pnl(), 50.0);

    // 3. Sell 5 lots at 105.00 (closing half of position)
    Trade t2{2, 2, 2000, 99, 101, Side::Sell, 10500, 5};
    acc.on_trade(t2, false, 0.01, 1.0, 105.00);

    EXPECT_DOUBLE_EQ(acc.cash(), 99'525.0); // 99000 + 525
    EXPECT_EQ(acc.inventory(), 5);
    EXPECT_DOUBLE_EQ(acc.realized_pnl(), 25.0); // 5 * (105 - 100) = 25
    EXPECT_DOUBLE_EQ(acc.unrealized_pnl(), 25.0); // remaining 5 * (105 - 100) = 25
    EXPECT_DOUBLE_EQ(acc.current_equity(), 100'050.0);
}

TEST(AccountingTest, FeesAndRebates) {
    // Maker gets -1 bps rebate (-0.0001), Taker pays +2 bps fee (+0.0002)
    PortfolioAccounting acc(100'000.0, -0.0001, 0.0002);

    // Trade where we are maker: sell 10 lots at 100.00 (notional = 1000.0)
    // Maker fee = 1000 * -0.0001 = -0.10 => cash increases by 0.10 rebate!
    Trade t1{1, 1, 1000, 100, 99, Side::Buy, 10000, 10};
    acc.on_trade(t1, true, 0.01, 1.0, 100.0);

    // cash = 100000 + 1000 (sale) - (-0.10 fee) = 101000.10
    EXPECT_NEAR(acc.cash(), 101000.10, 1e-4);
    EXPECT_NEAR(acc.cumulative_fees(), -0.10, 1e-4);
}

TEST(AccountingTest, MaximumDrawdown) {
    PortfolioAccounting acc(100.0, 0.0, 0.0);
    // Buy 1 lot at 100.00 => cash = 0, inventory = 1
    Trade t{1, 1, 1, 1, 2, Side::Buy, 10000, 1};
    acc.on_trade(t, false, 0.01, 1.0, 100.0);

    acc.record_equity_point(2, 120.0, 1.0); // peak = 120
    acc.record_equity_point(3, 90.0, 1.0);  // trough = 90 => dd = (90 - 120) / 120 = -30 / 120 = -0.25 (-25%)
    acc.record_equity_point(4, 110.0, 1.0);

    EXPECT_NEAR(acc.max_drawdown_pct(), -0.25, 1e-4);
    EXPECT_DOUBLE_EQ(acc.max_drawdown_abs(), 30.0);
}
