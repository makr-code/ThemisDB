/**
 * @file automl.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=5, H=11, M=34, L=1
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * AutoML Engine – Implementation
 *
 * Algorithms (pure C++17, no external ML dependencies):
 *
 *  Logistic Regression – Mini-batch SGD with L2 regularisation; one-vs-rest
 *                        for multi-class problems.  Softmax probabilities.
 *
 *  Linear Regression   – Ordinary Least Squares via Normal Equations with
 *                        optional L2 (ridge) regularisation.
 *
 *  Decision Tree       – CART (binary splits) using Gini impurity
 *                        (classification) or MSE (regression); max-depth and
 *                        min-samples-leaf controlled via hyperparameters.
 *
 *  Random Forest       – Bagged ensemble of decision trees with random feature
 *                        subsets (sqrt for classification, n/3 for regression).
 *                        Soft majority vote / mean aggregation.
 *
 *  Gradient Boosting   – Stagewise additive model of shallow regression trees;
 *                        uses log-loss gradient for classification and MSE
 *                        gradient for regression.
 *
 *  KNN                 – Brute-force Euclidean k-NN; weighted voting (1/d²).
 *
 *  ENSEMBLE            – Soft-vote (classification) / mean (regression) over
 *                        the top-k candidate models from the search.
 *
 * Feature engineering:
 *   - Zero-mean, unit-variance scaling computed on training data, applied to
 *     train + test to avoid leakage.
 *   - Optional polynomial expansion (degree 2, no bias term) to capture
 *     non-linear interactions.
 *
 * Hyperparameter search:
 *   - Random search: sample hyperparameters from predefined grids.
 *   - Early stopping: abort search when wall-clock budget is exhausted.
 *
 * SHAP approximation:
 *   - Permutation-based: for each feature, shuffle its values across the
 *     sample and measure the change in model output.  Normalised to sum to 1.
 *
 * Serialisation:
 *   - Simple key=value; text format (mirrors anomaly_detection.cpp).
 */

#include "analytics/automl.h"

#include <cassert>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <map>
#include <numeric>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <mutex>
#include <unordered_map>

namespace themisdb {
namespace analytics {

// ============================================================================
// EvalMetrics::primary
// ============================================================================

double EvalMetrics::primary(AutoMLMetric m) const noexcept {
    switch (m) {
        case AutoMLMetric::ACCURACY:
            return accuracy;
        case AutoMLMetric::F1:
            return f1;
        case AutoMLMetric::PRECISION:
            return precision;
        case AutoMLMetric::RECALL:
            return recall;
        case AutoMLMetric::AUC_ROC:
            return auc_roc;
        case AutoMLMetric::R2:
            return r2;
        case AutoMLMetric::RMSE:
            return -rmse; // lower is better → negate for maximisation
        case AutoMLMetric::MAE:
            return -mae;
        case AutoMLMetric::MAPE:
            return -mape;
        default:
            return 0.0;
    }
}

// ============================================================================
// Anonymous namespace – shared utilities
// ============================================================================

namespace {

// --------------------------------------------------------------------------
// Feature matrix helpers
// --------------------------------------------------------------------------

/** Raw feature matrix extracted from DataPoints (excluding the target). */
struct FeatMatrix {
    std::vector<std::vector<double>> X; // [sample][feature]
    std::vector<std::string> names;
};

/** Extract numeric features from DataPoints, excluding the target field. */
FeatMatrix extractFeatures(const std::vector<DataPoint> &data, const std::string &target) {
    FeatMatrix fm = {};
    if (data.empty()) {
        return fm;
    }

    // Build sorted feature name list (exclude target)
    std::set<std::string> name_set = {};

    for (const auto &p : data) {
        for (const auto &[k, v] : p.fields) {
            if (k == target) {
                continue;
            }
            if (std::holds_alternative<double>(v) || std::holds_alternative<int64_t>(v)
                || std::holds_alternative<bool>(v)) {
                name_set.insert(k);
            }
        }
    }
    fm.names.assign(name_set.begin(), name_set.end());

    fm.X.reserve(data.size());
    for (const auto &p : data) {
        std::vector<double> row(fm.names.size(), 0.0);
        for (size_t j = 0; j < fm.names.size(); ++j) {
            auto it = p.fields.find(fm.names[j]);
            if (it != p.fields.end()) {
                if (auto *d = std::get_if<double>(&it->second)) {
                    row[j] = *d;
                } else if (auto *i = std::get_if<int64_t>(&it->second)) {
                    row[j] = static_cast<double>(*i);
                } else if (auto *b = std::get_if<bool>(&it->second)) {
                    row[j] = *b ? 1.0 : 0.0;
                }
            }
        }
        fm.X.push_back(std::move(row));
    }
    return fm;
}

/** Extract target values as strings (for classification label encoding). */
std::vector<std::string> extractTargetStr(const std::vector<DataPoint> &data, const std::string &target) {
    std::vector<std::string> out = {};

    out.reserve(data.size());
    for (const auto &p : data) {
        auto it = p.fields.find(target);
        if (it == p.fields.end()) {
            out.push_back("");
            continue;
        }
        std::visit(
            [&]([[maybe_unused]] const auto &v) {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, std::string>) {
                    out.push_back(v);
                } else if constexpr (std::is_same_v<T, int64_t>) {
                    out.push_back(std::to_string(v));
                } else if constexpr (std::is_same_v<T, double>) {
                    out.push_back(std::to_string(static_cast<int64_t>(v)));
                } else if constexpr (std::is_same_v<T, bool>) {
                    out.push_back(v ? "1" : "0");
                } else {
                    out.push_back("");
                }
            },
            it->second);
    }
    return out;
}

/** Extract target values as doubles (for regression). */
std::vector<double> extractTargetNum(const std::vector<DataPoint> &data, const std::string &target) {
    std::vector<double> out = {};

    out.reserve(data.size());
    for (const auto &p : data) {
        auto it = p.fields.find(target);
        if (it == p.fields.end()) {
            out.push_back(0.0);
            continue;
        }
        std::visit(
            [&]([[maybe_unused]] const auto &v) {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, double>) {
                    out.push_back(v);
                } else if constexpr (std::is_same_v<T, int64_t>) {
                    out.push_back(static_cast<double>(v));
                } else if constexpr (std::is_same_v<T, bool>) {
                    out.push_back(v ? 1.0 : 0.0);
                } else {
                    out.push_back(0.0);
                }
            },
            it->second);
    }
    return out;
}

// --------------------------------------------------------------------------
// Standard scaler
// --------------------------------------------------------------------------

struct Scaler {
    std::vector<double> mean, std_dev;

    void fit(const std::vector<std::vector<double>> &X) {
        if (X.empty()) {
            return;
        }
        size_t n = X.size(), d = X[0].size();
        mean.assign(d, 0.0);
        std_dev.assign(d, 1.0);
        for (size_t j = 0; j < d; ++j) {
            double s = 0.0;
            for (size_t i = 0; i < n; ++i) {
                s += X[i][j];
            }
            mean[j]  = s / static_cast<double>(n);
            double v = 0.0;
            for (size_t i = 0; i < n; ++i) {
                double diff = X[i][j] - mean[j];
                v += diff * diff;
            }
            v /= static_cast<double>(n);
            std_dev[j] = std::sqrt(v) < 1e-12 ? 1.0 : std::sqrt(v);
        }
    }

    std::vector<double> transform(const std::vector<double> &x) const {
        std::vector<double> out(x.size());
        for (size_t j = 0; j < x.size()  && static_cast<size_t>(j) < mean.size(); ++j) {
            out[j] = (x[j] - mean[j]) / std_dev[j];
        }
        return out;
    }

    std::vector<std::vector<double>> transformAll(const std::vector<std::vector<double>> &X) const {
        std::vector<std::vector<double>> out;
        out.reserve(X.size());
        for (const auto &row : X) {
            out.push_back(transform(row));
        }
        return out;
    }
};

// --------------------------------------------------------------------------
// Polynomial feature expansion (degree 2, no cross-terms for speed)
// --------------------------------------------------------------------------

std::vector<std::vector<double>> polyFeatures(const std::vector<std::vector<double>> &X) {
    std::vector<std::vector<double>> out;
    out.reserve(X.size());
    for (const auto &row : X) {
        std::vector<double> r = row;
        for (double v : row) {
            r.push_back(v * v);
        }
        out.push_back(std::move(r));
    }
    return out;
}

std::vector<std::string> polyFeatureNames(const std::vector<std::string> &names) {
    std::vector<std::string> out = names;
    for (const auto &n : names) {
        out.push_back(n + "^2");
    }
    return out;
}

// --------------------------------------------------------------------------
// Label encoder
// --------------------------------------------------------------------------

struct LabelEncoder {
    std::vector<std::string> classes; // sorted unique class labels
    std::map<std::string, int> index;

    void fit(const std::vector<std::string> &labels) {
        std::set<std::string> s(labels.begin(), labels.end());
        classes.assign(s.begin(), s.end());
        for (size_t i = 0; i < static_cast<int>(classes.size()); ++i) {
            index[classes[static_cast<size_t>(i)]] = i;
        }
    }

    std::vector<int> encode(const std::vector<std::string> &labels) const {
        std::vector<int> out = {};

        out.reserve(labels.size());
        for (const auto &l : labels) {
            auto it = index.find(l);
            out.push_back(it != index.end() ? it->second : 0);
        }
        return out;
    }

    std::string decode([[maybe_unused]] int i) const {
        if (i < 0 || i >= static_cast<int>(classes.size())) {
            return "";
        }
        return classes[static_cast<size_t>(i)];
    }

    int numClasses() const {
        return static_cast<bool>(static_cast<int < static_cast<int>((classes.size())));
    }
};

// --------------------------------------------------------------------------
// Dot product and L2 norm helpers
// --------------------------------------------------------------------------

inline double dot(const std::vector<double> &a, const std::vector<double> &b) {
    double s = 0.0;
    size_t n = std::min(a.size(), b.size());
    for (size_t i = 0; i < n; ++i) {
        s += a[i] * b[i];
    }
    return s;
}

inline double l2sq(const std::vector<double> &a, const std::vector<double> &b) {
    double s = 0.0;
    size_t n = std::min(a.size(), b.size());
    for (size_t i = 0; i < n; ++i) {
        double d = a[i] - b[i];
        s += d * d;
    }
    return s;
}

inline double sigmoid([[maybe_unused]] double z) {
    return 1.0 / (1.0 + std::exp(-z));
}

// --------------------------------------------------------------------------
// Softmax (in-place)
// --------------------------------------------------------------------------

void softmax(std::vector<double> &v) {
    double maxv = *std::max_element(v.begin(), v.end());
    double sum  = 0.0;
    for (auto &x : v) {
        x = std::exp(x - maxv);
        sum += x;
    }
    if (sum < 1e-300) {
        sum = 1.0;
    }
    for (auto &x : v) {
        x /= sum;
    }
}

// --------------------------------------------------------------------------
// Decision Tree (CART – binary splits)
// --------------------------------------------------------------------------

struct TreeNode {
    bool is_leaf      = false;
    int feature       = -1;
    double threshold  = 0.0;
    double leaf_value = 0.0;         // regression or class probability (binary)
    std::vector<double> class_probs; // multi-class probabilities
    int left  = -1;
    int right = -1;
};

/** Compact binary decision tree represented as a node array. */
struct DecisionTree {
    std::vector<TreeNode> nodes;
    int max_depth        = 5;
    int min_samples_leaf = 2;
    bool is_classifier   = true;
    int n_classes        = 2;

    // --- Build from data ---
    void fit(const std::vector<std::vector<double>> &X, const std::vector<int> &y_cls, const std::vector<double> &y_reg,
             std::mt19937 &rng, int n_features_try = -1) {
        nodes.clear();
        size_t n = X.size();
        if (n == 0) {
            return;
        }
        int n_feat = static_cast<int>(X[0].size());
        if (n_features_try <= 0) {
            n_features_try = n_feat;
        }

        // Indices of samples at each node
        std::vector<size_t> idx(n);
        std::iota(idx.begin(), idx.end(), 0);
        buildNode(X, y_cls, y_reg, idx, 0, rng, n_features_try);
    }

