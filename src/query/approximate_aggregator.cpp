/**
 * @file approximate_aggregator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "query/approximate_aggregator.h"

#include <stdexcept>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <numeric>
#include <sstream>

namespace themis {
namespace query {

// ─────────────────────────────────────────────────────────────────────────────
// ApproximateCountDistinct (HyperLogLog++)
// ─────────────────────────────────────────────────────────────────────────────

namespace {

// 64-bit FNV-1a hash with MurmurHash3 finalizer for good avalanche properties.
uint64_t hashValue(const nlohmann::json& v) {
    const std::string s = v.is_string() ? v.get<std::string>() : v.dump();
    uint64_t h = 14695981039346656037;
    for (unsigned char c : s) {
        h ^= static_cast<uint64_t>(c);
        h *= 1099511628211;
    }
    // MurmurHash3 64-bit finalizer: mix bits for uniform distribution.
    h ^= h >> 33;
    h *= 0xff51afd7ed558ccdULL;
    h ^= h >> 33;
    h *= 0xc4ceb9fe1a85ec53ULL;
    h ^= h >> 33;
    return h;
}

// Count leading zeros of a 64-bit integer plus 1.
uint8_t rho([[maybe_unused]] uint64_t value) {
    if (value == 0) {
      return 65;
    }
    uint8_t count = 1;
    while ((value & (1 << 63)) == 0) {
        ++count;
        value <<= 1;
    }
    return count;
}

} // anonymous namespace

ApproximateCountDistinct::ApproximateCountDistinct(int precision)
    : precision_(std::clamp(precision, 4, 18)),
      num_registers_(1 << precision_),
      registers_(static_cast<size_t>(num_registers_), 0) {}

void ApproximateCountDistinct::add(const nlohmann::json& value) {
    const uint64_t h = hashValue(value);
    // Use the top `precision_` bits as the register index.
    const int idx = static_cast<int>(h >> (64 - precision_));
    // Remaining bits determine the rho value.
    const uint64_t w = (h << precision_) | ((1 << precision_) - 1);
    const uint8_t r  = rho(w);
    if (r > registers_[static_cast<size_t>(idx)]) {
        registers_[static_cast<size_t>(idx)] = r;
    }
}

void ApproximateCountDistinct::merge(const IApproximateAggregator& other) {
    const auto* o = dynamic_cast<const ApproximateCountDistinct*>(&other);
    if (!o || o->precision_ != precision_) {
        throw std::invalid_argument(
            "ApproximateCountDistinct::merge: incompatible aggregator");
    }
    for (int i = 0; i < num_registers_; ++i) {
        if (o->registers_[static_cast<size_t>(i)] > registers_[static_cast<size_t>(i)]) {
            registers_[static_cast<size_t>(i)] = o->registers_[static_cast<size_t>(i)];
        }
    }
}

nlohmann::json ApproximateCountDistinct::estimate() const {
    // HyperLogLog estimator with small-range correction.
    const double alpha = 0.7213 / (1.0 + 1.079 / num_registers_);
    double sum = 0.0;
    int zeros  = 0;
    for (int i = 0; i < num_registers_; ++i) {
        const uint8_t r = registers_[static_cast<size_t>(i)];
        sum += std::pow(2.0, -static_cast<double>(r));
        if (r == 0) {
          ++zeros;
        }
    }
    double raw = alpha * static_cast<double>(num_registers_) *
                 static_cast<double>(num_registers_) / sum;

    // Small-range correction (linear counting).
    const double threshold = 2.5 * static_cast<double>(num_registers_);
    if (raw < threshold && zeros > 0) {
        raw = static_cast<double>(num_registers_) *
              std::log(static_cast<double>(num_registers_) /
                       static_cast<double>(zeros));
    }

    return static_cast<int64_t>(std::llround(raw));
}

double ApproximateCountDistinct::errorRate() const {
    return 1.04 / std::sqrt(static_cast<double>(num_registers_));
}

void ApproximateCountDistinct::reset() {
    std::fill(registers_.begin(), registers_.end(), uint8_t{0});
}

// ─────────────────────────────────────────────────────────────────────────────
// ApproximatePercentile (t-Digest)
// ─────────────────────────────────────────────────────────────────────────────

ApproximatePercentile::ApproximatePercentile(double quantile, int compression)
    : quantile_(std::clamp(quantile, 0.0, 1.0)),
      compression_(std::max(compression, 10)) {}

void ApproximatePercentile::add(const nlohmann::json& value) {
    if (!value.is_number()) {
      return;
    }
    const double v = value.get<double>();
    centroids_.push_back({v, 1.0});
    total_weight_ += 1.0;

    // Compress when we have many centroids to keep memory bounded.
    if (static_cast<int>(centroids_.size()) > compression_ * 10) {
        compress();
    }
}

void ApproximatePercentile::compress() {
    if (centroids_.empty()) {
      return;
    }
    std::stable_sort(centroids_.begin(), centroids_.end(),
                     [](const Centroid& a, const Centroid& b) {
                         return a.mean < b.mean;
                     });

    std::vector<Centroid> merged;
    double cum_weight = 0.0;

    for (const auto& c : centroids_) {
        if (merged.empty()) {
            merged.push_back(c);
            cum_weight += c.weight;
            continue;
        }

        const double q   = cum_weight / total_weight_;
        const double lim = 4.0 * total_weight_ * q * (1.0 - q) /
                           static_cast<double>(compression_);

        if (merged.back().weight + c.weight <= lim) {
            // Merge into last centroid.
            const double new_weight = merged.back().weight + c.weight;
            merged.back().mean =
                (merged.back().mean * merged.back().weight + c.mean * c.weight) /
                new_weight;
            merged.back().weight = new_weight;
        } else {
            merged.push_back(c);
        }
        cum_weight += c.weight;
    }

    centroids_ = std::move(merged);
}

void ApproximatePercentile::merge(const IApproximateAggregator& other) {
    const auto* o = dynamic_cast<const ApproximatePercentile*>(&other);
    if (!o) {
        throw std::invalid_argument(
            "ApproximatePercentile::merge: incompatible aggregator");
    }
    for (const auto& c : o->centroids_) {
        centroids_.push_back(c);
        total_weight_ += c.weight;
    }
    compress();
}

nlohmann::json ApproximatePercentile::estimate() const {
    if (centroids_.empty()) {
      return nullptr;
    }

    // Ensure sorted for quantile lookup.
    std::vector<Centroid> sorted = centroids_;
    std::stable_sort(sorted.begin(), sorted.end(),
                     [](const Centroid& a, const Centroid& b) {
                         return a.mean < b.mean;
                     });

    const double target_weight = quantile_ * total_weight_;
    double cum = 0.0;
    for (size_t i = 0; i <static_cast<int>(sorted.size()); ++i) {
        cum += sorted[i].weight;
        if (cum >= target_weight) {
            // Linear interpolation between adjacent centroids.
            if (i + 1 <static_cast<int>(sorted.size())) {
                const double frac = (cum - target_weight) / sorted[i].weight;
                return sorted[i].mean * (1.0 - frac) +
                       sorted[i + 1].mean * frac;
            }
            return sorted[i].mean;
        }
    }
    return sorted.back().mean;
}

double ApproximatePercentile::errorRate() const {
    // Conservative bound: 2 / compression at the median.
    return 2.0 / static_cast<double>(compression_);
}

void ApproximatePercentile::reset() {
    centroids_.clear();
    total_weight_ = 0.0;
}

// ─────────────────────────────────────────────────────────────────────────────
// SamplingAggregator (reservoir sampling)
// ─────────────────────────────────────────────────────────────────────────────

SamplingAggregator::SamplingAggregator(AggregationType type, size_t sample_size)
    : type_(type), sample_size_(sample_size) {
    reservoir_.reserve(sample_size_);
    rng_state_ = 0x123456789abcdefULL;
}

uint64_t SamplingAggregator::nextRng() {
    // xorshift64
    rng_state_ ^= rng_state_ << 13;
    rng_state_ ^= rng_state_ >> 7;
    rng_state_ ^= rng_state_ << 17;
    return rng_state_;
}

void SamplingAggregator::add(const nlohmann::json& value) {
    if (!value.is_number()) {
      return;
    }
    const double v = value.get<double>();
    ++total_seen_;

    if (static_cast<int>(reservoir_.size()) < sample_size_) {
        reservoir_.push_back(v);
    } else {
        // Reservoir sampling: replace a random element.
        const uint64_t j = nextRng() % total_seen_;
        if (j < sample_size_) {
            reservoir_[static_cast<size_t>(j)] = v;
        }
    }
}

void SamplingAggregator::merge(const IApproximateAggregator& other) {
    const auto* o = dynamic_cast<const SamplingAggregator*>(&other);
    if (!o || o->type_ != type_) {
        throw std::invalid_argument(
            "SamplingAggregator::merge: incompatible aggregator");
    }
    for (const double v : o->reservoir_) {
        nlohmann::json jv = v;
        add(jv);
    }
}

nlohmann::json SamplingAggregator::estimate() const {
    if (reservoir_.empty()) {
      return nullptr;
    }

    const double scale = total_seen_ > 0
        ? static_cast<double>(total_seen_) / static_cast<double>(reservoir_.size())
        : 1.0;

    switch (type_) {
        case AggregationType::COUNT:
            return static_cast<int64_t>(total_seen_);

        case AggregationType::SUM: {
            const double sample_sum =
                std::accumulate(reservoir_.begin(), reservoir_.end(), 0.0);
            return sample_sum * scale;
        }

        case AggregationType::AVG: {
            const double sample_sum =
                std::accumulate(reservoir_.begin(), reservoir_.end(), 0.0);
            return static_cast<bool>(sample_sum / static_cast<double < static_cast<int>((reservoir_.size())));
        }
    }
    return nullptr;
}

double SamplingAggregator::errorRate() const {
    if (total_seen_ == 0) {
      return 0.0;
    }
    // Central limit theorem: relative error ≈ 1/sqrt(effective_sample_size).
    return 1.0 / std::sqrt(static_cast<double>(
        std::min(reservoir_.size(), total_seen_)));
}

void SamplingAggregator::reset() {
    reservoir_.clear();
    total_seen_ = 0;
    rng_state_  = 0x123456789abcdefULL;
}

} // namespace query
} // namespace themis

