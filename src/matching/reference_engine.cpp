#include "lob/matching/reference_engine.hpp"
#include <algorithm>

namespace lob {

std::optional<Price> ReferenceMatchingEngine::best_bid() const noexcept {
    if (bids_.empty()) return std::nullopt;
    return bids_.front().price;
}

std::optional<Price> ReferenceMatchingEngine::best_ask() const noexcept {
    if (asks_.empty()) return std::nullopt;
    return asks_.front().price;
}

Quantity ReferenceMatchingEngine::best_bid_qty() const noexcept {
    if (bids_.empty()) return 0;
    Price p = bids_.front().price;
    Quantity sum = 0;
    for (const auto& ord : bids_) {
        if (ord.price == p) sum += ord.remaining_qty;
        else break;
    }
    return sum;
}

Quantity ReferenceMatchingEngine::best_ask_qty() const noexcept {
    if (asks_.empty()) return 0;
    Price p = asks_.front().price;
    Quantity sum = 0;
    for (const auto& ord : asks_) {
        if (ord.price == p) sum += ord.remaining_qty;
        else break;
    }
    return sum;
}

size_t ReferenceMatchingEngine::total_orders() const noexcept {
    return bids_.size() + asks_.size();
}

std::vector<Trade> ReferenceMatchingEngine::process_order(const OrderRequest& req) {
    std::vector<Trade> trades;
    if (req.quantity <= 0 || req.order_id == 0) return trades;
    if (req.type == OrderType::Limit && req.price <= 0) return trades;

    // Check duplicate ID
    for (const auto& ord : bids_) {
        if (ord.order_id == req.order_id) return trades;
    }
    for (const auto& ord : asks_) {
        if (ord.order_id == req.order_id) return trades;
    }

    Quantity remaining = req.quantity;

    if (req.side == Side::Buy) {
        // Buy matches asks
        auto it = asks_.begin();
        while (it != asks_.end() && remaining > 0) {
            if (req.type == OrderType::Limit && it->price > req.price) {
                break;
            }
            Quantity exec_qty = std::min(remaining, it->remaining_qty);
            Price exec_price = it->price;

            Trade trade;
            trade.trade_id = next_trade_id_++;
            trade.sequence = ++seq_;
            trade.timestamp = req.timestamp;
            trade.maker_order_id = it->order_id;
            trade.taker_order_id = req.order_id;
            trade.taker_side = Side::Buy;
            trade.price = exec_price;
            trade.quantity = exec_qty;
            trades.push_back(trade);

            remaining -= exec_qty;
            it->remaining_qty -= exec_qty;

            if (it->remaining_qty == 0) {
                it = asks_.erase(it);
            } else {
                ++it;
            }
        }

        if (remaining > 0 && req.type == OrderType::Limit && req.tif == TimeInForce::GTC) {
            // Insert into bids in price-time priority
            SimpleOrder resting{req.order_id, Side::Buy, req.price, remaining, req.timestamp};
            auto insert_pos = bids_.begin();
            while (insert_pos != bids_.end() && insert_pos->price >= req.price) {
                ++insert_pos;
            }
            bids_.insert(insert_pos, resting);
        }
    } else {
        // Sell matches bids
        auto it = bids_.begin();
        while (it != bids_.end() && remaining > 0) {
            if (req.type == OrderType::Limit && it->price < req.price) {
                break;
            }
            Quantity exec_qty = std::min(remaining, it->remaining_qty);
            Price exec_price = it->price;

            Trade trade;
            trade.trade_id = next_trade_id_++;
            trade.sequence = ++seq_;
            trade.timestamp = req.timestamp;
            trade.maker_order_id = it->order_id;
            trade.taker_order_id = req.order_id;
            trade.taker_side = Side::Sell;
            trade.price = exec_price;
            trade.quantity = exec_qty;
            trades.push_back(trade);

            remaining -= exec_qty;
            it->remaining_qty -= exec_qty;

            if (it->remaining_qty == 0) {
                it = bids_.erase(it);
            } else {
                ++it;
            }
        }

        if (remaining > 0 && req.type == OrderType::Limit && req.tif == TimeInForce::GTC) {
            SimpleOrder resting{req.order_id, Side::Sell, req.price, remaining, req.timestamp};
            auto insert_pos = asks_.begin();
            while (insert_pos != asks_.end() && insert_pos->price <= req.price) {
                ++insert_pos;
            }
            asks_.insert(insert_pos, resting);
        }
    }

    return trades;
}

bool ReferenceMatchingEngine::process_cancel(OrderId order_id) {
    for (auto it = bids_.begin(); it != bids_.end(); ++it) {
        if (it->order_id == order_id) {
            bids_.erase(it);
            return true;
        }
    }
    for (auto it = asks_.begin(); it != asks_.end(); ++it) {
        if (it->order_id == order_id) {
            asks_.erase(it);
            return true;
        }
    }
    return false;
}

void ReferenceMatchingEngine::reset() {
    bids_.clear();
    asks_.clear();
    next_trade_id_ = 1;
    seq_ = 0;
}

} // namespace lob
