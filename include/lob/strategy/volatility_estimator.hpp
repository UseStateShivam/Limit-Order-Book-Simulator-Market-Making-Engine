#pragma once

#include <vector>
#include <deque>
#include <cmath>
#include <cstddef>

namespace lob {

class RollingVolatilityEstimator {
public:
    explicit RollingVolatilityEstimator(size_t window_size = 50, double default_volatility = 0.01)
        : window_size_(window_size), default_volatility_(default_volatility) {}

    void add_price(double price) {
        if (price <= 0.0) return;
        if (has_last_price_) {
            double ret = std::log(price / last_price_);
            returns_.push_back(ret);
            if (returns_.size() > window_size_) {
                returns_.pop_front();
            }
        }
        last_price_ = price;
        has_last_price_ = true;
    }

    [[nodiscard]] double estimate() const noexcept {
        if (returns_.size() < 2) {
            return default_volatility_;
        }

        double sum = 0.0;
        for (double r : returns_) sum += r;
        double mean = sum / static_cast<double>(returns_.size());

        double var_sum = 0.0;
        for (double r : returns_) {
            double diff = r - mean;
            var_sum += diff * diff;
        }

        double variance = var_sum / static_cast<double>(returns_.size() - 1);
        double vol = std::sqrt(variance);
        return vol > 1e-6 ? vol : default_volatility_;
    }

    [[nodiscard]] size_t observation_count() const noexcept {
        return returns_.size();
    }

    [[nodiscard]] bool is_ready() const noexcept {
        return returns_.size() >= window_size_;
    }

    void reset() {
        returns_.clear();
        has_last_price_ = false;
        last_price_ = 0.0;
    }

private:
    size_t window_size_{50};
    double default_volatility_{0.01};
    double last_price_{0.0};
    bool has_last_price_{false};
    std::deque<double> returns_;
};

} // namespace lob
