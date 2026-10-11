#include "lob/core/config.hpp"
#include "lob/core/error.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>

namespace lob {

namespace {

std::string trim(std::string_view s) {
    const auto start = s.find_first_not_of(" \t\r\n\"'");
    if (start == std::string_view::npos) return "";
    const auto end = s.find_last_not_of(" \t\r\n\"'");
    return std::string(s.substr(start, end - start + 1));
}

} // namespace

Config Config::default_config() {
    return Config{};
}

Config Config::load_from_file(const std::string& filepath) {
    Config cfg = default_config();
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw LobException(ErrorCode::ConfigError, "Unable to open config file: " + filepath);
    }

    std::string line;
    std::string current_section = "";

    while (std::getline(file, line)) {
        // Strip comments
        auto comment_pos = line.find('#');
        if (comment_pos != std::string::npos) {
            line = line.substr(0, comment_pos);
        }

        std::string trimmed = trim(line);
        if (trimmed.empty()) continue;

        // Check if section header (e.g., "instrument:")
        if (trimmed.back() == ':' && trimmed.find(':') == trimmed.length() - 1) {
            current_section = trim(trimmed.substr(0, trimmed.length() - 1));
            continue;
        }

        auto colon_pos = trimmed.find(':');
        if (colon_pos == std::string::npos) continue;

        std::string key = trim(trimmed.substr(0, colon_pos));
        std::string val = trim(trimmed.substr(colon_pos + 1));

        if (current_section == "instrument") {
            if (key == "symbol") cfg.instrument.symbol = val;
            else if (key == "tick_size") cfg.instrument.tick_size = std::stod(val);
            else if (key == "lot_size") cfg.instrument.lot_size = std::stod(val);
            else if (key == "min_price") cfg.instrument.min_price = std::stod(val);
            else if (key == "max_price") cfg.instrument.max_price = std::stod(val);
            else if (key == "min_quantity") cfg.instrument.min_quantity = std::stoll(val);
            else if (key == "max_quantity") cfg.instrument.max_quantity = std::stoll(val);
        } else if (current_section == "matching") {
            if (key == "arena_initial_capacity") cfg.matching.arena_initial_capacity = std::stoull(val);
            else if (key == "enable_invariant_checks") cfg.matching.enable_invariant_checks = (val == "true" || val == "1");
            else if (key == "allow_self_crossing") cfg.matching.allow_self_crossing = (val == "true" || val == "1");
        } else if (current_section == "strategy") {
            if (key == "type") cfg.strategy.type = val;
            else if (key == "gamma") cfg.strategy.gamma = std::stod(val);
            else if (key == "k") cfg.strategy.k = std::stod(val);
            else if (key == "A") cfg.strategy.A = std::stod(val);
            else if (key == "horizon_seconds") cfg.strategy.horizon_seconds = std::stod(val);
            else if (key == "volatility_window") cfg.strategy.volatility_window = std::stoull(val);
            else if (key == "fixed_volatility") cfg.strategy.fixed_volatility = std::stod(val);
            else if (key == "order_quantity") cfg.strategy.order_quantity = std::stoll(val);
            else if (key == "max_inventory") cfg.strategy.max_inventory = std::stoll(val);
            else if (key == "quote_update_threshold_ticks") cfg.strategy.quote_update_threshold_ticks = std::stoll(val);
            else if (key == "liquidity_skew_coeff") cfg.strategy.liquidity_skew_coeff = std::stod(val);
            else if (key == "fixed_half_spread_ticks") cfg.strategy.fixed_half_spread_ticks = std::stoll(val);
        } else if (current_section == "simulation") {
            if (key == "initial_cash") cfg.simulation.initial_cash = std::stod(val);
            else if (key == "maker_fee_rate") cfg.simulation.maker_fee_rate = std::stod(val);
            else if (key == "taker_fee_rate") cfg.simulation.taker_fee_rate = std::stod(val);
            else if (key == "data_latency_ns") cfg.simulation.data_latency_ns = std::stoull(val);
            else if (key == "order_latency_ns") cfg.simulation.order_latency_ns = std::stoull(val);
            else if (key == "random_seed") cfg.simulation.random_seed = std::stoull(val);
            else if (key == "event_count") cfg.simulation.event_count = std::stoull(val);
            else if (key == "scenario") cfg.simulation.scenario = val;
            else if (key == "output_dir") cfg.simulation.output_dir = val;
            else if (key == "input_file") cfg.simulation.input_file = val;
            else if (key == "replay_mode") cfg.simulation.replay_mode = val;
        }
    }

