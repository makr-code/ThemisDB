#include "../test_data_generator.h"
#include "../test_fixture.h"

#include <mutex>
#include <optional>
#include <unordered_map>

namespace themis { namespace test { 

namespace {

struct RagResponse {
    bool ok{false};
    bool cache_hit{false};
    std::string answer;
    std::vector<std::string> hits;
};

class RagAiPipeline {
public:
    RagAiPipeline(std::shared_ptr<MockPipelineLlmBackend> llm,
                  std::shared_ptr<MockPipelineIndex> index,
                  std::shared_ptr<PipelineAuditLog> audit)
        : llm_(std::move(llm)), index_(std::move(index)), audit_(std::move(audit)) {}

    void SetGraphContext(std::string term, std::string context) {
        std::lock_guard<std::mutex> lock(mutex_);
        graph_context_[std::move(term)] = std::move(context);
    }

    [[nodiscard]] RagResponse Ask(const std::string& query, const std::string& term) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            const auto cache_it = answer_cache_.find(query);
            if (cache_it != answer_cache_.end()) {
                audit_->Record({"cache", "hit", query});
                return {true, true, cache_it->second, {}};
            }
        }

        const auto embedding = llm_->GenerateEmbedding(query);
        if (!embedding.has_value()) {
            return {true, false, "fallback:embedding-unavailable", {}};
        }

        const auto hits = index_->Search(term);
        std::string context = hits.empty() ? "no_context" : hits.front();
        {
            std::lock_guard<std::mutex> lock(mutex_);
            const auto graph_it = graph_context_.find(term);
            if (graph_it != graph_context_.end()) {
                context += "|" + graph_it->second;
            }
        }

        const auto answer = llm_->Infer(query, context);
        if (!answer.has_value()) {
            return {true, false, "fallback:inference-unavailable", hits};
        }

        {
            std::lock_guard<std::mutex> lock(mutex_);
            answer_cache_[query] = *answer;
        }
        audit_->Record({"rag", "answer_generated", query});
        return {true, false, *answer, hits};
    }

private:
    std::shared_ptr<MockPipelineLlmBackend> llm_;
    std::shared_ptr<MockPipelineIndex> index_;
    std::shared_ptr<PipelineAuditLog> audit_;

    std::mutex mutex_;
    std::unordered_map<std::string, std::string> answer_cache_;
    std::unordered_map<std::string, std::string> graph_context_;
};

} // namespace

class RagAiPipelineTest : public IntegrationTestFixture {
protected:
    void SetUp() override {
        IntegrationTestFixture::SetUp();
        llm_ = CreateMockLlmBackend();
        index_ = CreateMockIndex();
        audit_ = CreateAuditLog();
        data_gen_ = std::make_unique<TestDataGenerator>();
        pipeline_ = std::make_unique<RagAiPipeline>(llm_, index_, audit_);
    }

    std::shared_ptr<MockPipelineLlmBackend> llm_;
    std::shared_ptr<MockPipelineIndex> index_;
    std::shared_ptr<PipelineAuditLog> audit_;
    std::unique_ptr<TestDataGenerator> data_gen_;
    std::unique_ptr<RagAiPipeline> pipeline_;
};

TEST_F(RagAiPipelineTest, RAG01_EmbeddingVectorSearchAndInferencePipelineWorks) {
    index_->IndexDocument("doc_rag_1", {"rag"});

    const auto response = pipeline_->Ask("What is ThemisDB?", "rag");

    ASSERT_TRUE(response.ok);
    EXPECT_FALSE(response.answer.empty());
    EXPECT_EQ(llm_->EmbeddingCalls(), 1U);
    EXPECT_EQ(llm_->InferenceCalls(), 1U);
    EXPECT_TRUE(audit_->Contains("rag", "answer_generated"));
}

TEST_F(RagAiPipelineTest, RAG02_RepeatedQuestionUsesCacheAndSkipsInference) {
    index_->IndexDocument("doc_rag_2", {"cache"});

    const auto first = pipeline_->Ask("Explain cache", "cache");
    const auto second = pipeline_->Ask("Explain cache", "cache");

    ASSERT_TRUE(first.ok);
    ASSERT_TRUE(second.ok);
    EXPECT_TRUE(second.cache_hit);
    EXPECT_EQ(llm_->EmbeddingCalls(), 1U);
    EXPECT_EQ(llm_->InferenceCalls(), 1U);
}

TEST_F(RagAiPipelineTest, RAG03_GraphContextIsIncludedInResponseContext) {
    index_->IndexDocument("doc_rag_3", {"graph"});
    pipeline_->SetGraphContext("graph", "entity:Permit#42");

    const auto response = pipeline_->Ask("graph context", "graph");

    ASSERT_TRUE(response.ok);
    EXPECT_NE(response.answer.find("entity:Permit#42"), std::string::npos);
}

TEST_F(RagAiPipelineTest, RAG04_InferenceFailureReturnsStableFallbackWithoutCorruption) {
    index_->IndexDocument("doc_rag_4", {"fallback"});
    llm_->SetInferenceFailure(true);

    const auto response = pipeline_->Ask("fallback question", "fallback");

    ASSERT_TRUE(response.ok);
    EXPECT_EQ(response.answer, "fallback:inference-unavailable");
    EXPECT_EQ(llm_->InferenceCalls(), 1U);
}

TEST_F(RagAiPipelineTest, RAG05_EmbeddingFailureDegradesWithoutRunningInference) {
    index_->IndexDocument("doc_rag_5", {"embed-fallback"});
    llm_->SetEmbeddingFailure(true);

    const auto response = pipeline_->Ask("embedding unavailable", "embed-fallback");

    ASSERT_TRUE(response.ok);
    EXPECT_EQ(response.answer, "fallback:embedding-unavailable");
    EXPECT_EQ(llm_->EmbeddingCalls(), 1U);
    EXPECT_EQ(llm_->InferenceCalls(), 0U);
}

TEST_F(RagAiPipelineTest, RAG06_CacheHitProducesDeterministicAnswerAcrossRepeatedRuns) {
    index_->IndexDocument("doc_rag_6", {"stable"});
    pipeline_->SetGraphContext("stable", "entity:Stable#1");

    const auto first = pipeline_->Ask("repeatable prompt", "stable");
    const auto second = pipeline_->Ask("repeatable prompt", "stable");

    ASSERT_TRUE(first.ok);
    ASSERT_TRUE(second.ok);
    EXPECT_EQ(first.answer, second.answer);
    EXPECT_TRUE(second.cache_hit);
}
} } // namespace themis::test
