/**
 * @file learning_to_rank.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.43
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace themis {

/**
 * @brief Feature vector associated with a candidate result during ranking.
 */
struct RankingFeatures {
    double bm25_score = 0.0;      ///< BM25 fulltext relevance score
    double vector_score = 0.0;    ///< Normalized vector-similarity score
    double rrf_score = 0.0;       ///< Reciprocal Rank Fusion score
    double recency = 0.0;         ///< Document recency (0 = oldest, 1 = newest)
    double click_count = 0.0;     ///< Historical click frequency (normalized)
    double popularity = 0.0;      ///< Query-document co-occurrence popularity
};

/**
 * @brief A single candidate result with its ranking features.
 */
struct RankedResult {
    std::string document_id;       ///< Primary key of the document
    RankingFeatures features;      ///< Feature vector used for ranking
    double final_score = 0.0;      ///< Score after LTR re-ranking
};

/**
 * @brief Click-through event used to train the LTR model.
 */
struct ClickEvent {
    std::string query;             ///< Query that was issued
    std::string document_id;       ///< Document that was clicked
    size_t result_position = 0;    ///< 0-based position in the result list when clicked
};

/**
 * @brief Linear feature-based Learning-to-Rank (LTR) re-ranker.
 *
 * LearningToRank implements a supervised linear re-ranker trained from
 * click-through data.  It:
 *
 * 1. Accepts a list of pre-scored candidates (e.g. from HybridSearch) together
 *    with their `RankingFeatures`.
 * 2. Applies a learned weight vector to produce a `final_score` per candidate.
 * 3. Provides a `train()` method that updates weights via online gradient descent
 *    using click-through events (position-based implicit feedback).
 * 4. Offers an A/B variant selector (`selectVariant()`) for controlled experiments.
 *
 * ### Workflow
 * ```cpp
 * LearningToRank::Config cfg;
 * cfg.learning_rate = 0.01;
 * LearningToRank ltr(cfg);
 *
 * // Re-rank results
 * std::vector<RankedResult> candidates = buildCandidates(hs_results, features);
 * auto ranked = ltr.rerank(candidates);
 *
 * // Collect clicks and train
 * ltr.recordClick({"machine learning", "doc_42", 3});
 * ltr.train();
 * ```
 *
 * @note Thread Safety: A single LearningToRank instance is NOT thread-safe.
 * @note Exception Safety: All methods are noexcept at runtime; the constructor
 *   throws `std::invalid_argument` on invalid config.
 */
class LearningToRank {
public:
    struct Config {
        double learning_rate = 0.01;     ///< Gradient-descent step size
        size_t max_click_buffer = 1000;  ///< Maximum stored click events before training
        double regularization = 0.001;  ///< L2 regularization coefficient
        static Config defaults() { return {}; }
    };

    /**
     * @brief A/B test variant definition.
     */
    struct Variant {
        std::string name;                ///< Variant identifier (e.g. "control", "ltr_v2")
        std::function<double(const RankingFeatures&)> scorer; ///< Scoring function
        double traffic_fraction = 0.5;  ///< Fraction of traffic to send to this variant
    };

    /**
     * @param config  LTR configuration.
     * @throws std::invalid_argument on invalid config.
     */
    explicit LearningToRank(const Config& config = Config::defaults());

    // -----------------------------------------------------------------------
    // Re-ranking
    // -----------------------------------------------------------------------

    /**
     * @brief Re-rank a list of candidates using the current weight vector.
     *
     * Computes `final_score = w · features` for each candidate and returns
     * the list sorted by `final_score` descending.
     *
     * @param candidates  Candidates with feature vectors populated.
     * @return Sorted copy of candidates with `final_score` filled in.
     */
    std::vector<RankedResult> rerank(std::vector<RankedResult> candidates) const;

    /**
     * @brief Apply a specific named variant's scoring function to candidates.
     *
     * Returns the same sorted list as `rerank()` but uses the variant scorer.
     * Falls back to the default linear model if the variant is not found.
     *
     * @param candidates  Candidates to score.
     * @param variant_name  Name of the registered variant.
     * @return Re-ranked candidates.
     */
    std::vector<RankedResult> rerankWithVariant(std::vector<RankedResult> candidates,
                                                 const std::string& variant_name) const;

    // -----------------------------------------------------------------------
    // Training
    // -----------------------------------------------------------------------

    /**
     * @brief Record a click event for later training.
     */
    void recordClick(const ClickEvent& event);

    /**
     * @brief Update model weights using all buffered click events.
     *
     * Implements pairwise gradient descent: for each click event, the clicked
     * document is treated as more relevant than documents ranked above it.
     * Clears the click buffer after training.
     *
     * @return Number of click events used for training.
     */
    size_t train();

    /**
     * @brief Return the current feature weight vector.
     */
    RankingFeatures getWeights() const { return weights_; }

    /**
     * @brief Set feature weights directly (e.g. to load a pre-trained model).
     */
    void setWeights(const RankingFeatures& weights) { weights_ = weights; }

    // -----------------------------------------------------------------------
    // A/B Testing
    // -----------------------------------------------------------------------

    /**
     * @brief Register a named scoring variant for A/B experiments.
     */
    void registerVariant(const Variant& variant);

    /**
     * @brief Select a variant name for a given request key (deterministic hash).
     *
     * Routes a fraction of requests to the variant based on
     * `Variant::traffic_fraction`.
     *
     * @param request_key  Any string uniquely identifying the request (e.g. session ID).
     * @return Name of the selected variant, or empty string for the default model.
     */
    std::string selectVariant(const std::string& request_key) const;

    const Config& getConfig() const { return config_; }

private:
    Config config_;
    RankingFeatures weights_;           ///< Current model weights
    std::vector<ClickEvent> clicks_;    ///< Pending click events
    std::map<std::string, Variant> variants_; ///< Named A/B variants

    double score(const RankingFeatures& f) const;
    static double dot(const RankingFeatures& w, const RankingFeatures& f);
    static RankingFeatures gradient(const RankingFeatures& f_pos,
                                     const RankingFeatures& f_neg);
    static RankingFeatures addScaled(const RankingFeatures& w,
                                      const RankingFeatures& g, double lr);
    static RankingFeatures regularize(const RankingFeatures& w, double reg);
};

} // namespace themis