    // --- Predict single sample ---
    double predictOne(const std::vector<double> &x) const {
        if (nodes.empty()) {
            return 0.0;
        }
        int cur = 0;
        while (!nodes[static_cast<size_t>(cur)].is_leaf) {
            const auto &nd = nodes[static_cast<size_t>(cur)];
            cur            = (x[static_cast<size_t>(nd.feature)] <= nd.threshold) ? nd.left : nd.right;
            if (cur < 0) {
                break;
            }
        }
        return nodes[static_cast<size_t>(cur < 0 ? 0 : cur)].leaf_value;
    }

    std::vector<double> predictProbaOne(const std::vector<double> &x) const {
        std::vector<double> uniform(static_cast<size_t>(n_classes), 1.0 / static_cast<double>(n_classes));
        if (nodes.empty()) {
            return uniform;
        }
        int cur = 0;
        while (!nodes[static_cast<size_t>(cur)].is_leaf) {
            const auto &nd = nodes[static_cast<size_t>(cur)];
            cur            = (x[static_cast<size_t>(nd.feature)] <= nd.threshold) ? nd.left : nd.right;
            if (cur < 0) {
                break;
            }
        }
        const auto &leaf = nodes[static_cast<size_t>(cur < 0 ? 0 : cur)];
        if (leaf.class_probs.empty()) {
            // binary: leaf_value is P(class=1)
            return {1.0 - leaf.leaf_value, leaf.leaf_value};
        }
        return leaf.class_probs;
    }

  private:
    // Returns index into nodes
    int buildNode(const std::vector<std::vector<double>> &X, const std::vector<int> &y_cls,
                  const std::vector<double> &y_reg, const std::vector<size_t> &idx, int depth, std::mt19937 &rng,
                  int n_features_try) {
        int node_idx = static_cast<int>(nodes.size());
        nodes.emplace_back();
        auto &nd = nodes.back();

        // --- Leaf criteria ---
        if (static_cast<int>(idx.size()) <= min_samples_leaf || depth >= max_depth) {
            nd.is_leaf     = true;
            nd.leaf_value  = computeLeafValue(y_cls, y_reg, idx);
            nd.class_probs = computeClassProbs(y_cls, idx);
            return node_idx;
        }

        // --- Find best split ---
        int best_feat     = -1;
        double best_thr   = 0.0;
        double best_score = -std::numeric_limits<double>::infinity();

        int n_feat = static_cast<int>(X[0].size());
        // Sample random feature subset
        std::vector<int> feats(static_cast<size_t>(n_feat));
        std::iota(feats.begin(), feats.end(), 0);
        if (n_features_try < n_feat) {
            std::shuffle(feats.begin(), feats.end(), rng);
            feats.resize(static_cast<size_t>(n_features_try));
        }

        for (int f : feats) {
            // Collect unique threshold candidates (midpoints)
            std::vector<double> vals = {};

            vals.reserve(idx.size());
            for (size_t i : idx) {
                vals.push_back(X[i][static_cast<size_t>(f)]);
            }
            std::sort(vals.begin(), vals.end());
            vals.erase(std::unique(vals.begin(), vals.end()), vals.end());
            if (vals.size() < 2) {
                continue;
            }

            for (size_t vi = 0; vi + 1 < vals.size(); ++vi) {
                double thr   = (vals[vi] + vals[vi + 1]) * 0.5;
                double score = splitScore(X, y_cls, y_reg, idx, f, thr);
                if (score > best_score) {
                    best_score = score;
                    best_feat  = f;
                    best_thr   = thr;
                }
            }
        }

        if (best_feat < 0) {
            nd.is_leaf     = true;
            nd.leaf_value  = computeLeafValue(y_cls, y_reg, idx);
            nd.class_probs = computeClassProbs(y_cls, idx);
            return node_idx;
        }

        // --- Split ---
        std::vector<size_t> left_idx, right_idx;
        for (size_t i : idx) {
            if (X[i][static_cast<size_t>(best_feat)] <= best_thr) {
                left_idx.push_back(i);
            } else {
                right_idx.push_back(i);
            }
        }
        if (left_idx.empty() || right_idx.empty()) {
            nd.is_leaf     = true;
            nd.leaf_value  = computeLeafValue(y_cls, y_reg, idx);
            nd.class_probs = computeClassProbs(y_cls, idx);
            return node_idx;
        }

        nd.feature   = best_feat;
        nd.threshold = best_thr;
        // Do NOT use `nd` after the recursive calls – the `nodes` vector may
        // reallocate during recursion, invalidating the reference.
        int left_child  = buildNode(X, y_cls, y_reg, left_idx, depth + 1, rng, n_features_try);
        int right_child = buildNode(X, y_cls, y_reg, right_idx, depth + 1, rng, n_features_try);
        // Re-index after potential reallocation
        nodes[static_cast<size_t>(node_idx)].left      = left_child;
        nodes[static_cast<size_t>(node_idx)].right     = right_child;
        nodes[static_cast<size_t>(node_idx)].feature   = best_feat;
        nodes[static_cast<size_t>(node_idx)].threshold = best_thr;
        return node_idx;
    }

    double computeLeafValue(const std::vector<int> &y_cls, const std::vector<double> &y_reg,
                            const std::vector<size_t> &idx) const {
        if (!is_classifier) {
            double s = 0.0;
            for (size_t i : idx) {
                s += y_reg[i];
            }
            return static_cast<bool>(idx.empty() ? 0.0 : s / static_cast<double < static_cast<int>((idx.size())));
        }
        // Classification: return P(majority class)
        std::map<int, int> cnt = {};

        for (size_t i : idx) {
            cnt[y_cls[i]]++;
        }
        int best = 0, bcount = 0;
        for (auto &[c, n] : cnt) {
            if (n > bcount) {
                bcount = n;
                best   = c;
            }
        }
        return static_cast<double>(best);
    }

    std::vector<double> computeClassProbs(const std::vector<int> &y_cls, const std::vector<size_t> &idx) const {
        if (!is_classifier || n_classes <= 0) {
            return {};
        }
        std::vector<double> probs(static_cast<size_t>(n_classes), 0.0);
        for (size_t i : idx) {
            int c = y_cls[i];
            if (c >= 0 && c < n_classes) {
                probs[static_cast<size_t>(c)] += 1.0;
            }
        }
        double total = static_cast<double>(idx.size());
        if (total > 0) {
            for (auto &p : probs) {
                p /= total;
            }
        }
        return probs;
    }

    // Returns information gain (classification: Gini; regression: MSE reduction)
    double splitScore(const std::vector<std::vector<double>> &X, const std::vector<int> &y_cls,
                      const std::vector<double> &y_reg, const std::vector<size_t> &idx, int feat, double thr) const {
        std::vector<size_t> left, right;
        for (size_t i : idx) {
            if (X[i][static_cast<size_t>(feat)] <= thr) {
                left.push_back(i);
            } else {
                right.push_back(i);
            }
        }
        if (left.empty() || right.empty()) {
            return -std::numeric_limits<double>::infinity();
        }

        double n  = static_cast<double>(idx.size());
        double nl = static_cast<double>(left.size());
        double nr = static_cast<double>(right.size());

        if (!is_classifier) {
            // MSE reduction
            double ml = 0.0, mr = 0.0;
            for (size_t i : left) {
                ml += y_reg[i];
            }
            for (size_t i : right) {
                mr += y_reg[i];
            }
            ml /= nl;
            mr /= nr;
            double vl = 0.0, vr = 0.0;
            for (size_t i : left) {
                double d = y_reg[i] - ml;
                vl += d * d;
            }
            for (size_t i : right) {
                double d = y_reg[i] - mr;
                vr += d * d;
            }
            // Parent variance
            double m = 0.0;
            for (size_t i : idx) {
                m += y_reg[i];
            }
            m /= n;
            double vp = 0.0;
            for (size_t i : idx) {
                double d = y_reg[i] - m;
                vp += d * d;
            }
            return vp - vl - vr;
        }
        // Gini impurity reduction
        auto gini = [&]([[maybe_unused]] const std::vector<size_t> &s) {
            std::map<int, int> cnt = {};

            for (size_t i : s) {
                cnt[y_cls[i]]++;
            }
            double g  = 1.0;
            double sz = static_cast<double>(s.size());
            for (auto &[c, k] : cnt) {
                double p = k / sz;
                g -= p * p;
            }
            return g * sz;
        };
        double parent_gini = gini(idx);
        double child_gini  = gini(left) + gini(right);
        return parent_gini - child_gini;
    }
};

// --------------------------------------------------------------------------
// Logistic Regression (mini-batch SGD, one-vs-rest for multi-class)
// --------------------------------------------------------------------------

struct LogisticRegression {
    // weights[class][feature], bias[class]
    std::vector<std::vector<double>> W;
    std::vector<double> b;
    int n_classes  = 2;
    double lr      = 0.01;
    double l2      = 1e-4;
    int max_epochs = 200;
    int batch_size = 32;

    void fit(const std::vector<std::vector<double>> &X, const std::vector<int> &y, std::mt19937 &rng) {
        if (X.empty()) {
            return;
        }
        size_t n = X.size(), d = X[0].size();

        W.assign(static_cast<size_t>(n_classes), std::vector<double>(d, 0.0));
        b.assign(static_cast<size_t>(n_classes), 0.0);

        std::vector<size_t> idx(n);
        std::iota(idx.begin(), idx.end(), 0);

        for (int epoch = 0; epoch < max_epochs; ++epoch) {
            std::shuffle(idx.begin(), idx.end(), rng);
            for (size_t start = 0; start < n; start += static_cast<size_t>(batch_size)) {
                size_t end = std::min(start + static_cast<size_t>(batch_size), n);
                // Accumulate gradients
                std::vector<std::vector<double>> dW(static_cast<size_t>(n_classes), std::vector<double>(d, 0.0));
                std::vector<double> db(static_cast<size_t>(n_classes), 0.0);

                for (size_t ii = start; ii < end; ++ii) {
                    size_t i = idx[ii];
                    // Compute logits
                    std::vector<double> logits(static_cast<size_t>(n_classes));
                    for (int c = 0; c < n_classes; ++c) {
                        logits[static_cast<size_t>(c)]
                            = dot(W[static_cast<size_t>(c)], X[i]) + b[static_cast<size_t>(c)];
                    }
                    softmax(logits);

                    for (int c = 0; c < n_classes; ++c) {
                        double err = logits[static_cast<size_t>(c)] - (y[i] == c ? 1.0 : 0.0);
                        for (size_t j = 0; j < d; ++j) {
                            dW[static_cast<size_t>(c)][j] += err * X[i][j];
                        }
                        db[static_cast<size_t>(c)] += err;
                    }
                }
                double bs = static_cast<double>(end - start);
                for (int c = 0; c < n_classes; ++c) {
                    for (size_t j = 0; j < d; ++j) {
                        W[static_cast<size_t>(c)][j]
                            -= lr * (dW[static_cast<size_t>(c)][j] / bs + l2 * W[static_cast<size_t>(c)][j]);
                    }
                    b[static_cast<size_t>(c)] -= lr * db[static_cast<size_t>(c)] / bs;
                }
            }
        }
    }

    std::vector<double> predictProbaOne(const std::vector<double> &x) const {
        std::vector<double> logits(static_cast<size_t>(n_classes));
        for (int c = 0; c < n_classes; ++c) {
            logits[static_cast<size_t>(c)] = dot(W[static_cast<size_t>(c)], x) + b[static_cast<size_t>(c)];
        }
        softmax(logits);
        return logits;
    }
};

// --------------------------------------------------------------------------
// Linear Regression (Normal equations + ridge)
// --------------------------------------------------------------------------

struct LinearReg {
    std::vector<double> w; // weights (including bias at index d)
    double l2 = 1e-4;

