/**
 * @file test_layered_retrieval_integration_phase4.cpp
 * @brief Focused integration tests for ANN → Tensor → Graph → LLM layered retrieval.
 */

#include <gtest/gtest.h>

#include "core/concerns/i_tracer.h"
#include "graph/knowledge_graph_reasoner.h"
#include "index/advanced_vector_index.h"
#include "llm/llm_client.h"
#include "search/layered_retrieval_orchestrator.h"
#include "storage/tensor_train_decomposer.h"
#include "tensor/tensor_fingerprint_graph.h"

#include <algorithm>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace themis::search::testing {
namespace {

struct ExactVectorBackendState {
    std::size_t dimension = 0;
    std::vector<std::vector<float>> vectors;
    std::vector<std::int64_t> ids;
};

std::shared_ptr<ExactVectorBackendState> makeExactVectorBackend() {
    auto state = std::make_shared<ExactVectorBackendState>();

    AdvancedVectorIndex::StubCallbacks callbacks;
    callbacks.initialize = [state](const std::size_t dimension,
                                   const AdvancedVectorIndex::Config&) {
        state->dimension = dimension;
        state->vectors.clear();
        state->ids.clear();
        return true;
    };
    callbacks.train = [](const float*, const std::size_t) {
        return true;
    };
    callbacks.add_with_ids = [state](const float* vectors,
                                     const std::int64_t* ids,
                                     const std::size_t count) {
        for (std::size_t i = 0; i < count; ++i) {
            std::vector<float> sample(state->dimension);
            for (std::size_t d = 0; d < state->dimension; ++d) {
                sample[d] = vectors[i * state->dimension + d];
            }
            state->vectors.push_back(std::move(sample));
            state->ids.push_back(ids[i]);
        }
        return true;
    };
    callbacks.search = [state](const float* query, const std::size_t k) {
        std::vector<std::pair<float, std::int64_t>> scored;
        for (std::size_t i = 0; i < state->vectors.size(); ++i) {
            float distance = 0.0f;
            for (std::size_t d = 0; d < state->dimension; ++d) {
                const float diff = state->vectors[i][d] - query[d];
                distance += diff * diff;
            }
            scored.emplace_back(distance, state->ids[i]);
        }

        std::sort(scored.begin(), scored.end(),
                  [](const auto& lhs, const auto& rhs) {
                      return lhs.first < rhs.first;
                  });

        AdvancedVectorIndex::SearchResult result;
        const auto count = std::min<std::size_t>(k, scored.size());
        result.ids.reserve(count);
        result.distances.reserve(count);
        for (std::size_t i = 0; i < count; ++i) {
            result.ids.push_back(scored[i].second);
            result.distances.push_back(scored[i].first);
        }
        return result;
    };
    AdvancedVectorIndex::setStubCallbacks(std::move(callbacks));
    return state;
}

storage::TTTrain makeTrain(const std::vector<float>& values) {
    storage::TTTrain train;
    train.mode_sizes = {values.size()};

    storage::TTCore core;
    core.r_left = 1;
    core.n = values.size();
    core.r_right = 1;
    core.data = values;
    train.cores.push_back(core);
    train.original_norm = 1.0;
    return train;
}

class RecordingTracer final : public core::concerns::ITracer {
public:
    struct CompletedSpan {
        std::string name;
        std::map<std::string, std::string> string_attributes;
        bool ok = true;
        std::string description;
    };

    class Span final : public ITracer::ISpan {
    public:
        Span(std::string name,
             std::vector<CompletedSpan>* out,
             std::mutex* out_mutex)
            : name_(std::move(name))
            , out_(out)
            , out_mutex_(out_mutex) {}

        void setAttribute(const std::string& key, const std::string& value) override {
            attributes_[key] = value;
        }

        void setAttribute(const std::string& key, const std::int64_t value) override {
            attributes_[key] = std::to_string(value);
        }

        void setAttribute(const std::string& key, const double value) override {
            attributes_[key] = std::to_string(value);
        }

        void setAttribute(const std::string& key, const bool value) override {
            attributes_[key] = value ? "true" : "false";
        }

        void recordError(const std::string& errorMessage) override {
            ok_ = false;
            description_ = errorMessage;
        }

