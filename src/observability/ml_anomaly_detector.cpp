/**
 * @file ml_anomaly_detector.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=3, M=9, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "observability/ml_anomaly_detector.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

namespace themis {
namespace observability {

using themisdb::analytics::AnomalyDetector;
using themisdb::analytics::AnomalyMethod;
using themisdb::analytics::DataPoint;
using themisdb::analytics::DecompositionResult;
using themisdb::analytics::ForecastMethod;
using themisdb::analytics::forecastMethodName;
using themisdb::analytics::ForecastPoint;

// ---------------------------------------------------------------------------
// Utility helpers
// ---------------------------------------------------------------------------

double MLAnomalyDetector::clamp01([[maybe_unused]] double v) noexcept {
    if (v < 0.0) {
      return 0.0;
    }
    if (v > 1.0) {
      return 1.0;
    }
    return v;
}

std::chrono::system_clock::time_point
MLAnomalyDetector::tsFromMs(int64_t ms) {
    return std::chrono::system_clock::time_point{
        std::chrono::milliseconds(ms)
    };
}

int64_t MLAnomalyDetector::toMs(std::chrono::system_clock::time_point tp) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               tp.time_since_epoch())
        .count();
}

double MLAnomalyDetector::mean(const std::vector<double>& v) {
    if (v.empty()) {
      return 0.0;
    }
    double s = std::accumulate(v.begin(), v.end(), 0.0);
    return s / static_cast<double>(v.size());
}

double MLAnomalyDetector::stddev(const std::vector<double>& v, double mu) {
    if (v.size() < 2) {
      return 0.0;
    }
    double ss = 0.0;
    for (double x : v) {
        double d = x - mu;
        ss += d * d;
    }
    return std::sqrt(ss / static_cast<double>(v.size() - 1));
}

double MLAnomalyDetector::medianIntervalMs(const ForecastSeries& series) const {
    const auto& pts = series.points();
    if (pts.size() < 2) {
      return 0.0;
    }
    std::vector<int64_t> diffs;
    diffs.reserve(pts.size() - 1);
    for (size_t i = 1; i < pts.size(); ++i) {
        diffs.push_back(pts[i].timestamp_ms - pts[i - 1].timestamp_ms);
    }
    std::sort(diffs.begin(), diffs.end());
    size_t mid = diffs.size() / 2;
    if (diffs.size() % 2 == 0) {
        return (static_cast<double>(diffs[mid - 1]) +
                static_cast<double>(diffs[mid])) / 2.0;
    }
    return static_cast<double>(diffs[mid]);
}

std::vector<int> MLAnomalyDetector::dbscanLabels(
    const std::vector<double>& values) const
{
    // Simple 1-D DBSCAN implementation sufficient for unit-scale test cases.
    const int UNVISITED = 0;
    const int NOISE     = -1;
    int label           = 0;
    std::vector<int> labels(values.size(), UNVISITED);

    auto regionQuery = [&]([[maybe_unused]] size_t idx) {
        std::vector<size_t> neighbours;
        for (size_t j = 0; j < values.size(); ++j) {
            if (std::fabs(values[j] - values[idx]) <= cfg_.dbscan_eps) {
                neighbours.push_back(j);
            }
        }
        return neighbours;
    };

    for (size_t i = 0; i < values.size(); ++i) {
        if (labels[i] != UNVISITED) {
          continue;
        }
        auto neighbours = regionQuery(i);
        if (neighbours.size() < cfg_.dbscan_min_samples) {
            labels[i] = NOISE;
            continue;
        }
        ++label;
        labels[i] = label;
        std::vector<size_t> queue(neighbours.begin(), neighbours.end());
        for (size_t qi = 0; qi < queue.size(); ++qi) {
            size_t nidx = queue[qi];
            if (labels[nidx] == NOISE) {
              labels[nidx] = label;
            }
            if (labels[nidx] != UNVISITED) {
              continue;
            }
            labels[nidx] = label;
            auto nn = regionQuery(nidx);
            if (nn.size() >= cfg_.dbscan_min_samples) {
                queue.insert(queue.end(), nn.begin(), nn.end());
            }
        }
    }
    return labels;
}

double MLAnomalyDetector::changePointScore(const std::vector<double>& values) const {
    if (values.size() < cfg_.min_training_points / 2) {
      return 0.0;
    }
    size_t mid = values.size() / 2;
    std::vector<double> left(values.begin(), values.begin() + static_cast<long>(mid));
    std::vector<double> right(values.begin() + static_cast<long>(mid), values.end());
    double lmean = mean(left);
    double rmean = mean(right);
    double lstd  = stddev(left, lmean);
    double rstd  = stddev(right, rmean);
    double pooled_std = std::max(1e-9, std::sqrt((lstd * lstd + rstd * rstd) / 2.0));
    double z = std::fabs(rmean - lmean) / pooled_std;
    double norm = z / cfg_.change_point_threshold;
    return clamp01(norm > 1.0 ? 1.0 : norm);
}

std::string MLAnomalyDetector::severityForScore([[maybe_unused]] double s) const {
    if (s >= 0.9) {
      return "critical";
    }
    if (s >= 0.75) {
      return "high";
    }
    if (s >= 0.5) {
      return "medium";
    }
    return "low";
}

std::vector<double> MLAnomalyDetector::buildSeasonalTemplate(
    const DecompositionResult& d) const
{
    // Residual is not used; we only keep seasonal pattern (optional).
    return d.seasonal;
}

AnomalyExplanation MLAnomalyDetector::buildExplanation(const Anomaly& anomaly) const {
    AnomalyExplanation e;
    e.timestamp        = anomaly.timestamp;
    e.metric_name      = anomaly.metric_name;
    e.confidence_score = anomaly.confidence_score;
    e.summary          = "Combined ML signal: outlier + forecast deviation + DBSCAN density + change-point";
    // These keys mirror the contributing_factors entries for transparency.
    for (const auto& factor : anomaly.contributing_factors) {
        // extract numeric suffix if present "factor:0.82"
        auto pos = factor.find(':');
        if (pos != std::string::npos && pos + 1 < factor.size()) {
            try {
                double v = std::stod(factor.substr(pos + 1));
                e.feature_importance.push_back({factor.substr(0, pos), v});
                continue;
            } catch (...) {
                // fallthrough
            }
        }
        e.feature_importance.push_back({factor, 0.0});
    }
    return e;
}

// ---------------------------------------------------------------------------
// Anomaly::toJson / AnomalyExplanation::toJson
// ---------------------------------------------------------------------------

json Anomaly::toJson() const {
    auto ts_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                     timestamp.time_since_epoch())
                     .count();
    return json{
        {"timestamp_ms",        ts_ms},
        {"metric_name",         metric_name},
        {"actual_value",        actual_value},
        {"expected_value",      expected_value},
        {"confidence_score",    confidence_score},
        {"severity",            severity},
        {"contributing_factors", contributing_factors}
    };
}

json AnomalyExplanation::toJson() const {
    auto ts_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                     timestamp.time_since_epoch())
                     .count();
    json fi = json::array();
    for (const auto& [name, value] : feature_importance) {
        fi.push_back({{"name", name}, {"value", value}});
    }
    return json{
        {"timestamp_ms", ts_ms},
        {"metric_name", metric_name},
        {"confidence_score", confidence_score},
        {"summary", summary},
        {"feature_importance", fi}
    };
}

// ---------------------------------------------------------------------------
// MLAnomalyDetector: construction
// ---------------------------------------------------------------------------

static ForecastMethod mapBackend(ForecastBackend b) {
    return (b == ForecastBackend::ARIMA)
         ? ForecastMethod::ARIMA
         : ForecastMethod::HOLT_WINTERS;  // Prophet-style seasonal model
}

MLAnomalyDetector::MLAnomalyDetector(const MLConfig& config)
    : cfg_(config)
    , forecast_model_(cfg_.forecast_config, mapBackend(cfg_.forecast_backend))
    , outlier_detector_(cfg_.outlier_config)
{}

// ---------------------------------------------------------------------------
// train
// ---------------------------------------------------------------------------

void MLAnomalyDetector::train(const std::vector<ForecastSeries>& training_data) {
    std::vector<themisdb::analytics::TimeSeriesPoint> merged;
    for (const auto& ts : training_data) {
        const auto& pts = ts.points();
        merged.insert(merged.end(), pts.begin(), pts.end());
    }
    std::sort(merged.begin(), merged.end(),
              [](const auto& a, const auto& b) { return a.timestamp_ms < b.timestamp_ms; });
    merged.erase(std::unique(merged.begin(), merged.end(),
                             [](const auto& a, const auto& b) {
                                 return a.timestamp_ms == b.timestamp_ms &&
                                        std::fabs(a.value - b.value) < 1e-12;
                             }),
                 merged.end());

    if (merged.size() < cfg_.min_training_points) {
        throw std::invalid_argument(
            "MLAnomalyDetector::train requires at least " +
            std::to_string(cfg_.min_training_points) + " points");
    }

    training_series_ = ForecastSeries(std::move(merged));
    baseline_values_ = training_series_.values();
    baseline_mean_   = mean(baseline_values_);
    baseline_stddev_ = stddev(baseline_values_, baseline_mean_);
    if (baseline_stddev_ < 1e-9) {
      baseline_stddev_ = 1e-9;
    }

    // Configure forecast model with Prophet-style defaults if requested.
    auto fcfg = cfg_.forecast_config;
    if (cfg_.forecast_backend == ForecastBackend::PROPHET) {
        if (cfg_.seasonality_period > 0 && fcfg.seasonality == 0) {
            fcfg.seasonality = static_cast<int>(cfg_.seasonality_period);
        }
        if (fcfg.alpha == 0.0) {
          fcfg.alpha = 0.3;
        }
        if (fcfg.beta == 0.0) {
          fcfg.beta  = 0.1;
        }
        if (fcfg.gamma == 0.0) {
          fcfg.gamma = 0.1;
        }
    }

    forecast_model_ = themisdb::analytics::ForecastModel(
        fcfg, mapBackend(cfg_.forecast_backend));
    forecast_model_.fit(training_series_);

    // Train outlier detector on baseline values.
    std::vector<DataPoint> dp;
    dp.reserve(baseline_values_.size());
    auto ts_vec = training_series_.timestamps();
    for (size_t i = 0; i < baseline_values_.size(); ++i) {
        DataPoint p;
        p.id = cfg_.metric_name;
        p.timestamp_ms = ts_vec[i];
        p.set("value", baseline_values_[i]);
        dp.push_back(std::move(p));
    }
    outlier_detector_ = AnomalyDetector(cfg_.outlier_config);
    outlier_detector_.train(dp);

    // Seasonal template (optional)
    if (cfg_.seasonality_period > 0 && forecast_model_.isFitted()) {
        auto decomp = forecast_model_.decompose(fcfg.multiplicative);
        seasonal_template_ = buildSeasonalTemplate(decomp);
    } else {
        seasonal_template_.clear();
    }

    trained_ = true;
}

// ---------------------------------------------------------------------------
// forecast
// ---------------------------------------------------------------------------

ForecastSeries MLAnomalyDetector::forecast(std::chrono::hours horizon) const {
    if (!trained_) {
        throw std::runtime_error("MLAnomalyDetector::forecast requires train() first");
    }
    double interval_ms = medianIntervalMs(training_series_);
    if (interval_ms <= 0.0) {
      interval_ms = 1000.0;
    }
    int steps = static_cast<int>(std::max<int64_t>(
        1, static_cast<int64_t>(
            std::llround(static_cast<double>(horizon.count()) * 3600.0 * 1000.0 / interval_ms))));

    auto points = forecast_model_.predict(steps);
    ForecastSeries out;
    for (const auto& p : points) {
        out.push(p.timestamp_ms, p.value);
    }
    return out;
}

// ---------------------------------------------------------------------------
// detectAnomalies
// ---------------------------------------------------------------------------

std::vector<Anomaly>
MLAnomalyDetector::detectAnomalies(const ForecastSeries& current_data) const {
    if (!trained_) {
        throw std::runtime_error("MLAnomalyDetector::detectAnomalies requires train() first");
    }
    std::vector<Anomaly> results;
    const auto& pts = current_data.points();
    if (pts.empty()) {
      return results;
    }

    // Forecast expected values for the same horizon.
    auto forecast_points = forecast_model_.predict(static_cast<int>(pts.size()));
    std::vector<double> values;
    values.reserve(pts.size());
    for (const auto& p : pts) {
      values.push_back(p.value);
    }

    // Density labels via DBSCAN for local outliers.
    auto labels = dbscanLabels(values);

    // Change-point score across the batch.
    double cp_score = changePointScore(values);

    for (size_t i = 0; i < pts.size(); ++i) {
        double actual   = pts[i].value;
        double expected = (i < forecast_points.size())
                            ? forecast_points[i].value
                            : baseline_mean_;
        double deviation = std::fabs(actual - expected);
        double deviation_z = deviation / baseline_stddev_;
        double deviation_score = clamp01(deviation_z / cfg_.change_point_threshold);

        // Outlier detector score.
        DataPoint p;
        p.id = cfg_.metric_name;
        p.timestamp_ms = pts[i].timestamp_ms;
        p.set("value", actual);
        auto outlier_res = outlier_detector_.predict(p);
        double outlier_score = outlier_res.score;

        // Density penalty if DBSCAN marked as noise.
        double dbscan_score = (labels[i] == -1) ? 1.0 : 0.0;

        // Seasonal residual score (if template available).
        double seasonal_score = 0.0;
        if (!seasonal_template_.empty()) {
            size_t idx = i % seasonal_template_.size();
            double seasonal_expected = seasonal_template_[idx];
            seasonal_score = clamp01(std::fabs(actual - seasonal_expected) /
                                     (std::fabs(seasonal_expected) + 1e-6));
        }

        // Combine signals (weighted).
        double combined = clamp01(
            0.35 * outlier_score +
            0.25 * deviation_score +
            0.15 * seasonal_score +
            0.15 * dbscan_score +
            0.10 * cp_score);

        Anomaly a;
        a.metric_name       = cfg_.metric_name;
        a.timestamp         = tsFromMs(pts[i].timestamp_ms);
        a.actual_value      = actual;
        a.expected_value    = expected;
        a.confidence_score  = combined;
        a.severity          = severityForScore(combined);

        if (outlier_score >= cfg_.anomaly_threshold) {
            a.contributing_factors.push_back(
                "outlier_score:" + std::to_string(outlier_score));
        }
        if (deviation_score >= cfg_.anomaly_threshold) {
            a.contributing_factors.push_back(
                "forecast_deviation:" + std::to_string(deviation_score));
        }
        if (dbscan_score > 0.5) {
            a.contributing_factors.push_back("dbscan_noise:1.0");
        }
        if (seasonal_score >= cfg_.anomaly_threshold) {
            a.contributing_factors.push_back(
                "seasonality_residual:" + std::to_string(seasonal_score));
        }
        if (cp_score >= cfg_.anomaly_threshold) {
            a.contributing_factors.push_back(
                "change_point:" + std::to_string(cp_score));
        }

        if (combined >= cfg_.anomaly_threshold) {
            results.push_back(std::move(a));
        }
    }

    return results;
}

// ---------------------------------------------------------------------------
// explainAnomaly
// ---------------------------------------------------------------------------

AnomalyExplanation MLAnomalyDetector::explainAnomaly(const Anomaly& anomaly) const {
    return buildExplanation(anomaly);
}

} // namespace observability
} // namespace themis



