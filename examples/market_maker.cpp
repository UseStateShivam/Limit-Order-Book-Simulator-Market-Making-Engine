#include "lob/simulation/simulator.hpp"
#include <iostream>

using namespace lob;

int main() {
    std::cout << "=== Avellaneda-Stoikov Market Maker Example ===" << std::endl;
    Config cfg = Config::default_config();
    cfg.simulation.event_count = 5000;
    cfg.simulation.scenario = "stable";
    cfg.strategy.type = "as_baseline";

    Simulator sim(cfg);
    auto summary = sim.run();

    std::cout << "\nExecution Results:" << std::endl;
    std::cout << "  Orders Submitted: " << summary.orders_submitted << std::endl;
    std::cout << "  Executed Volume:  " << summary.executed_volume << " lots" << std::endl;
    std::cout << "  Fill Rate:        " << summary.fill_rate_pct << "%" << std::endl;
    std::cout << "  Net P&L:          $" << summary.net_pnl << std::endl;
    std::cout << "  Max Drawdown:     " << summary.max_drawdown_pct << "%" << std::endl;
    std::cout << "  Avg Spread Cap.:  $" << summary.avg_spread_captured << " / lot" << std::endl;

    return 0;
}