        void setStatus(const bool ok, const std::string& description = "") override {
            ok_ = ok;
            description_ = description;
        }

        void end() override {
            if (ended_) {
                return;
            }
            ended_ = true;

            std::lock_guard<std::mutex> lock(*out_mutex_);
            out_->push_back({name_, attributes_, ok_, description_});
        }

        [[nodiscard]] bool isValid() const override {
            return true;
        }

    private:
        std::string name_;
        std::map<std::string, std::string> attributes_;
        std::vector<CompletedSpan>* out_ = nullptr;
        std::mutex* out_mutex_ = nullptr;
        bool ok_ = true;
        bool ended_ = false;
        std::string description_;
    };

    std::unique_ptr<ISpan> startSpan(const std::string& name) override {
        return std::make_unique<Span>(name, &completed_, &mutex_);
    }

    std::unique_ptr<ISpan> startChildSpan(const std::string& name,
                                          const ISpan&) override {
        return startSpan(name);
    }

    bool initialize(const std::string&, const std::string&) override {
        initialized_ = true;
        return true;
    }

    void shutdown() override {
        initialized_ = false;
    }

    [[nodiscard]] bool isInitialized() const override {
        return initialized_;
    }

    [[nodiscard]] core::concerns::ProbeResult isHealthy() const override {
        return core::concerns::ProbeResult::healthy();
    }

    [[nodiscard]] std::vector<CompletedSpan> completed() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return completed_;
    }

private:
    mutable std::mutex mutex_;
    std::vector<CompletedSpan> completed_;
    bool initialized_ = true;
};

class TestLlmClient final : public llm::LLMClient {
public:
    explicit TestLlmClient(const std::chrono::milliseconds latency = std::chrono::milliseconds{0},
                           const bool success = true)
        : latency_(latency)
        , success_(success) {}

    llm::GenerationResult generate(const std::string& prompt,
                                   const llm::GenerationOptions&) override {
        if (latency_.count() > 0) {
            std::this_thread::sleep_for(latency_);
        }

        llm::GenerationResult result;
        result.success = success_;
        if (success_) {
            result.text = "answer:" + prompt.substr(0, std::min<std::size_t>(prompt.size(), 32));
            result.finish_reason = "stop";
        } else {
            result.error_message = "generation failed";
            result.finish_reason = "error";
        }
        return result;
    }

    llm::GenerationResult generateAQL(const std::string& nl_query,
                                      const std::string&,
                                      const llm::GenerationOptions& options) override {
        return generate(nl_query, options);
    }

    [[nodiscard]] std::size_t estimateTokens(const std::string& text) const override {
        return text.size() / 4;
    }

    [[nodiscard]] std::string getProviderName() const override {
        return "test";
    }

    [[nodiscard]] bool isReady() const override {
        return true;
    }

private:
    std::chrono::milliseconds latency_;
    bool success_ = true;
};

class LayeredRetrievalIntegrationPhase4Test : public ::testing::Test {
protected:
    void SetUp() override {
        backend_state_ = makeExactVectorBackend();
    }

    void TearDown() override {
        AdvancedVectorIndex::setStubCallbacks({});
    }

    static std::shared_ptr<graph::KnowledgeGraphReasoner> makeReasoner() {
        auto reasoner = std::make_shared<graph::KnowledgeGraphReasoner>();
        reasoner->addRule({
            "reports_to_transitive",
            {{"alice", "reports_to", "bob"}, {"bob", "reports_to", "carol"}},
            {{"alice", "indirectly_reports_to", "carol"}},
            "",
            0.0,
        });
        reasoner->addFact({"alice", "reports_to", "bob"});
        reasoner->addFact({"bob", "reports_to", "carol"});
        return reasoner;
    }

    std::shared_ptr<ExactVectorBackendState> backend_state_;
};

