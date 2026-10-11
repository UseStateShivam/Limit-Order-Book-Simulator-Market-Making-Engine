#pragma once

#include <string>
#include <vector>

namespace lob {

class Cli {
public:
    static int run(int argc, char* argv[]);

private:
    static void print_help();
    static int handle_run_simulation(const std::vector<std::string>& args);
    static int handle_run_market_maker(const std::vector<std::string>& args);
    static int handle_validate_data(const std::vector<std::string>& args);
    static int handle_replay_data(const std::vector<std::string>& args);
    static int handle_benchmark(const std::vector<std::string>& args);
    static int handle_export_book(const std::vector<std::string>& args);
    static int handle_summarize_results(const std::vector<std::string>& args);
};

} // namespace lob