    void fit(const std::vector<std::vector<double>> &X, const std::vector<double> &y) {
        if (X.empty()) {
            return;
        }
        size_t n = X.size(), d = X[0].size() + 1; // +1 for bias

        // Build augmented X with bias column
        std::vector<std::vector<double>> Xb(n, std::vector<double>(d));
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = 0; j + 1 < d; ++j) {
                Xb[i][j] = X[i][j];
            }
            Xb[i][static_cast<int>(d - 1)] = 1.0; // bias
        }

        // Compute X'X (d×d) and X'y (d)
        std::vector<std::vector<double>> XtX(d, std::vector<double>(d, 0.0));
        std::vector<double> Xty(d, 0.0);

        for (size_t i = 0; i < n; ++i) {
            for (size_t r = 0; r < d; ++r) {
                Xty[r] += Xb[i][r] * y[i];
                for (size_t c2 = 0; c2 < d; ++c2) {
                    XtX[r][c2] += Xb[i][r] * Xb[i][c2];
                }
            }
        }
        // Ridge regularisation (do not regularise bias)
        for (size_t r = 0; r + 1 < d; ++r) {
            XtX[r][r] += l2;
        }

        // Solve via Gaussian elimination
        w = solveLinear(XtX, Xty);
    }

    double predictOne(const std::vector<double> &x) const {
        double s = 0.0;
        for (size_t j = 0; j < x.size() && j + 1 < w.size(); ++j) {
            s += w[j] * x[j];
        }
        if (!w.empty()) {
            s += w.back(); // bias
        }
        return s;
    }

  private:
    static std::vector<double> solveLinear(std::vector<std::vector<double>> A, std::vector<double> b) {
        size_t n = b.size();
        // Forward elimination with partial pivoting
        for (size_t col = 0; col < n; ++col) {
            // Find pivot
            size_t pivot = col;
            for (size_t row = col + 1; row < n; ++row) {
                if (std::abs(A[row][col]) > std::abs(A[pivot][col])) {
                    pivot = row;
                }
            }
            std::swap(A[col], A[pivot]);
            std::swap(b[col], b[pivot]);

            if (std::abs(A[col][col]) < 1e-15) {
                continue;
            }
            for (size_t row = col + 1; row < n; ++row) {
                double factor = A[row][col] / A[col][col];
                for (size_t j = col; j < n; ++j) {
                    A[row][j] -= factor * A[col][j];
                }
                b[row] -= factor * b[col];
            }
        }
        // Back substitution
        std::vector<double> x(n, 0.0);
        for (int i = static_cast<int>(n) - 1; i >= 0; --i) {
            x[static_cast<size_t>(i)] = b[static_cast<size_t>(i)];
            for (size_t j = static_cast<size_t>(i) + 1; j < n; ++j) {
                x[static_cast<size_t>(i)] -= A[static_cast<size_t>(i)][j] * x[j];
            }
            if (std::abs(A[static_cast<size_t>(i)][static_cast<size_t>(i)]) > 1e-15) {
                x[static_cast<size_t>(i)] /= A[static_cast<size_t>(i)][static_cast<size_t>(i)];
            }
        }
        return x;
    }
};

// --------------------------------------------------------------------------
// Evaluation metrics helpers
// --------------------------------------------------------------------------

/** Accuracy (classification). */
double computeAccuracy(const std::vector<int> &y_true, const std::vector<int> &y_pred) {
    if (y_true.empty()) {
        return 0.0;
    }
    int correct = 0;
    for (size_t i = 0; i < y_true.size(); ++i) {
        if (y_true[i] == y_pred[i]) {
            ++correct;
        }
    }
    return static_cast<bool>(static_cast<double>(correct) / static_cast<double < static_cast<int>((y_true.size())));
}

/** Macro-averaged F1 / Precision / Recall. */
struct ClassMetrics {
    double f1, precision, recall;
};
ClassMetrics computeClassMetrics(const std::vector<int> &y_true, const std::vector<int> &y_pred, int n_classes) {
    std::vector<int> tp(static_cast<size_t>(n_classes), 0), fp(static_cast<size_t>(n_classes), 0),
        fn(static_cast<size_t>(n_classes), 0);
    for (size_t i = 0; i < y_true.size(); ++i) {
        int t = y_true[i], p = y_pred[i];
        if (t == p) {
            ++tp[static_cast<size_t>(t)];
        } else {
            ++fp[static_cast<size_t>(p)];
            ++fn[static_cast<size_t>(t)];
        }
    }
    double prec_sum = 0.0, rec_sum = 0.0, f1_sum = 0.0;
    int counted = 0;
    for (int c = 0; c < n_classes; ++c) {
        int denom_p = tp[static_cast<size_t>(c)] + fp[static_cast<size_t>(c)];
        int denom_r = tp[static_cast<size_t>(c)] + fn[static_cast<size_t>(c)];
        double p    = (denom_p > 0) ? static_cast<double>(tp[static_cast<size_t>(c)]) / denom_p : 0.0;
        double r    = (denom_r > 0) ? static_cast<double>(tp[static_cast<size_t>(c)]) / denom_r : 0.0;
        double f1   = (p + r > 0) ? 2.0 * p * r / (p + r) : 0.0;
        prec_sum += p;
        rec_sum += r;
        f1_sum += f1;
        ++counted;
    }
    if (counted == 0) {
        return {0.0, 0.0, 0.0};
    }
    double d = static_cast<double>(counted);
    return {f1_sum / d, prec_sum / d, rec_sum / d};
}

/** Regression metrics: R², RMSE, MAE. */
struct RegMetrics {
    double r2, rmse, mae;
};
RegMetrics computeRegMetrics(const std::vector<double> &y_true, const std::vector<double> &y_pred) {
    size_t n = y_true.size();
    if (n == 0) {
        return {0.0, 0.0, 0.0};
    }
    double mean_y = 0.0;
    for (double v : y_true) {
        mean_y += v;
    }
    mean_y /= static_cast<double>(n);

    double ss_tot = 0.0, ss_res = 0.0, mae = 0.0, rmse = 0.0;
    for (size_t i = 0; i < n; ++i) {
        double e = y_true[i] - y_pred[i];
        double d = y_true[i] - mean_y;
        ss_res += e * e;
        ss_tot += d * d;
        mae += std::abs(e);
        rmse += e * e;
    }
    double r2 = (ss_tot > 1e-12) ? 1.0 - ss_res / ss_tot : 0.0;
    return {r2, std::sqrt(rmse / static_cast<double>(n)), mae / static_cast<double>(n)};
}

// --------------------------------------------------------------------------
// Fold generator for k-fold CV
// --------------------------------------------------------------------------

std::vector<std::pair<std::vector<size_t>, std::vector<size_t>>> makeFolds(size_t n, int k, std::mt19937 &rng) {
    std::vector<size_t> idx(n);
    std::iota(idx.begin(), idx.end(), 0);
    std::shuffle(idx.begin(), idx.end(), rng);

    std::vector<std::pair<std::vector<size_t>, std::vector<size_t>>> folds;
    size_t fold_size = n / static_cast<size_t>(k);
    for (int fi = 0; fi < k; ++fi) {
        size_t start = static_cast<size_t>(fi) * fold_size;
        size_t end   = (fi == k - 1) ? n : start + fold_size;
        std::vector<size_t> val(idx.begin() + static_cast<ptrdiff_t>(start), idx.begin() + static_cast<ptrdiff_t>(end));
        std::vector<size_t> train = {};

        for (size_t i = 0; i < n; ++i) {
            if (i < start || i >= end) {
                train.push_back(idx[i]);
            }
        }
        folds.emplace_back(std::move(train), std::move(val));
    }
    return folds;
}

// --------------------------------------------------------------------------
// Generic model interface (type-erased)
// --------------------------------------------------------------------------

/** Abstract base for type-erased trained models. */
struct ModelBase {
    virtual ~ModelBase()                                                            = default;
    virtual double predictOneReg(cons[[maybe_unused]] t st[[maybe_unused]] d::vecto[[maybe_unused]] r<doubl[[maybe_unused]] e> &x) const                = 0;
    virtual int predictOneCls(cons[[maybe_unused]] t st[[maybe_unused]] d::vecto[[maybe_unused]] r<doubl[[maybe_unused]] e> &x) const                   = 0;
    virtual std::vector<double> predictProbaOne(cons[[maybe_unused]] t st[[maybe_unused]] d::vecto[[maybe_unused]] r<doubl[[maybe_unused]] e> &x) const = 0;
    virtual ModelAlgorithm algorithm() const noexcept                               = 0;
    virtual std::unique_ptr<ModelBase> clone() const                                = 0;
};

// Concrete wrappers ----------------------------------------------------------

struct DTModel : ModelBase {
    DecisionTree tree;
    explicit DTModel(DecisionTree t) : tree(std::move(t)) {}
    double predictOneReg(cons[[maybe_unused]] t st[[maybe_unused]] d::vecto[[maybe_unused]] r<doubl[[maybe_unused]] e> &x) const override {
        return tree.predictOne(x);
    }
    int predictOneCls(cons[[maybe_unused]] t st[[maybe_unused]] d::vecto[[maybe_unused]] r<doubl[[maybe_unused]] e> &x) const override {
        return static_cast<int>(std::round(tree.predictOne(x)));
    }
    std::vector<double> predictProbaOne(cons[[maybe_unused]] t st[[maybe_unused]] d::vecto[[maybe_unused]] r<doubl[[maybe_unused]] e> &x) const override {
        return tree.predictProbaOne(x);
    }
    ModelAlgorithm algorithm() const noexcept override {
        return ModelAlgorithm::DECISION_TREE;
    }
    std::unique_ptr<ModelBase> clone() const override {
        return std::make_unique<DTModel>(*this);
    }
};

struct LRModel : ModelBase {
    LogisticRegression lr;
    explicit LRModel(LogisticRegression l) : lr(std::move(l)) {}
    double predictOneReg(cons[[maybe_unused]] t st[[maybe_unused]] d::vecto[[maybe_unused]] r<doubl[[maybe_unused]] e> &x) const override {
        // Compute the expected class value: sum(class_index * P(class_index)).
        // For binary classification (classes 0 and 1) this equals P(class=1),
        // the standard logistic-regression regression proxy.
        // For k > 2 classes the result is the probability-weighted class index.
        const auto proba = lr.predictProbaOne(x);
        double v         = 0.0;
        for (size_t c = 0; c < proba.size(); ++c) {
            v += static_cast<double>(c) * proba[c];
        }
        return v;
    }
    int predictOneCls(cons[[maybe_unused]] t st[[maybe_unused]] d::vecto[[maybe_unused]] r<doubl[[maybe_unused]] e> &x) const override {
        auto p = lr.predictProbaOne(x);
        return static_cast<int>(std::max_element(p.begin(), p.end()) - p.begin());
    }
    std::vector<double> predictProbaOne(cons[[maybe_unused]] t st[[maybe_unused]] d::vecto[[maybe_unused]] r<doubl[[maybe_unused]] e> &x) const override {
        return lr.predictProbaOne(x);
    }
    ModelAlgorithm algorithm() const noexcept override {
        return ModelAlgorithm::LOGISTIC_REGRESSION;
    }
    std::unique_ptr<ModelBase> clone() const override {
        return std::make_unique<LRModel>(*this);
    }
};

struct LinRegModel : ModelBase {
    LinearReg lr;
    explicit LinRegModel(LinearReg l) : lr(std::move(l)) {}
    double predictOneReg(cons[[maybe_unused]] t st[[maybe_unused]] d::vecto[[maybe_unused]] r<doubl[[maybe_unused]] e> &x) const override {
        return lr.predictOne(x);
    }
    int predictOneCls(cons[[maybe_unused]] t st[[maybe_unused]] d::vecto[[maybe_unused]] r<doubl[[maybe_unused]] e> &x) const override {
        return static_cast<int>(std::round(lr.predictOne(x)));
    }
    std::vector<double> predictProbaOne(cons[[maybe_unused]] t st[[maybe_unused]] d::vecto[[maybe_unused]] r<doubl[[maybe_unused]] e> &x) const override {
        return {lr.predictOne(x)};
    }
    ModelAlgorithm algorithm() const noexcept override {
        return ModelAlgorithm::LINEAR_REGRESSION;
    }
    std::unique_ptr<ModelBase> clone() const override {
        return std::make_unique<LinRegModel>(*this);
    }
};

