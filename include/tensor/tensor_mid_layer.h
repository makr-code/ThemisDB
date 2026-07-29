#pragma once

#include "index/ann_frontdoor.h"
#include "tensor/adapter_repository.h"
#include "tensor/tensor_fingerprint_graph.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace tensor {

/**
 * @brief Logical tensor-layer kind selected by the mid-layer planner.
 */

/**
 * @file tensor_mid_layer.h
 * @brief Mid-layer orchestrator for tensor processing pipelines.
 *
 * Coordinates the transformation stages that sit between raw tensor
 * ingestion and the storage/retrieval backend within ThemisDB.
 */
enum class TensorLayerKind : uint8_t {
    Adapter,
    Package,
    ShardSummary,
    FingerprintSummary,
};

/**
 * @brief Input context for tensor-layer planning and orchestration.
 *
 * The mid-layer uses the context to normalize adapter/package identifiers,
 * select a summary strategy, and build a stable explanation for downstream
 * routing layers.
 */
struct TensorLayerContext {
    std::string tenant_id;
    std::string domain;
    std::string base_model_id;
    std::string scope_id;
    std::vector<std::string> shard_scope_ids;
    std::size_t top_k = 10;
    bool use_fingerprint_summary = true;
    bool shard_aware = false;
};

/**
 * @brief Plan returned by the tensor mid-layer.
 */
struct TensorLayerPlan {
    TensorLayerKind layer_kind = TensorLayerKind::Adapter;
    index::AnnScopeKind ann_scope_kind = index::AnnScopeKind::Adapter;
    std::string scope_key;
    std::string reason;
    std::size_t candidate_count = 0;
};

/**
 * @brief Summary of a tensor-layer retrieval step.
 */
struct TensorLayerSummary {
    std::string scope_key;
    TensorLayerKind layer_kind = TensorLayerKind::Adapter;
    index::AnnScopeKind ann_scope_kind = index::AnnScopeKind::Adapter;
    std::vector<SimilarityResult> similar_adapters;
    std::string routing_reason;
    std::size_t candidate_count = 0;
    bool federated = false;
    std::size_t participating_shards = 0;
};

/**
 * @brief Aggregated federated tensor summary across shard scopes.
 */
struct FederatedTensorSummary {
    std::vector<TensorLayerSummary> shard_summaries;
    std::vector<SimilarityResult> merged_similar_adapters;
    std::string routing_reason;
};

/**
 * @brief Lightweight tensor mid-layer orchestration over adapter/fingerprint assets.
 *
 * Responsibilities:
 * - Normalize adapter/package/shard identifiers into a single scope key.
 * - Produce a stable routing plan for downstream layers.
 * - Use AdapterRepository and TensorFingerprintGraph as the first concrete
 *   tensor-summary stage above ANN retrieval.
 */
class TensorMidLayer {
public:
    TensorMidLayer() = default;
    ~TensorMidLayer() = default;

    void setAdapterRepository(std::shared_ptr<AdapterRepository> repository);
    void setFingerprintGraph(std::shared_ptr<TensorFingerprintGraph> graph);
    void setAnnFrontdoor(std::shared_ptr<index::AnnFrontdoor> frontdoor);

    [[nodiscard]] TensorLayerPlan plan(const TensorLayerContext& context) const noexcept;
    [[nodiscard]] TensorLayerSummary summarize(const TensorLayerContext& context) const;
    [[nodiscard]] FederatedTensorSummary summarizeFederatedShards(
        const TensorLayerContext& context) const;

private:
    [[nodiscard]] static TensorLayerKind classify(const TensorLayerContext& context) noexcept;
    [[nodiscard]] static index::AnnScopeKind annScopeKindForLayer(TensorLayerKind kind) noexcept;
    [[nodiscard]] static std::string buildScopeKey(const TensorLayerContext& context);
    [[nodiscard]] static std::vector<SimilarityResult> mergeSimilarityResults(
        const std::vector<TensorLayerSummary>& summaries,
        std::size_t top_k);

    std::shared_ptr<AdapterRepository> adapter_repository_;
    std::shared_ptr<TensorFingerprintGraph> fingerprint_graph_;
    std::shared_ptr<index::AnnFrontdoor> ann_frontdoor_;
};

} // namespace tensor
} // namespace themis
