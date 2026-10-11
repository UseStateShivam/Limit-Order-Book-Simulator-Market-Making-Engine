#include <gtest/gtest.h>
#include "lob/book/order_arena.hpp"

using namespace lob;

TEST(OrderArenaTest, AllocationAndValidity) {
    OrderArena arena(10);
    EXPECT_EQ(arena.size(), 0);

    OrderHandle h1 = arena.allocate_order();
    EXPECT_TRUE(arena.is_valid_handle(h1));
    EXPECT_EQ(arena.size(), 1);

    auto& rec = arena.get(h1);
    rec.order_id = 42;
    rec.price = 1000;
    rec.remaining_qty = 5;

    EXPECT_EQ(arena.get(h1).order_id, 42);
    EXPECT_EQ(arena.get(h1).price, 1000);
}

TEST(OrderArenaTest, FreelistRecyclingAndStaleHandleProtection) {
    OrderArena arena(10);
    OrderHandle h1 = arena.allocate_order();
    EXPECT_EQ(h1.index, 0);
    EXPECT_EQ(h1.generation, 1);

    arena.release_order(h1.index);
    EXPECT_FALSE(arena.is_valid_handle(h1)); // Stale handle must be invalid!
    EXPECT_EQ(arena.size(), 0);

    // Accessing stale handle must throw
    EXPECT_THROW((void)arena.get(h1), LobException);

    // Reallocating should reuse slot 0 with generation 2
    OrderHandle h2 = arena.allocate_order();
    EXPECT_EQ(h2.index, 0);
    EXPECT_EQ(h2.generation, 2);
    EXPECT_TRUE(arena.is_valid_handle(h2));
    EXPECT_FALSE(arena.is_valid_handle(h1));
}

TEST(OrderArenaTest, MultipleAllocationsAndReleases) {
    OrderArena arena(100);
    std::vector<OrderHandle> handles;
    for (int i = 0; i < 50; ++i) {
        handles.push_back(arena.allocate_order());
    }
    EXPECT_EQ(arena.size(), 50);

    // Release even indices
    for (size_t i = 0; i < handles.size(); i += 2) {
        arena.release_order(handles[i].index);
    }
    EXPECT_EQ(arena.size(), 25);

    // Odd indices remain valid
    for (size_t i = 1; i < handles.size(); i += 2) {
        EXPECT_TRUE(arena.is_valid_handle(handles[i]));
    }
}
