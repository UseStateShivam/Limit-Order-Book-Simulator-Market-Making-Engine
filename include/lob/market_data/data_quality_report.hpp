#pragma once

#include "lob/core/types.hpp"
#include <string>
#include <vector>
#include <set>
#include <sstream>

namespace lob {

struct DataQualityReport {
    std::string filename{""};
    size_t total_records{0};
    size_t valid_records{0};
    size_t rejected_records{0};
    size_t duplicate_events{0};
    size_t out_of_order_timestamps{0};
    size_t sequence_gaps{0};
    Timestamp min_timestamp{0};
    Timestamp max_timestamp{0};
    std::set<std::string> symbols;
    std::vector<std::string> error_log;

    [[nodiscard]] std::string to_string() const {
        std::ostringstream oss;
        oss << "=== Data Quality Report ===\n"
            << "File: " << filename << "\n"
            << "Total records: " << total_records << "\n"
            << "Valid records: " << valid_records << "\n"
            << "Rejected records: " << rejected_records << "\n"
            << "Duplicate events: " << duplicate_events << "\n"
            << "Out-of-order timestamps: " << out_of_order_timestamps << "\n"
            << "Sequence gaps: " << sequence_gaps << "\n"
            << "Time range: [" << min_timestamp << ", " << max_timestamp << "] ns\n"
            << "Symbols: ";
        for (const auto& sym : symbols) oss << sym << " ";
        oss << "\n";
        if (!error_log.empty()) {
            oss << "First " << std::min(error_log.size(), size_t(5)) << " errors:\n";
            for (size_t i = 0; i < std::min(error_log.size(), size_t(5)); ++i) {
                oss << "  - " << error_log[i] << "\n";
            }
        }
        return oss.str();
    }
};

} // namespace lob