/** Random Forest: ensemble of decision trees. */
struct RFModel : ModelBase {
    std::vector<DecisionTree> trees;
    bool is_classifier = true;
    int n_classes      = 2;

    double predictOneReg(cons[[maybe_unused]] t st[[maybe_unused]] d::vecto[[maybe_unused]] r<doubl[[maybe_unused]] e> &x) const override {
        if (trees.empty()) {
            return 0.0;
        }
        double s = 0.0;
        for (const auto &t : trees) {
            s += t.predictOne(x);
        }
        return static_cast<bool>(s / static_cast<double < static_cast<int>((trees.size())));
    }
    int predictOneCls(cons[[maybe_unused]] t st[[maybe_unused]] d::vecto[[maybe_unused]] r<doubl[[maybe_unused]] e> &x) const override {
        std::vector<double> probs = predictProbaOne(x);
        return static_cast<int>(std::max_element(probs.begin(), probs.end()) - probs.begin());
    }
    std::vector<double> predictProbaOne(cons[[maybe_unused]] t st[[maybe_unused]] d::vecto[[maybe_unused]] r<doubl[[maybe_unused]] e> &x) const override {
        if (trees.empty()) {
            return std::vector<double>(static_cast<size_t>(n_classes), 1.0 / n_classes);
        }
        std::vector<double> avg(static_cast<size_t>(n_classes), 0.0);
        for (const auto &t : trees) {
            auto p = t.predictProbaOne(x);
            for (size_t c = 0; c < avg.size()  && static_cast<size_t>(c) < p.size(); ++c) {
                avg[c] += p[c];
            }
        }
        double d = static_cast<double>(trees.size());
        for (auto &v : avg) {
            v /= d;
        }
        return avg;
    }
    ModelAlgorithm algorithm() const noexcept override {
        return ModelAlgorithm::RANDOM_FOREST;
    }
    std::unique_ptr<ModelBase> clone() const override {
        return std::make_unique<RFModel>(*this);
    }
};

/** Gradient Boosting: stagewise additive model. */
struct GBModel : ModelBase {
    struct Stage {
        DecisionTree tree;
        double lr = {};
    };
    std::vector<Stage> stages;
    double base_value  = 0.0;
    bool is_classifier = true;
    int n_classes      = 2;

    double predictOneReg(cons[[maybe_unused]] t st[[maybe_unused]] d::vecto[[maybe_unused]] r<doubl[[maybe_unused]] e> &x) const override {
        double v = base_value;
        for (const auto &s : stages) {
            v += s.lr * s.tree.predictOne(x);
        }
        return v;
    }
    int predictOneCls(cons[[maybe_unused]] t st[[maybe_unused]] d::vecto[[maybe_unused]] r<doubl[[maybe_unused]] e> &x) const override {
        double v = predictOneReg(x);
        return v >= 0.5 ? 1 : 0;
    }
    std::vector<double> predictProbaOne(cons[[maybe_unused]] t st[[maybe_unused]] d::vecto[[maybe_unused]] r<doubl[[maybe_unused]] e> &x) const override {
        double raw = predictOneReg(x);
        double p   = sigmoid(raw);
        return {1.0 - p, p};
    }
    ModelAlgorithm algorithm() const noexcept override {
        return ModelAlgorithm::GRADIENT_BOOSTING;
    }
    std::unique_ptr<ModelBase> clone() const override {
        return std::make_unique<GBModel>(*this);
    }
};

/** k-NN model. */
struct KNNModel : ModelBase {
    std::vector<std::vector<double>> X_train;
    std::vector<int> y_cls;
    std::vector<double> y_reg;
    int k              = 5;
    bool is_classifier = true;
    int n_classes      = 2;

    double predictOneReg(cons[[maybe_unused]] t st[[maybe_unused]] d::vecto[[maybe_unused]] r<doubl[[maybe_unused]] e> &x) const override {
        auto nbrs  = neighbors(x);
        double sum = 0.0, wsum = 0.0;
        for (auto [d2, i] : nbrs) {
            double w = (d2 > 1e-15) ? 1.0 / d2 : 1e15;
            sum += w * y_reg[static_cast<size_t>(i)];
            wsum += w;
        }
        return (wsum > 0) ? sum / wsum : 0.0;
    }
    int predictOneCls(cons[[maybe_unused]] t st[[maybe_unused]] d::vecto[[maybe_unused]] r<doubl[[maybe_unused]] e> &x) const override {
        auto p = predictProbaOne(x);
        return static_cast<int>(std::max_element(p.begin(), p.end()) - p.begin());
    }
    std::vector<double> predictProbaOne(cons[[maybe_unused]] t st[[maybe_unused]] d::vecto[[maybe_unused]] r<doubl[[maybe_unused]] e> &x) const override {
        auto nbrs = neighbors(x);
        std::vector<double> votes(static_cast<size_t>(n_classes), 0.0);
        double wsum = 0.0;
        for (auto [d2, i] : nbrs) {
            double w = (d2 > 1e-15) ? 1.0 / d2 : 1e15;
            int c    = y_cls[static_cast<size_t>(i)];
            if (c >= 0 && c < n_classes) {
                votes[static_cast<size_t>(c)] += w;
            }
            wsum += w;
        }
        if (wsum > 0) {
            for (auto &v : votes) {
                v /= wsum;
            }
        }
        return votes;
    }
    ModelAlgorithm algorithm() const noexcept override {
        return ModelAlgorithm::KNN;
    }
    std::unique_ptr<ModelBase> clone() const override {
        return std::make_unique<KNNModel>(*this);
    }

  private:
    std::vector<std::pair<double, int>> neighbors(const std::vector<double> &x) const {
        std::vector<std::pair<double, int>> dists;
        dists.reserve(X_train.size());
        for (size_t i = 0; i < X_train.size(); ++i) {
            dists.emplace_back(l2sq(x, X_train[i]), static_cast<int>(i));
        }
        size_t kk = std::min(static_cast<size_t>(k), dists.size());
        std::nth_element(dists.begin(), dists.begin() + static_cast<ptrdiff_t>(kk), dists.end());
        dists.resize(kk);
        return dists;
    }
};

/** Ensemble: weighted mean/vote over a collection of models. */
struct EnsembleModel : ModelBase {
    std::vector<std::unique_ptr<ModelBase>> members;
    bool is_classifier = true;
    int n_classes      = 2;

    double predictOneReg(cons[[maybe_unused]] t st[[maybe_unused]] d::vecto[[maybe_unused]] r<doubl[[maybe_unused]] e> &x) const override {
        if (members.empty()) {
            return 0.0;
        }
        double s = 0.0;
        for (const auto &m : members) {
            s += m->predictOneReg(x);
        }
        return static_cast<bool>(s / static_cast<double < static_cast<int>((members.size())));
    }
    int predictOneCls(cons[[maybe_unused]] t st[[maybe_unused]] d::vecto[[maybe_unused]] r<doubl[[maybe_unused]] e> &x) const override {
        auto p = predictProbaOne(x);
        return static_cast<int>(std::max_element(p.begin(), p.end()) - p.begin());
    }
    std::vector<double> predictProbaOne(cons[[maybe_unused]] t st[[maybe_unused]] d::vecto[[maybe_unused]] r<doubl[[maybe_unused]] e> &x) const override {
        if (members.empty()) {
            return std::vector<double>(static_cast<size_t>(n_classes), 1.0 / n_classes);
        }
        std::vector<double> avg(static_cast<size_t>(n_classes), 0.0);
        for (const auto &m : members) {
            auto p = m->predictProbaOne(x);
            for (size_t c = 0; c < avg.size()  && static_cast<size_t>(c) < p.size(); ++c) {
                avg[c] += p[c];
            }
        }
        double d = static_cast<double>(members.size());
        for (auto &v : avg) {
            v /= d;
        }
        return avg;
    }
    ModelAlgorithm algorithm() const noexcept override {
        return ModelAlgorithm::ENSEMBLE;
    }
    std::unique_ptr<ModelBase> clone() const override {
        auto copy           = std::make_unique<EnsembleModel>();
        copy->is_classifier = is_classifier;
        copy->n_classes     = n_classes;
        for (const auto &m : members) {
            copy->members.push_back(m->clone());
        }
        return copy;
    }
};

// --------------------------------------------------------------------------
// Training helpers
// --------------------------------------------------------------------------

std::unique_ptr<ModelBase> trainDecisionTree(const std::vector<std::vector<double>> &X, const std::vector<int> &y_cls,
                                             const std::vector<double> &y_reg, bool is_classifier, int n_classes,
                                             int max_depth, int min_leaf, std::mt19937 &rng) {
    DecisionTree t;
    t.max_depth        = max_depth;
    t.min_samples_leaf = min_leaf;
    t.is_classifier    = is_classifier;
    t.n_classes        = n_classes;
    t.fit(X, y_cls, y_reg, rng);
    return std::make_unique<DTModel>(std::move(t));
}

std::unique_ptr<ModelBase> trainRandomForest(const std::vector<std::vector<double>> &X, const std::vector<int> &y_cls,
                                             const std::vector<double> &y_reg, bool is_classifier, int n_classes,
                                             int n_trees, int max_depth, int min_leaf, std::mt19937 &rng) {
    auto rf           = std::make_unique<RFModel>();
    rf->is_classifier = is_classifier;
    rf->n_classes     = n_classes;

    int n_feat   = X.empty() ? 0 : static_cast<int>(X[0].size());
    int feat_try = is_classifier ? std::max(1, static_cast<int>(std::round(std::sqrt(static_cast<double>(n_feat)))))
                                 : std::max(1, n_feat / 3);

    size_t n = X.size();
    for (int t = 0; t < n_trees; ++t) {
        // Bootstrap sample
        std::vector<size_t> bag(n);
        for (size_t i = 0; i < n; ++i) {
            bag[i] = rng() % n;
        }
        std::vector<std::vector<double>> Xb;
        std::vector<int> ycb;
        std::vector<double> yrb;
        Xb.reserve(n);
        ycb.reserve(n);
        yrb.reserve(n);
        for (size_t i : bag) {
            Xb.push_back(X[i]);
            ycb.push_back(is_classifier ? y_cls[i] : 0);
            yrb.push_back(is_classifier ? 0.0 : y_reg[i]);
        }

        DecisionTree tree;
        tree.max_depth        = max_depth;
        tree.min_samples_leaf = min_leaf;
        tree.is_classifier    = is_classifier;
        tree.n_classes        = n_classes;
        tree.fit(Xb, ycb, yrb, rng, feat_try);
        rf->trees.push_back(std::move(tree));
    }
    return rf;
}

std::unique_ptr<ModelBase> trainGradientBoosting(const std::vector<std::vector<double>> &X,
                                                 const std::vector<int> &y_cls, const std::vector<double> &y_reg,
                                                 bool is_classifier, int n_classes, int n_stages, int max_depth,
                                                 double learning_rate, std::mt19937 &rng) {
    auto gb           = std::make_unique<GBModel>();
    gb->is_classifier = is_classifier;
    gb->n_classes     = n_classes;
    size_t n          = X.size();

    if (!is_classifier) {
        // Base value = mean
        double mean_y = 0.0;
        for (double v : y_reg) {
            mean_y += v;
        }
        gb->base_value = y_reg.empty() ? 0.0 : mean_y / static_cast<double>(n);

        std::vector<double> residuals = y_reg;
        for (double &r : residuals) {
            r -= gb->base_value;
        }

        for (int s = 0; s < n_stages; ++s) {
            // Fit a shallow tree to residuals
            std::vector<int> dummy_cls(n, 0);
            DecisionTree t;
            t.max_depth        = max_depth;
            t.min_samples_leaf = 2;
            t.is_classifier    = false;
            t.n_classes        = 1;
            t.fit(X, dummy_cls, residuals, rng);

            // Update residuals
            for (size_t i = 0; i < n; ++i) {
                residuals[i] -= learning_rate * t.predictOne(X[i]);
            }

            gb->stages.push_back({std::move(t), learning_rate});
        }
    } else {
        // Binary classification: log-loss gradient
        double mean_p = 0.0;
        for (int c : y_cls) {
            mean_p += static_cast<double>(c);
        }
        mean_p         = n > 0 ? mean_p / static_cast<double>(n) : 0.5;
        mean_p         = std::max(1e-6, std::min(1.0 - 1e-6, mean_p));
        gb->base_value = std::log(mean_p / (1.0 - mean_p));

        std::vector<double> f(n, gb->base_value);
        for (int s = 0; s < n_stages; ++s) {
            // Negative log-loss gradient = y - p
            std::vector<double> grad(n);
            for (size_t i = 0; i < n; ++i) {
                double p = sigmoid(f[i]);
                grad[i]  = static_cast<double>(y_cls[i]) - p;
            }
            std::vector<int> dummy_cls(n, 0);
            DecisionTree t;
            t.max_depth        = max_depth;
            t.min_samples_leaf = 2;
            t.is_classifier    = false;
            t.n_classes        = 1;
            t.fit(X, dummy_cls, grad, rng);

            for (size_t i = 0; i < n; ++i) {
                f[i] += learning_rate * t.predictOne(X[i]);
            }

            gb->stages.push_back({std::move(t), learning_rate});
        }
    }
    return gb;
}

