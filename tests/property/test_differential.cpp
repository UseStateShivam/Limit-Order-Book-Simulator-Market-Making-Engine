#include <gtest/gtest.h>
#include "lob/matching/matching_engine.hpp"
#include "lob/matching/reference_engine.hpp"
#include <random>

using namespace lob;

TEST(DifferentialTest, RandomizedStressDifferentialAgainstReferenceOracle) {
    MatchingEngine opt_engine;
    ReferenceMatchingEngine ref_engine;

    std::vector<Trade> opt_trades;
    opt_engine.set_trade_callback([&](const Trade& t) { opt_trades.push_back(t); });

    std::mt19937_64 rng(1337);
    std::uniform_int_distribution<Price> price_dist(9900, 10100);
    std::uniform_int_distribution<Quantity> qty_dist(1, 20);
    std::uniform_int_distribution<int> side_dist(0, 1);
    std::uniform_int_distribution<int> type_dist(0, 9); // 10% market orders, 90% limit

    constexpr size_t kEventCount = 2000;
    std::vector<OrderId> active_ids;

    for (size_t i = 1; i <= kEventCount; ++i) {
        // Occasionally cancel a resting order (15% chance if orders exist)
        if (!active_ids.empty() && (rng() % 100 < 15)) {
            size_t idx = rng() % active_ids.size();
            OrderId cancel_id = active_ids[idx];
            active_ids.erase(active_ids.begin() + idx);

            opt_engine.process_cancel({cancel_id, i * 1000});
            ref_engine.process_cancel(cancel_id);
            continue;
        }

        OrderRequest req;
        req.order_id = i;
        req.side = (side_dist(rng) == 0) ? Side::Buy : Side::Sell;
        req.quantity = qty_dist(rng);
        req.timestamp = i * 1000;

        bool is_market = (type_dist(rng) == 0);
        if (is_market) {
            req.type = OrderType::Market;
            req.tif = TimeInForce::IOC;
            req.price = 0;
        } else {
            req.type = OrderType::Limit;
            req.tif = TimeInForce::GTC;
            req.price = price_dist(rng);
        }

        // Process in optimized engine
        opt_engine.process_order(req);

        // Process in reference oracle
        auto ref_trades_step = ref_engine.process_order(req);

        active_ids.push_back(req.order_id);
    }

    // Compare final book state
    EXPECT_EQ(opt_engine.book().best_bid(), ref_engine.best_bid());
    EXPECT_EQ(opt_engine.book().best_ask(), ref_engine.best_ask());
    EXPECT_EQ(opt_engine.book().best_bid_qty(), ref_engine.best_bid_qty());
    EXPECT_EQ(opt_engine.book().best_ask_qty(), ref_engine.best_ask_qty());
    EXPECT_EQ(opt_engine.book().total_orders(), ref_engine.total_orders());

    // Verify optimized book invariants
    EXPECT_NO_THROW(opt_engine.verify_invariants());
}
