#include <gtest/gtest.h>
#include "lob/market_data/csv_parser.hpp"
#include "lob/core/error.hpp"

using namespace lob;

#ifndef TEST_DATA_DIR
#define TEST_DATA_DIR "data/sample"
#endif

TEST(CsvParserTest, ParseMarketEventsSample) {
    DataQualityReport report;
    std::string path = std::string(TEST_DATA_DIR) + "/market_events.csv";
    auto events = CsvParser::parse_market_events(path, 0.01, 1.0, &report);

    EXPECT_GT(events.size(), 0);
    EXPECT_EQ(report.rejected_records, 0);
    EXPECT_EQ(report.valid_records, events.size());
    EXPECT_TRUE(report.symbols.count("BTC-USDT") > 0);

    // Verify first event fields
    EXPECT_EQ(events[0].sequence, 1);
    EXPECT_EQ(events[0].bid_price, 10000); // 100.00 / 0.01 = 10000 ticks
    EXPECT_EQ(events[0].ask_price, 10002); // 100.02 / 0.01 = 10002 ticks
}

TEST(CsvParserTest, ParseOrderEventsSample) {
    DataQualityReport report;
    std::string path = std::string(TEST_DATA_DIR) + "/order_events.csv";
    auto orders = CsvParser::parse_order_events(path, 0.01, 1.0, &report);

    EXPECT_GT(orders.size(), 0);
    EXPECT_EQ(report.rejected_records, 0);
    EXPECT_EQ(orders[0].order_id, 1);
    EXPECT_EQ(orders[0].side, Side::Buy);
    EXPECT_EQ(orders[0].price, 10000);
}

TEST(CsvParserTest, NonexistentFileThrows) {
    EXPECT_THROW(CsvParser::parse_market_events("nonexistent_file.csv"), LobException);
}
