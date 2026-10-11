#include <gtest/gtest.h>
#include "lob/book/order_book.hpp"

using namespace lob;

TEST(OrderBookTest, EmptyBookQueries) {
    OrderBook book;
    EXPECT_FALSE(book.best_bid().has_value());
    EXPECT_FALSE(book.best_ask().has_value());
    EXPECT_FALSE(book.mid_price().has_value());
    EXPECT_FALSE(book.spread_ticks().has_value());
    EXPECT_EQ(book.total_orders(), 0);
    EXPECT_NO_THROW(book.verify_invariants());
}

TEST(OrderBookTest, InsertBidsAndAsks) {
    OrderBook book;

    OrderRequest b1{1, Side::Buy, 10000, 10, OrderType::Limit, TimeInForce::GTC, 1000};
    OrderRequest b2{2, Side::Buy, 9999, 15, OrderType::Limit, TimeInForce::GTC, 1001};
    OrderRequest a1{3, Side::Sell, 10002, 20, OrderType::Limit, TimeInForce::GTC, 1002};
    OrderRequest a2{4, Side::Sell, 10005, 25, OrderType::Limit, TimeInForce::GTC, 1003};

    book.add_resting_order(b1, 1);
    book.add_resting_order(b2, 2);
    book.add_resting_order(a1, 3);
    book.add_resting_order(a2, 4);

    EXPECT_EQ(book.best_bid().value(), 10000);
    EXPECT_EQ(book.best_ask().value(), 10002);
    EXPECT_EQ(book.best_bid_qty(), 10);
    EXPECT_EQ(book.best_ask_qty(), 20);
    EXPECT_DOUBLE_EQ(book.mid_price().value(), 10001.0);
    EXPECT_EQ(book.spread_ticks().value(), 2);
    EXPECT_EQ(book.total_orders(), 4);

    EXPECT_NO_THROW(book.verify_invariants());
}

TEST(OrderBookTest, L2AndL3Snapshots) {
    OrderBook book;
    // Add 2 orders at same bid price 10000
    book.add_resting_order({1, Side::Buy, 10000, 10, OrderType::Limit, TimeInForce::GTC, 100}, 1);
    book.add_resting_order({2, Side::Buy, 10000, 20, OrderType::Limit, TimeInForce::GTC, 101}, 2);
    book.add_resting_order({3, Side::Sell, 10002, 15, OrderType::Limit, TimeInForce::GTC, 102}, 3);

    auto l2 = book.get_l2_snapshot(5);
    ASSERT_EQ(l2.bids.size(), 1);
    EXPECT_EQ(l2.bids[0].price, 10000);
    EXPECT_EQ(l2.bids[0].quantity, 30);
    EXPECT_EQ(l2.bids[0].order_count, 2);

    auto l3 = book.get_l3_snapshot(5);
    ASSERT_EQ(l3.bids.size(), 2);
    // Queue order must be FIFO: order 1 first, order 2 second
    EXPECT_EQ(l3.bids[0].order_id, 1);
    EXPECT_EQ(l3.bids[0].queue_position, 1);
    EXPECT_EQ(l3.bids[1].order_id, 2);
    EXPECT_EQ(l3.bids[1].queue_position, 2);

    EXPECT_NO_THROW(book.verify_invariants());
}

TEST(OrderBookTest, RemoveOrderAndEmptyLevelPruning) {
    OrderBook book;
    book.add_resting_order({1, Side::Buy, 10000, 10, OrderType::Limit, TimeInForce::GTC, 100}, 1);
    EXPECT_EQ(book.bid_level_count(), 1);

    OrderRecord removed = book.remove_order(1);
    EXPECT_EQ(removed.order_id, 1);
    EXPECT_EQ(book.total_orders(), 0);
    EXPECT_EQ(book.bid_level_count(), 0); // Empty level must be pruned!
    EXPECT_FALSE(book.best_bid().has_value());
    EXPECT_NO_THROW(book.verify_invariants());
}

TEST(OrderBookTest, DownwardModificationPreservesPriority) {
    OrderBook book;
    book.add_resting_order({1, Side::Buy, 10000, 10, OrderType::Limit, TimeInForce::GTC, 100}, 1);
    book.add_resting_order({2, Side::Buy, 10000, 20, OrderType::Limit, TimeInForce::GTC, 101}, 2);

    // Reduce order 1 qty from 10 to 5
    bool ok = book.modify_order_quantity(1, 5);
    EXPECT_TRUE(ok);
    EXPECT_EQ(book.depth_at_price(Side::Buy, 10000), 25);

    auto l3 = book.get_l3_snapshot(5);
    // Order 1 must STILL be head of queue (priority preserved)
    EXPECT_EQ(l3.bids[0].order_id, 1);
    EXPECT_EQ(l3.bids[0].quantity, 5);
    EXPECT_EQ(l3.bids[1].order_id, 2);

    EXPECT_NO_THROW(book.verify_invariants());
}

TEST(OrderBookTest, UpwardModificationLosesPriority) {
    OrderBook book;
    book.add_resting_order({1, Side::Buy, 10000, 10, OrderType::Limit, TimeInForce::GTC, 100}, 1);
    book.add_resting_order({2, Side::Buy, 10000, 20, OrderType::Limit, TimeInForce::GTC, 101}, 2);

    // Increase order 1 qty from 10 to 15: loses time priority!
    bool ok = book.modify_order_quantity(1, 15);
    EXPECT_TRUE(ok);
    EXPECT_EQ(book.depth_at_price(Side::Buy, 10000), 35);

    auto l3 = book.get_l3_snapshot(5);
    // Order 2 must now be at HEAD, Order 1 moved to TAIL
    EXPECT_EQ(l3.bids[0].order_id, 2);
    EXPECT_EQ(l3.bids[1].order_id, 1);
    EXPECT_EQ(l3.bids[1].quantity, 15);

    EXPECT_NO_THROW(book.verify_invariants());
}
