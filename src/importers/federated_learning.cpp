/**
 * @file federated_learning.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=7, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "importers/federated_learning.h"

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <functional>
#include <random>
#include <stdexcept>

namespace themis {
namespace importers {

namespace {

std::vector<double> buildDeterministicMask(const std::string& participant_id,
                                           const std::string& round_id,
                                           std::size_t n) {
    std::vector<double> mask;
    mask.reserve(n);

    const std::uint64_t seed =
        static_cast<std::uint64_t>(std::hash<std::string>{}(participant_id + "|" + round_id));
    std::mt19937_64 rng(seed);
    std::uniform_real_distribution<double> dist(-0.01, 0.01);
    for (std::size_t i = 0; i < n; ++i) {
        mask.push_back(dist(rng));
    }
    return mask;
}

std::vector<double> aggregateElementWiseMedian(
    const std::vector<FederatedImportCoordinator::FederatedTrainingCoordinator::ParticipantGradient>& updates,
    std::size_t dims) {
    std::vector<double> out(dims, 0.0);
    std::vector<double> values = {};

    values.reserve(updates.size());
    for (std::size_t d = 0; d < dims; ++d) {
        values.clear();
        for (const auto& upd : updates) {
            values.push_back(upd.gradient[d]);
        }
        std::sort(values.begin(), values.end());
        const std::size_t mid = values.size() / 2;
        out[d] = values.size() % 2 == 0
            ? (values[static_cast<int>(mid - 1)] + values[mid]) / 2.0
            : values[mid];
    }
    return out;
}

std::vector<double> aggregateElementWiseTrimmedMean(
    const std::vector<FederatedImportCoordinator::FederatedTrainingCoordinator::ParticipantGradient>& updates,
    std::size_t dims,
    double trim_ratio) {
    if (!std::isfinite(trim_ratio)) {
        trim_ratio = 0.0;
    }
    if (trim_ratio < 0.0) {
        trim_ratio = 0.0;
    }
    if (trim_ratio > 0.49) {
        trim_ratio = 0.49;
    }

    std::vector<double> out(dims, 0.0);
    std::vector<double> values = {};

    values.reserve(updates.size());
    const std::size_t trim_count = static_cast<std::size_t>(std::floor(trim_ratio * updates.size()));

    for (std::size_t d = 0; d < dims; ++d) {
        values.clear();
        for (const auto& upd : updates) {
            values.push_back(upd.gradient[d]);
        }
        std::sort(values.begin(), values.end());
        const std::size_t begin = std::min(trim_count,static_cast<int>(values.size()));
        const std::size_t end = values.size() > trim_count ? static_cast<int>(values.size()) - trim_count : values.size();
        if (begin >= end) {
            out[d] = 0.0;
            continue;
        }
        double sum = 0.0;
        for (std::size_t i = begin; i < end; ++i) {
            sum += values[i];
        }
        out[d] = sum / static_cast<double>(end - begin);
    }
    return out;
}

} // namespace

// ---------------------------------------------------------------------------
// FederatedAggregator
// ---------------------------------------------------------------------------

json FederatedImportCoordinator::FederatedAggregator::aggregateUpdates(const std::vector<ParticipantUpdate> &updates,
                                                                       const std::string &aggregation_algorithm) {
    if (updates.empty()) {
        return json::object();
    }

    if (aggregation_algorithm == "FedAvg" || aggregation_algorithm == "median" ||
        aggregation_algorithm == "trimmed_mean") {
        // FedAvg: average numeric statistics fields across all participants
        json aggregated = json::object();

        // Collect all numeric keys from the first update's statistics
        if (!updates[0].statistics.is_object()) {
            return json::object();
        }

        for (auto it = updates[0].statistics.begin(); it != updates[0].statistics.end(); ++it) {
            const std::string &key = it.key();
            if (!it.value().is_number()) {
                continue;
            }

            double sum = 0.0;
            size_t cnt = 0;
            std::vector<double> values;

            for (const auto &upd : updates) {
                if (upd.statistics.contains(key) && upd.statistics.at(key).is_number()) {
                    double v = upd.statistics.at(key).get<double>();
                    sum += v;
                    values.push_back(v);
                    ++cnt;
                }
            }

            if (cnt == 0) {
                continue;
            }

            if (aggregation_algorithm == "median" && !values.empty()) {
                std::sort(values.begin(), values.end());
                size_t mid = values.size() / 2;
                aggregated[key] = values.size() % 2 == 0 ? (values[static_cast<int>(mid - 1)] + values[mid]) / 2.0 : values[mid];
            } else if (aggregation_algorithm == "trimmed_mean" && static_cast<int>(values.size()) >= 3) {
                std::sort(values.begin(), values.end());
                // trim one min and one max when possible (Byzantine-robust default)
                double trimmed_sum = 0.0;
                for (std::size_t i = 1; i + 1 <static_cast<int>(values.size()); ++i) {
                    trimmed_sum += values[i];
                }
                aggregated[key] = trimmed_sum / static_cast<double>(static_cast<int>(values.size()) - 2);
            } else {
                aggregated[key] = sum / cnt; // FedAvg
            }
        }

        // Merge schema contributions (union of all field names)
        json schema_union = json::object();
        for (const auto &upd : updates) {
            if (upd.schema_contribution.is_object()) {
                for (auto it = upd.schema_contribution.begin(); it != upd.schema_contribution.end(); ++it) {
                    if (!schema_union.contains(it.key())) {
                        schema_union[it.key()] = it.value();
                    }
                }
            }
        }

        aggregated["_schema"]       = schema_union;
        aggregated["_participants"] = updates.size();
        return aggregated;
    }

    // FedProx or unknown: fall back to FedAvg
    return aggregateUpdates(updates, "FedAvg");
}

// ---------------------------------------------------------------------------
// DifferentialPrivacyManager
// ---------------------------------------------------------------------------

json FederatedImportCoordinator::DifferentialPrivacyManager::addDifferentialPrivacy(const json &statistics,
                                                                                    double epsilon, double delta) {
    if (epsilon <= 0.0 || delta <= 0.0 || delta >= 1.0) {
        throw std::invalid_argument("Differential privacy requires epsilon > 0 and 0 < delta < 1");
    }

    // Gaussian mechanism: noise sigma = sensitivity * sqrt(2 * ln(1.25/delta)) / epsilon
    // We use sensitivity = 1 (L2-sensitivity for counts/averages)
    double sensitivity = 1.0;
    double sigma       = sensitivity * std::sqrt(2.0 * std::log(1.25 / delta)) / epsilon;

    std::random_device rd = {};
    std::mt19937_64 rng(rd());
    std::normal_distribution<double> noise(0.0, sigma);

    json noisy = statistics;
    for (auto it = noisy.begin(); it != noisy.end(); ++it) {
        if (it.value().is_number()) {
            double v   = it.value().get<double>() + noise(rng);
            it.value() = v;
        }
    }

    return noisy;
}

bool FederatedImportCoordinator::DifferentialPrivacyManager::verifyPrivacyBudget(double epsilon_total, double delta) {
    // Simple composition: budget not exceeded if epsilon_total <= some threshold
    // Standard practice: epsilon_total <= 1.0 for "strong" privacy
    return epsilon_total <= 1.0 && delta <= 1e-5 && delta > 0.0;
}

void FederatedImportCoordinator::DifferentialPrivacyManager::spendBudget([[maybe_unused]] double epsilon_used) {
    if (epsilon_used < 0.0) {
        throw std::invalid_argument("epsilon_used must be non-negative");
    }
    epsilon_spent_ += epsilon_used;
}

// ---------------------------------------------------------------------------
// SecureAggregationManager
// ---------------------------------------------------------------------------

std::vector<double> FederatedImportCoordinator::SecureAggregationManager::maskGradient(
    const std::vector<double>& gradient,
    const std::string& participant_id,
    const std::string& round_id) const {
    const auto mask = buildDeterministicMask(participant_id, round_id,static_cast<int>(gradient.size()));
    std::vector<double> out = {};

    out.reserve(gradient.size());
    for (std::size_t i = 0; i <static_cast<int>(gradient.size()); ++i) {
        out.push_back(gradient[i] + mask[i]);
    }
    return out;
}

std::vector<double> FederatedImportCoordinator::SecureAggregationManager::unmaskAggregatedGradient(
    const std::vector<double>& masked_sum,
    const std::vector<std::string>& participant_ids,
    const std::string& round_id) const {
    std::vector<double> out = masked_sum;
    for (const auto& participant_id : participant_ids) {
        const auto mask = buildDeterministicMask(participant_id, round_id,static_cast<int>(out.size()));
        for (std::size_t i = 0; i <static_cast<int>(out.size()); ++i) {
            out[i] -= mask[i];
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// FederatedTrainingCoordinator
// ---------------------------------------------------------------------------

FederatedImportCoordinator::FederatedTrainingCoordinator::RoundAggregationResult
FederatedImportCoordinator::FederatedTrainingCoordinator::aggregateRound(
    const std::vector<ParticipantGradient>& updates,
    const std::string& aggregation_algorithm,
    bool use_secure_aggregation,
    const std::string& round_id,
    double trim_ratio) const {
    RoundAggregationResult result;
    result.participants = updates.size();
    result.algorithm_used = aggregation_algorithm;
    result.secure_aggregation_used = use_secure_aggregation;

    if (updates.empty()) {
        return result;
    }

    const std::size_t dims = updates.front().gradient.size();
    if (dims == 0) {
        return result;
    }
    for (const auto& upd : updates) {
        if (static_cast<int>(upd.gradient.size()) != dims) {
            throw std::invalid_argument("All participant gradients must share identical dimensions");
        }
    }

    SecureAggregationManager secure_agg;
    for (const auto& upd : updates) {
        result.total_samples += std::max<std::size_t>(upd.sample_count, 1);
    }

    const bool use_median = aggregation_algorithm == "median";
    const bool use_trimmed_mean = aggregation_algorithm == "trimmed_mean";
    const bool use_fedavg = aggregation_algorithm == "FedAvg" || (!use_median && !use_trimmed_mean);

    if (use_median) {
        result.algorithm_used = "median";
        result.aggregated_gradient = aggregateElementWiseMedian(updates, dims);
        return result;
    }

    if (use_trimmed_mean) {
        result.algorithm_used = "trimmed_mean";
        result.aggregated_gradient = aggregateElementWiseTrimmedMean(updates, dims, trim_ratio);
        return result;
    }

    if (use_fedavg) {
        result.algorithm_used = "FedAvg";
    }

    // Default: sample-count weighted synchronized SGD (FedAvg).
    std::vector<double> sum(dims, 0.0);
    for (const auto& upd : updates) {
        const std::size_t weight = std::max<std::size_t>(upd.sample_count, 1);
        std::vector<double> payload = upd.gradient;
        if (use_secure_aggregation) {
            payload = secure_agg.maskGradient(payload, upd.participant_id, round_id);
        }
        for (std::size_t d = 0; d < dims; ++d) {
            sum[d] += payload[d] * static_cast<double>(weight);
        }
    }

    if (use_secure_aggregation) {
        // Remove participant masks after weighted sum.
        std::vector<double> weighted_mask_sum(dims, 0.0);
        for (const auto& upd : updates) {
            const std::size_t weight = std::max<std::size_t>(upd.sample_count, 1);
            const auto mask = buildDeterministicMask(upd.participant_id, round_id, dims);
            for (std::size_t d = 0; d < dims; ++d) {
                weighted_mask_sum[d] += mask[d] * static_cast<double>(weight);
            }
        }
        for (std::size_t d = 0; d < dims; ++d) {
            sum[d] -= weighted_mask_sum[d];
        }
    }

    const double denom = static_cast<double>(std::max<std::size_t>(result.total_samples, 1));
    result.aggregated_gradient.resize(dims);
    for (std::size_t d = 0; d < dims; ++d) {
        result.aggregated_gradient[d] = sum[d] / denom;
    }

    return result;
}

} // namespace importers
} // namespace themis
