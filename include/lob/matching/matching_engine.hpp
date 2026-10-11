#pragma once

#include "lob/core/types.hpp"
#include "lob/core/config.hpp"
#include "lob/book/order_book.hpp"
#include <vector>
#include <functional>

namespace lob {

class MatchingEngine {
public:
    using TradeCallback = std::function<void(const Trade&)>;
    using ExecutionCallback = std::function<void(const ExecutionReport&)>;

    explicit MatchingEngine(const InstrumentConfig& inst_cfg = InstrumentConfig{},
                           const MatchingConfig& match_cfg = MatchingConfig{});

    // Main exchange operations
    ExecutionReport process_order(const OrderRequest& req);
    ExecutionReport process_cancel(const CancelRequest& req);
    ExecutionReport process_modify(const ModifyRequest& req);
    ExecutionReport process_replace(const ReplaceRequest& req);

    // Callbacks for downstream listeners (simulation, strategy, analytics)
    void set_trade_callback(TradeCallback cb) { trade_cb_ = std::move(cb); }
    void set_execution_callback(ExecutionCallback cb) { exec_cb_ = std::move(cb); }

    // Direct state access
    [[nodiscard]] const OrderBook& book() const noexcept { return book_; }
    [[nodiscard]] OrderBook& book() noexcept { return book_; }
    [[nodiscard]] const InstrumentConfig& instrument_config() const noexcept { return inst_cfg_; }
    [[nodiscard]] const MatchingConfig& matching_config() const noexcept { return match_cfg_; }

    [[nodiscard]] SequenceNumber current_sequence() const noexcept { return seq_; }
    [[nodiscard]] TradeId current_trade_id() const noexcept { return next_trade_id_; }

    // Invariant check
    void verify_invariants() const;

    void reset();

private:
    [[nodiscard]] bool can_fill_fok(const OrderRequest& req) const noexcept;
    void emit_trade(const Trade& trade);
    void emit_execution(const ExecutionReport& report);

    InstrumentConfig inst_cfg_;
    MatchingConfig match_cfg_;
    OrderBook book_;

    SequenceNumber seq_{0};
    TradeId next_trade_id_{1};

    TradeCallback trade_cb_;
    ExecutionCallback exec_cb_;
};

} // namespace lob
