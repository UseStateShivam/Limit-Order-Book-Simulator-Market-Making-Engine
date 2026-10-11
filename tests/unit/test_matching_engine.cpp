#include <gtest/gtest.h>
#include "lob/matching/matching_engine.hpp"

using namespace lob;

TEST(MatchingEngineTest, ExactPriceMatchAndFIFOExecution) {
    MatchingEngine engine;
    std::vector<Trade> trades;
    engine.set_trade_callback([&](const Trade& t) { trades.push_back(t); });

    // Place two resting sell orders at price 100
    // Order 1: qty 10, Order 2: qty 20
    engine.process_order({1, Side::Sell, 100, 10, OrderType::Limit, TimeInForce::GTC, 1000});
    engine.process_order({2, Side::Sell, 100, 20, OrderType::Limit, TimeInForce::GTC, 1001});

    // Incoming buy for 15 at price 100:
    // Should fully fill Order 1 (10 qty) and partially fill Order 2 (5 qty)
    auto rep = engine.process_order({3, Side::Buy, 100, 15, OrderType::Limit, TimeInForce::GTC, 1002});

    EXPECT_EQ(rep.order_status, OrderStatus::Filled);
    EXPECT_EQ(rep.cum_qty, 15);
    ASSERT_EQ(trades.size(), 2);

    // Trade 1 with Order 1 (FIFO head)
    EXPECT_EQ(trades[0].maker_order_id, 1);
    EXPECT_EQ(trades[0].taker_order_id, 3);
    EXPECT_EQ(trades[0].price, 100);
    EXPECT_EQ(trades[0].quantity, 10);

    // Trade 2 with Order 2
    EXPECT_EQ(trades[1].maker_order_id, 2);
    EXPECT_EQ(trades[1].taker_order_id, 3);
    EXPECT_EQ(trades[1].price, 100);
    EXPECT_EQ(trades[1].quantity, 5);

    // Remaining in book: Order 2 with 15 qty
    EXPECT_EQ(engine.book().best_ask().value(), 100);
    EXPECT_EQ(engine.book().best_ask_qty(), 15);
    EXPECT_NO_THROW(engine.verify_invariants());
}

TEST(MatchingEngineTest, BetterPricePriorityOverMultipleLevels) {
    MatchingEngine engine;
    std::vector<Trade> trades;
    engine.set_trade_callback([&](const Trade& t) { trades.push_back(t); });

    // Place sells at 102, 101, 103 (submitted out of price order)
    engine.process_order({1, Side::Sell, 102, 10, OrderType::Limit, TimeInForce::GTC, 100});
    engine.process_order({2, Side::Sell, 101, 10, OrderType::Limit, TimeInForce::GTC, 101});
    engine.process_order({3, Side::Sell, 103, 10, OrderType::Limit, TimeInForce::GTC, 102});

    // Aggressive buy at 105 for 15 lots
    engine.process_order({4, Side::Buy, 105, 15, OrderType::Limit, TimeInForce::GTC, 103});

    ASSERT_EQ(trades.size(), 2);
    // Best ask (101) must execute first!
    EXPECT_EQ(trades[0].maker_order_id, 2);
    EXPECT_EQ(trades[0].price, 101);
    EXPECT_EQ(trades[0].quantity, 10);

    // Next best ask (102) executes second for remaining 5
    EXPECT_EQ(trades[1].maker_order_id, 1);
    EXPECT_EQ(trades[1].price, 102);
    EXPECT_EQ(trades[1].quantity, 5);

    // Book should now have 5 lots resting at 102 and 10 lots at 103
    EXPECT_EQ(engine.book().best_ask().value(), 102);
    EXPECT_EQ(engine.book().best_ask_qty(), 5);
    EXPECT_NO_THROW(engine.verify_invariants());
}

TEST(MatchingEngineTest, MarketOrderNeverRests) {
    MatchingEngine engine;
    engine.process_order({1, Side::Sell, 100, 5, OrderType::Limit, TimeInForce::GTC, 100});

    // Market buy for 10: fills 5 available, remaining 5 canceled (never rests)
    auto rep = engine.process_order({2, Side::Buy, 0, 10, OrderType::Market, TimeInForce::IOC, 101});

    EXPECT_EQ(rep.order_status, OrderStatus::PartiallyFilled);
    EXPECT_EQ(rep.cum_qty, 5);
    EXPECT_EQ(rep.leaves_qty, 0); // Unfilled remainder canceled
    EXPECT_FALSE(engine.book().best_bid().has_value());
    EXPECT_FALSE(engine.book().best_ask().has_value());
}

TEST(MatchingEngineTest, FillOrKillSemantics) {
    MatchingEngine engine;
    engine.process_order({1, Side::Sell, 100, 10, OrderType::Limit, TimeInForce::GTC, 100});

    // FOK buy for 15: only 10 available -> must be rejected completely with 0 trades!
    auto rep = engine.process_order({2, Side::Buy, 100, 15, OrderType::Limit, TimeInForce::FOK, 101});
    EXPECT_EQ(rep.order_status, OrderStatus::Rejected);
    EXPECT_EQ(rep.reject_reason, RejectReason::InsufficientLiquidity);
    EXPECT_EQ(engine.book().best_ask_qty(), 10); // Resting liquidity untouched

    // FOK buy for 10: exactly matches -> executed fully!
    auto rep2 = engine.process_order({3, Side::Buy, 100, 10, OrderType::Limit, TimeInForce::FOK, 102});
    EXPECT_EQ(rep2.order_status, OrderStatus::Filled);
    EXPECT_EQ(rep2.cum_qty, 10);
    EXPECT_FALSE(engine.book().best_ask().has_value());
}

TEST(MatchingEngineTest, CancellationAndRejection) {
    MatchingEngine engine;
    engine.process_order({1, Side::Buy, 100, 10, OrderType::Limit, TimeInForce::GTC, 100});

    // Cancel active order
    auto rep = engine.process_cancel({1, 101});
    EXPECT_EQ(rep.order_status, OrderStatus::Canceled);
    EXPECT_FALSE(engine.book().contains_order(1));

    // Cancel nonexistent order
    auto rep2 = engine.process_cancel({999, 102});
    EXPECT_EQ(rep2.order_status, OrderStatus::Rejected);
    EXPECT_EQ(rep2.reject_reason, RejectReason::OrderNotFound);

    // Duplicate order ID rejected
    engine.process_order({2, Side::Buy, 100, 10, OrderType::Limit, TimeInForce::GTC, 103});
    auto rep3 = engine.process_order({2, Side::Buy, 100, 10, OrderType::Limit, TimeInForce::GTC, 104});
    EXPECT_EQ(rep3.order_status, OrderStatus::Rejected);
    EXPECT_EQ(rep3.reject_reason, RejectReason::DuplicateOrderId);
}
