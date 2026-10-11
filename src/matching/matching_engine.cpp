#include "lob/matching/matching_engine.hpp"
#include <algorithm>

namespace lob {

MatchingEngine::MatchingEngine(const InstrumentConfig& inst_cfg, const MatchingConfig& match_cfg)
    : inst_cfg_(inst_cfg), match_cfg_(match_cfg), book_(match_cfg.arena_initial_capacity) {}

void MatchingEngine::emit_trade(const Trade& trade) {
    if (trade_cb_) trade_cb_(trade);
}

void MatchingEngine::emit_execution(const ExecutionReport& report) {
    if (exec_cb_) exec_cb_(report);
}

bool MatchingEngine::can_fill_fok(const OrderRequest& req) const noexcept {
    Quantity available = 0;
    if (req.side == Side::Buy) {
        for (const auto& [price, level] : book_.asks()) {
            if (req.type == OrderType::Limit && price > req.price) break;
            available += level.aggregate_qty;
            if (available >= req.quantity) return true;
        }
    } else {
        for (const auto& [price, level] : book_.bids()) {
            if (req.type == OrderType::Limit && price < req.price) break;
            available += level.aggregate_qty;
            if (available >= req.quantity) return true;
        }
    }
    return available >= req.quantity;
}

ExecutionReport MatchingEngine::process_order(const OrderRequest& req) {
    // 1. Validation
    ExecutionReport base_rep;
    base_rep.sequence = ++seq_;
    base_rep.timestamp = req.timestamp;
    base_rep.order_id = req.order_id;
    base_rep.side = req.side;
    base_rep.order_type = req.type;
    base_rep.price = req.price;
    base_rep.leaves_qty = req.quantity;
    base_rep.cum_qty = 0;

    if (req.order_id == 0) {
        base_rep.exec_type = ExecutionType::Rejected;
        base_rep.order_status = OrderStatus::Rejected;
        base_rep.reject_reason = RejectReason::Unknown;
        emit_execution(base_rep);
        return base_rep;
    }

    if (req.quantity <= 0 || req.quantity < inst_cfg_.min_quantity || req.quantity > inst_cfg_.max_quantity) {
        base_rep.exec_type = ExecutionType::Rejected;
        base_rep.order_status = OrderStatus::Rejected;
        base_rep.reject_reason = RejectReason::InvalidQuantity;
        emit_execution(base_rep);
        return base_rep;
    }

    if (req.type == OrderType::Limit && req.price <= 0) {
        base_rep.exec_type = ExecutionType::Rejected;
        base_rep.order_status = OrderStatus::Rejected;
        base_rep.reject_reason = RejectReason::InvalidPrice;
        emit_execution(base_rep);
        return base_rep;
    }

    if (book_.contains_order(req.order_id)) {
        base_rep.exec_type = ExecutionType::Rejected;
        base_rep.order_status = OrderStatus::Rejected;
        base_rep.reject_reason = RejectReason::DuplicateOrderId;
        emit_execution(base_rep);
        return base_rep;
    }

    // 2. Fill-Or-Kill pre-check
    if (req.tif == TimeInForce::FOK && !can_fill_fok(req)) {
        base_rep.exec_type = ExecutionType::Rejected;
        base_rep.order_status = OrderStatus::Rejected;
        base_rep.reject_reason = RejectReason::InsufficientLiquidity;
        emit_execution(base_rep);
        return base_rep;
    }

    Quantity remaining_qty = req.quantity;
    Quantity cum_filled = 0;
    ExecutionReport last_taker_report = base_rep;

    // 3. Price-time-priority matching loop
    if (req.side == Side::Buy) {
        while (remaining_qty > 0 && !book_.asks().empty()) {
            auto ask_it = book_.asks().begin();
            Price best_ask = ask_it->first;

            if (req.type == OrderType::Limit && best_ask > req.price) {
                break; // Limit price does not cross
            }

            uint32_t head_idx = ask_it->second.head_order_idx;
            const auto& maker_rec = book_.arena().get_by_index(head_idx);
            OrderId maker_id = maker_rec.order_id;
            Price exec_price = best_ask;
            Quantity exec_qty = std::min(remaining_qty, maker_rec.remaining_qty);

            remaining_qty -= exec_qty;
            cum_filled += exec_qty;

            // Generate trade
            Trade trade;
            trade.trade_id = next_trade_id_++;
            trade.sequence = ++seq_;
            trade.timestamp = req.timestamp;
            trade.maker_order_id = maker_id;
            trade.taker_order_id = req.order_id;
            trade.taker_side = Side::Buy;
            trade.price = exec_price;
            trade.quantity = exec_qty;
            emit_trade(trade);

            // Emit maker execution report
            ExecutionReport maker_rep;
            maker_rep.sequence = ++seq_;
            maker_rep.timestamp = req.timestamp;
            maker_rep.order_id = maker_id;
            maker_rep.side = Side::Sell;
            maker_rep.order_type = maker_rec.type;
            maker_rep.exec_type = ExecutionType::Trade;
            maker_rep.price = maker_rec.price;
            maker_rep.last_qty = exec_qty;
            maker_rep.last_price = exec_price;
            maker_rep.leaves_qty = maker_rec.remaining_qty - exec_qty;
            maker_rep.cum_qty = maker_rec.original_qty - maker_rep.leaves_qty;
            maker_rep.order_status = (maker_rep.leaves_qty == 0) ? OrderStatus::Filled : OrderStatus::PartiallyFilled;
            maker_rep.trade_id = trade.trade_id;
            emit_execution(maker_rep);

            // Update maker in book
            book_.reduce_order_quantity(maker_id, exec_qty);

            // Taker execution report
            last_taker_report.sequence = ++seq_;
            last_taker_report.exec_type = ExecutionType::Trade;
            last_taker_report.last_qty = exec_qty;
            last_taker_report.last_price = exec_price;
            last_taker_report.leaves_qty = remaining_qty;
            last_taker_report.cum_qty = cum_filled;
            last_taker_report.order_status = (remaining_qty == 0) ? OrderStatus::Filled : OrderStatus::PartiallyFilled;
            last_taker_report.trade_id = trade.trade_id;
            emit_execution(last_taker_report);
        }
    } else {
        // Sell side
        while (remaining_qty > 0 && !book_.bids().empty()) {
            auto bid_it = book_.bids().begin();
            Price best_bid = bid_it->first;

            if (req.type == OrderType::Limit && best_bid < req.price) {
                break; // Limit price does not cross
            }

            uint32_t head_idx = bid_it->second.head_order_idx;
            const auto& maker_rec = book_.arena().get_by_index(head_idx);
            OrderId maker_id = maker_rec.order_id;
            Price exec_price = best_bid;
            Quantity exec_qty = std::min(remaining_qty, maker_rec.remaining_qty);

            remaining_qty -= exec_qty;
            cum_filled += exec_qty;

            // Generate trade
            Trade trade;
            trade.trade_id = next_trade_id_++;
            trade.sequence = ++seq_;
            trade.timestamp = req.timestamp;
            trade.maker_order_id = maker_id;
            trade.taker_order_id = req.order_id;
            trade.taker_side = Side::Sell;
            trade.price = exec_price;
            trade.quantity = exec_qty;
            emit_trade(trade);

            // Maker report
            ExecutionReport maker_rep;
            maker_rep.sequence = ++seq_;
            maker_rep.timestamp = req.timestamp;
            maker_rep.order_id = maker_id;
            maker_rep.side = Side::Buy;
            maker_rep.order_type = maker_rec.type;
            maker_rep.exec_type = ExecutionType::Trade;
            maker_rep.price = maker_rec.price;
            maker_rep.last_qty = exec_qty;
            maker_rep.last_price = exec_price;
            maker_rep.leaves_qty = maker_rec.remaining_qty - exec_qty;
            maker_rep.cum_qty = maker_rec.original_qty - maker_rep.leaves_qty;
            maker_rep.order_status = (maker_rep.leaves_qty == 0) ? OrderStatus::Filled : OrderStatus::PartiallyFilled;
            maker_rep.trade_id = trade.trade_id;
            emit_execution(maker_rep);

            // Update maker in book
            book_.reduce_order_quantity(maker_id, exec_qty);

            // Taker report
            last_taker_report.sequence = ++seq_;
            last_taker_report.exec_type = ExecutionType::Trade;
            last_taker_report.last_qty = exec_qty;
            last_taker_report.last_price = exec_price;
            last_taker_report.leaves_qty = remaining_qty;
            last_taker_report.cum_qty = cum_filled;
            last_taker_report.order_status = (remaining_qty == 0) ? OrderStatus::Filled : OrderStatus::PartiallyFilled;
            last_taker_report.trade_id = trade.trade_id;
            emit_execution(last_taker_report);
        }
    }

    // 4. Post-matching handling of unfilled remainder
    if (remaining_qty > 0) {
        if (req.type == OrderType::Market || req.tif == TimeInForce::IOC) {
            // Market and IOC orders cancel any unfilled remainder
            ExecutionReport cancel_rep = last_taker_report;
            cancel_rep.sequence = ++seq_;
            cancel_rep.exec_type = ExecutionType::Canceled;
            cancel_rep.order_status = (cum_filled > 0) ? OrderStatus::PartiallyFilled : OrderStatus::Canceled;
            cancel_rep.leaves_qty = 0;
            emit_execution(cancel_rep);
            last_taker_report = cancel_rep;
        } else if (req.type == OrderType::Limit && req.tif == TimeInForce::GTC) {
            // GTC limit order rests on book
            OrderRequest resting_req = req;
            resting_req.quantity = remaining_qty;
            book_.add_resting_order(resting_req, ++seq_);

            if (cum_filled == 0) {
                last_taker_report.sequence = ++seq_;
                last_taker_report.exec_type = ExecutionType::New;
                last_taker_report.order_status = OrderStatus::Accepted;
                emit_execution(last_taker_report);
            }
        }
    }

    if (match_cfg_.enable_invariant_checks) {
        book_.verify_invariants();
    }

    return last_taker_report;
}

ExecutionReport MatchingEngine::process_cancel(const CancelRequest& req) {
    ExecutionReport rep;
    rep.sequence = ++seq_;
    rep.timestamp = req.timestamp;
    rep.order_id = req.order_id;

    if (!book_.contains_order(req.order_id)) {
        rep.exec_type = ExecutionType::Rejected;
        rep.order_status = OrderStatus::Rejected;
        rep.reject_reason = RejectReason::OrderNotFound;
        emit_execution(rep);
        return rep;
    }

    OrderRecord rec = book_.remove_order(req.order_id);
    rep.side = rec.side;
    rep.order_type = rec.type;
    rep.price = rec.price;
    rep.exec_type = ExecutionType::Canceled;
    rep.order_status = OrderStatus::Canceled;
    rep.leaves_qty = 0;
    rep.cum_qty = rec.original_qty - rec.remaining_qty;
    emit_execution(rep);

    if (match_cfg_.enable_invariant_checks) {
        book_.verify_invariants();
    }
    return rep;
}

ExecutionReport MatchingEngine::process_modify(const ModifyRequest& req) {
    ExecutionReport rep;
    rep.sequence = ++seq_;
    rep.timestamp = req.timestamp;
    rep.order_id = req.order_id;

    const auto* rec = book_.find_order(req.order_id);
    if (!rec) {
        rep.exec_type = ExecutionType::Rejected;
        rep.order_status = OrderStatus::Rejected;
        rep.reject_reason = RejectReason::OrderNotFound;
        emit_execution(rep);
        return rep;
    }

    rep.side = rec->side;
    rep.order_type = rec->type;
    rep.price = rec->price;

    if (req.new_quantity <= 0) {
        return process_cancel({req.order_id, req.timestamp});
    }

    book_.modify_order_quantity(req.order_id, req.new_quantity);
    rep.exec_type = ExecutionType::Modified;
    rep.order_status = OrderStatus::Accepted;
    rep.leaves_qty = req.new_quantity;
    rep.cum_qty = rec->original_qty - rec->remaining_qty;
    emit_execution(rep);

    if (match_cfg_.enable_invariant_checks) {
        book_.verify_invariants();
    }
    return rep;
}

ExecutionReport MatchingEngine::process_replace(const ReplaceRequest& req) {
    if (!book_.contains_order(req.old_order_id)) {
        ExecutionReport rep;
        rep.sequence = ++seq_;
        rep.timestamp = req.timestamp;
        rep.order_id = req.old_order_id;
        rep.exec_type = ExecutionType::Rejected;
        rep.order_status = OrderStatus::Rejected;
        rep.reject_reason = RejectReason::OrderNotFound;
        emit_execution(rep);
        return rep;
    }

    // Cancel old order
    process_cancel({req.old_order_id, req.timestamp});

    // Submit new order
    OrderRequest new_order_req;
    new_order_req.order_id = req.new_order_id;
    new_order_req.side = req.side;
    new_order_req.price = req.new_price;
    new_order_req.quantity = req.new_quantity;
    new_order_req.type = req.type;
    new_order_req.tif = req.tif;
    new_order_req.timestamp = req.timestamp;

    return process_order(new_order_req);
}

void MatchingEngine::verify_invariants() const {
    book_.verify_invariants();
}

void MatchingEngine::reset() {
    book_.clear();
    seq_ = 0;
    next_trade_id_ = 1;
}

} // namespace lob
