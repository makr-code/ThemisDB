/**
 * @file layered_retrieval_orchestrator.cpp
 * @brief Layered retrieval execution wiring for ANN, tensor, graph, and LLM stages.
 */

#include "search/layered_retrieval_orchestrator.h"

#include "core/concerns/i_tracer.h"
#include "graph/knowledge_graph_reasoner.h"
#include "index/advanced_vector_index.h"
#include "llm/llm_client.h"
#include "tensor/tensor_fingerprint_graph.h"
#include "utils/logger.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <exception>
#include <future>
#include <sstream>
#include <thread>
#include <utility>

namespace themis::search {
namespace {

using Clock = std::chrono::steady_clock;
using Milliseconds = std::chrono::milliseconds;

enum class DeadlineState : std::uint8_t {
    Completed,
    TimedOut,
    Failed,
};

std::string decisionToString(const LayerRoutingDecision decision) {
    switch (decision) {
        case LayerRoutingDecision::EXECUTED:
            return "executed";
        case LayerRoutingDecision::FALLBACK:
            return "fallback";
        case LayerRoutingDecision::TIMEOUT_SKIP:
            return "timeout_skip";
        case LayerRoutingDecision::GUARDRAIL_SKIP:
            return "guardrail_skip";
        case LayerRoutingDecision::DISABLED:
        [[fallthrough]];\n        default:
            return "disabled";
    }
}

template <typename Fn>
DeadlineState runWithDeadline(Fn&& fn, const Milliseconds timeout, std::string& error) {
    auto completion = std::make_shared<std::promise<void>>();
    auto future = completion->get_future();
    auto failure = std::make_shared<std::exception_ptr>();

    std::thread([completion, failure, task = std::forward<Fn>(fn)]() mutable {
        try {
            task();
        } catch (...) {
            *failure = std::current_exception();
        }

        try {
            completion->set_value();
        } catch (...) {
        }
    }).detach();

    if (future.wait_for(timeout) == std::future_status::timeout) {
        error = "layer deadline exceeded";
        return DeadlineState::TimedOut;
    }

    if (*failure != nullptr) {
        try {
            std::rethrow_exception(*failure);
        } catch (const std::exception& ex) {
            error = ex.what();
        } catch (...) {
            error = "unknown layer failure";
        }
        return DeadlineState::Failed;
    }

    return DeadlineState::Completed;
}

std::string buildFallbackAnswer(const LayeredRetrievalResult& result) {
    if (!result.provenance.empty()) {
        const auto& edge = result.provenance.front();
        std::ostringstream oss;
        oss << edge.subject << ' ' << edge.predicate << ' ' << edge.object;
        return oss.str();
    }

    if (!result.tensor_candidates.empty()) {
        return "tensor fallback: " + result.tensor_candidates.front().adapter_key;
    }

    if (!result.ann_candidates.empty()) {
        return "ann fallback candidate id=" + std::to_string(result.ann_candidates.front().id);
    }

    return "no layered retrieval answer available";
}

std::size_t clampCandidateLimit(const LayeredRetrievalConfig& config,
                                const std::size_t requested,
                                const std::size_t guardrail_limit,
                                bool& pruned) {
    if (!config.guardrails.enabled) {
        return requested;
    }

    const std::size_t effective = std::min(requested, guardrail_limit);
    if (effective != requested) {
        pruned = true;
    }
    return effective;
}

using SpanPtr = std::unique_ptr<core::concerns::ITracer::ISpan>;

SpanPtr startLayerSpan(const std::shared_ptr<core::concerns::ITracer>& tracer,
                       core::concerns::ITracer::ISpan* parent,
                       const std::string& name) {
    if (!tracer) {
        return {};
    }

    if (parent != nullptr && parent->isValid()) {
        return tracer->startChildSpan(name, *parent);
    }
    return tracer->startSpan(name);
}

void finalizeSpan(SpanPtr& span,
                  const LayerRoutingDecision decision,
                  const std::uint64_t latency_ms,
                  const std::string& detail,
                  const std::size_t output_count) {
    if (!span) {
        return;
    }

    span->setAttribute("layer.status", decisionToString(decision));
    span->setAttribute("layer.latency_ms", static_cast<std::int64_t>(latency_ms));
    span->setAttribute("layer.output_count", static_cast<std::int64_t>(output_count));
    if (!detail.empty()) {
        span->setAttribute("layer.detail", detail);
    }

    const bool ok = decision == LayerRoutingDecision::EXECUTED;
    span->setStatus(ok, detail);
    if (!ok && !detail.empty()) {
        span->recordError(detail);
    }
    span->end();
    span.reset();
}

} // namespace

LayeredRetrievalOrchestrator::LayeredRetrievalOrchestrator(LayeredRetrievalConfig config)
    : config_(std::move(config)) {}

void LayeredRetrievalOrchestrator::setAnnIndex(std::shared_ptr<AdvancedVectorIndex> index) {
    ann_index_ = std::move(index);
}

void LayeredRetrievalOrchestrator::setTensorGraph(
    std::shared_ptr<tensor::TensorFingerprintGraph> graph) {
    tensor_graph_ = std::move(graph);
}

void LayeredRetrievalOrchestrator::setGraphReasoner(
    std::shared_ptr<graph::KnowledgeGraphReasoner> reasoner) {
    graph_reasoner_ = std::move(reasoner);
}

void LayeredRetrievalOrchestrator::setLlmClient(std::shared_ptr<llm::LLMClient> client) {
    llm_client_ = std::move(client);
}

void LayeredRetrievalOrchestrator::setTracer(std::shared_ptr<core::concerns::ITracer> tracer) {
    tracer_ = std::move(tracer);
}

void LayeredRetrievalOrchestrator::setConfig(const LayeredRetrievalConfig& config) {
    config_ = config;
}

const LayeredRetrievalConfig& LayeredRetrievalOrchestrator::getConfig() const noexcept {
    return config_;
}

LayeredRetrievalResult LayeredRetrievalOrchestrator::execute(
    const LayeredRetrievalContext& context) const {
    LayeredRetrievalResult result;

    auto root_span = tracer_ ? tracer_->startSpan("search.layered_retrieval.execute") : SpanPtr{};
    if (root_span) {
        root_span->setAttribute("correlation.id", context.correlation_id);
        root_span->setAttribute("guardrails.enabled", config_.guardrails.enabled);
    }

    std::size_t layers_started = 0;

    const auto add_record = [&]([[maybe_unused]] LayerDecisionRecord record) {
        result.layer_latencies[record.layer_name] = record.latency_ms;
        result.routing_decisions.push_back(std::move(record));
    };

    const auto guardrail_blocks_layer = [&]([[maybe_unused]] const std::string& layer_name) -> bool {
        if (!config_.guardrails.enabled) {
            return false;
        }

        if (layers_started >= config_.guardrails.max_layers) {
            result.guardrail_pruned = true;
            result.diagnostics.push_back(layer_name + " skipped by max_layers guardrail");
            add_record({layer_name, LayerRoutingDecision::GUARDRAIL_SKIP,
                        "guardrail max_layers exceeded", 0, 0, 0});
            return true;
        }
        return false;
    };

    if (!config_.ann_enabled) {
        add_record({"ann", LayerRoutingDecision::DISABLED, "ann layer disabled", 0, 0, 0});
    } else if (guardrail_blocks_layer("ann")) {
    } else if (!ann_index_) {
        result.diagnostics.push_back("ann layer requested without AdvancedVectorIndex backend");
        add_record({"ann", LayerRoutingDecision::FALLBACK,
                    "missing AdvancedVectorIndex backend", 0, 0, 0});
    } else if (context.query_vector.empty()) {
        result.diagnostics.push_back("ann layer skipped because query_vector is empty");
        add_record({"ann", LayerRoutingDecision::FALLBACK,
                    "missing query vector", 0, 0, 0});
    } else {
        ++layers_started;
        auto span = startLayerSpan(tracer_, root_span.get(), "search.layer.ann");
        if (span) {
            span->setAttribute("layer.name", "ann");
            span->setAttribute("correlation.id", context.correlation_id);
        }

        bool guardrail_pruned = false;
        const auto top_k = clampCandidateLimit(
            config_,
            config_.ann_top_k,
            config_.guardrails.max_ann_candidates,
            guardrail_pruned);

        auto search_result = std::make_shared<AdvancedVectorIndex::SearchResult>();
        std::string error;
        const auto started_at = Clock::now();
        const auto state = runWithDeadline(
            [backend = ann_index_,
             query = context.query_vector,
             top_k,
             search_result]() {
                *search_result = backend->search(query.data(), top_k);
            },
            Milliseconds(config_.layer_timeout_ms),
            error);
        const auto latency_ms =
            static_cast<std::uint64_t>(std::chrono::duration_cast<Milliseconds>(
                Clock::now() - started_at).count());

        LayerDecisionRecord record;
        record.layer_name = "ann";
        record.input_count = context.query_vector.size();
        record.latency_ms = latency_ms;

        if (state == DeadlineState::TimedOut) {
            result.timed_out = true;
            result.diagnostics.push_back("ann layer timed out");
            record.decision = LayerRoutingDecision::TIMEOUT_SKIP;
            record.detail = error;
        } else if (state == DeadlineState::Failed) {
            result.diagnostics.push_back("ann layer failed: " + error);
            record.decision = LayerRoutingDecision::FALLBACK;
            record.detail = error;
        } else {
            const auto count = std::min(search_result->ids.size(), search_result->distances.size());
            result.ann_candidates.reserve(count);
            for (std::size_t i = 0; i < count; ++i) {
                const float distance = search_result->distances[i];
                const float score = 1.0f / (1.0f + std::max(distance, 0.0f));
                result.ann_candidates.push_back({search_result->ids[i], distance, score});
            }
            if (guardrail_pruned) {
                result.guardrail_pruned = true;
                result.diagnostics.push_back("ann candidates pruned by guardrail");
            }

            record.decision = result.ann_candidates.empty()
                ? LayerRoutingDecision::FALLBACK
                : LayerRoutingDecision::EXECUTED;
            record.detail = result.ann_candidates.empty()
                ? "ann backend returned no candidates"
                : "ann retrieval completed";
            record.output_count = result.ann_candidates.size();
        }

        add_record(record);
        finalizeSpan(span, record.decision, latency_ms, record.detail, record.output_count);
    }

    if (!config_.tensor_enabled) {
        add_record({"tensor", LayerRoutingDecision::DISABLED, "tensor layer disabled", 0, 0, 0});
    } else if (guardrail_blocks_layer("tensor")) {
    } else if (!tensor_graph_) {
        result.diagnostics.push_back("tensor layer requested without TensorFingerprintGraph backend");
        add_record({"tensor", LayerRoutingDecision::FALLBACK,
                    "missing TensorFingerprintGraph backend", 0, 0, 0});
    } else if (context.tensor_query_key.empty()) {
        result.diagnostics.push_back("tensor layer skipped because tensor_query_key is empty");
        add_record({"tensor", LayerRoutingDecision::FALLBACK,
                    "missing tensor query key", 0, 0, 0});
    } else {
        ++layers_started;
        auto span = startLayerSpan(tracer_, root_span.get(), "search.layer.tensor");
        if (span) {
            span->setAttribute("layer.name", "tensor");
            span->setAttribute("correlation.id", context.correlation_id);
        }

        bool guardrail_pruned = false;
        const auto top_k = clampCandidateLimit(
            config_,
            std::max<std::size_t>(1, result.ann_candidates.empty() ? config_.ann_top_k
                                                                    : result.ann_candidates.size()),
            config_.guardrails.max_tensor_candidates,
            guardrail_pruned);

        auto tensor_result = std::make_shared<std::vector<tensor::SimilarityResult>>();
        std::string error;
        const auto started_at = Clock::now();
        const auto state = runWithDeadline(
            [graph = tensor_graph_,
             query_key = context.tensor_query_key,
             top_k,
             tensor_result]() {
                *tensor_result = graph->findSimilar(query_key, top_k);
            },
            Milliseconds(config_.layer_timeout_ms),
            error);
        const auto latency_ms =
            static_cast<std::uint64_t>(std::chrono::duration_cast<Milliseconds>(
                Clock::now() - started_at).count());

        LayerDecisionRecord record;
        record.layer_name = "tensor";
        record.input_count = top_k;
        record.latency_ms = latency_ms;

        if (state == DeadlineState::TimedOut) {
            result.timed_out = true;
            result.diagnostics.push_back("tensor layer timed out");
            record.decision = LayerRoutingDecision::TIMEOUT_SKIP;
            record.detail = error;
        } else if (state == DeadlineState::Failed) {
            result.diagnostics.push_back("tensor layer failed: " + error);
            record.decision = LayerRoutingDecision::FALLBACK;
            record.detail = error;
        } else {
            result.tensor_candidates.reserve(tensor_result->size());
            for (const auto& candidate : *tensor_result) {
                result.tensor_candidates.push_back(
                    {candidate.adapter_key, candidate.domain, candidate.base_model_id, candidate.score});
            }
            if (guardrail_pruned) {
                result.guardrail_pruned = true;
                result.diagnostics.push_back("tensor candidates pruned by guardrail");
            }

            record.decision = result.tensor_candidates.empty()
                ? LayerRoutingDecision::FALLBACK
                : LayerRoutingDecision::EXECUTED;
            record.detail = result.tensor_candidates.empty()
                ? "tensor backend returned no candidates"
                : "tensor retrieval completed";
            record.output_count = result.tensor_candidates.size();
        }

        add_record(record);
        finalizeSpan(span, record.decision, latency_ms, record.detail, record.output_count);
    }

    if (!config_.graph_enabled) {
        add_record({"graph", LayerRoutingDecision::DISABLED, "graph layer disabled", 0, 0, 0});
    } else if (guardrail_blocks_layer("graph")) {
    } else if (!graph_reasoner_) {
        result.diagnostics.push_back("graph layer requested without KnowledgeGraphReasoner backend");
        add_record({"graph", LayerRoutingDecision::FALLBACK,
                    "missing KnowledgeGraphReasoner backend", 0, 0, 0});
    } else if (context.graph_subject_id.empty()) {
        result.diagnostics.push_back("graph layer skipped because graph_subject_id is empty");
        add_record({"graph", LayerRoutingDecision::FALLBACK,
                    "missing graph subject id", 0, 0, 0});
    } else {
        ++layers_started;
        auto span = startLayerSpan(tracer_, root_span.get(), "search.layer.graph");
        if (span) {
            span->setAttribute("layer.name", "graph");
            span->setAttribute("correlation.id", context.correlation_id);
        }

        auto chain = std::make_shared<graph::InferenceChain>();
        std::string error;
        const auto started_at = Clock::now();
        const auto state = runWithDeadline(
            [reasoner = graph_reasoner_,
             subject = context.graph_subject_id,
             hops = config_.graph_max_hops,
             adapter_id = context.lora_adapter_id,
             chain]() {
                *chain = reasoner->infer(subject, hops);
                if (!adapter_id.empty()) {
                    reasoner->applyLoRAScore(*chain, adapter_id);
                }
            },
            Milliseconds(config_.layer_timeout_ms),
            error);
        const auto latency_ms =
            static_cast<std::uint64_t>(std::chrono::duration_cast<Milliseconds>(
                Clock::now() - started_at).count());

        LayerDecisionRecord record;
        record.layer_name = "graph";
        record.input_count = static_cast<std::size_t>(config_.graph_max_hops);
        record.latency_ms = latency_ms;

        if (state == DeadlineState::TimedOut) {
            result.timed_out = true;
            result.diagnostics.push_back("graph layer timed out");
            record.decision = LayerRoutingDecision::TIMEOUT_SKIP;
            record.detail = error;
        } else if (state == DeadlineState::Failed) {
            result.diagnostics.push_back("graph layer failed: " + error);
            record.decision = LayerRoutingDecision::FALLBACK;
            record.detail = error;
        } else {
            std::size_t edge_limit = chain->edges.size();
            if (config_.guardrails.enabled && edge_limit > config_.guardrails.max_graph_edges) {
                edge_limit = config_.guardrails.max_graph_edges;
                result.guardrail_pruned = true;
                result.diagnostics.push_back("graph provenance pruned by guardrail");
            }

            result.provenance.reserve(edge_limit);
            for (std::size_t i = 0; i < edge_limit; ++i) {
                const auto& edge = chain->edges[i];
                result.provenance.push_back({
                    edge.fact.subject,
                    edge.fact.predicate,
                    edge.fact.object,
                    edge.rule_id,
                    edge.lora_score,
                });
            }

            record.decision = result.provenance.empty()
                ? LayerRoutingDecision::FALLBACK
                : LayerRoutingDecision::EXECUTED;
            record.detail = result.provenance.empty()
                ? "graph reasoner returned no provenance"
                : "graph provenance completed";
            record.output_count = result.provenance.size();
        }

        add_record(record);
        finalizeSpan(span, record.decision, latency_ms, record.detail, record.output_count);
    }

    if (!config_.llm_enabled) {
        add_record({"llm", LayerRoutingDecision::DISABLED, "llm layer disabled", 0, 0, 0});
    } else if (guardrail_blocks_layer("llm")) {
        result.final_answer = buildFallbackAnswer(result);
    } else if (!llm_client_) {
        result.diagnostics.push_back("llm layer requested without LLMClient backend");
        add_record({"llm", LayerRoutingDecision::FALLBACK,
                    "missing LLMClient backend", 0, 0, 0});
        result.final_answer = buildFallbackAnswer(result);
    } else if (context.query.empty()) {
        result.diagnostics.push_back("llm layer skipped because query is empty");
        add_record({"llm", LayerRoutingDecision::FALLBACK, "missing query text", 0, 0, 0});
        result.final_answer = buildFallbackAnswer(result);
    } else {
        ++layers_started;
        auto span = startLayerSpan(tracer_, root_span.get(), "search.layer.llm");
        if (span) {
            span->setAttribute("layer.name", "llm");
            span->setAttribute("correlation.id", context.correlation_id);
        }

        std::ostringstream prompt_builder;
        if (!context.llm_prompt_prefix.empty()) {
            prompt_builder << context.llm_prompt_prefix << "\n\n";
        }
        prompt_builder << "Query: " << context.query << '\n';
        if (!result.ann_candidates.empty()) {
            prompt_builder << "ANN candidates:";
            for (const auto& candidate : result.ann_candidates) {
                prompt_builder << " [id=" << candidate.id << ",score=" << candidate.score << ']';
            }
            prompt_builder << '\n';
        }
        if (!result.tensor_candidates.empty()) {
            prompt_builder << "Tensor candidates:";
            for (const auto& candidate : result.tensor_candidates) {
                prompt_builder << " [adapter=" << candidate.adapter_key
                               << ",score=" << candidate.score << ']';
            }
            prompt_builder << '\n';
        }
        if (!result.provenance.empty()) {
            prompt_builder << "Provenance:";
            for (const auto& entry : result.provenance) {
                prompt_builder << " [" << entry.subject << ' '
                               << entry.predicate << ' ' << entry.object << ']';
            }
        }

        auto prompt = prompt_builder.str();
        if (config_.guardrails.enabled && prompt.size() > config_.guardrails.max_prompt_chars) {
            prompt.resize(config_.guardrails.max_prompt_chars);
            result.guardrail_pruned = true;
            result.diagnostics.push_back("llm prompt truncated by guardrail");
        }

        auto generation = std::make_shared<llm::GenerationResult>();
        std::string error;
        const auto started_at = Clock::now();
        const auto state = runWithDeadline(
            [client = llm_client_,
             prompt = std::move(prompt),
             timeout_ms = config_.layer_timeout_ms,
             generation]() {
                llm::GenerationOptions options;
                options.timeout_ms = timeout_ms;
                options.max_tokens = 256;
                *generation = client->generate(prompt, options);
            },
            Milliseconds(config_.layer_timeout_ms),
            error);
        const auto latency_ms =
            static_cast<std::uint64_t>(std::chrono::duration_cast<Milliseconds>(
                Clock::now() - started_at).count());

        LayerDecisionRecord record;
        record.layer_name = "llm";
        record.input_count = context.query.size();
        record.latency_ms = latency_ms;

        if (state == DeadlineState::TimedOut) {
            result.timed_out = true;
            result.diagnostics.push_back("llm layer timed out");
            record.decision = LayerRoutingDecision::TIMEOUT_SKIP;
            record.detail = error;
            result.final_answer = buildFallbackAnswer(result);
        } else if (state == DeadlineState::Failed) {
            result.diagnostics.push_back("llm layer failed: " + error);
            record.decision = LayerRoutingDecision::FALLBACK;
            record.detail = error;
            result.final_answer = buildFallbackAnswer(result);
        } else if (!generation->success || generation->text.empty()) {
            const std::string detail = generation->error_message.empty()
                ? "llm generation returned no text"
                : generation->error_message;
            result.diagnostics.push_back("llm layer fallback: " + detail);
            record.decision = LayerRoutingDecision::FALLBACK;
            record.detail = detail;
            result.final_answer = buildFallbackAnswer(result);
        } else {
            result.final_answer = generation->text;
            record.decision = LayerRoutingDecision::EXECUTED;
            record.detail = "llm generation completed";
            record.output_count = generation->text.size();
        }

        add_record(record);
        finalizeSpan(span, record.decision, latency_ms, record.detail, record.output_count);
    }

    if (root_span) {
        root_span->setAttribute("layers.executed",
                                static_cast<std::int64_t>(layers_started));
        root_span->setAttribute("result.timed_out", result.timed_out);
        root_span->setAttribute("result.guardrail_pruned", result.guardrail_pruned);
        root_span->setStatus(!result.final_answer.empty(), result.final_answer.empty()
            ? "no final answer produced"
            : "layered retrieval completed");
        root_span->end();
        root_span.reset();
    }

    THEMIS_INFO("Layered retrieval completed: correlation_id='{}' decisions={} timed_out={} guardrail_pruned={}",
                context.correlation_id,
                result.routing_decisions.size(),
                result.timed_out,
                result.guardrail_pruned);
    return result;
}

} // namespace themis::search
