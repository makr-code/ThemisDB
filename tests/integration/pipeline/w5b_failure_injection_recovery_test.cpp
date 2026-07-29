/*
 * ThemisDB | File: w5b_failure_injection_recovery_test.cpp | Version: 0.1.0
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Wave 5-B: Failure Injection & Recovery Validation
 *
 * Verifies that the pipeline handles targeted failure scenarios
 * deterministically and recovers to consistent state:
 *
 *   FIR-01  LLM embedding failure: RAG pipeline returns error, no partial state
 *   FIR-02  LLM inference failure: embedding done, infer fails, no answer stored
 *   FIR-03  Empty transaction: zero-write commit rejected cleanly
 *   FIR-04  Missing document ID: ingest boundary rejects before any storage write
 *   FIR-05  Index miss: query for absent term returns empty hits, no error
 *   FIR-06  Auth recovery: denied token then re-authorized token succeeds
 *   FIR-07  Partial-batch failure: errors on some docs do not block valid docs
 *   FIR-08  RAG no-hits recovery: retrieval finds nothing, error reported cleanly
 *
 * Design constraints:
 *   - No external runtime dependencies.
 *   - Failure modes injected via mock controls (SetEmbeddingFailure, etc.).
 *   - All tests are fully self-contained and leave no cross-test state.
 */

#include "../test_data_generator.h"
#include "../test_fixture.h"

#include <string>
#include <vector>

namespace themis { namespace test { 

// ─────────────────────────────────────────────────────────────────────────────
// Thin pipeline orchestrator — reused from W5-A, inlined here to keep
// each test file independently compilable.
// ─────────────────────────────────────────────────────────────────────────────

namespace {

struct IngestResult {
    bool ok{false};
    std::string error;
};

struct QueryResult {
    bool ok{false};
    std::vector<std::string> hits;
    std::string error;
};

struct RagResult {
    bool ok{false};
    std::string answer;
    std::string error;
};

struct TxResult {
    bool committed{false};
    std::string status;
};

/**
 * @brief Minimal pipeline façade used by FIR tests.
 *
 * @note Failure behaviour is injected exclusively via mock controls so that
 *       the pipeline code path itself is not altered for testing.
 */
class FailureTestPipeline {
public:
    FailureTestPipeline(std::shared_ptr<MockPipelineAuth>          auth,
                        std::shared_ptr<InMemoryPipelineStorage>   storage,
                        std::shared_ptr<MockPipelineIndex>         index,
                        std::shared_ptr<MockPipelineLlmBackend>    llm,
                        std::shared_ptr<PipelineAuditLog>          audit)
        : auth_(std::move(auth))
        , storage_(std::move(storage))
        , index_(std::move(index))
        , llm_(std::move(llm))
        , audit_(std::move(audit)) {}

    [[nodiscard]] IngestResult Ingest(const nlohmann::json& doc,
                                      const std::vector<std::string>& terms = {}) {
        const std::string id = doc.value("id", "");
        if (id.empty()) {
            audit_->Record({"ingest", "error", "missing_id"});
            return {false, "missing_id"};
        }
        if (!doc.contains("title")) {
            audit_->Record({"ingest", "schema_error", id});
            return {false, "schema_validation_error"};
        }
        storage_->Write(id, doc.dump());
        index_->IndexDocument(id, terms.empty() ? std::vector<std::string>{id} : terms);
        audit_->Record({"ingest", "ok", id});
        return {true, ""};
    }

    [[nodiscard]] QueryResult Query(const std::string& token, const std::string& term) {
        const auto ar = auth_->Authorize(token);
        if (!ar.authorized) {
            audit_->Record({"query", "auth_failed", token});
            return {false, {}, "401 unauthorized"};
        }
        const auto hits = index_->Search(term);
        audit_->Record({"query", "executed", term});
        return {true, hits, ""};
    }

    [[nodiscard]] RagResult Rag(const std::string& query, const std::string& term) {
        const auto emb = llm_->GenerateEmbedding(query);
        if (!emb.has_value()) {
            audit_->Record({"rag", "embed_failed", query});
            return {false, "", "embedding_failed"};
        }
        const auto hits = index_->Search(term);
        if (hits.empty()) {
            audit_->Record({"rag", "no_hits", term});
            return {false, "", "no_documents"};
        }
        const auto ctx_opt = storage_->Read(hits.front());
        const std::string ctx = ctx_opt.value_or("{}");
        const auto ans = llm_->Infer(query, ctx);
        if (!ans.has_value()) {
            audit_->Record({"rag", "infer_failed", query});
            return {false, "", "inference_failed"};
        }
        audit_->Record({"rag", "ok", query});
        return {true, *ans, ""};
    }

