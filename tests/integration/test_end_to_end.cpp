#include <gtest/gtest.h>
#include "lob/simulation/simulator.hpp"
#include <filesystem>

using namespace lob;

TEST(IntegrationTest, DeterministicSimulationReproducibility) {
    Config cfg1 = Config::default_config();
    cfg1.simulation.random_seed = 12345;
    cfg1.simulation.event_count = 1000;
    cfg1.simulation.output_dir = "results/test_run_1";

    Config cfg2 = Config::default_config();
    cfg2.simulation.random_seed = 12345;
    cfg2.simulation.event_count = 1000;
    cfg2.simulation.output_dir = "results/test_run_2";

    Simulator sim1(cfg1);
    auto summary1 = sim1.run();

    Simulator sim2(cfg2);
    auto summary2 = sim2.run();

    // 1. Summaries must match bit-for-bit
    EXPECT_DOUBLE_EQ(summary1.ending_equity, summary2.ending_equity);
    EXPECT_DOUBLE_EQ(summary1.net_pnl, summary2.net_pnl);
    EXPECT_DOUBLE_EQ(summary1.gross_pnl, summary2.gross_pnl);
    EXPECT_EQ(summary1.orders_submitted, summary2.orders_submitted);
    EXPECT_EQ(summary1.orders_canceled, summary2.orders_canceled);
    EXPECT_EQ(summary1.completed_fills, summary2.completed_fills);
    EXPECT_EQ(summary1.partial_fills, summary2.partial_fills);
    EXPECT_EQ(summary1.executed_volume, summary2.executed_volume);
    EXPECT_DOUBLE_EQ(summary1.max_drawdown_pct, summary2.max_drawdown_pct);

    // 2. Trades list must be identical
    ASSERT_EQ(sim1.trades().size(), sim2.trades().size());
    for (size_t i = 0; i < sim1.trades().size(); ++i) {
        const auto& t1 = sim1.trades()[i];
        const auto& t2 = sim2.trades()[i];
        EXPECT_EQ(t1.price, t2.price);
        EXPECT_EQ(t1.quantity, t2.quantity);
        EXPECT_EQ(t1.maker_order_id, t2.maker_order_id);
        EXPECT_EQ(t1.taker_order_id, t2.taker_order_id);
    }

    // 3. Artifact files exist
    EXPECT_TRUE(std::filesystem::exists("results/test_run_1/summary.json"));
    EXPECT_TRUE(std::filesystem::exists("results/test_run_1/report.md"));
    EXPECT_TRUE(std::filesystem::exists("results/test_run_1/trades.csv"));
    EXPECT_TRUE(std::filesystem::exists("results/test_run_1/equity_curve.csv"));
    EXPECT_TRUE(std::filesystem::exists("results/test_run_1/strategy_decisions.csv"));
}