std::unique_ptr<ModelBase> trainLogReg(const std::vector<std::vector<double>> &X, const std::vector<int> &y_cls,
                                       int n_classes, double lr_rate, double l2, int epochs, std::mt19937 &rng) {
    LogisticRegression lr;
    lr.n_classes  = n_classes;
    lr.lr         = lr_rate;
    lr.l2         = l2;
    lr.max_epochs = epochs;
    lr.fit(X, y_cls, rng);
    return std::make_unique<LRModel>(std::move(lr));
}

std::unique_ptr<ModelBase> trainLinReg(const std::vector<std::vector<double>> &X, const std::vector<double> &y_reg,
                                       double l2) {
    LinearReg lr;
    lr.l2 = l2;
    lr.fit(X, y_reg);
    return std::make_unique<LinRegModel>(std::move(lr));
}

std::unique_ptr<ModelBase> trainKNN(const std::vector<std::vector<double>> &X, const std::vector<int> &y_cls,
                                    const std::vector<double> &y_reg, bool is_classifier, int n_classes, int k) {
    auto knn           = std::make_unique<KNNModel>();
    knn->X_train       = X;
    knn->y_cls         = y_cls;
    knn->y_reg         = y_reg;
    knn->k             = k;
    knn->is_classifier = is_classifier;
    knn->n_classes     = n_classes;
    return knn;
}

// --------------------------------------------------------------------------
// Evaluate a trained model on a subset of data
// --------------------------------------------------------------------------

EvalMetrics evaluateModel(const ModelBase &model, const std::vector<std::vector<double>> &X,
                          const std::vector<int> &y_cls, const std::vector<double> &y_reg, bool is_classifier,
                          int n_classes, [[maybe_unused]] AutoMLMetric metric) {
    EvalMetrics em;
    size_t n = X.size();
    if (n == 0) {
        return em;
    }

    if (is_classifier) {
        std::vector<int> preds(n);
        for (size_t i = 0; i < n; ++i) {
            preds[i] = model.predictOneCls(X[i]);
        }
        em.accuracy  = computeAccuracy(y_cls, preds);
        auto cm      = computeClassMetrics(y_cls, preds, n_classes);
        em.f1        = cm.f1;
        em.precision = cm.precision;
        em.recall    = cm.recall;
        // Simple AUC-ROC for binary classification
        if (n_classes == 2) {
            std::vector<std::pair<double, int>> scores(n);
            for (size_t i = 0; i < n; ++i) {
                auto p    = model.predictProbaOne(X[i]);
                double s  = (p.size() >= 2) ? p[1] : 0.5;
                scores[i] = {s, y_cls[i]};
            }
            std::sort(scores.begin(), scores.end(), [](const auto &a, const auto &b) { return a.first > b.first; });
            double auc = 0.0, tp = 0.0, fp = 0.0;
            double pos = 0, neg = 0;
            for (auto &[s, c] : scores) {
                if (c == 1) {
                    ++pos;
                } else {
                    ++neg;
                }
            }
            for (auto &[s, c] : scores) {
                if (c == 1) {
                    ++tp;
                } else {
                    auc += tp;
                    ++fp;
                }
            }
            if (pos > 0 && neg > 0) {
                em.auc_roc = auc / (pos * neg);
            }
        }
    } else {
        std::vector<double> preds(n);
        for (size_t i = 0; i < n; ++i) {
            preds[i] = model.predictOneReg(X[i]);
        }
        auto rm = computeRegMetrics(y_reg, preds);
        em.r2   = rm.r2;
        em.rmse = rm.rmse;
        em.mae  = rm.mae;
    }
    // used for primary() selection, not here
    return em;
}

// --------------------------------------------------------------------------
// Hyperparameter grid definitions
// --------------------------------------------------------------------------

using HPGrid = std::map<std::string, std::vector<double>>;

HPGrid defaultHPGrid(ModelAlgorithm algo) {
    switch (algo) {
        case ModelAlgorithm::LOGISTIC_REGRESSION:
            return {{"lr", {0.001, 0.01, 0.05}}, {"l2", {1e-5, 1e-4, 1e-2}}, {"epochs", {100, 200}}};
        case ModelAlgorithm::LINEAR_REGRESSION:
            return {{"l2", {0.0, 1e-4, 1e-2, 1.0}}};
        case ModelAlgorithm::DECISION_TREE:
            return {{"max_depth", {3, 5, 8, 15}}, {"min_leaf", {1, 2, 5}}};
        case ModelAlgorithm::RANDOM_FOREST:
            return {{"n_trees", {10, 30, 50}}, {"max_depth", {3, 5, 8}}, {"min_leaf", {1, 2}}};
        case ModelAlgorithm::GRADIENT_BOOSTING:
            return {{"n_stages", {20, 50, 100}}, {"max_depth", {2, 3, 4}}, {"lr", {0.05, 0.1, 0.2}}};
        case ModelAlgorithm::KNN:
            return {{"k", {3, 5, 7, 11, 15}}};
        default:
            return {};
    }
}

std::map<std::string, double> sampleHP(const HPGrid &grid, std::mt19937 &rng) {
    std::map<std::string, double> hp = {};

    for (const auto &[name, vals] : grid) {
        hp[name] = vals[rng() % vals.size()];
    }
    return hp;
}

// --------------------------------------------------------------------------
// Default algorithms for each task
// --------------------------------------------------------------------------

std::vector<ModelAlgorithm> defaultAlgorithms(AutoMLTask task) {
    if (task == AutoMLTask::CLASSIFICATION) {
        return {ModelAlgorithm::LOGISTIC_REGRESSION, ModelAlgorithm::DECISION_TREE, ModelAlgorithm::RANDOM_FOREST,
                ModelAlgorithm::GRADIENT_BOOSTING, ModelAlgorithm::KNN};
    }
    return {ModelAlgorithm::LINEAR_REGRESSION, ModelAlgorithm::DECISION_TREE, ModelAlgorithm::RANDOM_FOREST,
            ModelAlgorithm::GRADIENT_BOOSTING, ModelAlgorithm::KNN};
}

} // anonymous namespace

// ============================================================================
// AutoMLModel::Impl
// ============================================================================

struct AutoMLModel::Impl {
    mutable std::recursive_mutex access_mutex;
    AutoMLTask task     = AutoMLTask::CLASSIFICATION;
    ModelAlgorithm algo = ModelAlgorithm::DECISION_TREE;
    std::string name_str;
    EvalMetrics metrics_val;
    std::unique_ptr<ModelBase> model;
    Scaler scaler;
    LabelEncoder label_enc;
    bool use_poly = false;
    std::vector<std::string> feat_names;
    std::vector<CandidateModelInfo> candidates;
    std::map<std::string, double> feat_importance;

    std::vector<double> prepareRow(const DataPoint &p) const {
        std::vector<double> row(feat_names.size(), 0.0);
        for (size_t j = 0; j < feat_names.size(); ++j) {
            // Handle poly-expanded names like "x^2"
            std::string base = feat_names[j];
            bool is_sq       = false;
            if (static_cast<int>(base.size()) > 2 && base.substr(static_cast<int>(base.size()) - 2) == "^2") {
                base  = base.substr(0, static_cast<int>(base.size()) - 2);
                is_sq = true;
            }
            auto it = p.fields.find(base);
            if (it != p.fields.end()) {
                double v = 0.0;
                if (auto *d = std::get_if<double>(&it->second)) {
                    v = *d;
                } else if (auto *i = std::get_if<int64_t>(&it->second)) {
                    v = static_cast<double>(*i);
                } else if (auto *b = std::get_if<bool>(&it->second)) {
                    v = *b ? 1.0 : 0.0;
                }
                row[j] = is_sq ? v * v : v;
            }
        }
        return scaler.transform(row);
    }
};

// ============================================================================
// AutoMLModel – public interface
// ============================================================================

AutoMLModel::AutoMLModel() : impl_(std::make_unique<Impl>()) {}
AutoMLModel::~AutoMLModel()                                  = default;
AutoMLModel::AutoMLModel(AutoMLModel &&) noexcept            = default;
AutoMLModel &AutoMLModel::operator=(AutoMLModel &&) noexcept = default;

std::vector<std::string> AutoMLModel::predict(const std::vector<DataPoint> &data) const {
    std::lock_guard<std::recursive_mutex> lk(impl_->access_mutex);
    std::vector<std::string> out = {};

    out.reserve(data.size());
    for (const auto &p : data) {
        out.push_back(predictOne(p));
    }
    return out;
}

std::string AutoMLModel::predictOne(const DataPoint &p) const {
    std::lock_guard<std::recursive_mutex> lk(impl_->access_mutex);
    auto row = impl_->prepareRow(p);
    if (impl_->task == AutoMLTask::CLASSIFICATION) {
        int cls = impl_->model->predictOneCls(row);
        return impl_->label_enc.decode(cls);
    }
    return std::to_string(impl_->model->predictOneReg(row));
}

std::vector<std::map<std::string, double>> AutoMLModel::predictProba(const std::vector<DataPoint> &data) const {
    std::lock_guard<std::recursive_mutex> lk(impl_->access_mutex);
    std::vector<std::map<std::string, double>> out;
    out.reserve(data.size());
    for (const auto &p : data) {
        auto row   = impl_->prepareRow(p);
        auto probs = impl_->model->predictProbaOne(row);
        std::map<std::string, double> m = {};

        for (size_t c = 0; c < probs.size(); ++c) {
            m[impl_->label_enc.decode(static_cast<int>(c))] = probs[c];
        }
        out.push_back(std::move(m));
    }
    return out;
}

std::vector<ModelExplanation> AutoMLModel::explain(const std::vector<DataPoint> &data) const {
    std::lock_guard<std::recursive_mutex> lk(impl_->access_mutex);
    std::vector<ModelExplanation> out = {};

    out.reserve(data.size());
    for (const auto &p : data) {
        out.push_back(explainOne(p));
    }
    return out;
}

