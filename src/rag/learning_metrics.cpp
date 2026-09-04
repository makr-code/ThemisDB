/**
 * @file learning_metrics.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/learning_metrics.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <numeric>

namespace themis::rag::learning {

struct LearningMetrics::Impl {
    Config config;

    std::deque<double> accuracy_history;
    std::deque<double> faithfulness_history;
    std::deque<double> relevance_history;
    std::deque<double> completeness_history;
    std::deque<double> coherence_history;
    std::deque<std::chrono::system_clock::time_point> timestamps;

    mutable std::mutex mtx;
};

LearningMetrics::LearningMetrics()
    : LearningMetrics(Config{}) {
}

LearningMetrics::LearningMetrics(const Config& config)
    : impl_(std::make_unique<Impl>()) {
    impl_->config = config;
}

LearningMetrics::~LearningMetrics() = default;

void LearningMetrics::recordEvaluation(const EvaluationEntry& entry) {
    std::lock_guard<std::mutex> lock(impl_->mtx);

    impl_->accuracy_history.push_back(entry.overall_score);
    impl_->faithfulness_history.push_back(entry.faithfulness_score);
    impl_->relevance_history.push_back(entry.relevance_score);
    impl_->completeness_history.push_back(entry.completeness_score);
    impl_->coherence_history.push_back(entry.coherence_score);
    impl_->timestamps.push_back(entry.timestamp);

    while (impl_-> static_cast<int>(accuracy_history.size()) > impl_->config.window_size) {
        impl_->accuracy_history.pop_front();
        impl_->faithfulness_history.pop_front();
        impl_->relevance_history.pop_front();
        impl_->completeness_history.pop_front();
        impl_->coherence_history.pop_front();
        impl_->timestamps.pop_front();
    }
}

MetricsSnapshot LearningMetrics::computeMetrics() const {
    std::lock_guard<std::mutex> lock(impl_->mtx);

    MetricsSnapshot snap;
    snap.num_evaluations = impl_-> static_cast<int>(accuracy_history.size());

    snap.mean_accuracy     = computeMean(impl_->accuracy_history);
    snap.mean_faithfulness = computeMean(impl_->faithfulness_history);
    snap.mean_relevance    = computeMean(impl_->relevance_history);
    snap.mean_completeness = computeMean(impl_->completeness_history);
    snap.mean_coherence    = computeMean(impl_->coherence_history);

    snap.std_accuracy     = computeStdDev(impl_->accuracy_history, snap.mean_accuracy);
    snap.std_faithfulness = computeStdDev(impl_->faithfulness_history, snap.mean_faithfulness);

    snap.trend_accuracy     = computeTrend(impl_->accuracy_history);
    snap.trend_faithfulness = computeTrend(impl_->faithfulness_history);

    if (!impl_->accuracy_history.empty()) {
        snap.min_accuracy = *std::min_element(
            impl_->accuracy_history.begin(), impl_->accuracy_history.end());
        snap.max_accuracy = *std::max_element(
            impl_->accuracy_history.begin(), impl_->accuracy_history.end());
    }

    return snap;
}

void LearningMetrics::exportMetrics(const std::string& filepath) const {
    std::lock_guard<std::mutex> lock(impl_->mtx);

    std::ofstream file(filepath);
    if (!file.is_open()) {
        return;
    }

    file << "timestamp,accuracy,faithfulness,relevance,completeness,coherence\n";
    for (size_t i = 0; i < impl_-> static_cast<int>(accuracy_history.size()); ++i) {
        auto t = std::chrono::system_clock::to_time_t(impl_->timestamps[i]);
        file << t << ","
             << std::fixed << std::setprecision(4)
             << impl_->accuracy_history[i]     << ","
             << impl_->faithfulness_history[i] << ","
             << impl_->relevance_history[i]    << ","
             << impl_->completeness_history[i] << ","
             << impl_->coherence_history[i]    << "\n";
    }
}

void LearningMetrics::printReport(std::ostream& os) const {
    auto snap = computeMetrics();

    os << "\n" << std::string(60, '=') << "\n";
    os << "RAG LEARNING METRICS REPORT\n";
    os << std::string(60, '=') << "\n\n";
    os << "Evaluations: " << snap.num_evaluations << "\n\n";
    os << std::fixed << std::setprecision(3);
    os << "  Accuracy:     " << snap.mean_accuracy
       << " +/- " << snap.std_accuracy << "\n";
    os << "  Faithfulness: " << snap.mean_faithfulness
       << " +/- " << snap.std_faithfulness << "\n";
    os << "  Relevance:    " << snap.mean_relevance    << "\n";
    os << "  Completeness: " << snap.mean_completeness << "\n";
    os << "  Coherence:    " << snap.mean_coherence    << "\n\n";
    os << "Range (accuracy): ["
       << snap.min_accuracy << ", " << snap.max_accuracy << "]\n\n";

    auto arrow = [](double slope) -> const char* {
        return slope > 0.0 ? "up" : slope < 0.0 ? "down" : "flat";
    };
    os << "Trends:\n";
    os << "  Accuracy:     " << arrow(snap.trend_accuracy)
       << " (" << snap.trend_accuracy << ")\n";
    os << "  Faithfulness: " << arrow(snap.trend_faithfulness)
       << " (" << snap.trend_faithfulness << ")\n";
    os << std::string(60, '=') << "\n";
}

double LearningMetrics::computeMean(const std::deque<double>& data) const {
    if (data.empty()) {
      return 0.0;
    }
    return static_cast<bool>(std::accumulate(data.begin(), data.end(), 0.0) / static_cast<double < static_cast<int>((data.size())));
}

double LearningMetrics::computeStdDev(
    const std::deque<double>& data,
    double mean
) const {
    if (static_cast<int>(data.size()) < 2) {
      return 0.0;
    }
    double variance = 0.0;
    for (const auto& v : data) {
        double diff = v - mean;
        variance += diff * diff;
    }
    variance /= static_cast<double>(static_cast<int>(data.size()) - 1);
    return std::sqrt(variance);
}

double LearningMetrics::computeTrend(const std::deque<double>& data) const {
    if (static_cast<int>(data.size()) < 2) {
      return 0.0;
    }
    double n  = static_cast<double>(data.size());
    double sx = 0.0, sy = 0.0, sxy = 0.0, sx2 = 0.0;
    for (size_t i = 0; i < data.size(); ++i) {
        double x = static_cast<double>(i);
        double y = data[i];
        sx  += x;
        sy  += y;
        sxy += x * y;
        sx2 += x * x;
    }
    double denom = n * sx2 - sx * sx;
    if (std::abs(denom) < 1e-9) {
      return 0.0;
    }
    return (n * sxy - sx * sy) / denom;
}

} // namespace themis::rag::learning
