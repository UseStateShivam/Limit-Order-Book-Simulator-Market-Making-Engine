#pragma once

#include "lob/core/types.hpp"
#include "lob/core/error.hpp"
#include <vector>

namespace lob {

class OrderArena {
public:
    explicit OrderArena(size_t initial_capacity = 65536) {
        records_.reserve(initial_capacity);
        free_indices_.reserve(initial_capacity / 2);
    }

    void reserve(size_t capacity) {
        records_.reserve(capacity);
    }

    [[nodiscard]] OrderHandle allocate_order() {
        uint32_t idx = kInvalidIndex;
        if (!free_indices_.empty()) {
            idx = free_indices_.back();
            free_indices_.pop_back();
            auto& rec = records_[idx];
            rec.active = true;
            rec.prev_idx = kInvalidIndex;
            rec.next_idx = kInvalidIndex;
            return OrderHandle{idx, rec.generation};
        }

        idx = static_cast<uint32_t>(records_.size());
        records_.emplace_back();
        auto& rec = records_.back();
        rec.active = true;
        rec.generation = 1;
        rec.prev_idx = kInvalidIndex;
        rec.next_idx = kInvalidIndex;
        return OrderHandle{idx, rec.generation};
    }

    void release_order(uint32_t index) {
        if (index >= records_.size()) {
            throw LobException(ErrorCode::CorruptArenaState, "Invalid arena index during release");
        }
        auto& rec = records_[index];
        rec.active = false;
        rec.generation++; // Invalidate existing handles to this slot
        if (rec.generation == 0) rec.generation = 1; // Prevent overflow to zero
        rec.prev_idx = kInvalidIndex;
        rec.next_idx = kInvalidIndex;
        free_indices_.push_back(index);
    }

    [[nodiscard]] bool is_valid_handle(OrderHandle handle) const noexcept {
        if (handle.index >= records_.size()) return false;
        const auto& rec = records_[handle.index];
        return rec.active && (rec.generation == handle.generation);
    }

    [[nodiscard]] OrderRecord& get(OrderHandle handle) {
        if (!is_valid_handle(handle)) {
            throw LobException(ErrorCode::CorruptArenaState, "Stale or invalid order handle accessed");
        }
        return records_[handle.index];
    }

    [[nodiscard]] const OrderRecord& get(OrderHandle handle) const {
        if (!is_valid_handle(handle)) {
            throw LobException(ErrorCode::CorruptArenaState, "Stale or invalid order handle accessed");
        }
        return records_[handle.index];
    }

    [[nodiscard]] OrderRecord& get_by_index(uint32_t index) {
        return records_[index];
    }

    [[nodiscard]] const OrderRecord& get_by_index(uint32_t index) const {
        return records_[index];
    }

    [[nodiscard]] size_t capacity() const noexcept { return records_.capacity(); }
    [[nodiscard]] size_t size() const noexcept { return records_.size() - free_indices_.size(); }
    [[nodiscard]] size_t total_slots() const noexcept { return records_.size(); }
    [[nodiscard]] size_t free_slots() const noexcept { return free_indices_.size(); }

    void clear() {
        records_.clear();
        free_indices_.clear();
    }

private:
    std::vector<OrderRecord> records_;
    std::vector<uint32_t> free_indices_;
};

} // namespace lob