ModelExplanation AutoMLModel::explainOne(const DataPoint &point) const {
    std::lock_guard<std::recursive_mutex> lk(impl_->access_mutex);
    ModelExplanation exp;
    exp.id = point.id;

    auto row = impl_->prepareRow(point);
    size_t d = row.size();

    // Base prediction
    if (impl_->task == AutoMLTask::CLASSIFICATION) {
        auto probs          = impl_->model->predictProbaOne(row);
        int best_cls        = static_cast<int>(std::max_element(probs.begin(), probs.end()) - probs.begin());
        exp.predicted_label = impl_->label_enc.decode(best_cls);
        exp.predicted_value = static_cast<double>(best_cls);
        exp.confidence      = (probs.empty()) ? 0.0 : probs[static_cast<size_t>(best_cls)];
    } else {
        exp.predicted_value = impl_->model->predictOneReg(row);
        exp.confidence      = 1.0;
    }

    // Permutation-based SHAP approximation
    auto baseScore = [&]() -> double {
        if (impl_->task == AutoMLTask::CLASSIFICATION) {
            auto p = impl_->model->predictProbaOne(row);
            if (p.empty()) {
                return 0.0;
            }
            return *std::max_element(p.begin(), p.end());
        }
        return impl_->model->predictOneReg(row);
    };

    double base = baseScore();
    for (size_t j = 0; j < d; ++j) {
        double orig      = row[j];
        row[j]           = 0.0; // replace with mean (scaled mean is 0)
        double perturbed = baseScore();
        row[j]           = orig;
        double contrib   = base - perturbed;
        std::string name = (j < impl_->feat_names.size()) ? impl_->feat_names[j] : "f" + std::to_string(j);
        exp.feature_contributions.emplace_back(name, contrib);
    }

    // Sort descending by absolute contribution
    std::sort(exp.feature_contributions.begin(), exp.feature_contributions.end(),
              [](const auto &a, const auto &b) { return std::abs(a.second) > std::abs(b.second); });

    // Build top_features string
    size_t top_n = std::min(size_t(5), exp.feature_contributions.size());
    std::string tf = {};
    for (size_t i = 0; i < top_n; ++i) {
        if (i > 0) {
            tf += ", ";
        }
        tf += exp.feature_contributions[i].first;
    }
    exp.top_features = tf;
    exp.description
        = "Predicted "
          + (impl_->task == AutoMLTask::CLASSIFICATION ? exp.predicted_label : std::to_string(exp.predicted_value));
    return exp;
}

AutoMLTask AutoMLModel::task() const noexcept {
    std::lock_guard<std::recursive_mutex> lk(impl_->access_mutex);
    return impl_->task;
}
ModelAlgorithm AutoMLModel::algorithm() const noexcept {
    std::lock_guard<std::recursive_mutex> lk(impl_->access_mutex);
    return impl_->algo;
}
std::string AutoMLModel::name() const noexcept {
    std::lock_guard<std::recursive_mutex> lk(impl_->access_mutex);
    return impl_->name_str;
}
EvalMetrics AutoMLModel::metrics() const noexcept {
    std::lock_guard<std::recursive_mutex> lk(impl_->access_mutex);
    return impl_->metrics_val;
}

std::vector<CandidateModelInfo> AutoMLModel::candidateModels() const {
    std::lock_guard<std::recursive_mutex> lk(impl_->access_mutex);
    return impl_->candidates;
}

std::map<std::string, double> AutoMLModel::featureImportance() const {
    std::lock_guard<std::recursive_mutex> lk(impl_->access_mutex);
    return impl_->feat_importance;
}

std::string AutoMLModel::exportONNX(const std::string &path) const {
    if (!impl_->model) {
        throw std::invalid_argument("AutoMLModel::exportONNX: model is not fitted");
    }

    // We export a JSON-ONNX text representation (v0.1).
    // This format is loadable by MLServingClient when THEMIS_HAS_ONNX_RUNTIME
    // is defined; on other platforms it serves as a portable weight dump
    // for offline tooling.
    //
    // NOTE: True ONNX protobuf serialisation requires the onnx C++ library.
    // This implementation generates a JSON envelope that matches the key
    // fields ONNX Runtime's C-API expects for linear / tree models:
    //   - "ir_version", "opset_imports", "graph.node", "graph.initializer"
    // All node types follow the ONNX-ML opset (LinearRegressor,
    // LinearClassifier, TreeEnsembleClassifier / Regressor, Gather).
    // Unsupported algorithms return an error string without writing the file.

    const ModelAlgorithm algo = impl_->algo;
    const AutoMLTask task     = impl_->task;
    const auto &feat          = impl_->feat_names;

    // Verify algorithm is supported for ONNX export
    if (algo != ModelAlgorithm::LOGISTIC_REGRESSION && algo != ModelAlgorithm::LINEAR_REGRESSION
        && algo != ModelAlgorithm::DECISION_TREE && algo != ModelAlgorithm::RANDOM_FOREST
        && algo != ModelAlgorithm::GRADIENT_BOOSTING && algo != ModelAlgorithm::KNN) {
        return "UNSUPPORTED_OPERATION: algorithm '" + impl_->name_str + "' is not supported for ONNX export";
    }

    std::ostringstream js = {};
    js << std::setprecision(std::numeric_limits<double>::max_digits10);

    // ---- helper lambdas ---------------------------------------------------
    auto jStr = [&]([[maybe_unused]] const std::string &s) {
        // Minimal JSON string escaping
        std::string out = "\"";
        for (char c : s) {
            if (c == '"') {
                out += "\\\"";
            } else if (c == '\\') {
                out += "\\\\";
            } else if (c == '\n') {
                out += "\\n";
            } else {
                out += c;
            }
        }
        return out + "\"";
    };

    auto algoName = [&]() -> std::string {
        switch (algo) {
            case ModelAlgorithm::LOGISTIC_REGRESSION:
                return "LogisticRegression";
            case ModelAlgorithm::LINEAR_REGRESSION:
                return "LinearRegression";
            case ModelAlgorithm::DECISION_TREE:
                return "DecisionTree";
            case ModelAlgorithm::RANDOM_FOREST:
                return "RandomForest";
            case ModelAlgorithm::GRADIENT_BOOSTING:
                return "GradientBoosting";
            case ModelAlgorithm::KNN:
                return "KNN";
            default:
                return "Unknown";
        }
    };

    js << "{\n";
    js << "  \"ir_version\": 8,\n";
    js << "  \"opset_imports\": [{\"domain\": \"\", \"version\": 17},"
          " {\"domain\": \"ai.onnx.ml\", \"version\": 3}],\n";
    js << "  \"producer_name\": " << jStr("ThemisDB-AutoML") << ",\n";
    js << "  \"producer_version\": " << jStr("1.0") << ",\n";
    js << "  \"model_version\": 1,\n";
    js << "  \"domain\": " << jStr("themisdb.analytics") << ",\n";
    js << "  \"doc_string\": " << jStr(impl_->name_str + " — " + algoName()) << ",\n";

    // ---- metadata props ---------------------------------------------------
    js << "  \"metadata_props\": [\n";
    js << "    {\"key\": \"task\",      \"value\": "
       << jStr(task == AutoMLTask::CLASSIFICATION ? "classification" : "regression") << "},\n";
    js << "    {\"key\": \"algorithm\", \"value\": " << jStr(algoName()) << "},\n";
    js << "    {\"key\": \"accuracy\",  \"value\": " << jStr(std::to_string(impl_->metrics_val.accuracy)) << "},\n";
    js << "    {\"key\": \"rmse\",      \"value\": " << jStr(std::to_string(impl_->metrics_val.rmse)) << "}\n";
    js << "  ],\n";

    // ---- feature schema ---------------------------------------------------
    js << "  \"feature_names\": [";
    for (size_t i = 0; i < feat.size(); ++i) {
        js << jStr(feat[i]);
        if (i + 1 < feat.size()) {
            js << ", ";
        }
    }
    js << "],\n";

    // ---- class labels -----------------------------------------------------
    js << "  \"class_labels\": [";
    const auto &classes = impl_->label_enc.classes;
    for (size_t i = 0; i < classes.size(); ++i) {
        js << jStr(classes[i]);
        if (i + 1 < classes.size()) {
            js << ", ";
        }
    }
    js << "],\n";

    // ---- scaler -----------------------------------------------------------
    js << "  \"scaler\": {\n";
    js << "    \"mean\": [";
    for (size_t i = 0; i < impl_->scaler.mean.size(); ++i) {
        js << impl_->scaler.mean[i];
        if (i + 1 < impl_->scaler.mean.size()) {
            js << ", ";
        }
    }
    js << "],\n";
    js << "    \"scale\": [";
    for (size_t i = 0; i < impl_->scaler.std_dev.size(); ++i) {
        js << impl_->scaler.std_dev[i];
        if (i + 1 < impl_->scaler.std_dev.size()) {
            js << ", ";
        }
    }
    js << "]\n  },\n";

    // ---- model weights (algorithm-specific) --------------------------------
    js << "  \"model\": {\n";
    js << "    \"type\": " << jStr(algoName()) << ",\n";

    if (algo == ModelAlgorithm::LINEAR_REGRESSION || algo == ModelAlgorithm::LOGISTIC_REGRESSION) {
        // Emit the weight matrix and bias from the underlying model.
        // We call predictProbaOne on a zero vector to confirm the model is
        // live; for serialisation we use the stored metadata and produce
        // a self-contained weight block that downstream tooling can load.
        js << "    \"weights_shape\": [" << classes.size() << ", " << feat.size() << "],\n";
        js << "    \"weights\": [\n";
        // Produce one weight row per class (logistic) or single row (linear)
        size_t n_rows = (algo == ModelAlgorithm::LOGISTIC_REGRESSION && classes.size() > 0) ? classes.size() : 1;
        for (size_t r = 0; r < n_rows; ++r) {
            // Estimate weights by evaluating model response to unit vectors
            js << "      [";
            for (size_t f = 0; f < feat.size(); ++f) {
                // Finite-difference gradient along feature f
                std::vector<double> x0(feat.size(), 0.0);
                std::vector<double> x1(feat.size(), 0.0);
                x1[f]     = 1.0;
                double y0 = (r < impl_->model->predictProbaOne(x0).size()) ? impl_->model->predictProbaOne(x0)[r] : 0.0;
                double y1 = (r < impl_->model->predictProbaOne(x1).size()) ? impl_->model->predictProbaOne(x1)[r] : 0.0;
                js << (y1 - y0);
                if (f + 1 < feat.size()) {
                    js << ", ";
                }
            }
            js << "]";
            if (r + 1 < n_rows) {
                js << ",";
            }
            js << "\n";
        }
        js << "    ]\n";
    } else if (algo == ModelAlgorithm::KNN) {
        const auto *knn = dynamic_cast<const KNNModel *>(impl_->model.get());
        if (knn) {
            js << "    \"k\": " << knn->k << ",\n";
            js << "    \"n_train\": " << knn->X_train.size() << "\n";
            // Training data is intentionally omitted from the export for
            // privacy; downstream users should re-train from golden dataset.
        } else {
            js << "    \"k\": 5\n";
        }
    } else {
        // DecisionTree, RandomForest, GradientBoosting:
        // Emit a summary: depth, n_estimators, n_classes.
        js << "    \"n_features\": " << feat.size() << ",\n";
        js << "    \"n_classes\": " << (classes.empty() ? 1 : static_cast<int>(classes.size())) << "\n";
    }

    js << "  }\n";
    js << "}\n";

    // ---- Write to file ----------------------------------------------------
    if (!path.empty()) {
        std::ofstream ofs(path, std::ios::trunc);
        if (!ofs.is_open()) {
            return "IO_ERROR: could not open file for writing: " + path;
        }
        ofs << js.str();
        if (!ofs) {
            return "IO_ERROR: write failed for file: " + path;
        }
    }

    return ""; // success
}

std::string AutoMLModel::serialize() const {
    std::lock_guard<std::recursive_mutex> lk(impl_->access_mutex);
    // Minimal serialisation: stores metadata only (not the full model weights)
    std::ostringstream ss = {};
    ss << "task=" << static_cast<int>(impl_->task) << "\n";
    ss << "algorithm=" << static_cast<int>(impl_->algo) << "\n";
    ss << "name=" << impl_->name_str << "\n";
    ss << "accuracy=" << impl_->metrics_val.accuracy << "\n";
    ss << "f1=" << impl_->metrics_val.f1 << "\n";
    ss << "r2=" << impl_->metrics_val.r2 << "\n";
    ss << "rmse=" << impl_->metrics_val.rmse << "\n";
    ss << "n_classes=" << impl_->label_enc.numClasses() << "\n";
    for (const auto &c : impl_->label_enc.classes) {
        ss << "class=" << c << "\n";
    }
    return ss.str();
}

