/**
 * @file layered_retrieval_orchestrator.h
 * @brief Four-layer retrieval orchestrator wiring ANN, tensor, graph, and LLM stages.
 *
 * This contract exposes the production retrieval chain used by search-side
 * layered retrieval experiments:
 *   ANN → Tensor → Graph → LLM
 *
 * Each layer is optional and independently fail-safe. The orchestrator records
 * a per-layer routing decision, enforces per-layer deadlines, and emits
 * tracing metadata for observability.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {

class AdvancedVectorIndex;

namespace core::concerns {
class ITracer;
}

namespace graph {
class KnowledgeGraphReasoner;
}

namespace llm {
class LLMClient;
}

namespace tensor {
class TensorFingerprintGraph;
}

namespace search {

/**
 * @brief Decision taken for an individual retrieval layer.
 */
enum class LayerRoutingDecision : std::uint8_t {
    EXECUTED,
    FALLBACK,
    TIMEOUT_SKIP,
    GUARDRAIL_SKIP,
    DISABLED,
};

/**
 * @brief One ANN candidate returned from the front retrieval layer.
 */
struct AnnLayerCandidate {
    std::int64_t id = -1;
    float distance = 0.0f;
    float score = 0.0f;
};

/**
 * @brief One tensor-layer candidate returned from the tensor fingerprint graph.
 */
struct TensorLayerCandidate {
    std::string adapter_key;
    std::string domain;
    std::string base_model_id;
    float score = 0.0f;
};

/**
 * @brief One provenance edge produced by the graph layer.
 */
struct ProvenanceEntry {
    std::string subject;
    std::string predicate;
    std::string object;
    std::string rule_id;
    double lora_score = -1.0;
};

/**
 * @brief Per-layer execution metadata recorded by the orchestrator.
 */
struct LayerDecisionRecord {
    std::string layer_name;
    LayerRoutingDecision decision = LayerRoutingDecision::DISABLED;
    std::string detail;
    std::uint64_t latency_ms = 0;
    std::size_t input_count = 0;
    std::size_t output_count = 0;
};

/**
 * @brief Per-query hard limits used to prune expensive layered retrieval work.
 *
 * Guardrails are disabled when @ref enabled is false. When enabled, the
 * orchestrator clamps ANN fan-out, provenance depth, and prompt size before a
 * downstream layer is executed.
 */
struct PerQueryRetrievalGuardrails {
    bool enabled = false;
    std::size_t max_ann_candidates = 10;
    std::size_t max_tensor_candidates = 10;
    std::size_t max_graph_edges = 8;
    std::size_t max_prompt_chars = 4096;
    std::size_t max_layers = 4;
};

/**
 * @brief Runtime configuration for layered retrieval execution.
 */
struct LayeredRetrievalConfig {
    bool ann_enabled = true;
    bool tensor_enabled = true;
    bool graph_enabled = true;
    bool llm_enabled = true;

    std::uint32_t layer_timeout_ms = 50;
    std::size_t ann_top_k = 10;
    int graph_max_hops = 2;

    PerQueryRetrievalGuardrails guardrails;
};

/**
 * @brief Query payload consumed by @ref LayeredRetrievalOrchestrator.
 *
 * @param query Natural-language query used by the final LLM layer.
 * @param query_vector Dense embedding used by the ANN layer. Empty disables ANN execution.
 * @param tensor_query_key Existing tensor fingerprint key queried against the tensor layer.
 * @param graph_subject_id Subject/entity whose provenance chain should be inferred.
 * @param correlation_id Stable request identifier propagated into trace attributes.
 * @param llm_prompt_prefix Optional system prefix prepended to the generated prompt.
 * @param lora_adapter_id Optional adapter identifier used for graph-layer plausibility scoring.
 */
struct LayeredRetrievalContext {
    std::string query;
    std::vector<float> query_vector;
    std::string tensor_query_key;
    std::string graph_subject_id;
    std::string correlation_id;
    std::string llm_prompt_prefix;
    std::string lora_adapter_id;
};

/**
 * @brief Unified output returned by the layered retrieval chain.
 */
