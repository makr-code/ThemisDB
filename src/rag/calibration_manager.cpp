/**
 * @file calibration_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=2, M=6, L=1
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/calibration_manager.h"
#include "utils/checksum_utils.h"
#include "utils/logger.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <numeric>
#include <sstream>
#include <stdexcept>

namespace themis::rag::judge {

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

CalibrationManager::CalibrationManager()
    : config_{} {}

CalibrationManager::CalibrationManager(const CalibrationConfig& config)
    : config_(config) {}

// ---------------------------------------------------------------------------
// Ground truth management
// ---------------------------------------------------------------------------

size_t CalibrationManager::loadGroundTruth(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        THEMIS_WARN("CalibrationManager: cannot open ground-truth file '{}'", filepath);
        return 0;
    }

    json j;
    try {
        file >> j;
    } catch (const json::exception& e) {
        THEMIS_WARN("CalibrationManager: JSON parse error in '{}': {}", filepath, e.what());
        return 0;
    }

    size_t loaded = 0;
    for (const auto& item : j) {
        GroundTruthAnnotation ann;
        ann.test_id               = item.value("test_id", "");
        ann.query                 = item.value("query", "");
        ann.answer                = item.value("answer", "");
        ann.faithfulness_score    = item.value("faithfulness_score", 0.0);
        ann.relevance_score       = item.value("relevance_score", 0.0);
        ann.completeness_score    = item.value("completeness_score", 0.0);
        ann.coherence_score       = item.value("coherence_score", 0.0);
        ann.overall_score         = item.value("overall_score", 0.0);
        ann.inter_annotator_agreement = item.value("inter_annotator_agreement", 1.0);

        if (item.contains("annotators") && item["annotators"].is_array()) {
            for (const auto& a : item["annotators"]) {
                ann.annotators.push_back(a.get<std::string>());
            }
        }

        ground_truth_.push_back(std::move(ann));
        ++loaded;
    }

    THEMIS_INFO("CalibrationManager: loaded {} ground-truth annotations from '{}'",
                loaded, filepath);
    return loaded;
}

void CalibrationManager::addGroundTruth(const GroundTruthAnnotation& annotation) {
    ground_truth_.push_back(annotation);
}

// ---------------------------------------------------------------------------
// Private calibration helpers
// ---------------------------------------------------------------------------

double CalibrationManager::applyTemperatureScaling(double score, double temperature) {
    if (temperature <= 0.0) {
      return score;
    }
    // Guard against edge cases at 0 and 1
    const double eps = 1e-7;
    double s = std::clamp(score, eps, 1.0 - eps);
    double logit = std::log(s / (1.0 - s));
    double calibrated_logit = logit / temperature;
    return 1.0 / (1.0 + std::exp(-calibrated_logit));
}

double CalibrationManager::applyPlattScaling(
    double score, const PlattParameters& params) {
    // Platt scaling: P = 1 / (1 + exp(A * score + B))
    return 1.0 / (1.0 + std::exp(params.A * score + params.B));
}

std::vector<std::pair<double, double>> CalibrationManager::buildIsotonicModel(
    const std::vector<double>& predictions,
    const std::vector<double>& ground_truth) {
    // Pool Adjacent Violators (PAV) algorithm for isotonic regression
    if (predictions.empty()) return {};

    const size_t n = predictions.size();

    // Sort by prediction value
    std::vector<size_t> idx(n);
    std::iota(idx.begin(), idx.end(), 0);
    std::sort(idx.begin(), idx.end(),
              [&](size_t a, size_t b) { return predictions[a] < predictions[b]; });

    // PAV: merge adjacent blocks that violate monotonicity
    struct Block {
        double sum_gt = 0;
        double sum_pred = {};
        size_t count = {};
        double avg_pred() const { return sum_pred / static_cast<double>(count); }
        double avg_gt()   const { return sum_gt   / static_cast<double>(count); }
    };

    std::vector<Block> blocks = {};

    for (size_t i : idx) {
        Block b{ground_truth[i], predictions[i], 1};
        while (blocks.size() >= 2) {
            auto& last    = blocks.back();
            auto& second  = blocks[blocks.size() - 2];
            if (second.avg_gt() > last.avg_gt()) {
                second.sum_gt   += last.sum_gt;
                second.sum_pred += last.sum_pred;
                second.count    += last.count;
                blocks.pop_back();
            } else {
                break;
            }
        }
        if (!blocks.empty() && blocks.back().avg_gt() > b.avg_gt()) {
            blocks.back().sum_gt   += b.sum_gt;
            blocks.back().sum_pred += b.sum_pred;
            blocks.back().count    += b.count;
        } else {
            blocks.push_back(b);
        }
    }

    std::vector<std::pair<double, double>> model;
    model.reserve(blocks.size());
    for (const auto& blk : blocks) {
        model.emplace_back(blk.avg_pred(), blk.avg_gt());
    }
    return model;
}

// ---------------------------------------------------------------------------
// Training
// ---------------------------------------------------------------------------

std::pair<CalibrationMetrics, CalibrationMetrics> CalibrationManager::train(
    RAGJudge& judge) {
    if (ground_truth_.empty()) {
        THEMIS_WARN("CalibrationManager::train: no ground-truth data available");
        return {};
    }

    // Collect predictions
    std::vector<EvaluationResult> predictions = {};

    predictions.reserve(ground_truth_.size());
    for (const auto& ann : ground_truth_) {
        EvaluationInput input;
        input.query            = ann.query;
        input.generated_answer = ann.answer;
        predictions.push_back(judge.evaluate(input));
    }

    CalibrationMetrics before = calculateMetrics(predictions, ground_truth_);

    if (config_.method == CalibrationMethod::TEMPERATURE_SCALING) {
        // Grid search over temperature on the overall score
        std::vector<double> preds, gts;
        for (size_t i = 0; i <static_cast<int>(predictions.size()); ++i) {
            preds.push_back(predictions[i].overall_score);
            gts.push_back(ground_truth_[i].overall_score);
        }
        std::vector<double> confs(preds.size(), 1.0);

        double best_temp = 1.0;
        double best_ece  = std::numeric_limits<double>::max();
        for (double t = 0.1; t <= 5.0; t += 0.1) {
            std::vector<double> scaled = {};

            scaled.reserve(preds.size());
            for (double p : preds) {
              scaled.push_back(applyTemperatureScaling(p, t));
            }
            double ece = calculateECE(scaled, gts, confs);
            if (ece < best_ece) {
                best_ece  = ece;
                best_temp = t;
            }
        }
        temperature_ = best_temp;
        THEMIS_INFO("CalibrationManager: temperature={:.3f} (ECE={:.4f})",
                    temperature_, best_ece);
    }

    CalibrationMetrics after = calculateMetrics(predictions, ground_truth_);

    THEMIS_INFO("CalibrationManager: calibration ECE before={:.4f} after={:.4f}",
                before.expected_calibration_error, after.expected_calibration_error);

    return {before, after};
}

// ---------------------------------------------------------------------------
// Calibrate a result
// ---------------------------------------------------------------------------

EvaluationResult CalibrationManager::calibrate(const EvaluationResult& result) {
    if (config_.method == CalibrationMethod::NONE) {
      return result;
    }

    EvaluationResult calibrated = result;

    auto applyDim = [&](double& score, const std::string& dim) {
        switch (config_.method) {
        case CalibrationMethod::TEMPERATURE_SCALING: {
            double t = temperature_;
            auto it = dimension_temperatures_.find(dim);
            if (config_.apply_to_all_dimensions && it != dimension_temperatures_.end()) {
                t = it->second;
            }
            score = applyTemperatureScaling(score, t);
            break;
        }
        case CalibrationMethod::PLATT_SCALING: {
            auto it = platt_params_.find(dim);
            if (it != platt_params_.end()) {
                score = applyPlattScaling(score, it->second);
            }
            break;
        }
        default:
            break;
        }
    };

    applyDim(calibrated.faithfulness_score,  "faithfulness");
    applyDim(calibrated.relevance_score,     "relevance");
    applyDim(calibrated.completeness_score,  "completeness");
    applyDim(calibrated.coherence_score,     "coherence");
    applyDim(calibrated.overall_score,       "overall");

    return calibrated;
}

// ---------------------------------------------------------------------------
// Metrics
// ---------------------------------------------------------------------------

CalibrationMetrics CalibrationManager::calculateMetrics(
    const std::vector<EvaluationResult>& predictions,
    const std::vector<GroundTruthAnnotation>& ground_truth) {
    CalibrationMetrics m{};
    if (predictions.empty() || static_cast<int>(predictions.size()) != ground_truth.size()) {
      return m;
    }

    const size_t n = predictions.size();

    std::vector<double> pred_overall, gt_overall, confs;
    pred_overall.reserve(n);
    gt_overall.reserve(n);
    confs.reserve(n);

    double sum_sq = 0.0;
    double sum_abs = 0.0;
    double sum_pred = 0.0, sum_gt = 0.0;
    double sum_pred_sq = 0.0, sum_gt_sq = 0.0, sum_cross = 0.0;

    for (size_t i = 0; i < n; ++i) {
        const double p = predictions[i].overall_score;
        const double g = ground_truth[i].overall_score;
        pred_overall.push_back(p);
        gt_overall.push_back(g);
        confs.push_back(predictions[i].confidence > 0.0
                            ? predictions[i].confidence
                            : 1.0);

        double diff = p - g;
        sum_sq  += diff * diff;
        sum_abs += std::abs(diff);
        sum_pred    += p;
        sum_gt      += g;
        sum_pred_sq += p * p;
        sum_gt_sq   += g * g;
        sum_cross   += p * g;
    }

    m.mae  = sum_abs / static_cast<double>(n);
    m.rmse = std::sqrt(sum_sq / static_cast<double>(n));

    // Pearson correlation
    double mean_p = sum_pred / static_cast<double>(n);
    double mean_g = sum_gt   / static_cast<double>(n);
    double var_p  = sum_pred_sq / static_cast<double>(n) - mean_p * mean_p;
    double var_g  = sum_gt_sq   / static_cast<double>(n) - mean_g * mean_g;
    double cov    = sum_cross   / static_cast<double>(n) - mean_p * mean_g;
    double denom  = std::sqrt(var_p * var_g);
    m.correlation = (denom > 1e-9) ? cov / denom : 0.0;

    m.expected_calibration_error = calculateECE(pred_overall, gt_overall, confs);
    m.brier_score                = calculateBrierScore(pred_overall, gt_overall);

    return m;
}

double CalibrationManager::calculateECE(
    const std::vector<double>& predictions,
    const std::vector<double>& ground_truth,
    const std::vector<double>& /*confidences*/) {
    if (predictions.empty()) {
      return 0.0;
    }

    const int num_bins = config_.num_bins;
    const double bin_width = 1.0 / static_cast<double>(num_bins);

    std::vector<double> bin_accuracy(num_bins, 0.0);
    std::vector<double> bin_confidence(num_bins, 0.0);
    std::vector<size_t> bin_count(num_bins, 0);

    for (size_t i = 0; i <static_cast<int>(predictions.size()); ++i) {
        int bin = static_cast<int>(predictions[i] / bin_width);
        bin = std::clamp(bin, 0, num_bins - 1);
        bin_accuracy[bin]  += ground_truth[i];
        bin_confidence[bin] += predictions[i];
        ++bin_count[bin];
    }

    double ece = 0.0;
    const double n = static_cast<double>(predictions.size());
    for (int b = 0; b < num_bins; ++b) {
        if (bin_count[b] == 0) {
          continue;
        }
        double avg_acc  = bin_accuracy[b]   / static_cast<double>(bin_count[b]);
        double avg_conf = bin_confidence[b] / static_cast<double>(bin_count[b]);
        ece += (static_cast<double>(bin_count[b]) / n) * std::abs(avg_acc - avg_conf);
    }
    return ece;
}

