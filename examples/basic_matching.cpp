#include "lob/matching/matching_engine.hpp"
#include <iostream>

using namespace lob;

int main() {
    std::cout << "=== Limit Order Book Basic Matching Example ===" << std::endl;
    MatchingEngine engine;

    engine.set_trade_callback([](const Trade& trade) {
        std::cout << "[TRADE] ID: " << trade.trade_id
                  << " | Price: " << trade.price << " ticks"
                  << " | Qty: " << trade.quantity
                  << " | Maker Order: " << trade.maker_order_id
                  << " | Taker Order: " << trade.taker_order_id << std::endl;
    });

    std::cout << "\n1. Inserting resting sell orders..." << std::endl;
    engine.process_order({1, Side::Sell, 10005, 10, OrderType::Limit, TimeInForce::GTC, 1000});
    engine.process_order({2, Side::Sell, 10002, 15, OrderType::Limit, TimeInForce::GTC, 1001});
    engine.process_order({3, Side::Sell, 10002, 5, OrderType::Limit, TimeInForce::GTC, 1002});

    std::cout << "Best Ask: " << *engine.book().best_ask()
              << " ticks | Qty: " << engine.book().best_ask_qty() << std::endl;

    std::cout << "\n2. Submitting aggressive buy limit order crossing the best ask..." << std::endl;
    // Buy 18 lots at 10003: consumes order 2 (15 lots), then order 3 (3 lots) at 10002!
    auto rep = engine.process_order({4, Side::Buy, 10003, 18, OrderType::Limit, TimeInForce::GTC, 1003});

    std::cout << "Taker Execution Status: " << order_status_to_string(rep.order_status)
              << " | Cumulative Executed: " << rep.cum_qty << std::endl;
    std::cout << "Remaining at Ask 10002: " << engine.book().depth_at_price(Side::Sell, 10002) << " lots" << std::endl;

    return 0;
}
