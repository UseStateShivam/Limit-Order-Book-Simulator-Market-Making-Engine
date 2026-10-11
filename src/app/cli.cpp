#include "src/app/cli.hpp"
#include "lob/core/config.hpp"
#include "lob/simulation/simulator.hpp"
#include "lob/market_data/csv_parser.hpp"
#include "lob/matching/matching_engine.hpp"
#include <iostream>
#include <fstream>
#include <chrono>

namespace lob {

namespace {

std::string get_arg(const std::vector<std::string>& args, const std::string& flag, const std::string& def = "") {
    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == flag && i + 1 < args.size()) {
            return args[i + 1];
        }
    }
    return def;
}

} // namespace

void Cli::print_help() {
    std::cout << R"(Limit Order Book Simulator & Market-Making Engine (lob_sim)

Usage:
  lob_sim <command> [options]

Commands:
  run-simulation     Run a deterministic synthetic market simulation
  run-market-maker   Execute the Avellaneda-Stoikov market maker against a simulated exchange
  replay-data        Replay historical or external market event CSV
  validate-data      Inspect and validate market data files for sequence gaps & errors
  benchmark          Run fast internal matching engine microbenchmarks
  export-book        Construct an order book scenario and export L2 / L3 snapshots
  summarize-results  Display metrics summary from a completed results directory

Options:
  --config <path>    Configuration file path (YAML/JSON)
  --input <path>     Input market data CSV file path
  --output <dir>     Output directory for results and artifacts
  --seed <uint>      Random seed for deterministic simulation
  --scenario <name>  Scenario name (stable, high_volatility, liquidity_imbalance,
                     sparse_liquidity, toxic_flow, inventory_stress)
  --events <count>   Number of events to generate/simulate
  --help, -h         Display this help message
)" << std::endl;
}

int Cli::handle_run_simulation(const std::vector<std::string>& args) {
    std::string config_path = get_arg(args, "--config", "");
    Config cfg = config_path.empty() ? Config::default_config() : Config::load_from_file(config_path);

    std::string output_dir = get_arg(args, "--output", "");
    if (!output_dir.empty()) cfg.simulation.output_dir = output_dir;

    std::string seed_str = get_arg(args, "--seed", "");
    if (!seed_str.empty()) cfg.simulation.random_seed = std::stoull(seed_str);

    std::string scenario = get_arg(args, "--scenario", "");
    if (!scenario.empty()) cfg.simulation.scenario = scenario;

    std::string events_str = get_arg(args, "--events", "");
    if (!events_str.empty()) cfg.simulation.event_count = std::stoull(events_str);

    std::cout << "Starting simulation [" << cfg.simulation.scenario 
              << "] (Seed: " << cfg.simulation.random_seed 
              << ", Events: " << cfg.simulation.event_count << ")..." << std::endl;

    Simulator sim(cfg);
    auto summary = sim.run();

    std::cout << "Simulation completed successfully!" << std::endl;
    std::cout << summary.to_markdown() << std::endl;
    std::cout << "Artifacts written to: " << cfg.simulation.output_dir << std::endl;
    return 0;
}

int Cli::handle_run_market_maker(const std::vector<std::string>& args) {
    std::string config_path = get_arg(args, "--config", "");
    Config cfg = config_path.empty() ? Config::default_config() : Config::load_from_file(config_path);

    std::string output_dir = get_arg(args, "--output", "");
    if (!output_dir.empty()) cfg.simulation.output_dir = output_dir;

    std::cout << "Running Market Maker Strategy [" << cfg.strategy.type << "]..." << std::endl;
    Simulator sim(cfg);
    auto summary = sim.run();

    std::cout << "Market Maker run complete!" << std::endl;
    std::cout << summary.to_markdown() << std::endl;
    std::cout << "Artifacts written to: " << cfg.simulation.output_dir << std::endl;
    return 0;
}