    [[nodiscard]] TxResult Commit(const std::vector<std::pair<std::string, std::string>>& writes) {
        if (writes.empty()) {
            return {false, "empty_tx"};
        }
        for (const auto& [key, value] : writes) {
            storage_->Write(key, value);
        }
        audit_->Record({"tx", "committed", std::to_string(writes.size())});
        return {true, "committed"};
    }

    [[nodiscard]] MockPipelineLlmBackend& Llm() { return *llm_; }
    [[nodiscard]] PipelineAuditLog& Audit() { return *audit_; }

private:
    std::shared_ptr<MockPipelineAuth>        auth_;
    std::shared_ptr<InMemoryPipelineStorage> storage_;
    std::shared_ptr<MockPipelineIndex>       index_;
    std::shared_ptr<MockPipelineLlmBackend>  llm_;
    std::shared_ptr<PipelineAuditLog>        audit_;
};

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Test fixture
// ─────────────────────────────────────────────────────────────────────────────

class W5BFailureInjectionRecoveryTest : public IntegrationTestFixture {
protected:
    void SetUp() override {
        IntegrationTestFixture::SetUp();

        data_gen_ = std::make_unique<TestDataGenerator>();
        auth_     = CreateMockAuth();
        storage_  = CreateInMemoryStorage();
        index_    = CreateMockIndex();
        llm_      = CreateMockLlmBackend();
        audit_    = CreateAuditLog();
        pipeline_ = std::make_unique<FailureTestPipeline>(
            auth_, storage_, index_, llm_, audit_);

        auth_->AllowToken("tok-valid");
        auth_->DenyToken("tok-denied");
    }

