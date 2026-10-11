#include "lob/market_data/synthetic_generator.hpp"
#include <cmath>
#include <algorithm>

namespace lob {

SyntheticGenerator::SyntheticGenerator(const Config& config)
    : config_(config),
      scenario_(config.simulation.scenario),
      seed_(config.simulation.random_seed),
      rng_(config.simulation.random_seed) {}

std::vector<OrderRequest> SyntheticGenerator::generate_events(size_t event_count) {
    std::vector<OrderRequest> events;
    events.reserve(event_count);

    // Initial reference mid-price in ticks
    Price mid_price = price_to_ticks(100.0, config_.instrument.tick_size); // 100.00 base price
    Timestamp current_time = 1'000'000'000ULL; // 1 second in nanoseconds
    OrderId next_order_id = 100'000;

    // Distributions
    std::uniform_real_distribution<double> uniform_dist(0.0, 1.0);
    std::normal_distribution<double> normal_dist(0.0, 1.0);

    // Scenario parameters
    double vol_step = 1.0;          // Price volatility diffusion in ticks
    Price half_spread = 2;          // Base half-spread in ticks
    double market_order_prob = 0.25;// Probability an event is a market order vs limit order
    double bid_skew_prob = 0.5;     // Probability of buy vs sell
    double price_drift = 0.0;       // Directional drift

    if (scenario_ == "high_volatility") {
        vol_step = 4.0;
        half_spread = 4;
        market_order_prob = 0.35;
    } else if (scenario_ == "liquidity_imbalance") {
        vol_step = 1.2;
        bid_skew_prob = 0.70; // 70% buys (buy pressure)
    } else if (scenario_ == "sparse_liquidity") {
        vol_step = 2.0;
        half_spread = 10; // Wide spread
        market_order_prob = 0.40;
    } else if (scenario_ == "toxic_flow") {
        vol_step = 2.5;
        market_order_prob = 0.45;
        // In toxic flow, taker trades precede price jumps
    } else if (scenario_ == "inventory_stress") {
        vol_step = 1.5;
        price_drift = 0.08; // Steady drift up in ticks per event
        bid_skew_prob = 0.75;
    }

    // Pre-populate initial book with 20 resting orders on each side
    for (int i = 1; i <= 20; ++i) {
        // Bid level
        OrderRequest bid_req;
        bid_req.order_id = ++next_order_id;
        bid_req.side = Side::Buy;
        bid_req.price = mid_price - half_spread - (i - 1);
        bid_req.quantity = 5 + (rng_() % 10);
        bid_req.type = OrderType::Limit;
        bid_req.tif = TimeInForce::GTC;
        bid_req.timestamp = current_time;
        events.push_back(bid_req);

        // Ask level
        OrderRequest ask_req;
        ask_req.order_id = ++next_order_id;
        ask_req.side = Side::Sell;
        ask_req.price = mid_price + half_spread + (i - 1);
        ask_req.quantity = 5 + (rng_() % 10);
        ask_req.type = OrderType::Limit;
        ask_req.tif = TimeInForce::GTC;
        ask_req.timestamp = current_time;
        events.push_back(ask_req);
    }

    // Main generation loop
    while (events.size() < event_count) {
        // Advance time: exponential arrival with mean 50 microseconds (50,000 ns)
        double u = uniform_dist(rng_);
        if (u <= 0.0) u = 0.0001;
        uint64_t dt_ns = static_cast<uint64_t>(-std::log(u) * 50'000.0);
        if (dt_ns < 1000) dt_ns = 1000;
        current_time += dt_ns;

        // Update reference price
        double d_price = normal_dist(rng_) * vol_step + price_drift;
        mid_price += static_cast<Price>(std::round(d_price));
        if (mid_price < 100) mid_price = 100; // Floor

        bool is_market = (uniform_dist(rng_) < market_order_prob);
        bool is_buy = (uniform_dist(rng_) < bid_skew_prob);

        if (scenario_ == "toxic_flow") {
            // When toxic taker buy occurs, mid-price jumps upward immediately afterwards
            if (is_market) {
                if (is_buy) mid_price += 1 + (rng_() % 3);
                else mid_price -= 1 + (rng_() % 3);
            }
        }

        OrderRequest req;
        req.order_id = ++next_order_id;
        req.side = is_buy ? Side::Buy : Side::Sell;
        req.timestamp = current_time;

        if (is_market) {
            req.type = OrderType::Market;
            req.tif = TimeInForce::IOC;
            req.price = 0; // Market order has price 0
            req.quantity = 1 + (rng_() % 4); // Small market order
        } else {
            req.type = OrderType::Limit;
            req.tif = TimeInForce::GTC;
            // Generate resting limit order near inside market
            Price offset = half_spread + static_cast<Price>(rng_() % 5);
            req.price = is_buy ? (mid_price - offset) : (mid_price + offset);
            req.quantity = 2 + (rng_() % 10);
        }

        events.push_back(req);
    }

    return events;
}

} // namespace lob