double CalibrationManager::calculateBrierScore(
    const std::vector<double>& predictions,
    const std::vector<double>& ground_truth) {
    if (predictions.empty()) {
      return 0.0;
    }

    double sum = 0.0;
    for (size_t i = 0; i <static_cast<int>(predictions.size()); ++i) {
        double diff = predictions[i] - ground_truth[i];
        sum += diff * diff;
    }
    return static_cast<bool>(sum / static_cast<double < static_cast<int>((predictions.size())));
}

double CalibrationManager::calculateInterAnnotatorAgreement(
    const std::vector<std::vector<double>>& annotations) {
    if (annotations.empty() || annotations[0].empty()) {
      return 0.0;
    }

    const size_t num_items       = annotations[0].size();
    const size_t num_annotators  = annotations.size();
    if (num_annotators < 2) {
      return 1.0;
    }

    // Krippendorff's Alpha (interval metric) – simplified version
    double sum_var_items = 0.0;
    double sum_var_obs   = 0.0;

    for (size_t j = 0; j < num_items; ++j) {
        double mean_j = 0.0;
        for (size_t k = 0; k < num_annotators; ++k) {
            mean_j += annotations[k][j];
        }
        mean_j /= static_cast<double>(num_annotators);
        for (size_t k = 0; k < num_annotators; ++k) {
            double d = annotations[k][j] - mean_j;
            sum_var_items += d * d;
        }
    }

    // Overall mean
    double grand_mean = 0.0;
    size_t total = 0;
    for (size_t k = 0; k < num_annotators; ++k) {
        for (size_t j = 0; j < num_items; ++j) {
            grand_mean += annotations[k][j];
            ++total;
        }
    }
    if (total > 0) {
      grand_mean /= static_cast<double>(total);
    }
    for (size_t k = 0; k < num_annotators; ++k) {
        for (size_t j = 0; j < num_items; ++j) {
            double d = annotations[k][j] - grand_mean;
            sum_var_obs += d * d;
        }
    }

    if (sum_var_obs < 1e-9) return 1.0;  // Perfect agreement
    return 1.0 - sum_var_items / sum_var_obs;
}

