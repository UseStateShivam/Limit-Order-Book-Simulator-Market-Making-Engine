#include "lob/book/order_book.hpp"

namespace lob {

OrderBook::OrderBook(size_t arena_capacity)
    : arena_(arena_capacity) {
    order_index_.reserve(arena_capacity);
}

std::optional<Price> OrderBook::best_bid() const noexcept {
    if (bids_.empty()) return std::nullopt;
    return bids_.begin()->first;
}

std::optional<Price> OrderBook::best_ask() const noexcept {
    if (asks_.empty()) return std::nullopt;
    return asks_.begin()->first;
}

Quantity OrderBook::best_bid_qty() const noexcept {
    if (bids_.empty()) return 0;
    return bids_.begin()->second.aggregate_qty;
}

Quantity OrderBook::best_ask_qty() const noexcept {
    if (asks_.empty()) return 0;
    return asks_.begin()->second.aggregate_qty;
}

std::optional<double> OrderBook::mid_price() const noexcept {
    if (bids_.empty() || asks_.empty()) return std::nullopt;
    return static_cast<double>(bids_.begin()->first + asks_.begin()->first) / 2.0;
}

std::optional<Price> OrderBook::spread_ticks() const noexcept {
    if (bids_.empty() || asks_.empty()) return std::nullopt;
    return asks_.begin()->first - bids_.begin()->first;
}

Quantity OrderBook::depth_at_price(Side side, Price price) const noexcept {
    if (side == Side::Buy) {
        auto it = bids_.find(price);
        return it != bids_.end() ? it->second.aggregate_qty : 0;
    } else {
        auto it = asks_.find(price);
        return it != asks_.end() ? it->second.aggregate_qty : 0;
    }
}

Quantity OrderBook::total_bid_quantity(size_t max_depth) const noexcept {
    Quantity total = 0;
    size_t count = 0;
    for (const auto& [price, level] : bids_) {
        total += level.aggregate_qty;
        if (max_depth > 0 && ++count >= max_depth) break;
    }
    return total;
}

Quantity OrderBook::total_ask_quantity(size_t max_depth) const noexcept {
    Quantity total = 0;
    size_t count = 0;
    for (const auto& [price, level] : asks_) {
        total += level.aggregate_qty;
        if (max_depth > 0 && ++count >= max_depth) break;
    }
    return total;
}

L2Snapshot OrderBook::get_l2_snapshot(size_t max_depth, Timestamp ts, SequenceNumber seq) const {
    L2Snapshot snap;
    snap.timestamp = ts;
    snap.sequence = seq;
    snap.bids.reserve(max_depth > 0 ? max_depth : bids_.size());
    snap.asks.reserve(max_depth > 0 ? max_depth : asks_.size());

    size_t b_count = 0;
    for (const auto& [price, level] : bids_) {
        snap.bids.push_back({price, level.aggregate_qty, level.order_count});
        if (max_depth > 0 && ++b_count >= max_depth) break;
    }

    size_t a_count = 0;
    for (const auto& [price, level] : asks_) {
        snap.asks.push_back({price, level.aggregate_qty, level.order_count});
        if (max_depth > 0 && ++a_count >= max_depth) break;
    }

    return snap;
}

L3Snapshot OrderBook::get_l3_snapshot(size_t max_depth, Timestamp ts, SequenceNumber seq) const {
    L3Snapshot snap;
    snap.timestamp = ts;
    snap.sequence = seq;

    size_t b_level = 0;
    for (const auto& [price, level] : bids_) {
        uint32_t curr = level.head_order_idx;
        uint32_t pos = 1;
        while (curr != kInvalidIndex) {
            const auto& ord = arena_.get_by_index(curr);
            snap.bids.push_back({ord.order_id, ord.price, ord.remaining_qty, pos++, ord.timestamp});
            curr = ord.next_idx;
        }
        if (max_depth > 0 && ++b_level >= max_depth) break;
    }

    size_t a_level = 0;
    for (const auto& [price, level] : asks_) {
        uint32_t curr = level.head_order_idx;
        uint32_t pos = 1;
        while (curr != kInvalidIndex) {
            const auto& ord = arena_.get_by_index(curr);
            snap.asks.push_back({ord.order_id, ord.price, ord.remaining_qty, pos++, ord.timestamp});
            curr = ord.next_idx;
        }
        if (max_depth > 0 && ++a_level >= max_depth) break;
    }

    return snap;
}

bool OrderBook::contains_order(OrderId order_id) const noexcept {
    return order_index_.find(order_id) != order_index_.end();
}

const OrderRecord* OrderBook::find_order(OrderId order_id) const noexcept {
    auto it = order_index_.find(order_id);
    if (it == order_index_.end()) return nullptr;
    if (!arena_.is_valid_handle(it->second)) return nullptr;
    return &arena_.get(it->second);
}

size_t OrderBook::total_orders() const noexcept {
    return order_index_.size();
}

size_t OrderBook::bid_level_count() const noexcept {
    return bids_.size();
}

size_t OrderBook::ask_level_count() const noexcept {
    return asks_.size();
}

OrderHandle OrderBook::add_resting_order(const OrderRequest& req, SequenceNumber seq) {
    if (contains_order(req.order_id)) {
        throw LobException(ErrorCode::DuplicateOrderId, "Order ID already exists in book");
    }

    OrderHandle handle = arena_.allocate_order();
    auto& rec = arena_.get(handle);
    rec.order_id = req.order_id;
    rec.side = req.side;
    rec.price = req.price;
    rec.original_qty = req.quantity;
    rec.remaining_qty = req.quantity;
    rec.sequence = seq;
    rec.timestamp = req.timestamp;
    rec.type = req.type;
    rec.tif = req.tif;
    rec.status = OrderStatus::Accepted;

    if (req.side == Side::Buy) {
        auto [it, inserted] = bids_.try_emplace(req.price, req.price);
        it->second.append_order(arena_, handle.index);
    } else {
        auto [it, inserted] = asks_.try_emplace(req.price, req.price);
        it->second.append_order(arena_, handle.index);
    }

    order_index_.emplace(req.order_id, handle);
    return handle;
}

OrderRecord OrderBook::remove_order(OrderId order_id) {
    auto it = order_index_.find(order_id);
    if (it == order_index_.end()) {
        throw LobException(ErrorCode::OrderNotFound, "Order ID not found for removal");
    }

    OrderHandle handle = it->second;
    OrderRecord rec = arena_.get(handle);

    if (rec.side == Side::Buy) {
        auto level_it = bids_.find(rec.price);
        if (level_it != bids_.end()) {
            level_it->second.remove_order(arena_, handle.index);
            if (level_it->second.empty()) {
                bids_.erase(level_it);
            }
        }
    } else {
        auto level_it = asks_.find(rec.price);
        if (level_it != asks_.end()) {
            level_it->second.remove_order(arena_, handle.index);
            if (level_it->second.empty()) {
                asks_.erase(level_it);
            }
        }
    }

    order_index_.erase(it);
    arena_.release_order(handle.index);
    return rec;
}

bool OrderBook::modify_order_quantity(OrderId order_id, Quantity new_qty) {
    auto it = order_index_.find(order_id);
    if (it == order_index_.end()) return false;

    OrderHandle handle = it->second;
    auto& rec = arena_.get(handle);

    if (new_qty <= 0) {
        remove_order(order_id);
        return true;
    }

    if (new_qty < rec.remaining_qty) {
        // Downward modification: preserve queue time priority
        Quantity reduction = rec.remaining_qty - new_qty;
        if (rec.side == Side::Buy) {
            auto level_it = bids_.find(rec.price);
            if (level_it != bids_.end()) {
                level_it->second.reduce_order_qty(arena_, handle.index, reduction);
            }
        } else {
            auto level_it = asks_.find(rec.price);
            if (level_it != asks_.end()) {
                level_it->second.reduce_order_qty(arena_, handle.index, reduction);
            }
        }
        return true;
    } else if (new_qty > rec.remaining_qty) {
        // Upward modification: loses queue time priority (move to tail of price level)
        Quantity addition = new_qty - rec.remaining_qty;
        if (rec.side == Side::Buy) {
            auto level_it = bids_.find(rec.price);
            if (level_it != bids_.end()) {
                level_it->second.remove_order(arena_, handle.index);
                rec.remaining_qty = new_qty;
                rec.original_qty += addition;
                level_it->second.append_order(arena_, handle.index);
            }
        } else {
            auto level_it = asks_.find(rec.price);
            if (level_it != asks_.end()) {
                level_it->second.remove_order(arena_, handle.index);
                rec.remaining_qty = new_qty;
                rec.original_qty += addition;
                level_it->second.append_order(arena_, handle.index);
            }
        }
        return true;
    }

    return true; // No change
}

void OrderBook::reduce_order_quantity(OrderId order_id, Quantity executed_qty) {
    auto it = order_index_.find(order_id);
    if (it == order_index_.end()) {
        throw LobException(ErrorCode::OrderNotFound, "Order ID not found for fill reduction");
    }

    OrderHandle handle = it->second;
    auto& rec = arena_.get(handle);

    if (executed_qty >= rec.remaining_qty) {
        remove_order(order_id);
    } else {
        if (rec.side == Side::Buy) {
            auto level_it = bids_.find(rec.price);
            if (level_it != bids_.end()) {
                level_it->second.reduce_order_qty(arena_, handle.index, executed_qty);
            }
        } else {
            auto level_it = asks_.find(rec.price);
            if (level_it != asks_.end()) {
                level_it->second.reduce_order_qty(arena_, handle.index, executed_qty);
            }
        }
    }
}

void OrderBook::verify_invariants() const {
    // 1. Crossed book check
    if (!bids_.empty() && !asks_.empty()) {
        if (bids_.begin()->first >= asks_.begin()->first) {
            throw LobException(ErrorCode::CrossedBookDetected,
                "Invariant violated: Best bid >= Best ask (crossed book)");
        }
    }

    // 2. Validate all bid levels
    size_t total_orders_in_levels = 0;
    for (const auto& [price, level] : bids_) {
        if (level.empty()) {
            throw LobException(ErrorCode::InvariantViolation, "Empty bid level present in map");
        }
        Quantity sum_qty = 0;
        uint32_t count = 0;
        uint32_t curr = level.head_order_idx;
        uint32_t prev = kInvalidIndex;

        while (curr != kInvalidIndex) {
            const auto& order = arena_.get_by_index(curr);
            if (!order.active) {
                throw LobException(ErrorCode::InvariantViolation, "Inactive order found in bid queue");
            }
            if (order.side != Side::Buy || order.price != price) {
                throw LobException(ErrorCode::InvariantViolation, "Order side or price mismatch in bid queue");
            }
            if (order.prev_idx != prev) {
                throw LobException(ErrorCode::InvariantViolation, "Invalid prev link in bid queue");
            }
            sum_qty += order.remaining_qty;
            count++;
            prev = curr;
            curr = order.next_idx;
        }

        if (prev != level.tail_order_idx) {
            throw LobException(ErrorCode::InvariantViolation, "Tail order index mismatch in bid queue");
        }
        if (count != level.order_count) {
            throw LobException(ErrorCode::InvariantViolation, "Order count mismatch in bid level");
        }
        if (sum_qty != level.aggregate_qty) {
            throw LobException(ErrorCode::InvariantViolation, "Aggregate quantity mismatch in bid level");
        }
        total_orders_in_levels += count;
    }

    // 3. Validate all ask levels
    for (const auto& [price, level] : asks_) {
        if (level.empty()) {
            throw LobException(ErrorCode::InvariantViolation, "Empty ask level present in map");
        }
        Quantity sum_qty = 0;
        uint32_t count = 0;
        uint32_t curr = level.head_order_idx;
        uint32_t prev = kInvalidIndex;

        while (curr != kInvalidIndex) {
            const auto& order = arena_.get_by_index(curr);
            if (!order.active) {
                throw LobException(ErrorCode::InvariantViolation, "Inactive order found in ask queue");
            }
            if (order.side != Side::Sell || order.price != price) {
                throw LobException(ErrorCode::InvariantViolation, "Order side or price mismatch in ask queue");
            }
            if (order.prev_idx != prev) {
                throw LobException(ErrorCode::InvariantViolation, "Invalid prev link in ask queue");
            }
            sum_qty += order.remaining_qty;
            count++;
            prev = curr;
            curr = order.next_idx;
        }

        if (prev != level.tail_order_idx) {
            throw LobException(ErrorCode::InvariantViolation, "Tail order index mismatch in ask queue");
        }
        if (count != level.order_count) {
            throw LobException(ErrorCode::InvariantViolation, "Order count mismatch in ask level");
        }
        if (sum_qty != level.aggregate_qty) {
            throw LobException(ErrorCode::InvariantViolation, "Aggregate quantity mismatch in ask level");
        }
        total_orders_in_levels += count;
    }

    // 4. Validate order index matches level counts
    if (order_index_.size() != total_orders_in_levels) {
        throw LobException(ErrorCode::InvariantViolation, "Order index size does not match level order count");
    }
}

void OrderBook::clear() {
    bids_.clear();
    asks_.clear();
    order_index_.clear();
    arena_.clear();
}

} // namespace lob