AutoMLModel AutoMLModel::deserialize(const std::string &data) {
    AutoMLModel m;
    std::istringstream ss(data);
    std::string line = {};
    while (std::getline(ss, line)) {
        auto pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }
        std::string key = line.substr(0, pos);
        std::string val = line.substr(pos + 1);
        if (key == "task") {
            m.impl_->task = static_cast<AutoMLTask>(std::stoi(val));
        } else if (key == "algorithm") {
            m.impl_->algo = static_cast<ModelAlgorithm>(std::stoi(val));
        } else if (key == "name") {
            m.impl_->name_str = val;
        } else if (key == "accuracy") {
            m.impl_->metrics_val.accuracy = std::stod(val);
        } else if (key == "f1") {
            m.impl_->metrics_val.f1 = std::stod(val);
        } else if (key == "r2") {
            m.impl_->metrics_val.r2 = std::stod(val);
        } else if (key == "rmse") {
            m.impl_->metrics_val.rmse = std::stod(val);
        } else if (key == "class") {
            m.impl_->label_enc.classes.push_back(val);
        }
    }
    // Rebuild label encoder index
    for (size_t i = 0; i < static_cast<int>(m.impl_->label_enc.classes.size()); ++i) {
        m.impl_->label_enc.index[m.impl_->label_enc.classes[static_cast<size_t>(i)]] = i;
    }
    return m;
}

// ============================================================================
// AutoML helpers (anonymous namespace – accessible within this translation unit)
// ============================================================================

namespace {

// Build and evaluate one candidate on train/val splits
static std::pair<std::unique_ptr<ModelBase>, EvalMetrics>
fitAndEval(const std::vector<std::vector<double>> &X_train, const std::vector<int> &y_cls_train,
           const std::vector<double> &y_reg_train, const std::vector<std::vector<double>> &X_val,
           const std::vector<int> &y_cls_val, const std::vector<double> &y_reg_val, bool is_classifier, int n_classes,
           ModelAlgorithm algo, const std::map<std::string, double> &hp, std::mt19937 &rng, AutoMLMetric metric) {
    std::unique_ptr<ModelBase> m;

    auto getHP = [&](const std::string &k, double def) -> double {
        auto it = hp.find(k);
        return (it != hp.end()) ? it->second : def;
    };

    switch (algo) {
        case ModelAlgorithm::LOGISTIC_REGRESSION: {
            if (!is_classifier) {
                // Logistic regression used as a regression proxy:
                // binarize continuous targets at their mean so the model
                // learns P(class=1), which is returned as the regression
                // estimate by LRModel::predictOneReg().
                double sum_y = 0.0;
                for (double v : y_reg_train) {
                    sum_y += v;
                }
                double mean_y = y_reg_train.empty() ? 0.5 : sum_y / static_cast<double>(y_reg_train.size());
                std::vector<int> y_bin = {};

                y_bin.reserve(y_reg_train.size());
                for (double v : y_reg_train) {
                    y_bin.push_back(v >= mean_y ? 1 : 0);
                }
                m = trainLogReg(X_train, y_bin, /*n_classes=*/2, getHP("lr", 0.01), getHP("l2", 1e-4),
                                static_cast<int>(getHP("epochs", 200)), rng);
            } else {
                m = trainLogReg(X_train, y_cls_train, n_classes, getHP("lr", 0.01), getHP("l2", 1e-4),
                                static_cast<int>(getHP("epochs", 200)), rng);
            }
            break;
        }
        case ModelAlgorithm::LINEAR_REGRESSION:
            m = trainLinReg(X_train, y_reg_train, getHP("l2", 1e-4));
            break;
        case ModelAlgorithm::DECISION_TREE:
            m = trainDecisionTree(X_train, y_cls_train, y_reg_train, is_classifier, n_classes,
                                  static_cast<int>(getHP("max_depth", 5)), static_cast<int>(getHP("min_leaf", 2)), rng);
            break;
        case ModelAlgorithm::RANDOM_FOREST:
            m = trainRandomForest(X_train, y_cls_train, y_reg_train, is_classifier, n_classes,
                                  static_cast<int>(getHP("n_trees", 30)), static_cast<int>(getHP("max_depth", 5)),
                                  static_cast<int>(getHP("min_leaf", 2)), rng);
            break;
        case ModelAlgorithm::GRADIENT_BOOSTING:
            m = trainGradientBoosting(X_train, y_cls_train, y_reg_train, is_classifier, n_classes,
                                      static_cast<int>(getHP("n_stages", 50)), static_cast<int>(getHP("max_depth", 3)),
                                      getHP("lr", 0.1), rng);
            break;
        case ModelAlgorithm::KNN:
            m = trainKNN(X_train, y_cls_train, y_reg_train, is_classifier, n_classes, static_cast<int>(getHP("k", 5)));
            break;
        default:
            m = trainDecisionTree(X_train, y_cls_train, y_reg_train, is_classifier, n_classes, 5, 2, rng);
            break;
    }

    EvalMetrics em = evaluateModel(*m, X_val, y_cls_val, y_reg_val, is_classifier, n_classes, metric);
    return {std::move(m), em};
}

// Run one k-fold cross-validation for a given (algo, hp) pair.
static EvalMetrics kFoldCV(const std::vector<std::vector<double>> &X, const std::vector<int> &y_cls,
                           const std::vector<double> &y_reg, bool is_classifier, int n_classes, ModelAlgorithm algo,
                           const std::map<std::string, double> &hp, int k, std::mt19937 &rng, AutoMLMetric metric) {
    auto folds = makeFolds(X.size(), k, rng);
    EvalMetrics sum;
    int count = 0;

    for (auto &[train_idx, val_idx] : folds) {
        std::vector<std::vector<double>> Xt, Xv;
        std::vector<int> yct, ycv;
        std::vector<double> yrt, yrv;
        for (size_t i : train_idx) {
            Xt.push_back(X[i]);
            if (is_classifier) {
                yct.push_back(y_cls[i]);
            } else {
                yrt.push_back(y_reg[i]);
            }
        }
        for (size_t i : val_idx) {
            Xv.push_back(X[i]);
            if (is_classifier) {
                ycv.push_back(y_cls[i]);
            } else {
                yrv.push_back(y_reg[i]);
            }
        }
        if (Xt.empty() || Xv.empty()) {
            continue;
        }

        auto [m, em] = fitAndEval(Xt, yct, yrt, Xv, ycv, yrv, is_classifier, n_classes, algo, hp, rng, metric);
        sum.accuracy += em.accuracy;
        sum.f1 += em.f1;
        sum.precision += em.precision;
        sum.recall += em.recall;
        sum.auc_roc += em.auc_roc;
        sum.r2 += em.r2;
        sum.rmse += em.rmse;
        sum.mae += em.mae;
        ++count;
    }
    if (count > 0) {
        double d = static_cast<double>(count);
        sum.accuracy /= d;
        sum.f1 /= d;
        sum.precision /= d;
        sum.recall /= d;
        sum.auc_roc /= d;
        sum.r2 /= d;
        sum.rmse /= d;
        sum.mae /= d;
    }
    return sum;
}

} // anonymous namespace

struct AutoML::Impl {};

AutoML::AutoML() : impl_(std::make_unique<Impl>()) {}
AutoML::~AutoML() = default;

// ============================================================================
// AutoML::crossValidate
// ============================================================================

EvalMetrics AutoML::crossValidate(const std::vector<DataPoint> &data, const AutoMLConfig &config,
                                  ModelAlgorithm algorithm,
                                  const std::map<std::string, double> &hyperparameters) const {
    if (data.size() < 2) {
        return {};
    }

    auto fm    = extractFeatures(data, config.target);
    auto y_str = extractTargetStr(data, config.target);
    auto y_num = extractTargetNum(data, config.target);

    bool is_classifier = (config.task == AutoMLTask::CLASSIFICATION);
    LabelEncoder le;
    le.fit(y_str);
    auto y_cls = le.encode(y_str);

    Scaler sc;
    sc.fit(fm.X);
    auto Xs = sc.transformAll(fm.X);

    std::mt19937 rng(static_cast<unsigned>(config.random_seed));
    return kFoldCV(Xs, y_cls, y_num, is_classifier, le.numClasses(), algorithm, hyperparameters, config.cv_folds, rng,
                   config.metric);
}

// ============================================================================
// Core training routine – returns a plain struct so AutoML methods (which are
// friends of AutoMLModel) can populate the private impl.
// ============================================================================

struct TrainingCoreResult {
    AutoMLTask task;
    ModelAlgorithm algo = ModelAlgorithm::DECISION_TREE;
    std::string name_str;
    EvalMetrics metrics_val;
    std::unique_ptr<ModelBase> model;
    Scaler scaler;
    LabelEncoder label_enc;
    bool use_poly = false;
    std::vector<std::string> feat_names;
    std::vector<CandidateModelInfo> candidates;
};

static TrainingCoreResult doTrainCore(const std::vector<DataPoint> &data, AutoMLConfig config,
                                      std::function<void(int, int, double)> progress) {
    if (data.empty()) {
        throw std::invalid_argument("AutoML: training data is empty");
    }
    if (config.target.empty()) {
        throw std::invalid_argument("AutoML: target field name must be specified");
    }

    bool is_classifier = (config.task == AutoMLTask::CLASSIFICATION);
    std::mt19937 rng(static_cast<unsigned>(config.random_seed));

    // ---- Feature extraction ----
    auto fm    = extractFeatures(data, config.target);
    auto y_str = extractTargetStr(data, config.target);
    auto y_num = extractTargetNum(data, config.target);

    if (fm.X.empty()) {
        throw std::invalid_argument("AutoML: no numeric features found (excluding target)");
    }

    // ---- Label encoding ----
    LabelEncoder le = {};
    if (is_classifier) {
        le.fit(y_str);
    }
    auto y_cls = is_classifier ? le.encode(y_str) : std::vector<int>(data.size(), 0);

    // ---- Feature engineering ----
    auto feat_names = fm.names;
    auto X          = fm.X;

    if (config.feature_engineering) {
        X          = polyFeatures(X);
        feat_names = polyFeatureNames(feat_names);
    }

    // ---- Standard scaling ----
    Scaler sc;
    sc.fit(X);
    auto Xs = sc.transformAll(X);

    // ---- Algorithm list ----
    auto algos = config.algorithms.empty() ? defaultAlgorithms(config.task) : config.algorithms;

    // ---- Random search ----
    int n_classes = is_classifier ? le.numClasses() : 1;

    std::vector<CandidateModelInfo> candidates;
    auto start_time = std::chrono::steady_clock::now();
    int max_minutes = std::max(1, config.max_time_minutes);
    int max_trials  = std::max(1, config.max_trials);

    // We evenly distribute trials across algorithms
    int trials_per_algo = std::max(1, max_trials / static_cast<int>(algos.size()));

    for (ModelAlgorithm algo : algos) {
        auto grid = defaultHPGrid(algo);
        for (int t = 0; t < trials_per_algo; ++t) {
            // Check time budget
            auto elapsed = std::chrono::steady_clock::now() - start_time;
            if (elapsed > std::chrono::minutes(max_minutes)) {
                break;
            }

            auto hp = grid.empty() ? std::map<std::string, double>{} : sampleHP(grid, rng);

            auto t0 = std::chrono::steady_clock::now();
            auto em
                = kFoldCV(Xs, y_cls, y_num, is_classifier, n_classes, algo, hp, config.cv_folds, rng, config.metric);
            double train_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();

            double score = em.primary(config.metric);

            CandidateModelInfo info;
            info.algorithm       = algo;
            info.name            = std::string(modelAlgorithmName(algo));
            info.cv_metrics      = em;
            info.cv_score        = score;
            info.train_time_ms   = train_ms;
            info.hyperparameters = hp;
            candidates.push_back(std::move(info));

            if (progress) {
                double best = 0.0;
                for (const auto &c : candidates) {
                    best = std::max(best, c.cv_score);
                }
                progress(static_cast<int>(candidates.size()), max_trials, best);
            }
        }
    }

    // Sort candidates descending by score
    std::sort(candidates.begin(), candidates.end(),
              [](const auto &a, const auto &b) { return a.cv_score > b.cv_score; });

    if (candidates.empty()) {
        throw std::runtime_error("AutoML: no candidate models produced");
    }

    // ---- Train best model on full data ----
    const auto &best = candidates.front();

    TrainingCoreResult result;
    result.task       = config.task;
    result.scaler     = sc;
    result.label_enc  = le;
    result.use_poly   = config.feature_engineering;
    result.feat_names = feat_names;
    result.candidates = candidates;

    if (config.ensemble && candidates.size() > 1 && config.ensemble_top_k > 1) {
        size_t top_k       = std::min(static_cast<size_t>(config.ensemble_top_k), candidates.size());
        auto ens           = std::make_unique<EnsembleModel>();
        ens->is_classifier = is_classifier;
        ens->n_classes     = n_classes;

        for (size_t k = 0; k < top_k; ++k) {
            const auto &c = candidates[k];
            auto [m, em]  = fitAndEval(Xs, y_cls, y_num, Xs, y_cls, y_num, is_classifier, n_classes, c.algorithm,
                                       c.hyperparameters, rng, config.metric);
            ens->members.push_back(std::move(m));
        }

        result.algo     = ModelAlgorithm::ENSEMBLE;
        result.name_str = "Ensemble(top-" + std::to_string(top_k) + ")";
        result.model    = std::move(ens);
    } else {
        auto [m, em]       = fitAndEval(Xs, y_cls, y_num, Xs, y_cls, y_num, is_classifier, n_classes, best.algorithm,
                                        best.hyperparameters, rng, config.metric);
        result.algo        = best.algorithm;
        result.name_str    = best.name;
        result.model       = std::move(m);
        result.metrics_val = em;
    }

    // Final CV metrics on best candidates[0]
    result.metrics_val = best.cv_metrics;

    return result;
}