TEST_F(LayeredRetrievalIntegrationPhase4Test, ExecutesAllFourLayersAndEmitsPerLayerSpans) {
    AdvancedVectorIndex::Config ann_config;
    ann_config.index_type = AdvancedVectorIndex::Config::Type::HNSW_FLAT;
    auto ann_index = std::make_shared<AdvancedVectorIndex>(3, ann_config);

    const float vectors[] = {
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f,
    };
    const std::int64_t ids[] = {101, 102, 103};
    ASSERT_TRUE(ann_index->train(vectors, 3));
    ASSERT_TRUE(ann_index->addWithIds(vectors, ids, 3));

    auto tensor_graph = std::make_shared<tensor::TensorFingerprintGraph>();
    ASSERT_TRUE(tensor_graph->addAdapter("query_adapter", makeTrain({1.0f, 0.9f, 0.8f}),
                                         "legal", "llama3", "tenant-a"));
    ASSERT_TRUE(tensor_graph->addAdapter("similar_adapter", makeTrain({1.0f, 0.88f, 0.79f}),
                                         "legal", "llama3", "tenant-a"));

    auto reasoner = makeReasoner();
    auto llm_client = std::make_shared<TestLlmClient>();
    auto tracer = std::make_shared<RecordingTracer>();

    LayeredRetrievalConfig config;
    config.layer_timeout_ms = 50;
    config.ann_top_k = 2;

    LayeredRetrievalOrchestrator orchestrator(config);
    orchestrator.setAnnIndex(ann_index);
    orchestrator.setTensorGraph(tensor_graph);
    orchestrator.setGraphReasoner(reasoner);
    orchestrator.setLlmClient(llm_client);
    orchestrator.setTracer(tracer);

    LayeredRetrievalContext context;
    context.query = "who does alice indirectly report to?";
    context.query_vector = {0.95f, 0.02f, 0.01f};
    context.tensor_query_key = "query_adapter";
    context.graph_subject_id = "alice";
    context.correlation_id = "corr-4layer";
    context.llm_prompt_prefix = "Summarize the layered retrieval evidence.";

    const auto result = orchestrator.execute(context);

    ASSERT_FALSE(result.ann_candidates.empty());
    EXPECT_EQ(result.ann_candidates.front().id, 101);

    ASSERT_FALSE(result.tensor_candidates.empty());
    EXPECT_EQ(result.tensor_candidates.front().adapter_key, "similar_adapter");

    ASSERT_FALSE(result.provenance.empty());
    EXPECT_EQ(result.provenance.front().subject, "alice");
    EXPECT_EQ(result.provenance.front().predicate, "indirectly_reports_to");
    EXPECT_EQ(result.provenance.front().object, "carol");

    EXPECT_FALSE(result.final_answer.empty());
    ASSERT_EQ(result.routing_decisions.size(), 4U);
    for (const auto& decision : result.routing_decisions) {
        EXPECT_EQ(decision.decision, LayerRoutingDecision::EXECUTED);
    }

    const auto spans = tracer->completed();
    std::vector<std::string> span_names;
    span_names.reserve(spans.size());
    for (const auto& span : spans) {
        span_names.push_back(span.name);
    }

    EXPECT_NE(std::find(span_names.begin(), span_names.end(), "search.layer.ann"), span_names.end());
    EXPECT_NE(std::find(span_names.begin(), span_names.end(), "search.layer.tensor"), span_names.end());
    EXPECT_NE(std::find(span_names.begin(), span_names.end(), "search.layer.graph"), span_names.end());
    EXPECT_NE(std::find(span_names.begin(), span_names.end(), "search.layer.llm"), span_names.end());
}

