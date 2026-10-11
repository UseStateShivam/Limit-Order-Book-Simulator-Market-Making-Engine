#include <benchmark/benchmark.h>
#include "lob/book/order_book.hpp"

using namespace lob;

static void BM_OrderArenaAllocationAndRelease(benchmark::State& state) {
    OrderArena arena(100'000);

    for (auto _ : state) {
        OrderHandle h = arena.allocate_order();
        arena.release_order(h.index);
        benchmark::DoNotOptimize(h);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_OrderArenaAllocationAndRelease);

static void BM_OrderBookL2Snapshot(benchmark::State& state) {
    OrderBook book;
    for (int i = 1; i <= 50; ++i) {
        book.add_resting_order({static_cast<OrderId>(i), Side::Buy, 10000 - i, 10, OrderType::Limit, TimeInForce::GTC, 1000}, i);
        book.add_resting_order({static_cast<OrderId>(100 + i), Side::Sell, 10000 + i, 10, OrderType::Limit, TimeInForce::GTC, 1000}, 100 + i);
    }

    for (auto _ : state) {
        auto snap = book.get_l2_snapshot(10);
        benchmark::DoNotOptimize(snap);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_OrderBookL2Snapshot);

static void BM_OrderBookL3Snapshot(benchmark::State& state) {
    OrderBook book;
    for (int i = 1; i <= 50; ++i) {
        book.add_resting_order({static_cast<OrderId>(i), Side::Buy, 10000 - i, 10, OrderType::Limit, TimeInForce::GTC, 1000}, i);
        book.add_resting_order({static_cast<OrderId>(100 + i), Side::Sell, 10000 + i, 10, OrderType::Limit, TimeInForce::GTC, 1000}, 100 + i);
    }

    for (auto _ : state) {
        auto snap = book.get_l3_snapshot(10);
        benchmark::DoNotOptimize(snap);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_OrderBookL3Snapshot);

static void BM_OrderBookSpreadAndMidQuery(benchmark::State& state) {
    OrderBook book;
    book.add_resting_order({1, Side::Buy, 10000, 10, OrderType::Limit, TimeInForce::GTC, 1000}, 1);
    book.add_resting_order({2, Side::Sell, 10002, 10, OrderType::Limit, TimeInForce::GTC, 1000}, 2);

    for (auto _ : state) {
        auto mid = book.mid_price();
        auto spread = book.spread_ticks();
        benchmark::DoNotOptimize(mid);
        benchmark::DoNotOptimize(spread);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_OrderBookSpreadAndMidQuery);