    std::unique_ptr<TestDataGenerator>   data_gen_;
    std::shared_ptr<MockPipelineAuth>    auth_;
    std::shared_ptr<InMemoryPipelineStorage> storage_;
    std::shared_ptr<MockPipelineIndex>   index_;
    std::shared_ptr<MockPipelineLlmBackend> llm_;
    std::shared_ptr<PipelineAuditLog>    audit_;
    std::unique_ptr<FailureTestPipeline> pipeline_;
};

// ─────────────────────────────────────────────────────────────────────────────
// FIR-01  LLM embedding failure
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test FIR-01: When the LLM backend is configured to fail on embedding,
 *       RAG returns an error immediately.  No partial answer or storage write
 *       must occur.
 *
 * Acceptance criteria:
 *   - Rag() returns ok==false, error=="embedding_failed".
 *   - answer is empty.
 *   - Audit records rag::embed_failed.
 *   - Storage size is unchanged.
 */
TEST_F(W5BFailureInjectionRecoveryTest, FIR01_EmbeddingFailureRagReturnsError) {
    // Pre-ingest a document so retrieval would otherwise succeed
    auto doc = data_gen_->GenerateTestDocument("fir01");
    pipeline_->Ingest(doc, {"fir01-term"});

    const size_t size_before = storage_->Size();

    // Inject embedding failure
    pipeline_->Llm().SetEmbeddingFailure(true);

    const auto result = pipeline_->Rag("test query", "fir01-term");

    EXPECT_FALSE(result.ok)                     << "RAG must fail when embedding fails";
    EXPECT_EQ(result.error, "embedding_failed") << "Error code must be embedding_failed";
    EXPECT_TRUE(result.answer.empty())           << "No answer must be returned on embed failure";
    EXPECT_TRUE(audit_->Contains("rag", "embed_failed"))
        << "Audit must record embed_failed";
    EXPECT_EQ(storage_->Size(), size_before)
        << "Storage must not change after embedding failure";
}

// ─────────────────────────────────────────────────────────────────────────────
// FIR-02  LLM inference failure
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test FIR-02: Embedding succeeds but inference fails.  RAG must return
 *       inference_failed without committing any answer to storage.
 *
 * Acceptance criteria:
 *   - Rag() returns ok==false, error=="inference_failed".
 *   - Embedding call count == 1 (one call was made).
 *   - Inference call count == 1 (attempt was made, failed).
 *   - Audit records rag::infer_failed.
 */
TEST_F(W5BFailureInjectionRecoveryTest, FIR02_InferenceFailureNoPartialAnswer) {
    auto doc = data_gen_->GenerateTestDocument("fir02");
    pipeline_->Ingest(doc, {"fir02-term"});

    // Inject inference failure only (embedding is healthy)
    pipeline_->Llm().SetInferenceFailure(true);

    const auto result = pipeline_->Rag("test query", "fir02-term");

    EXPECT_FALSE(result.ok)                      << "RAG must fail when inference fails";
    EXPECT_EQ(result.error, "inference_failed")  << "Error code must be inference_failed";
    EXPECT_TRUE(result.answer.empty());

    EXPECT_EQ(pipeline_->Llm().EmbeddingCalls(), 1U)
        << "Embedding must have been called exactly once";
    EXPECT_EQ(pipeline_->Llm().InferenceCalls(), 1U)
        << "Inference must have been attempted exactly once";
    EXPECT_TRUE(audit_->Contains("rag", "infer_failed"));
}

// ─────────────────────────────────────────────────────────────────────────────
// FIR-03  Empty transaction rejected cleanly
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test FIR-03: A commit with zero writes must be rejected without any
 *       side effect.  The pipeline must return empty_tx and leave storage
 *       and audit unchanged.
 *
 * Acceptance criteria:
 *   - Commit({}) returns committed==false, status=="empty_tx".
 *   - Storage size is unchanged.
 *   - Audit does not record tx::committed.
 */
TEST_F(W5BFailureInjectionRecoveryTest, FIR03_EmptyTransactionRejectedCleanly) {
    const size_t size_before = storage_->Size();

    const auto tx = pipeline_->Commit({});

    EXPECT_FALSE(tx.committed)        << "Empty transaction must not commit";
    EXPECT_EQ(tx.status, "empty_tx")  << "Status must be empty_tx";
    EXPECT_EQ(storage_->Size(), size_before)
        << "Storage must not change for empty transaction";
    EXPECT_FALSE(audit_->Contains("tx", "committed"))
        << "Audit must not record committed for empty transaction";
}

// ─────────────────────────────────────────────────────────────────────────────
// FIR-04  Missing document ID rejected before storage write
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test FIR-04: A document with no "id" field must be rejected at the ingest
 *       boundary.  The storage must not receive any write.
 *
 * Acceptance criteria:
 *   - Ingest returns ok==false, error=="missing_id".
 *   - Storage size is unchanged.
 *   - Audit records ingest::error.
 */
TEST_F(W5BFailureInjectionRecoveryTest, FIR04_MissingIdRejectedBeforeStorageWrite) {
    nlohmann::json bad_doc = {
        {"title",   "no id document"},
        {"content", "body"},
    };

    const size_t size_before = storage_->Size();

    const auto result = pipeline_->Ingest(bad_doc);

    EXPECT_FALSE(result.ok)               << "Document without id must be rejected";
    EXPECT_EQ(result.error, "missing_id") << "Error must be missing_id";
    EXPECT_EQ(storage_->Size(), size_before)
        << "Storage must not be written for id-less document";
    EXPECT_TRUE(audit_->Contains("ingest", "error"));
}

// ─────────────────────────────────────────────────────────────────────────────
// FIR-05  Index miss returns empty hits without error
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test FIR-05: A query for a term that was never indexed must return an
 *       empty hit list with ok==true (not an error).  The query path is
 *       healthy; there simply are no matching documents.
 *
 * Acceptance criteria:
 *   - Query returns ok==true (auth succeeded).
 *   - hits is empty.
 *   - Audit records query::executed.
 */
TEST_F(W5BFailureInjectionRecoveryTest, FIR05_IndexMissReturnsEmptyHitsNoError) {
    const auto result = pipeline_->Query("tok-valid", "never-indexed-term");

    EXPECT_TRUE(result.ok)           << "Query itself must succeed (auth ok)";
    EXPECT_TRUE(result.hits.empty()) << "No hits expected for un-indexed term";
    EXPECT_EQ(result.error, "")      << "No error expected for index miss";
    EXPECT_TRUE(audit_->Contains("query", "executed"));
}

// ─────────────────────────────────────────────────────────────────────────────
// FIR-06  Auth recovery: denied → re-authorised → success
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test FIR-06: A token that was explicitly denied first results in a 401.
 *       After the token is added to the allowed set (simulating a permission
 *       grant or token rotation), the same token must succeed on re-query.
 *
 * Acceptance criteria:
 *   - First query with denied token → ok==false.
 *   - After AllowToken() the same token → ok==true.
 */
TEST_F(W5BFailureInjectionRecoveryTest, FIR06_AuthRecoveryDeniedThenReauthorised) {
    auto doc = data_gen_->GenerateTestDocument("fir06");
    pipeline_->Ingest(doc, {"recovery-term"});

    // Phase 1: denied
    const auto denied = pipeline_->Query("tok-denied", "recovery-term");
    EXPECT_FALSE(denied.ok);

    // Re-authorise the previously denied token
    auth_->AllowToken("tok-denied");

    // Phase 2: should succeed now
    const auto allowed = pipeline_->Query("tok-denied", "recovery-term");
    EXPECT_TRUE(allowed.ok)
        << "Re-authorised token must succeed on retry";
    EXPECT_FALSE(allowed.hits.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// FIR-07  Partial-batch failure: valid docs succeed despite some errors
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test FIR-07: A batch of mixed documents (some valid, some schema-invalid)
 *       must allow all valid documents to be ingested.  Invalid documents are
 *       rejected individually without aborting the remainder of the batch.
 *
 * Acceptance criteria:
 *   - 3 valid docs ingested → storage size == 3.
 *   - 2 invalid docs rejected → ingest::schema_error recorded twice.
 *   - Total audit count reflects both success and failure events.
 */
TEST_F(W5BFailureInjectionRecoveryTest, FIR07_PartialBatchFailureValidDocsSucceed) {
    // Mix of 3 valid and 2 invalid (missing title) documents
    std::vector<nlohmann::json> batch = {
        {{"id", "fir07_v1"}, {"title", "valid 1"}, {"content", "ok"}},
        {{"id", "fir07_bad1"}, {"content", "no title"}},           // invalid
        {{"id", "fir07_v2"}, {"title", "valid 2"}, {"content", "ok"}},
        {{"id", "fir07_bad2"}, {"content", "also no title"}},      // invalid
        {{"id", "fir07_v3"}, {"title", "valid 3"}, {"content", "ok"}},
    };

    int success = 0;
    int failures = 0;

    for (const auto& doc : batch) {
        const auto res = pipeline_->Ingest(doc, {"batch-mixed"});
        if (res.ok) { ++success; } else { ++failures; }
    }

    EXPECT_EQ(success,  3) << "Exactly 3 valid documents must be ingested";
    EXPECT_EQ(failures, 2) << "Exactly 2 invalid documents must be rejected";
    EXPECT_EQ(storage_->Size(), 3U)
        << "Only valid documents should be in storage";
    size_t schema_error_count = 0;
    for (const auto& event : audit_->Snapshot()) {
        if (event.module == "ingest" && event.action == "schema_error") {
            ++schema_error_count;
        }
    }
    EXPECT_EQ(schema_error_count, 2U)
        << "Audit must record schema_error twice for the two invalid documents";
    EXPECT_EQ(audit_->Count(), batch.size())
        << "Audit count must reflect one result event for each document in the mixed batch";
}

// ─────────────────────────────────────────────────────────────────────────────
// FIR-08  RAG no-hits recovery: retrieval finds nothing, error reported cleanly
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test FIR-08: When RAG embedding succeeds but the index has no matching
 *       documents for the search term, the pipeline must return a clean
 *       no_documents error without invoking inference.
 *
 * Acceptance criteria:
 *   - Rag() returns ok==false, error=="no_documents".
 *   - Inference call count == 0 (inference must not be attempted).
 *   - Audit records rag::no_hits.
 */
TEST_F(W5BFailureInjectionRecoveryTest, FIR08_RagNoHitsRecoveryReportsCleanError) {
    // Do NOT ingest any document with the search term
    const auto result = pipeline_->Rag("test query", "term-not-indexed");

    EXPECT_FALSE(result.ok)                   << "RAG must fail when index has no hits";
    EXPECT_EQ(result.error, "no_documents")   << "Error must be no_documents";
    EXPECT_TRUE(result.answer.empty());

    EXPECT_EQ(pipeline_->Llm().EmbeddingCalls(), 1U)
        << "Embedding must have been called (before retrieval)";
    EXPECT_EQ(pipeline_->Llm().InferenceCalls(), 0U)
        << "Inference must NOT be called when there are no retrieved documents";
    EXPECT_TRUE(audit_->Contains("rag", "no_hits"))
        << "Audit must record no_hits";
}
} } // namespace themis::test
