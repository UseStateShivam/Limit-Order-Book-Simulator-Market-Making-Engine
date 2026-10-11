#include "lob/market_data/csv_parser.hpp"
#include "lob/core/error.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace lob {

namespace {

std::vector<std::string> split_csv_line(const std::string& line) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream token_stream(line);
    while (std::getline(token_stream, token, ',')) {
        // Trim whitespace
        const auto start = token.find_first_not_of(" \t\r\n\"");
        if (start != std::string::npos) {
            const auto end = token.find_last_not_of(" \t\r\n\"");
            tokens.push_back(token.substr(start, end - start + 1));
        } else {
            tokens.push_back("");
        }
    }
    return tokens;
}

} // namespace

std::vector<MarketDataEvent> CsvParser::parse_market_events(
    const std::string& filepath,
    double tick_size,
    double lot_size,
    DataQualityReport* report) {

    std::vector<MarketDataEvent> events;
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw LobException(ErrorCode::DataParseError, "Unable to open market events CSV: " + filepath);
    }

    if (report) {
        report->filename = filepath;
    }

    std::string line;
    bool is_header = true;
    Timestamp prev_ts = 0;
    SequenceNumber prev_seq = 0;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        if (report) report->total_records++;

        auto tokens = split_csv_line(line);
        if (is_header) {
            is_header = false;
            // Check if first line is indeed header
            if (!tokens.empty() && (tokens[0] == "timestamp_ns" || tokens[0] == "timestamp")) {
                if (report) report->total_records--;
                continue;
            }
        }

        if (tokens.size() < 4) {
            if (report) {
                report->rejected_records++;
                report->error_log.push_back("Insufficient columns in line: " + line);
            }
            continue;
        }

        try {
            MarketDataEvent ev;
            ev.timestamp = std::stoull(tokens[0]);
            ev.sequence = std::stoull(tokens[1]);
            std::string type_str = tokens[2];
            ev.symbol = tokens[3];

            if (type_str == "TRADE" || type_str == "Trade") ev.type = MarketEventType::Trade;
            else if (type_str == "BBO" || type_str == "bbo") ev.type = MarketEventType::BBO;
            else if (type_str == "SNAPSHOT") ev.type = MarketEventType::Snapshot;
            else ev.type = MarketEventType::BBO;

            if (tokens.size() >= 8) {
                double bid_p = std::stod(tokens[4]);
                double bid_q = std::stod(tokens[5]);
                double ask_p = std::stod(tokens[6]);
                double ask_q = std::stod(tokens[7]);
                ev.bid_price = price_to_ticks(bid_p, tick_size);
                ev.bid_quantity = qty_to_lots(bid_q, lot_size);
                ev.ask_price = price_to_ticks(ask_p, tick_size);
                ev.ask_quantity = qty_to_lots(ask_q, lot_size);
            }

            if (tokens.size() >= 10) {
                double tr_p = std::stod(tokens[8]);
                double tr_q = std::stod(tokens[9]);
                ev.trade_price = price_to_ticks(tr_p, tick_size);
                ev.trade_quantity = qty_to_lots(tr_q, lot_size);
            }

            if (report) {
                if (report->valid_records == 0) {
                    report->min_timestamp = ev.timestamp;
                    report->max_timestamp = ev.timestamp;
                } else {
                    if (ev.timestamp < prev_ts) report->out_of_order_timestamps++;
                    if (ev.timestamp > report->max_timestamp) report->max_timestamp = ev.timestamp;
                    if (ev.timestamp < report->min_timestamp) report->min_timestamp = ev.timestamp;
                }
                if (prev_seq > 0 && ev.sequence > prev_seq + 1) {
                    report->sequence_gaps++;
                }
                if (prev_seq > 0 && ev.sequence == prev_seq) {
                    report->duplicate_events++;
                }
                report->symbols.insert(ev.symbol);
                report->valid_records++;
            }

            prev_ts = ev.timestamp;
            prev_seq = ev.sequence;
            events.push_back(ev);
        } catch (const std::exception& e) {
            if (report) {
                report->rejected_records++;
                report->error_log.push_back(std::string("Parse exception: ") + e.what() + " on line: " + line);
            }
        }
    }

    return events;
}

std::vector<OrderRequest> CsvParser::parse_order_events(
    const std::string& filepath,
    double tick_size,
    double lot_size,
    DataQualityReport* report) {

    std::vector<OrderRequest> requests;
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw LobException(ErrorCode::DataParseError, "Unable to open order events CSV: " + filepath);
    }

    if (report) report->filename = filepath;

    std::string line;
    bool is_header = true;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        if (report) report->total_records++;

        auto tokens = split_csv_line(line);
        if (is_header) {
            is_header = false;
            if (!tokens.empty() && (tokens[0] == "timestamp_ns" || tokens[0] == "timestamp")) {
                if (report) report->total_records--;
                continue;
            }
        }

        if (tokens.size() < 7) {
            if (report) {
                report->rejected_records++;
                report->error_log.push_back("Insufficient columns in line: " + line);
            }
            continue;
        }

        try {
            OrderRequest req;
            req.timestamp = std::stoull(tokens[0]);
            // tokens[1] = sequence
            // tokens[2] = event_type (ADD)
            req.order_id = std::stoull(tokens[3]);
            auto side_opt = string_to_side(tokens[4]);
            if (!side_opt) {
                if (report) report->rejected_records++;
                continue;
            }
            req.side = *side_opt;
            double p = std::stod(tokens[5]);
            double q = std::stod(tokens[6]);
            req.price = price_to_ticks(p, tick_size);
            req.quantity = qty_to_lots(q, lot_size);
            req.type = OrderType::Limit;
            req.tif = TimeInForce::GTC;

            if (tokens.size() >= 8) {
                auto type_opt = string_to_order_type(tokens[7]);
                if (type_opt) req.type = *type_opt;
            }

            if (report) report->valid_records++;
            requests.push_back(req);
        } catch (const std::exception& e) {
            if (report) {
                report->rejected_records++;
                report->error_log.push_back(std::string("Parse exception: ") + e.what() + " on line: " + line);
            }
        }
    }

    return requests;
}

void CsvParser::write_market_events(
    const std::string& filepath,
    const std::vector<MarketDataEvent>& events,
    double tick_size,
    double lot_size) {

    std::ofstream out(filepath);
    if (!out.is_open()) {
        throw LobException(ErrorCode::DataParseError, "Unable to write market events to: " + filepath);
    }

    out << "timestamp_ns,sequence,event_type,symbol,bid_price,bid_qty,ask_price,ask_qty,trade_price,trade_qty\n";
    for (const auto& ev : events) {
        out << ev.timestamp << ","
            << ev.sequence << ","
            << market_event_type_to_string(ev.type) << ","
            << ev.symbol << ","
            << ticks_to_price(ev.bid_price, tick_size) << ","
            << lots_to_qty(ev.bid_quantity, lot_size) << ","
            << ticks_to_price(ev.ask_price, tick_size) << ","
            << lots_to_qty(ev.ask_quantity, lot_size) << ","
            << ticks_to_price(ev.trade_price, tick_size) << ","
            << lots_to_qty(ev.trade_quantity, lot_size) << "\n";
    }
}

} // namespace lob
