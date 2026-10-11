#include <benchmark/benchmark.h>
#include "lob/simulation/simulator.hpp"
#include "lob/market_data/synthetic_generator.hpp"

using namespace lob;

static void BM_EndToEndSimulationThroughput(benchmark::State& state) {
    Config cfg = Config::default_config();
    cfg.simulation.event_count = 1000;
    cfg.simulation.output_dir = ""; // Disable disk I/O in benchmark

    SyntheticGenerator generator(cfg);
    auto events = generator.generate_events(1000);

    for (auto _ : state) {
        Simulator sim(cfg);
        auto summary = sim.run_with_events(events);
        benchmark::DoNotOptimize(summary);
    }
    state.SetItemsProcessed(state.iterations() * events.size());
}
BENCHMARK(BM_EndToEndSimulationThroughput);