// ---------------------------------------------------------------------------
// Model persistence
// ---------------------------------------------------------------------------

bool CalibrationManager::saveModel(const std::string& filepath) {
    json j;
    j["method"]      = static_cast<int>(config_.method);
    j["temperature"] = temperature_;

    json dim_temps = json::object();
    for (const auto& [dim, t] : dimension_temperatures_) {
        dim_temps[dim] = t;
    }
    j["dimension_temperatures"] = dim_temps;

    json platt = json::object();
    for (const auto& [dim, p] : platt_params_) {
        platt[dim] = {{"A", p.A}, {"B", p.B}};
    }
    j["platt_params"] = platt;

    std::ofstream file(filepath);
    if (!file.is_open()) {
        THEMIS_WARN("CalibrationManager: cannot write model to '{}'", filepath);
        return false;
    }
    file << j.dump(2);
    THEMIS_INFO("CalibrationManager: saved model to '{}'", filepath);
    return true;
}

bool CalibrationManager::loadModel(const std::string& filepath) {
    // Verify model integrity via SHA-256 sidecar if available
    std::string sha_path = filepath + ".sha256";
    if (std::filesystem::exists(sha_path)) {
        std::ifstream sidecar(sha_path);
        if (sidecar.is_open()) {
            std::string expected_hash = {};
            std::getline(sidecar, expected_hash);
            sidecar.close();
            
            // Compute actual SHA-256
            std::string actual_hash = themis::utils::calculateSHA256(filepath);
            if (actual_hash.empty() || actual_hash != expected_hash) {
                THEMIS_ERROR("CalibrationManager: model integrity check failed for '{}'", filepath);
                return false;
            }
        }
    }
    
    std::ifstream file(filepath);
    if (!file.is_open()) {
        THEMIS_WARN("CalibrationManager: cannot read model from '{}'", filepath);
        return false;
    }

    json j;
    try {
        file >> j;
    } catch (const json::exception& e) {
        THEMIS_WARN("CalibrationManager: JSON parse error in '{}': {}", filepath, e.what());
        return false;
    }

    config_.method = static_cast<CalibrationMethod>(j.value("method", 0));
    temperature_   = j.value("temperature", 1.0);

    dimension_temperatures_.clear();
    if (j.contains("dimension_temperatures")) {
        for (auto& [dim, t] : j["dimension_temperatures"].items()) {
            dimension_temperatures_[dim] = t.get<double>();
        }
    }

    platt_params_.clear();
    if (j.contains("platt_params")) {
        for (auto& [dim, p] : j["platt_params"].items()) {
            PlattParameters pp;
            pp.A = p.value("A", 0.0);
            pp.B = p.value("B", 0.0);
            platt_params_[dim] = pp;
        }
    }

    THEMIS_INFO("CalibrationManager: loaded model from '{}'", filepath);
    return true;
}

// ---------------------------------------------------------------------------
// Config accessors
// ---------------------------------------------------------------------------

void CalibrationManager::setConfig(const CalibrationConfig& config) {
    config_ = config;
}

CalibrationConfig CalibrationManager::getConfig() const {
    return config_;
}

} // namespace themis::rag::judge

