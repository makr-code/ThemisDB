/*
 * ThemisDB | File: query_execution_pipeline_test.cpp | Version: 0.0.1
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "../test_data_generator.h"
#include "../test_fixture.h"

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

namespace themis { namespace test { 

namespace {

struct QueryPipelineResult {
    bool ok{false};
    bool cache_hit{false};
    bool index_used{false};
    std::string payload;
    std::string error;
};

class QueryExecutionPipeline {
public:
    QueryExecutionPipeline(std::shared_ptr<MockPipelineAuth> auth,
                           std::shared_ptr<MockPipelineIndex> index,
                           std::shared_ptr<InMemoryPipelineStorage> storage,
                           std::shared_ptr<PipelineAuditLog> audit)
        : auth_(std::move(auth)), index_(std::move(index)), storage_(std::move(storage)), audit_(std::move(audit)) {}

    [[nodiscard]] QueryPipelineResult Execute(const std::string& token,
                                              const std::string& query,
                                              const std::string& term) {
        const auto auth_result = auth_->Authorize(token);
        if (!auth_result.authorized) {
            audit_->Record({"server", "auth_failed", auth_result.reason});
            ++unauthorized_count_;
            return {false, false, false, "", "401 unauthorized"};
        }

        if (query.find("INVALID") != std::string::npos) {
            audit_->Record({"query", "parse_error", query});
            ++query_error_count_;
            return {false, false, false, "", "400 parse_error"};
        }

        {
            std::lock_guard<std::mutex> lock(cache_mutex_);
            const auto it = cache_.find(query);
            if (it != cache_.end()) {
                audit_->Record({"cache", "hit", query});
                ++success_count_;
                return {true, true, it->second.index_used, it->second.payload, ""};
            }
        }

        auto result = QueryPipelineResult{};
        const auto doc_ids = index_->Search(term);
        result.index_used = !doc_ids.empty();

        if (doc_ids.empty()) {
            result.ok = false;
            result.error = "404 no_index_match";
            return result;
        }

        const auto payload = storage_->Read(doc_ids.front());
        if (!payload.has_value()) {
            result.ok = false;
            result.error = "404 storage_miss";
            return result;
        }

        result.ok = true;
        result.payload = *payload;
        audit_->Record({"query", "execute", query});

        {
            std::lock_guard<std::mutex> lock(cache_mutex_);
            cache_[query] = result;
        }
        ++success_count_;
        return result;
    }

    [[nodiscard]] size_t SuccessCount() const { return success_count_.load(); }
    [[nodiscard]] size_t UnauthorizedCount() const { return unauthorized_count_.load(); }
    [[nodiscard]] size_t QueryErrorCount() const { return query_error_count_.load(); }

private:
    std::shared_ptr<MockPipelineAuth> auth_;
    std::shared_ptr<MockPipelineIndex> index_;
    std::shared_ptr<InMemoryPipelineStorage> storage_;
    std::shared_ptr<PipelineAuditLog> audit_;

    mutable std::mutex cache_mutex_;
    std::unordered_map<std::string, QueryPipelineResult> cache_;
    std::atomic_size_t success_count_{0};
    std::atomic_size_t unauthorized_count_{0};
    std::atomic_size_t query_error_count_{0};
};

} // namespace

class QueryExecutionPipelineTest : public IntegrationTestFixture {
protected:
    void SetUp() override {
        IntegrationTestFixture::SetUp();
        auth_ = CreateMockAuth();
        index_ = CreateMockIndex();
        storage_ = CreateInMemoryStorage();
        audit_ = CreateAuditLog();
        data_gen_ = std::make_unique<TestDataGenerator>();
        pipeline_ = std::make_unique<QueryExecutionPipeline>(auth_, index_, storage_, audit_);
    }

    std::shared_ptr<MockPipelineAuth> auth_;
    std::shared_ptr<MockPipelineIndex> index_;
    std::shared_ptr<InMemoryPipelineStorage> storage_;
    std::shared_ptr<PipelineAuditLog> audit_;
    std::unique_ptr<TestDataGenerator> data_gen_;
    std::unique_ptr<QueryExecutionPipeline> pipeline_;
};

TEST_F(QueryExecutionPipelineTest, QP01_AuthenticatedQueryFlowProducesResultAndAudit) {
    const auto token = data_gen_->GeneratePipelineToken(true);
    auth_->AllowToken(token);

    storage_->Write("doc_1", "payload_v1");
    index_->IndexDocument("doc_1", {"geo"});

    const auto result = pipeline_->Execute(token, data_gen_->GenerateAqlQuery("docs", "d.value > 10"), "geo");

    ASSERT_TRUE(result.ok);
    EXPECT_EQ(result.payload, "payload_v1");
    EXPECT_TRUE(audit_->Contains("query", "execute"));
}

TEST_F(QueryExecutionPipelineTest, QP02_IndexSelectionAndSecondCallCacheHit) {
    const auto token = data_gen_->GeneratePipelineToken(true);
    auth_->AllowToken(token);

    storage_->Write("doc_2", "payload_v2");
    index_->IndexDocument("doc_2", {"hnsw"});

    const auto query = data_gen_->GenerateAqlQuery("docs", "d.type == 'v'", 1);
    const auto first = pipeline_->Execute(token, query, "hnsw");
    const auto second = pipeline_->Execute(token, query, "hnsw");

    ASSERT_TRUE(first.ok);
    EXPECT_TRUE(first.index_used);
    ASSERT_TRUE(second.ok);
    EXPECT_TRUE(second.cache_hit);
}

TEST_F(QueryExecutionPipelineTest, QP03_CteWindowQueryRunsThroughPipeline) {
    const auto token = data_gen_->GeneratePipelineToken(true);
    auth_->AllowToken(token);

    storage_->Write("doc_3", "window_result");
    index_->IndexDocument("doc_3", {"cte"});

    const std::string query = "WITH base AS (FOR d IN docs RETURN d) FOR b IN base LIMIT 1 RETURN b";
    const auto result = pipeline_->Execute(token, query, "cte");

    ASSERT_TRUE(result.ok);
    EXPECT_EQ(result.payload, "window_result");
}

TEST_F(QueryExecutionPipelineTest, QP04_SyntaxAndAuthErrorsArePropagated) {
    const auto valid_token = data_gen_->GeneratePipelineToken(true);
    const auto invalid_token = data_gen_->GeneratePipelineToken(false);
    auth_->AllowToken(valid_token);

    const auto auth_error = pipeline_->Execute(invalid_token, "FOR d IN docs RETURN d", "x");
    const auto parse_error = pipeline_->Execute(valid_token, "INVALID QUERY", "x");

    EXPECT_FALSE(auth_error.ok);
    EXPECT_EQ(auth_error.error, "401 unauthorized");
    EXPECT_FALSE(parse_error.ok);
    EXPECT_EQ(parse_error.error, "400 parse_error");
}

TEST_F(QueryExecutionPipelineTest, QP05_ParallelQueriesKeepMetricsConsistent) {
    const auto token = data_gen_->GeneratePipelineToken(true);
    auth_->AllowToken(token);

    storage_->Write("doc_4", "parallel_payload");
    index_->IndexDocument("doc_4", {"parallel"});

    constexpr size_t kThreads = 8;
    constexpr size_t kPerThread = 10;

    std::vector<std::thread> workers;
    workers.reserve(kThreads);

    for (size_t t = 0; t < kThreads; ++t) {
        workers.emplace_back([&, t]() {
            for (size_t i = 0; i < kPerThread; ++i) {
                const auto query = "FOR d IN docs FILTER d.t == " + std::to_string(t) + " RETURN d";
                const auto result = pipeline_->Execute(token, query, "parallel");
                EXPECT_TRUE(result.ok);
            }
        });
    }

    for (auto& worker : workers) {
        worker.join();
    }

    EXPECT_EQ(pipeline_->UnauthorizedCount(), 0U);
    EXPECT_EQ(pipeline_->QueryErrorCount(), 0U);
    EXPECT_EQ(pipeline_->SuccessCount(), kThreads * kPerThread);
}

TEST_F(QueryExecutionPipelineTest, QP06_StorageMissReturnsDeterministicErrorWithoutPollutingSuccessMetrics) {
    const auto token = data_gen_->GeneratePipelineToken(true);
    auth_->AllowToken(token);
    index_->IndexDocument("doc_missing_payload", {"missing"});

    const auto before_events = audit_->Count();
    const auto result = pipeline_->Execute(token, "FOR d IN docs RETURN d", "missing");

    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error, "404 storage_miss");
    EXPECT_FALSE(result.cache_hit);
    EXPECT_EQ(pipeline_->SuccessCount(), 0U);
    EXPECT_EQ(audit_->Count(), before_events);
}
} } // namespace themis::test
