/**
 * @file rotate_completion.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief A (head, relation, tail) triple for training or scoring.
 */
struct KGTriple {
    std::string head = {};
    std::string relation;
    std::string tail = {};
};

/**
 * @brief Scored link prediction result.
 */
struct LinkPrediction {
    std::string entity;        ///< Predicted entity (head or tail)
    double      score = 0.0;   ///< Distance score (lower = more plausible)
    double      rank  = 0.0;   ///< Predicted rank among all entities (1-based)
};

/**
 * @brief Training configuration for RotatEModel.
 */
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

/**
 * @brief Aggregated training metrics.
 */
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

/**
 * @brief RotatE embedding model: entity/relation embeddings + training loop.
 *
 * Thread-safety: all public methods acquire an internal mutex and are safe to
 * call concurrently from multiple threads.  Concurrent calls to `train()` are
 * serialised.
 */
class RotatEModel {
public:
    explicit RotatEModel(RotatEConfig cfg = {});
    ~RotatEModel();

    // ------------------------------------------------------------------
    // Entity / relation registry
    // ------------------------------------------------------------------

    /**
     * @brief Register a new entity.  Ignored if already registered.
     * @return Internal numeric index assigned to the entity.
     */
    size_t addEntity(const std::string& id);

    /**
     * @brief Register a new relation type.  Ignored if already registered.
     * @return Internal numeric index assigned to the relation.
     */
    size_t addRelation(const std::string& id);

    /// Return the number of registered entities.
    size_t entityCount() const;

    /// Return the number of registered relations.
    size_t relationCount() const;

    // ------------------------------------------------------------------
    // Training
    // ------------------------------------------------------------------

    /**
     * @brief Train the RotatE model on the supplied positive triples.
     *
     * Internally generates negative samples per the config and minimises the
     * self-adversarial negative sampling loss.
     *
     * @param triples  Positive (h, r, t) triples; all entities and relations
     *                 must have been registered via addEntity()/addRelation().
     * @return Training result with final loss and epoch count.
     * @throws std::invalid_argument if any triple references an unregistered entity/relation.
     */
    RotatETrainResult train(const std::vector<KGTriple>& triples);

    // ------------------------------------------------------------------
    // Scoring
    // ------------------------------------------------------------------

    /**
     * @brief Compute the RotatE distance score for a triple.
     *
     * Lower scores indicate more plausible triples.
     *
     * @param h  Head entity id.
     * @param r  Relation id.
     * @param t  Tail entity id.
     * @return   Distance score ‖h ∘ r − t‖₁.
     * @throws   std::out_of_range if h, r, or t is not registered.
     */
    double score(const std::string& h,
                 const std::string& r,
                 const std::string& t) const;

    /**
     * @brief Return true if the model has been trained (train() called at least once).
     */
    bool isTrained() const;

    // ------------------------------------------------------------------
    // Embedding access (for external benchmarking)
    // ------------------------------------------------------------------

    /**
     * @brief Export entity embedding (real + imaginary parts interleaved).
     * @return Vector of length 2 × embedding_dim, or empty if not trained.
     */
    std::vector<float> entityEmbedding(const std::string& id) const;

    /**
     * @brief Export relation phase embedding.
     * @return Vector of length embedding_dim, or empty if not trained.
     */
    std::vector<float> relationPhase(const std::string& id) const;

    /**
     * @brief Rank all entities as tail predictions for (head, relation, ?).
     * @return Sorted (ascending score) list of all entities.
     */
    std::vector<LinkPrediction> rankTail(const std::string& head,
                                          const std::string& relation,
                                          size_t             top_k) const;

    /**
     * @brief Rank all entities as head predictions for (?, relation, tail).
     * @return Sorted (ascending score) list of all entities.
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

/**
 * @brief Link-prediction head: given (head, relation) predict top-k tails,
 * or given (relation, tail) predict top-k heads.
 */
class LinkPredictionHead {
public:
    explicit LinkPredictionHead(RotatEModel& model);

    /**
     * @brief Predict the most plausible tail entities for (head, relation, ?).
     *
     * @param head      Head entity id.
     * @param relation  Relation id.
     * @param top_k     Maximum number of candidates to return.
     * @return Sorted (ascending score) list of link predictions.
     */
    std::vector<LinkPrediction> predictTail(const std::string& head,
                                             const std::string& relation,
                                             size_t             top_k = 10) const;

    /**
     * @brief Predict the most plausible head entities for (?, relation, tail).
     *
     * @param relation  Relation id.
     * @param tail      Tail entity id.
     * @param top_k     Maximum number of candidates to return.
     * @return Sorted (ascending score) list of link predictions.
     */
    std::vector<LinkPrediction> predictHead(const std::string& relation,
                                             const std::string& tail,
                                             size_t             top_k = 10) const;

private:
    RotatEModel& model_;
};

// ============================================================================
// KGCompletionEngine
// ============================================================================

/**
 * @brief High-level orchestrator: RotatEModel + LinkPredictionHead
 *        with optional KnowledgeGraphReasoner integration.
 */
class KGCompletionEngine {
public:
    explicit KGCompletionEngine(RotatEConfig cfg = {});

    // ------------------------------------------------------------------
    // Reasoner integration
    // ------------------------------------------------------------------

    /**
     * @brief Wire a KnowledgeGraphReasoner so that predicted links with score
     *        below `inject_threshold` are added as inferred facts.
     *
     * @param reasoner          Reasoner to inject predicted triples into.
     * @param inject_threshold  RotatE distance threshold (lower = confident).
     *                          Only predictions with score < threshold are injected.
     */
    void setReasoner(KnowledgeGraphReasoner* reasoner,
                     double                  inject_threshold = 2.0);

    // ------------------------------------------------------------------
    // Building the model
    // ------------------------------------------------------------------

    /// Register an entity.  Delegates to RotatEModel::addEntity().
    size_t addEntity(const std::string& id);

    /// Register a relation type.  Delegates to RotatEModel::addRelation().
    size_t addRelation(const std::string& id);

    /**
     * @brief Train the RotatE model on the supplied triples.
     * @param triples Positive (h, r, t) training triples.
     */
    RotatETrainResult train(const std::vector<KGTriple>& triples);

    // ------------------------------------------------------------------
    // Inference
    // ------------------------------------------------------------------

    /**
     * @brief Predict top-k tail completions for (head, relation, ?).
     *        If a reasoner is wired, high-confidence predictions are injected.
     */
    std::vector<LinkPrediction> completeTail(const std::string& head,
                                              const std::string& relation,
                                              size_t             top_k = 10);

    /**
     * @brief Predict top-k head completions for (?, relation, tail).
     */
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