TEST_F(LayeredRetrievalIntegrationPhase4Test, EnforcesHardDeadlineAndMarksTimeoutSkip) {
    AdvancedVectorIndex::Config ann_config;
    ann_config.index_type = AdvancedVectorIndex::Config::Type::HNSW_FLAT;
    auto ann_index = std::make_shared<AdvancedVectorIndex>(2, ann_config);

    const float vectors[] = {
        1.0f, 0.0f,
        0.0f, 1.0f,
    };
    const std::int64_t ids[] = {1, 2};
    ASSERT_TRUE(ann_index->train(vectors, 2));
    ASSERT_TRUE(ann_index->addWithIds(vectors, ids, 2));

    auto tensor_graph = std::make_shared<tensor::TensorFingerprintGraph>();
    ASSERT_TRUE(tensor_graph->addAdapter("slow_query", makeTrain({1.0f, 0.0f}),
                                         "ops", "llama3", "tenant-a"));
    ASSERT_TRUE(tensor_graph->addAdapter("slow_peer", makeTrain({0.9f, 0.1f}),
                                         "ops", "llama3", "tenant-a"));

    LayeredRetrievalConfig config;
    config.layer_timeout_ms = 5;
    config.ann_top_k = 1;

    LayeredRetrievalOrchestrator orchestrator(config);
    orchestrator.setAnnIndex(ann_index);
    orchestrator.setTensorGraph(tensor_graph);
    orchestrator.setGraphReasoner(makeReasoner());
    orchestrator.setLlmClient(std::make_shared<TestLlmClient>(std::chrono::milliseconds{25}));

    LayeredRetrievalContext context;
    context.query = "timed llm query";
    context.query_vector = {1.0f, 0.0f};
    context.tensor_query_key = "slow_query";
    context.graph_subject_id = "alice";

    const auto result = orchestrator.execute(context);

    ASSERT_EQ(result.routing_decisions.size(), 4U);
    EXPECT_EQ(result.routing_decisions.back().layer_name, "llm");
    EXPECT_EQ(result.routing_decisions.back().decision, LayerRoutingDecision::TIMEOUT_SKIP);
    EXPECT_TRUE(result.timed_out);
    EXPECT_FALSE(result.final_answer.empty());
}

TEST_F(LayeredRetrievalIntegrationPhase4Test, AppliesPerQueryGuardrailsAcrossLayers) {
    AdvancedVectorIndex::Config ann_config;
    ann_config.index_type = AdvancedVectorIndex::Config::Type::HNSW_FLAT;
    auto ann_index = std::make_shared<AdvancedVectorIndex>(2, ann_config);

    const float vectors[] = {
        1.0f, 0.0f,
        0.9f, 0.1f,
        0.8f, 0.2f,
    };
    const std::int64_t ids[] = {11, 12, 13};
    ASSERT_TRUE(ann_index->train(vectors, 3));
    ASSERT_TRUE(ann_index->addWithIds(vectors, ids, 3));

    auto tensor_graph = std::make_shared<tensor::TensorFingerprintGraph>();
    ASSERT_TRUE(tensor_graph->addAdapter("guardrail_query", makeTrain({1.0f, 0.9f, 0.8f}),
                                         "finance", "llama3", "tenant-a"));
    ASSERT_TRUE(tensor_graph->addAdapter("guardrail_peer_a", makeTrain({1.0f, 0.88f, 0.78f}),
                                         "finance", "llama3", "tenant-a"));
    ASSERT_TRUE(tensor_graph->addAdapter("guardrail_peer_b", makeTrain({0.95f, 0.87f, 0.76f}),
                                         "finance", "llama3", "tenant-a"));

    LayeredRetrievalConfig config;
    config.layer_timeout_ms = 50;
    config.ann_top_k = 3;
    config.guardrails.enabled = true;
    config.guardrails.max_ann_candidates = 1;
    config.guardrails.max_tensor_candidates = 1;
    config.guardrails.max_graph_edges = 1;
    config.guardrails.max_prompt_chars = 64;

    LayeredRetrievalOrchestrator orchestrator(config);
    orchestrator.setAnnIndex(ann_index);
    orchestrator.setTensorGraph(tensor_graph);
    orchestrator.setGraphReasoner(makeReasoner());
    orchestrator.setLlmClient(std::make_shared<TestLlmClient>());

    LayeredRetrievalContext context;
    context.query = "Provide a long answer about alice and retrieval guardrails.";
    context.query_vector = {1.0f, 0.0f};
    context.tensor_query_key = "guardrail_query";
    context.graph_subject_id = "alice";
    context.llm_prompt_prefix = "This prefix is intentionally long to force prompt trimming.";

    const auto result = orchestrator.execute(context);

    EXPECT_EQ(result.ann_candidates.size(), 1U);
    EXPECT_EQ(result.tensor_candidates.size(), 1U);
    EXPECT_EQ(result.provenance.size(), 1U);
    EXPECT_TRUE(result.guardrail_pruned);
    EXPECT_FALSE(result.final_answer.empty());
}

} // namespace
} // namespace themis::search::testing