int Cli::handle_validate_data(const std::vector<std::string>& args) {
    std::string input_path = get_arg(args, "--input", "");
    if (input_path.empty()) {
        std::cerr << "Error: --input <path> is required for validate-data" << std::endl;
        return 1;
    }

    std::cout << "Validating market data file: " << input_path << "..." << std::endl;
    DataQualityReport report;
    auto events = CsvParser::parse_market_events(input_path, 0.01, 1.0, &report);
    if (events.empty()) {
        // Try parsing as order events
        DataQualityReport ord_report;
        auto ords = CsvParser::parse_order_events(input_path, 0.01, 1.0, &ord_report);
        std::cout << ord_report.to_string() << std::endl;
    } else {
        std::cout << report.to_string() << std::endl;
    }
    return 0;
}

int Cli::handle_replay_data(const std::vector<std::string>& args) {
    std::string input_path = get_arg(args, "--input", "");
    if (input_path.empty()) {
        std::cerr << "Error: --input <path> is required for replay-data" << std::endl;
        return 1;
    }

    std::string config_path = get_arg(args, "--config", "");
    Config cfg = config_path.empty() ? Config::default_config() : Config::load_from_file(config_path);

    std::string output_dir = get_arg(args, "--output", "");
    if (!output_dir.empty()) cfg.simulation.output_dir = output_dir;

    std::cout << "Replaying market events from: " << input_path << "..." << std::endl;
    Simulator sim(cfg);
    auto summary = sim.replay_file(input_path);

    std::cout << "Replay completed successfully!" << std::endl;
    std::cout << summary.to_markdown() << std::endl;
    std::cout << "Artifacts written to: " << cfg.simulation.output_dir << std::endl;
    return 0;
}

int Cli::handle_benchmark(const std::vector<std::string>& /*args*/) {
    std::cout << "=== Running Matching Engine Latency & Throughput Benchmark ===" << std::endl;
    MatchingConfig match_cfg;
    match_cfg.enable_invariant_checks = false;
    MatchingEngine engine(InstrumentConfig{}, match_cfg);

    constexpr size_t kIterations = 50'000;
    std::vector<OrderRequest> orders;
    orders.reserve(kIterations);

    // Deterministic pre-generated workload
    for (size_t i = 1; i <= kIterations; ++i) {
        OrderRequest req;
        req.order_id = i;
        req.side = (i % 2 == 0) ? Side::Buy : Side::Sell;
        req.price = (req.side == Side::Buy) ? (10000 - static_cast<Price>(i % 50)) : (10100 + static_cast<Price>(i % 50));
        req.quantity = 10;
        req.type = OrderType::Limit;
        req.tif = TimeInForce::GTC;
        req.timestamp = i * 1000;
        orders.push_back(req);
    }

    // Benchmark 1: Limit Order Insertions
    auto start = std::chrono::high_resolution_clock::now();
    for (const auto& ord : orders) {
        engine.process_order(ord);
    }
    auto end = std::chrono::high_resolution_clock::now();
    double insert_sec = std::chrono::duration<double>(end - start).count();
    double insert_ns = (insert_sec * 1e9) / static_cast<double>(kIterations);
    double insert_ops = static_cast<double>(kIterations) / insert_sec;

    std::cout << "1. Limit Order Insertion:" << std::endl;
    std::cout << "   - Total Operations: " << kIterations << std::endl;
    std::cout << "   - Average Latency:  " << insert_ns << " ns/op" << std::endl;
    std::cout << "   - Throughput:       " << insert_ops / 1e6 << " M ops/sec" << std::endl;

    // Benchmark 2: Cancellations
    start = std::chrono::high_resolution_clock::now();
    for (size_t i = 1; i <= kIterations; i += 2) {
        engine.process_cancel({i, i * 2000});
    }
    end = std::chrono::high_resolution_clock::now();
    size_t cancel_count = kIterations / 2;
    double cancel_sec = std::chrono::duration<double>(end - start).count();
    double cancel_ns = (cancel_sec * 1e9) / static_cast<double>(cancel_count);
    double cancel_ops = static_cast<double>(cancel_count) / cancel_sec;

    std::cout << "2. Order Cancellation:" << std::endl;
    std::cout << "   - Total Operations: " << cancel_count << std::endl;
    std::cout << "   - Average Latency:  " << cancel_ns << " ns/op" << std::endl;
    std::cout << "   - Throughput:       " << cancel_ops / 1e6 << " M ops/sec" << std::endl;

    // Benchmark 3: Aggressive Market Order Crossing / Matching
    constexpr size_t kMatchOrders = 50'000;
    std::vector<OrderRequest> match_orders;
    match_orders.reserve(kMatchOrders);
    for (size_t i = 1; i <= kMatchOrders; ++i) {
        OrderRequest req;
        req.order_id = 1'000'000 + i;
        req.side = Side::Buy;
        req.price = 10200; // Crosses asks
        req.quantity = 1;
        req.type = OrderType::Market;
        req.tif = TimeInForce::IOC;
        req.timestamp = i * 3000;
        match_orders.push_back(req);
    }

    start = std::chrono::high_resolution_clock::now();
    for (const auto& ord : match_orders) {
        engine.process_order(ord);
    }
    end = std::chrono::high_resolution_clock::now();
    double match_sec = std::chrono::duration<double>(end - start).count();
    double match_ns = (match_sec * 1e9) / static_cast<double>(kMatchOrders);
    double match_ops = static_cast<double>(kMatchOrders) / match_sec;

    std::cout << "3. Taker Matching / Execution:" << std::endl;
    std::cout << "   - Total Operations: " << kMatchOrders << std::endl;
    std::cout << "   - Average Latency:  " << match_ns << " ns/op" << std::endl;
    std::cout << "   - Throughput:       " << match_ops / 1e6 << " M ops/sec" << std::endl;

    return 0;
}

