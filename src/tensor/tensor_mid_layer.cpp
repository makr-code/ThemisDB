#include "tensor/tensor_mid_layer.h"

#include <algorithm>
#include <unordered_map>
#include <stdexcept>
#include <utility>

namespace themis {
namespace tensor {

void TensorMidLayer::setAdapterRepository(std::shared_ptr<AdapterRepository> repository) {
    adapter_repository_ = std::move(repository);
}

void TensorMidLayer::setFingerprintGraph(std::shared_ptr<TensorFingerprintGraph> graph) {
    fingerprint_graph_ = std::move(graph);
}

void TensorMidLayer::setAnnFrontdoor(std::shared_ptr<index::AnnFrontdoor> frontdoor) {
    ann_frontdoor_ = std::move(frontdoor);
}

TensorLayerPlan TensorMidLayer::plan(const TensorLayerContext& context) const noexcept {
    TensorLayerPlan plan;
    plan.layer_kind = classify(context);
    plan.ann_scope_kind = annScopeKindForLayer(plan.layer_kind);
    plan.scope_key = buildScopeKey(context);
    plan.candidate_count = context.top_k;

    switch (plan.layer_kind) {
        case TensorLayerKind::Package:
            plan.reason = "package scope routed through adapter repository";
            break;
        case TensorLayerKind::ShardSummary:
            plan.reason = "shard summary routed through ANN fan-out and tensor summarization";
            break;
        case TensorLayerKind::FingerprintSummary:
            plan.reason = "fingerprint summary routed through TensorFingerprintGraph";
            break;
        case TensorLayerKind::Adapter:
        default:
            plan.reason = "adapter scope routed through TensorFingerprintGraph";
            break;
    }

    if (ann_frontdoor_) {
        index::AnnQueryContext ann_context;
        ann_context.scope_id = plan.scope_key;
        ann_context.shard_aware = context.shard_aware;
        ann_context.dataset_size = context.top_k;
        ann_context.hot_tier = true;
        (void)ann_frontdoor_->planRetrieval(ann_context);
    }

    return plan;
}

TensorLayerSummary TensorMidLayer::summarize(const TensorLayerContext& context) const {
    TensorLayerSummary summary;
    summary.scope_key = buildScopeKey(context);
    summary.layer_kind = classify(context);
    summary.ann_scope_kind = annScopeKindForLayer(summary.layer_kind);

    // Prefer repository-provided exact-similarity overrides (tests and
    // production exact TT inner-product backends). If the repository yields
    // no candidates, fall back to the fingerprint graph (approximate).
    if (adapter_repository_ && !context.domain.empty() && !context.base_model_id.empty()) {
        summary.similar_adapters = adapter_repository_->findSimilarAdapters(
            context.domain,
            context.base_model_id,
            context.top_k);
        if (ann_frontdoor_) {
            ann_frontdoor_->registerScopeKind(summary.scope_key, summary.ann_scope_kind);
        }
    }

    if (summary.similar_adapters.empty() && fingerprint_graph_) {
        summary.similar_adapters = fingerprint_graph_->findSimilar(summary.scope_key, context.top_k);
    }

    summary.candidate_count = summary.similar_adapters.size();
    summary.routing_reason = plan(context).reason;
    return summary;
}

FederatedTensorSummary TensorMidLayer::summarizeFederatedShards(
    const TensorLayerContext& context) const {
    FederatedTensorSummary summary;
    if (context.shard_scope_ids.empty()) {
        summary.routing_reason = "no shard scopes provided for federated tensor summary";
        return summary;
    }

    summary.shard_summaries.reserve(context.shard_scope_ids.size());
    for (const auto& shard_scope_id : context.shard_scope_ids) {
        auto shard_context = context;
        shard_context.scope_id = shard_scope_id;
        shard_context.shard_aware = true;
        shard_context.shard_scope_ids.clear();

        auto shard_summary = summarize(shard_context);
        // Diagnostic: log shard-level similar adapter counts to aid debugging
        // Emit a small stderr diagnostic so focused test runners capture it reliably.
        try {
            std::fprintf(stderr, "[TFML] shard='%s' similar_adapters=%zu candidates=%zu\n",
                         shard_scope_id.c_str(), shard_summary.similar_adapters.size(), shard_summary.candidate_count);
        } catch (...) {
        }
        shard_summary.federated = true;
        shard_summary.participating_shards = context.shard_scope_ids.size();
        summary.shard_summaries.push_back(std::move(shard_summary));
    }

    summary.merged_similar_adapters = mergeSimilarityResults(
        summary.shard_summaries,
        context.top_k);
    summary.routing_reason = "federated shard summaries merged through tensor mid-layer";
    return summary;
}

TensorLayerKind TensorMidLayer::classify(const TensorLayerContext& context) noexcept {
    // Shard-aware context (explicit flag or explicit shard list) should
    // be classified as `ShardSummary` even when `scope_id` is empty.
    if (context.shard_aware || !context.shard_scope_ids.empty()) {
        return TensorLayerKind::ShardSummary;
    }

    if (!context.scope_id.empty()) {
        // If scope id explicitly encodes a shard or shard-summary, prefer that.
        if (context.scope_id.rfind("shard:", 0) == 0 || context.scope_id.rfind("shard-summary:", 0) == 0) {
            return TensorLayerKind::ShardSummary;
        }
        if (context.scope_id.rfind("pkg:", 0) == 0 || context.scope_id.rfind("package:", 0) == 0) {
            return TensorLayerKind::Package;
        }
    }
    if (!context.domain.empty() && (context.domain.rfind("pkg:", 0) == 0 || context.domain.rfind("package:", 0) == 0)) {
        return TensorLayerKind::Package;
    }
    if (!context.use_fingerprint_summary) {
        return TensorLayerKind::Adapter;
    }
    return TensorLayerKind::FingerprintSummary;
}

index::AnnScopeKind TensorMidLayer::annScopeKindForLayer(TensorLayerKind kind) noexcept {
    switch (kind) {
        case TensorLayerKind::Package:
            return index::AnnScopeKind::Package;
        case TensorLayerKind::ShardSummary:
            return index::AnnScopeKind::ShardSummary;
        case TensorLayerKind::FingerprintSummary:
        case TensorLayerKind::Adapter:
        default:
            return index::AnnScopeKind::Adapter;
    }
}

std::string TensorMidLayer::buildScopeKey(const TensorLayerContext& context) {
    if (!context.scope_id.empty()) {
        return context.scope_id;
    }
    if (!context.tenant_id.empty() && !context.domain.empty() && !context.base_model_id.empty()) {
        return "__adapters__:" + context.tenant_id + ":" + context.domain + ":" + context.base_model_id;
    }
    if (!context.tenant_id.empty() && !context.domain.empty()) {
        return "__package__:" + context.tenant_id + ":" + context.domain;
    }
    return context.domain.empty() ? "__tensor_mid_layer__" : context.domain;
}

std::vector<SimilarityResult> TensorMidLayer::mergeSimilarityResults(
    const std::vector<TensorLayerSummary>& summaries,
    std::size_t top_k) {
    std::unordered_map<std::string, SimilarityResult> merged;

    for (const auto& summary : summaries) {
        for (const auto& candidate : summary.similar_adapters) {
            auto& slot = merged[candidate.adapter_key];
            if (slot.adapter_key.empty() || candidate.score > slot.score) {
                slot = candidate;
            }
        }
    }

    std::vector<SimilarityResult> out;
    out.reserve(merged.size());
    for (auto& [_, candidate] : merged) {
        out.push_back(std::move(candidate));
    }

    std::sort(out.begin(), out.end(),
              [](const auto& lhs, const auto& rhs) {
                  return lhs.score > rhs.score;
              });
    if (top_k > 0 && out.size() > top_k) {
        out.resize(top_k);
    }
    return out;
}

} // namespace tensor
} // namespace themis
