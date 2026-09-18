/**
 * @file rotate_completion.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "graph/knowledge_graph_reasoner.h"

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace graph {

// ============================================================================
// Supporting structures
// ============================================================================

struct KGTriple {
    std::string head = {};
    std::string relation;
    std::string tail = {};
};

struct LinkPrediction {
    std::string entity;        ///< Predicted entity (head or tail)
    double      score = 0.0;   ///< Distance score (lower = more plausible)
    double      rank  = 0.0;   ///< Predicted rank among all entities (1-based)
};

struct RotatEConfig {
    size_t embedding_dim    = 64;    ///< Complex embedding dimension d (total 2d floats)
    size_t neg_samples      = 64;    ///< Negative samples per positive triple
    size_t epochs           = 100;   ///< Training epochs
    float  learning_rate    = 1e-3f; ///< SGD / Adam learning rate
    float  margin           = 6.0f;  ///< Margin γ for self-adversarial loss
    float  adv_temperature  = 0.5f;  ///< Temperature for adversarial weight
    bool   uniform_neg      = true;  ///< True: uniform negative sampling; false: self-adversarial
    size_t batch_size       = 512;   ///< Training batch size (triples per step)
};

struct RotatETrainResult {
    bool   success         = false;
    double final_loss      = 0.0;
    size_t epochs_run      = 0;
    size_t entities        = 0;
    size_t relations       = 0;
    size_t triples         = 0;
};

// ============================================================================
// RotatEModel
// ============================================================================

class RotatEModel {
public:
    explicit RotatEModel(RotatEConfig cfg = {});
    ~RotatEModel();

    // ------------------------------------------------------------------
    // Entity / relation registry
    // ------------------------------------------------------------------

    /**
     * @brief Add Entity.
     * @param[in] id Input parameter.
     * @return Return value.
     */
    size_t addEntity(const std::string& id);

    /**
     * @brief Add Relation.
     * @param[in] id Input parameter.
     * @return Return value.
     */
    size_t addRelation(const std::string& id);

    /**
     * @brief Entity Count.
     * @return Return value.
     */
    size_t entityCount() const;

    /**
     * @brief Relation Count.
     * @return Return value.
     */
    size_t relationCount() const;

    // ------------------------------------------------------------------
    // Training
    // ------------------------------------------------------------------

    /**
     * @brief Train.
     * @param[in] triples Input parameter.
     * @return Return value.
     */
    RotatETrainResult train(const std::vector<KGTriple>& triples);

    // ------------------------------------------------------------------
    // Scoring
    // ------------------------------------------------------------------

    /**
     * @brief Score.
     * @param[in] h Input parameter.
     * @param[in] r Input parameter.
     * @param[in] t Input parameter.
     * @return Return value.
     */
    double score(const std::string& h,
                 const std::string& r,
                 const std::string& t) const;

    /**
     * @brief Is Trained.
     * @return True when the operation succeeds.
     */
    bool isTrained() const;

    /**
     * @brief ------------------------------------------------------------------ Embedding access (for external benchmarking) ------------------------------------------------------------------
     * @param[in] id Input parameter.
     * @return Return value.
     */

    std::vector<float> entityEmbedding(const std::string& id) const;

    /**
     * @brief Relation Phase.
     * @param[in] id Input parameter.
     * @return Return value.
     */
    std::vector<float> relationPhase(const std::string& id) const;

    /**
     * @brief Rank Tail.
     * @param[in] head Input parameter.
     * @param[in] relation Input parameter.
     * @param[in] top_k Input parameter.
     * @return Return value.
     */
    std::vector<LinkPrediction> rankTail(const std::string& head,
                                          const std::string& relation,
                                          size_t             top_k) const;

    /**
     * @brief Rank Head.
     * @param[in] relation Input parameter.
     * @param[in] tail Input parameter.
     * @param[in] top_k Input parameter.
     * @return Return value.
     */
    std::vector<LinkPrediction> rankHead(const std::string& relation,
                                          const std::string& tail,
                                          size_t             top_k) const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// LinkPredictionHead
// ============================================================================

class LinkPredictionHead {
public:
    /**
     * @brief Link Prediction Head.
     * @param[in,out] model Input/output parameter.
     * @return Return value.
     */
    explicit LinkPredictionHead(RotatEModel& model);

    std::vector<LinkPrediction> predictTail(const std::string& head,
                                             const std::string& relation,
                                             size_t             top_k = 10) const;

    std::vector<LinkPrediction> predictHead(const std::string& relation,
                                             const std::string& tail,
                                             size_t             top_k = 10) const;

private:
    RotatEModel& model_;
};

// ============================================================================
// KGCompletionEngine
// ============================================================================

class KGCompletionEngine {
public:
    explicit KGCompletionEngine(RotatEConfig cfg = {});

    // ------------------------------------------------------------------
    // Reasoner integration
    // ------------------------------------------------------------------

    void setReasoner(KnowledgeGraphReasoner* reasoner,
                     double                  inject_threshold = 2.0);

    // ------------------------------------------------------------------
    // Building the model
    // ------------------------------------------------------------------

    /**
     * @brief Add Entity.
     * @param[in] id Input parameter.
     * @return Return value.
     */
    size_t addEntity(const std::string& id);

    /**
     * @brief Add Relation.
     * @param[in] id Input parameter.
     * @return Return value.
     */
    size_t addRelation(const std::string& id);

    /**
     * @brief Train.
     * @param[in] triples Input parameter.
     * @return Return value.
     */
    RotatETrainResult train(const std::vector<KGTriple>& triples);

    // ------------------------------------------------------------------
    // Inference
    // ------------------------------------------------------------------

    std::vector<LinkPrediction> completeTail(const std::string& head,
                                              const std::string& relation,
                                              size_t             top_k = 10);

    std::vector<LinkPrediction> completeHead(const std::string& relation,
                                              const std::string& tail,
                                              size_t             top_k = 10);

    // ------------------------------------------------------------------
    // Accessors
    // ------------------------------------------------------------------

    RotatEModel&           model()      noexcept { return model_; }
    LinkPredictionHead&    linkHead()   noexcept { return link_head_; }
    const RotatEConfig&    config()  const noexcept { return cfg_; }

private:
    RotatEConfig        cfg_;
    RotatEModel         model_;
    LinkPredictionHead  link_head_;
    KnowledgeGraphReasoner* reasoner_       = nullptr;
    double                  inject_threshold_ = 2.0;
};

} // namespace graph
} // namespace themis