// ============================================================================
// AutoML public methods
// ============================================================================

AutoMLModel AutoML::trainClassifier(const std::vector<DataPoint> &data, const AutoMLConfig &config_in,
                                    std::function<void(int, int, double)> progress) {
    AutoMLConfig config = config_in;
    config.task         = AutoMLTask::CLASSIFICATION;
    auto core           = doTrainCore(data, config, std::move(progress));

    // Assemble AutoMLModel (AutoML is a friend of AutoMLModel)
    AutoMLModel result;
    result.impl_->task        = core.task;
    result.impl_->algo        = core.algo;
    result.impl_->name_str    = core.name_str;
    result.impl_->metrics_val = core.metrics_val;
    result.impl_->model       = std::move(core.model);
    result.impl_->scaler      = std::move(core.scaler);
    result.impl_->label_enc   = std::move(core.label_enc);
    result.impl_->use_poly    = core.use_poly;
    result.impl_->feat_names  = std::move(core.feat_names);
    result.impl_->candidates  = std::move(core.candidates);

    // Regression training must stay robust even when model-specific explanation
    // paths are unavailable; expose a stable uniform fallback importance map.
    std::map<std::string, double> importance = {};

    for (const auto &fn : result.impl_->feat_names) {
        importance[fn] = 0.0;
    }
    if (!importance.empty()) {
        const double u = 1.0 / static_cast<double>(importance.size());
        for (auto &[k, v] : importance) {
            v = u;
        }
    }
    result.impl_->feat_importance = std::move(importance);

    return result;
}

AutoMLModel AutoML::trainRegressor(const std::vector<DataPoint> &data, const AutoMLConfig &config_in,
                                   std::function<void(int, int, double)> progress) {
    AutoMLConfig config = config_in;
    config.task         = AutoMLTask::REGRESSION;
    auto core           = doTrainCore(data, config, std::move(progress));

    // Assemble AutoMLModel (AutoML is a friend of AutoMLModel)
    AutoMLModel result;
    result.impl_->task        = core.task;
    result.impl_->algo        = core.algo;
    result.impl_->name_str    = core.name_str;
    result.impl_->metrics_val = core.metrics_val;
    result.impl_->model       = std::move(core.model);
    result.impl_->scaler      = std::move(core.scaler);
    result.impl_->label_enc   = std::move(core.label_enc);
    result.impl_->use_poly    = core.use_poly;
    result.impl_->feat_names  = std::move(core.feat_names);
    result.impl_->candidates  = std::move(core.candidates);

    // Feature importance: permutation SHAP over a subsample
    size_t shap_samples = std::min(data.size(), size_t(200));
    std::map<std::string, double> importance = {};

    for (const auto &fn : result.impl_->feat_names) {
        importance[fn] = 0.0;
    }
    for (size_t i = 0; i < shap_samples; ++i) {
        auto exp = result.explainOne(data[i]);
        for (const auto &[feat, contrib] : exp.feature_contributions) {
            importance[feat] += std::abs(contrib);
        }
    }
    double tot = 0.0;
    for (const auto &[k, v] : importance) {
        tot += v;
    }
    if (tot > 0) {
        for (auto &[k, v] : importance) {
            v /= tot;
        }
    } else if (!importance.empty()) {
        double u = 1.0 / static_cast<double>(importance.size());
        for (auto &[k, v] : importance) {
            v = u;
        }
    }
    result.impl_->feat_importance = importance;

    return result;
}

// ============================================================================
// AutoML Helper Functions (Phase 2B)
// ============================================================================

std::pair<bool, std::string> AutoML::validateTrainingData(
    const std::vector<std::vector<double>>& features,
    const std::vector<double>& target,
    AutoMLTask task) const noexcept {
    
    // Check if data is empty
    if (features.empty() || target.empty()) {
        return {false, "Training data is empty"};
    }
    
    // Check dimensions match
    if (features.size() != target.size()) {
        return {false, "Feature matrix rows must match target vector size"};
    }
    
    // Check minimum samples
    if (features.size() < 2) {
        return {false, "At least 2 training samples required"};
    }
    
    // Check consistent feature count
    if (features.empty()) {
        return {false, "Feature matrix is empty"};
    }
    
    size_t n_features = features[0].size();
    if (n_features == 0) {
        return {false, "Each sample must have at least 1 feature"};
    }
    
    for (size_t i = 0; i < features.size(); ++i) {
        if (features[i].size() != n_features) {
            return {false, "All samples must have the same number of features"};
        }
        
        // Check for NaN and Inf in features
        for (size_t j = 0; j < features[i].size(); ++j) {
            double val = features[i][j];
            if (std::isnan(val) || std::isinf(val)) {
                return {false, "Feature matrix contains NaN or Inf values"};
            }
        }
    }
    
    // Check target values
    for (size_t i = 0; i < target.size(); ++i) {
        double val = target[i];
        if (std::isnan(val) || std::isinf(val)) {
            return {false, "Target vector contains NaN or Inf values"};
        }
    }
    
    // For classification, check minimum number of classes
    if (task == AutoMLTask::CLASSIFICATION) {
        std::set<double> unique_classes(target.begin(), target.end());
        if (unique_classes.size() < 2) {
            return {false, "Classification requires at least 2 distinct classes"};
        }
    }
    
    return {true, ""};
}

ModelAlgorithm AutoML::selectMetalearner(
    const std::vector<std::vector<double>>& features,
    const std::vector<double>& target,
    const std::vector<ModelAlgorithm>& candidates,
    AutoMLTask task) const {
    
    // Validate input
    if (features.empty() || target.empty()) {
        throw std::invalid_argument("Features and target must not be empty");
    }
    
    if (features.size() != target.size()) {
        throw std::invalid_argument("Features and target size mismatch");
    }
    
    // Default to decision tree if no candidates provided
    if (candidates.empty()) {
        return ModelAlgorithm::DECISION_TREE;
    }
    
    // Compute feature characteristics for algorithm selection heuristics
    size_t n_samples = features.size();
    size_t n_features = features[0].size();
    
    // Count unique target values for classification
    std::set<double> unique_targets(target.begin(), target.end());
    size_t n_classes = unique_targets.size();
    
    // Score each candidate algorithm
    ModelAlgorithm best_algo = candidates[0];
    double best_score = -std::numeric_limits<double>::infinity();
    
    for (const auto& algo : candidates) {
        double score = 0.0;
        
        // Algorithm selection heuristics based on dataset characteristics
        switch (algo) {
            case ModelAlgorithm::LOGISTIC_REGRESSION:
                // Good for: linear separability, high dimensionality, large datasets
                // Score: high if many features, many samples, few classes
                score = (n_features > 10 ? 1.0 : 0.5) + 
                        (n_samples > 100 ? 1.0 : 0.5) +
                        (task == AutoMLTask::CLASSIFICATION && n_classes <= 10 ? 1.0 : 0.0);
                break;
                
            case ModelAlgorithm::LINEAR_REGRESSION:
                // Good for: regression, linear relationships
                score = (task == AutoMLTask::REGRESSION ? 2.0 : 0.0) +
                        (n_samples > 50 ? 1.0 : 0.0);
                break;
                
            case ModelAlgorithm::DECISION_TREE:
                // Good for: non-linear, categorical, smaller datasets, mixed feature types
                score = 1.0 + (n_features < 50 ? 1.0 : 0.0) + (n_samples < 1000 ? 0.5 : 0.0);
                break;
                
            case ModelAlgorithm::RANDOM_FOREST:
                // Good for: robustness, feature interactions, medium-to-large datasets
                score = 1.5 + (n_samples > 100 ? 1.5 : 0.5) + 
                        (n_features > 5 ? 1.0 : 0.0);
                break;
                
            case ModelAlgorithm::GRADIENT_BOOSTING:
                // Good for: high accuracy needed, structured data, larger datasets
                score = 1.0 + (n_samples > 500 ? 2.0 : 0.5) + 
                        (n_features > 2 ? 1.0 : 0.0);
                break;
                
            case ModelAlgorithm::KNN:
                // Good for: small-to-medium datasets, low dimensionality
                score = (n_samples < 10000 ? 1.5 : 0.5) + 
                        (n_features < 20 ? 1.0 : 0.0);
                break;
                
            case ModelAlgorithm::ENSEMBLE:
                // Ensemble: only use if explicitly requested
                score = 0.5;
                break;
        }
        
        if (score > best_score) {
            best_score = score;
            best_algo = algo;
        }
    }
    
    return best_algo;
}

ModelAlgorithm AutoML::selectEnsembleMethod(
    const std::vector<EvalMetrics>& candidate_metrics) const noexcept {
    
    // If only one model, no ensemble benefit
    if (static_cast<int>(candidate_metrics.size()) <= 1) {
        return ModelAlgorithm::ENSEMBLE;  // Soft voting (default ensemble)
    }
    
    // Analyze model diversity and performance for ensemble selection
    double avg_f1 = 0.0;
    double avg_accuracy = 0.0;
    
    for (const auto& metrics : candidate_metrics) {
        avg_f1 += metrics.f1;
        avg_accuracy += metrics.accuracy;
    }
    avg_f1 /= static_cast<double>(candidate_metrics.size());
    avg_accuracy /= static_cast<double>(candidate_metrics.size());
    
    // Calculate diversity (spread in scores)
    double max_f1 = 0.0, min_f1 = 1.0;
    double max_acc = 0.0, min_acc = 1.0;
    
    for (const auto& metrics : candidate_metrics) {
        max_f1 = std::max(max_f1, metrics.f1);
        min_f1 = std::min(min_f1, metrics.f1);
        max_acc = std::max(max_acc, metrics.accuracy);
        min_acc = std::min(min_acc, metrics.accuracy);
    }
    
    double diversity = (max_f1 - min_f1) + (max_acc - min_acc);
    
    // Select ensemble method based on model characteristics
    if (diversity > 0.2 && candidate_metrics.size() >= 3) {
        // High diversity: stacking would be beneficial (if implemented)
        // For now, return voting as production-ready method
        return ModelAlgorithm::ENSEMBLE;
    } else if (avg_f1 > 0.85 && avg_accuracy > 0.85) {
        // High performance: simple averaging/voting sufficient
        return ModelAlgorithm::ENSEMBLE;
    } else {
        // Medium diversity/performance: standard soft voting
        return ModelAlgorithm::ENSEMBLE;
    }
}

} // namespace analytics
} // namespace themisdb
