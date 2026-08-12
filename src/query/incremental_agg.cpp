/**
 * @file incremental_agg.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "query/incremental_agg.h"
#include <cmath>
#include <stdexcept>

namespace themis {
namespace query {

IncrementalAgg::IncrementalAgg(AggOp op) noexcept : op_(op) {}

void IncrementalAgg::add(double value) {
    ++count_;
    switch (op_) {
        case AggOp::SUM:
        case AggOp::AVG:
            sum_ += value;
            break;
        case AggOp::MIN:
            if (value < min_) { min_ = value; }
            break;
        case AggOp::MAX:
            if (value > max_) { max_ = value; }
            break;
        case AggOp::COUNT:
            break;  // count_ already incremented above
    }
}

void IncrementalAgg::remove(double value) {
    if (count_ <= 0) { return; }
    --count_;
    switch (op_) {
        case AggOp::SUM:
        case AggOp::AVG:
            sum_ -= value;
            break;
        case AggOp::MIN:
            if (value <= min_) { rescan_needed_ = true; }
            break;
        case AggOp::MAX:
            if (value >= max_) { rescan_needed_ = true; }
            break;
        case AggOp::COUNT:
            break;
    }
}

void IncrementalAgg::rescan(const std::vector<double>& values) {
    if (op_ == AggOp::MIN || op_ == AggOp::MAX) {
        min_ = std::numeric_limits<double>::max();
        max_ = std::numeric_limits<double>::lowest();
        for (const auto v : values) {
            if (v < min_) { min_ = v; }
            if (v > max_) { max_ = v; }
        }
        rescan_needed_ = false;
    }
}

double IncrementalAgg::result() const noexcept {
    if (count_ == 0) { return 0.0; }
    switch (op_) {
        case AggOp::COUNT: return static_cast<double>(count_);
        case AggOp::SUM:   return sum_;
        case AggOp::AVG:   return sum_ / static_cast<double>(count_);
        case AggOp::MIN:   return (min_ == std::numeric_limits<double>::max()) ? 0.0 : min_;
        case AggOp::MAX:   return (max_ == std::numeric_limits<double>::lowest()) ? 0.0 : max_;
    }
    return 0.0;
}

void IncrementalAgg::reset() noexcept {
    count_         = 0;
    sum_           = 0.0;
    min_           = std::numeric_limits<double>::max();
    max_           = std::numeric_limits<double>::lowest();
    rescan_needed_ = false;
}

}  // namespace query
}  // namespace themis
