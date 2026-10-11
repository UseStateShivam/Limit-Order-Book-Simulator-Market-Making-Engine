#pragma once

#include "lob/market_data/market_event.hpp"
#include "lob/market_data/data_quality_report.hpp"
#include <string>
#include <vector>

namespace lob {

class CsvParser {
public:
    static std::vector<MarketDataEvent> parse_market_events(
        const std::string& filepath,
        double tick_size = 0.01,
        double lot_size = 1.0,
        DataQualityReport* report = nullptr);

    static std::vector<OrderRequest> parse_order_events(
        const std::string& filepath,
        double tick_size = 0.01,
        double lot_size = 1.0,
        DataQualityReport* report = nullptr);

    static void write_market_events(
        const std::string& filepath,
        const std::vector<MarketDataEvent>& events,
        double tick_size = 0.01,
        double lot_size = 1.0);
};

} // namespace lob
