#pragma once

#include "lob/core/types.hpp"
#include "lob/core/config.hpp"
#include <vector>
#include <random>
#include <string>

namespace lob {

class SyntheticGenerator {
public:
    explicit SyntheticGenerator(const Config& config);

    // Generates a sequence of order requests simulating background market participants
    // (both limit orders adding/canceling resting depth and market orders executing against liquidity)
    [[nodiscard]] std::vector<OrderRequest> generate_events(size_t event_count = 10000);

    // Set scenario explicitly
    void set_scenario(std::string_view scenario) { scenario_ = scenario; }
    void set_seed(uint64_t seed) { seed_ = seed; rng_.seed(seed); }

private:
    Config config_;
    std::string scenario_{"stable"};
    uint64_t seed_{42};
    std::mt19937_64 rng_;
};

} // namespace lob