    // Sanity validations
    if (cfg.instrument.tick_size <= 0.0) {
        throw LobException(ErrorCode::ConfigError, "tick_size must be positive");
    }
    if (cfg.instrument.lot_size <= 0.0) {
        throw LobException(ErrorCode::ConfigError, "lot_size must be positive");
    }
    if (cfg.strategy.gamma <= 0.0) {
        throw LobException(ErrorCode::ConfigError, "gamma must be positive");
    }
    if (cfg.strategy.k <= 0.0) {
        throw LobException(ErrorCode::ConfigError, "k must be positive");
    }
    if (cfg.strategy.max_inventory <= 0) {
        throw LobException(ErrorCode::ConfigError, "max_inventory must be positive");
    }

    return cfg;
}

void Config::save_to_file(const std::string& filepath) const {
    std::ofstream out(filepath);
    if (!out.is_open()) {
        throw LobException(ErrorCode::ConfigError, "Unable to write config file: " + filepath);
    }

    out << "# Limit Order Book Configuration\n\n";
    out << "instrument:\n";
    out << "  symbol: \"" << instrument.symbol << "\"\n";
    out << "  tick_size: " << instrument.tick_size << "\n";
    out << "  lot_size: " << instrument.lot_size << "\n";
    out << "  min_price: " << instrument.min_price << "\n";
    out << "  max_price: " << instrument.max_price << "\n";
    out << "  min_quantity: " << instrument.min_quantity << "\n";
    out << "  max_quantity: " << instrument.max_quantity << "\n\n";

    out << "matching:\n";
    out << "  arena_initial_capacity: " << matching.arena_initial_capacity << "\n";
    out << "  enable_invariant_checks: " << (matching.enable_invariant_checks ? "true" : "false") << "\n";
    out << "  allow_self_crossing: " << (matching.allow_self_crossing ? "true" : "false") << "\n\n";

    out << "strategy:\n";
    out << "  type: \"" << strategy.type << "\"\n";
    out << "  gamma: " << strategy.gamma << "\n";
    out << "  k: " << strategy.k << "\n";
    out << "  A: " << strategy.A << "\n";
    out << "  horizon_seconds: " << strategy.horizon_seconds << "\n";
    out << "  volatility_window: " << strategy.volatility_window << "\n";
    out << "  fixed_volatility: " << strategy.fixed_volatility << "\n";
    out << "  order_quantity: " << strategy.order_quantity << "\n";
    out << "  max_inventory: " << strategy.max_inventory << "\n";
    out << "  quote_update_threshold_ticks: " << strategy.quote_update_threshold_ticks << "\n";
    out << "  liquidity_skew_coeff: " << strategy.liquidity_skew_coeff << "\n";
    out << "  fixed_half_spread_ticks: " << strategy.fixed_half_spread_ticks << "\n\n";

    out << "simulation:\n";
    out << "  initial_cash: " << simulation.initial_cash << "\n";
    out << "  maker_fee_rate: " << simulation.maker_fee_rate << "\n";
    out << "  taker_fee_rate: " << simulation.taker_fee_rate << "\n";
    out << "  data_latency_ns: " << simulation.data_latency_ns << "\n";
    out << "  order_latency_ns: " << simulation.order_latency_ns << "\n";
    out << "  random_seed: " << simulation.random_seed << "\n";
    out << "  event_count: " << simulation.event_count << "\n";
    out << "  scenario: \"" << simulation.scenario << "\"\n";
    out << "  output_dir: \"" << simulation.output_dir << "\"\n";
    out << "  input_file: \"" << simulation.input_file << "\"\n";
    out << "  replay_mode: \"" << simulation.replay_mode << "\"\n";
}

} // namespace lob