struct LayeredRetrievalResult {
    std::vector<AnnLayerCandidate> ann_candidates;
    std::vector<TensorLayerCandidate> tensor_candidates;
    std::vector<ProvenanceEntry> provenance;
    std::string final_answer;

    std::vector<LayerDecisionRecord> routing_decisions;
    std::unordered_map<std::string, std::uint64_t> layer_latencies;
    std::vector<std::string> diagnostics;

    bool timed_out = false;
    bool guardrail_pruned = false;
};

/**
 * @brief Orchestrates the ANN → Tensor → Graph → LLM retrieval pipeline.
 *
 * The orchestrator is intentionally fail-safe:
 * - Missing backends degrade to FALLBACK decisions instead of throwing.
 * - Each enabled layer is wrapped in a hard wait budget controlled by
 *   @ref LayeredRetrievalConfig::layer_timeout_ms.
 * - Per-query guardrails clamp expensive fan-out before downstream work begins.
 *
 * Thread safety: configuration and backend wiring must be completed before
 * concurrent execute() calls. Read-only execute() calls are safe when the
 * injected backends themselves are safe for concurrent use.
 */
class LayeredRetrievalOrchestrator {
public:
    /**
     * @brief Construct an orchestrator with the given configuration.
     * @param config Layer enablement, timeouts, and guardrails.
     */
    explicit LayeredRetrievalOrchestrator(
        LayeredRetrievalConfig config = LayeredRetrievalConfig{});

    /**
     * @brief Inject the ANN backend used by the first retrieval layer.
     * @param index Shared AdvancedVectorIndex instance; null disables the ANN layer.
     */
    void setAnnIndex(std::shared_ptr<AdvancedVectorIndex> index);

    /**
     * @brief Inject the tensor fingerprint graph used by the tensor layer.
     * @param graph Shared TensorFingerprintGraph instance; null disables the tensor layer.
     */
    void setTensorGraph(std::shared_ptr<tensor::TensorFingerprintGraph> graph);

    /**
     * @brief Inject the knowledge-graph reasoner used by the provenance layer.
     * @param reasoner Shared KnowledgeGraphReasoner instance; null disables the graph layer.
     */
    void setGraphReasoner(std::shared_ptr<graph::KnowledgeGraphReasoner> reasoner);

    /**
     * @brief Inject the LLM client used by the final answer layer.
     * @param client Shared LLMClient instance; null disables the LLM layer.
     */
    void setLlmClient(std::shared_ptr<llm::LLMClient> client);

    /**
     * @brief Inject the tracer used for per-layer span emission.
     * @param tracer Shared tracer; null disables tracing.
     */
    void setTracer(std::shared_ptr<core::concerns::ITracer> tracer);

    /**
     * @brief Replace the current runtime configuration.
     * @param config New layer/time/guardrail configuration.
     */
    void setConfig(const LayeredRetrievalConfig& config);

    /**
     * @brief Return the active runtime configuration.
     */
    [[nodiscard]] const LayeredRetrievalConfig& getConfig() const noexcept;

    /**
     * @brief Execute the configured four-layer retrieval chain.
     *
     * The orchestrator never throws. Layer failures are surfaced through
     * @ref LayeredRetrievalResult::routing_decisions and
     * @ref LayeredRetrievalResult::diagnostics.
     *
     * @param context Query payload and layer-specific lookup keys.
     * @return Unified retrieval result with layer outputs and diagnostics.
     */
    [[nodiscard]] LayeredRetrievalResult execute(
        const LayeredRetrievalContext& context) const;

private:
    LayeredRetrievalConfig config_;
    std::shared_ptr<AdvancedVectorIndex> ann_index_;
    std::shared_ptr<tensor::TensorFingerprintGraph> tensor_graph_;
    std::shared_ptr<graph::KnowledgeGraphReasoner> graph_reasoner_;
    std::shared_ptr<llm::LLMClient> llm_client_;
    std::shared_ptr<core::concerns::ITracer> tracer_;
};

} // namespace search
} // namespace themis
