#include <benchmark/benchmark.h>
#include "lob/matching/matching_engine.hpp"

using namespace lob;

static void BM_LimitOrderInsertNonCrossing(benchmark::State& state) {
    MatchingConfig match_cfg;
    match_cfg.enable_invariant_checks = false; // Isolated benchmark of core engine matching path
    MatchingEngine engine(InstrumentConfig{}, match_cfg);
    OrderId id = 1;

    for (auto _ : state) {
        OrderRequest req;
        req.order_id = ++id;
        req.side = Side::Buy;
        req.price = 10000 - static_cast<Price>(id % 100);
        req.quantity = 10;
        req.type = OrderType::Limit;
        req.tif = TimeInForce::GTC;
        req.timestamp = id * 1000;

        auto rep = engine.process_order(req);
        benchmark::DoNotOptimize(rep);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LimitOrderInsertNonCrossing);

static void BM_OrderCancellation(benchmark::State& state) {
    MatchingConfig match_cfg;
    match_cfg.enable_invariant_checks = false;
    MatchingEngine engine(InstrumentConfig{}, match_cfg);

    OrderId id = 1;
    for (auto _ : state) {
        state.PauseTiming();
        OrderRequest req;
        req.order_id = ++id;
        req.side = Side::Buy;
        req.price = 10000 - static_cast<Price>(id % 50);
        req.quantity = 10;
        req.type = OrderType::Limit;
        req.tif = TimeInForce::GTC;
        req.timestamp = id * 1000;
        engine.process_order(req);
        state.ResumeTiming();

        auto rep = engine.process_cancel({id, id * 1000});
        benchmark::DoNotOptimize(rep);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_OrderCancellation);

static void BM_MarketOrderMatchingSingleLevel(benchmark::State& state) {
    MatchingConfig match_cfg;
    match_cfg.enable_invariant_checks = false;
    MatchingEngine engine(InstrumentConfig{}, match_cfg);
    OrderId maker_id = 1;
    OrderId taker_id = 10'000'000;

    for (auto _ : state) {
        state.PauseTiming();
        if (engine.book().asks().empty() || engine.book().best_ask_qty() < 10) {
            engine.process_order({++maker_id, Side::Sell, 10000, 1000, OrderType::Limit, TimeInForce::GTC, 1000});
        }
        state.ResumeTiming();

        OrderRequest req;
        req.order_id = ++taker_id;
        req.side = Side::Buy;
        req.price = 0;
        req.quantity = 1;
        req.type = OrderType::Market;
        req.tif = TimeInForce::IOC;
        req.timestamp = taker_id * 1000;

        auto rep = engine.process_order(req);
        benchmark::DoNotOptimize(rep);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MarketOrderMatchingSingleLevel);

static void BM_MarketOrderMatchingMultiLevel(benchmark::State& state) {
    MatchingConfig match_cfg;
    match_cfg.enable_invariant_checks = false;
    MatchingEngine engine(InstrumentConfig{}, match_cfg);
    OrderId maker_id = 1;
    OrderId taker_id = 10'000'000;

    for (auto _ : state) {
        state.PauseTiming();
        if (engine.book().ask_level_count() < 5) {
            for (int lvl = 1; lvl <= 5; ++lvl) {
                engine.process_order({++maker_id, Side::Sell, 10000 + lvl, 2, OrderType::Limit, TimeInForce::GTC, 1000});
            }
        }
        state.ResumeTiming();

        OrderRequest req;
        req.order_id = ++taker_id;
        req.side = Side::Buy;
        req.price = 0;
        req.quantity = 6; // Sweeps across 3 levels
        req.type = OrderType::Market;
        req.tif = TimeInForce::IOC;
        req.timestamp = taker_id * 1000;

        auto rep = engine.process_order(req);
        benchmark::DoNotOptimize(rep);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MarketOrderMatchingMultiLevel);

static void BM_MixedWorkload(benchmark::State& state) {
    MatchingConfig match_cfg;
    match_cfg.enable_invariant_checks = false;
    MatchingEngine engine(InstrumentConfig{}, match_cfg);
    OrderId id = 1;

    for (auto _ : state) {
        size_t mod = id % 10;
        if (mod < 5) {
            OrderRequest req{++id, Side::Buy, 9900 + static_cast<Price>(id % 50), 10, OrderType::Limit, TimeInForce::GTC, id * 1000};
            auto rep = engine.process_order(req);
            benchmark::DoNotOptimize(rep);
        } else if (mod < 8) {
            OrderId target = (id > 10) ? (id - 5) : id;
            auto rep = engine.process_cancel({target, id * 1000});
            benchmark::DoNotOptimize(rep);
            ++id;
        } else {
            OrderRequest req{++id, Side::Sell, 0, 5, OrderType::Market, TimeInForce::IOC, id * 1000};
            auto rep = engine.process_order(req);
            benchmark::DoNotOptimize(rep);
        }
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MixedWorkload);
