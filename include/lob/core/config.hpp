#pragma once

#include "lob/core/types.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace lob {

struct InstrumentConfig {
    std::string symbol{"BTC-USDT"};
    double tick_size{0.01};
    double lot_size{1.0};
    double min_price{0.01};
    double max_price{1000000.0};
    Quantity min_quantity{1};
    Quantity max_quantity{1000000};
};

struct MatchingConfig {
    size_t arena_initial_capacity{100000};
    bool enable_invariant_checks{true};
    bool allow_self_crossing{false};
};

struct StrategyConfig {
    std::string type{"as_baseline"}; // "as_baseline", "as_liquidity_aware", "fixed_spread"
    double gamma{0.1};               // Risk aversion parameter
    double k{1.5};                   // Order arrival decay parameter
    double A{140.0};                 // Base order arrival intensity
    double horizon_seconds{600.0};   // Total trading horizon (T)
    size_t volatility_window{50};    // Realized volatility observation window
    double fixed_volatility{0.0};    // If > 0, overrides estimated volatility
    Quantity order_quantity{1};      // Order quantity per quote
    Quantity max_inventory{10};      // Symmetric limit [-max_inventory, +max_inventory]
    Price quote_update_threshold_ticks{1}; // Minimum ticks price must move to trigger requote
    double liquidity_skew_coeff{1.0};      // Multiplier for top-of-book imbalance
    Price fixed_half_spread_ticks{2};      // Used by fixed_spread strategy
};

struct SimulationConfig {
    double initial_cash{100000.0};
    double maker_fee_rate{-0.0001};  // -1 bps rebate
    double taker_fee_rate{0.0002};   // +2 bps fee
    uint64_t data_latency_ns{0};     // Market data latency
    uint64_t order_latency_ns{0};    // Order routing latency
    uint64_t random_seed{42};
    size_t event_count{10000};
    std::string scenario{"stable"};  // "stable", "high_volatility", "liquidity_imbalance", "sparse_liquidity", "toxic_flow", "inventory_stress"
    std::string output_dir{"results/default"};
    std::string input_file{""};
    std::string replay_mode{"exact"}; // "exact", "reference", "reconstruction"
};

struct Config {
    InstrumentConfig instrument;
    MatchingConfig matching;
    StrategyConfig strategy;
    SimulationConfig simulation;

    static Config load_from_file(const std::string& filepath);
    static Config default_config();
    void save_to_file(const std::string& filepath) const;
};

} // namespace lob
