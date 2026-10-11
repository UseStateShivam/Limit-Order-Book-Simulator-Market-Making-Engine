#include "lob/market_data/synthetic_generator.hpp"
#include "lob/matching/matching_engine.hpp"
#include <iostream>

using namespace lob;

int main() {
    std::cout << "=== Synthetic Market Generator Example ===" << std::endl;
    Config cfg = Config::default_config();
    cfg.simulation.scenario = "high_volatility";

    SyntheticGenerator generator(cfg);
    auto events = generator.generate_events(100);

    std::cout << "Generated " << events.size() << " synthetic market events for scenario: "
              << cfg.simulation.scenario << std::endl;

    MatchingEngine engine;
    size_t trade_count = 0;
    engine.set_trade_callback([&](const Trade&) { trade_count++; });

    for (const auto& ev : events) {
        engine.process_order(ev);
    }

    std::cout << "Processed events through engine. Trades generated: " << trade_count << std::endl;
    if (engine.book().best_bid() && engine.book().best_ask()) {
        std::cout << "Final Best Bid: " << *engine.book().best_bid()
                  << " | Best Ask: " << *engine.book().best_ask()
                  << " | Spread: " << *engine.book().spread_ticks() << " ticks" << std::endl;
    }
    return 0;
}