int Cli::handle_export_book(const std::vector<std::string>& /*args*/) {
    std::cout << "Constructing demonstration order book and exporting snapshot..." << std::endl;
    OrderBook book;
    for (int i = 1; i <= 5; ++i) {
        OrderRequest b{static_cast<OrderId>(i), Side::Buy, 10000 - i, 10 * i, OrderType::Limit, TimeInForce::GTC, 1000};
        book.add_resting_order(b, i);
        OrderRequest a{static_cast<OrderId>(100 + i), Side::Sell, 10000 + i, 10 * i, OrderType::Limit, TimeInForce::GTC, 1000};
        book.add_resting_order(a, 100 + i);
    }

    auto l2 = book.get_l2_snapshot(5);
    std::cout << "=== L2 Market Depth ===" << std::endl;
    std::cout << "ASKS:" << std::endl;
    for (auto it = l2.asks.rbegin(); it != l2.asks.rend(); ++it) {
        std::cout << "  " << it->price << " ticks | Qty: " << it->quantity << " | Orders: " << it->order_count << std::endl;
    }
    std::cout << "  ---------------- [ Spread: " << (*book.spread_ticks()) << " ticks ] ----------------" << std::endl;
    std::cout << "BIDS:" << std::endl;
    for (const auto& b : l2.bids) {
        std::cout << "  " << b.price << " ticks | Qty: " << b.quantity << " | Orders: " << b.order_count << std::endl;
    }
    return 0;
}

int Cli::handle_summarize_results(const std::vector<std::string>& args) {
    std::string input_dir = get_arg(args, "--input", "results/default");
    std::ifstream file(input_dir + "/summary.json");
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open " << input_dir << "/summary.json" << std::endl;
        return 1;
    }
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::cout << content << std::endl;
    return 0;
}

int Cli::run(int argc, char* argv[]) {
    if (argc < 2) {
        print_help();
        return 0;
    }

    std::string command = argv[1];
    if (command == "--help" || command == "-h" || command == "help") {
        print_help();
        return 0;
    }

    std::vector<std::string> args;
    for (int i = 2; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }

    try {
        if (command == "run-simulation") {
            return handle_run_simulation(args);
        } else if (command == "run-market-maker") {
            return handle_run_market_maker(args);
        } else if (command == "validate-data") {
            return handle_validate_data(args);
        } else if (command == "replay-data") {
            return handle_replay_data(args);
        } else if (command == "benchmark") {
            return handle_benchmark(args);
        } else if (command == "export-book") {
            return handle_export_book(args);
        } else if (command == "summarize-results") {
            return handle_summarize_results(args);
        } else {
            std::cerr << "Unknown command: " << command << "\n" << std::endl;
            print_help();
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error executing command '" << command << "': " << e.what() << std::endl;
        return 1;
    }
}

} // namespace lob
