/**
 * @file cross_module_ingest_index_query_test.cpp
 * @brief Wave 2 cross-module integration tests: Ingest → Index → Query pipeline.
 *
 * ## Rationale (W2-A / W2-B)
 *
 * Wave 1 covered each pipeline stage in isolation (ingestion_pipeline_test,
 * query_execution_pipeline_test).  Wave 2 validates the *composed* flow where
 * documents written by the ingestion stage are immediately visible to the query
 * stage through the shared index and storage — without any intermediate reset.
 *
 * This suite covers:
 *
 *   CMX-01  Happy-path: single document ingested and retrieved end-to-end.
 *   CMX-02  Batch ingest with rate-limit checkpoint → bulk query returns all docs.
 *   CMX-03  RAG ask after ingest uses LLM embedding on indexed context.
 *   CMX-04  CDC events emitted by ingestion are consistent with index/storage state.
 *   CMX-05  Content error during ingest blocks query (no partial artifact visible).
 *   CMX-06  Parallel ingest + concurrent queries maintain count consistency.
 *
 * ## Run
 * @code
 *   ctest -R cross_module_ingest_index_query_test -L cross_module --output-on-failure
 * @endcode
 *
 * Copyright (c) 2026 ThemisDB Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "../test_data_generator.h"
#include "../test_fixture.h"

#include <algorithm>
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

namespace themis { namespace test { 

namespace {

// ---------------------------------------------------------------------------
// Lightweight pipeline facades used only in this file.
// They deliberately reuse the same InMemoryPipelineStorage / MockPipelineIndex
// instances so that data written by IngestionFacade is visible to QueryFacade.
// ---------------------------------------------------------------------------

struct IngestResult {
    bool ok{false};
    std::string error = {};
};

/**
 * @brief Ingestion facade for cross-module tests.
 *
 * Writes documents to a shared storage + index and emits CDC tokens.
 * Content errors suppress all writes, leaving no partial artifacts.
 */
class IngestionFacade {
public:
    IngestionFacade(std::shared_ptr<InMemoryPipelineStorage> storage,
                    std::shared_ptr<MockPipelineIndex> index,
                    std::shared_ptr<PipelineAuditLog> audit,
                    size_t rate_limit)
        : storage_(std::move(storage)),
          index_(std::move(index)),
          audit_(std::move(audit)),
          rate_limit_(rate_limit) {}

    [[nodiscard]] IngestResult Ingest(const nlohmann::json& doc,
                                      bool content_error = false) {
        const auto id = doc.value("id", std::string{});
        if (id.empty()) {
            return {false, "missing_id"};
        }
        if (content_error) {
            audit_->Record({"ingest", "content_error", id});
            return {false, "content_error"};
        }
        if (!doc.contains("title")) {
            return {false, "schema_error"};
        }

        storage_->Write(id, doc.dump());

        const auto terms_it = doc.find("terms");
        std::vector<std::string> terms = {};

        if (terms_it != doc.end() && terms_it->is_array()) {
            terms = terms_it->get<std::vector<std::string>>();
        } else {
            terms.push_back(id);
        }
        index_->IndexDocument(id, terms);

        {
            std::lock_guard<std::mutex> lock(cdc_mutex_);
            cdc_events_.push_back("cdc:" + id);
        }
        audit_->Record({"ingest", "ok", id});
        return {true, ""};
    }

    /**
     * @brief Ingest up to `rate_limit_` documents starting from `offset`.
     * @return Index of the next un-ingested document (checkpoint).
     */
    [[nodiscard]] size_t BatchIngest(const std::vector<nlohmann::json>& docs,
                                     size_t offset = 0) {
        const size_t end = std::min(docs.size(), offset + rate_limit_);
        for (size_t i = offset; i < end; ++i) {
            const auto result = Ingest(docs[i]);
            if (!result.ok) {
                break;
            }
            checkpoint_ = i + 1;
        }
        return checkpoint_;
    }

    [[nodiscard]] const std::vector<std::string>& CdcEvents() const {
        return cdc_events_;
    }

    [[nodiscard]] size_t Checkpoint() const { return checkpoint_; }

private:
    std::shared_ptr<InMemoryPipelineStorage> storage_;
    std::shared_ptr<MockPipelineIndex> index_;
    std::shared_ptr<PipelineAuditLog> audit_;
    size_t rate_limit_{1};
    size_t checkpoint_{0};
    mutable std::mutex cdc_mutex_;
    std::vector<std::string> cdc_events_;
};

struct QueryResult {
    bool ok{false};
    bool cache_hit{false};
    std::string payload;
    std::string error;
};

/**
 * @brief Query facade for cross-module tests.
 *
 * Reads from the shared index and storage that IngestionFacade writes to.
 */
class QueryFacade {
public:
    QueryFacade(std::shared_ptr<MockPipelineAuth> auth,
                std::shared_ptr<MockPipelineIndex> index,
                std::shared_ptr<InMemoryPipelineStorage> storage,
                std::shared_ptr<PipelineAuditLog> audit)
        : auth_(std::move(auth)),
          index_(std::move(index)),
          storage_(std::move(storage)),
          audit_(std::move(audit)) {}

    [[nodiscard]] QueryResult Execute(const std::string& token,
                                      const std::string& query_label,
                                      const std::string& term) {
        if (!auth_->Authorize(token).authorized) {
            audit_->Record({"query", "auth_failed", token});
            return {false, false, "", "401"};
        }

        {
            std::lock_guard<std::mutex> lock(cache_mutex_);
            const auto it = cache_.find(query_label);
            if (it != cache_.end()) {
                audit_->Record({"cache", "hit", query_label});
                return {true, true, it->second, ""};
            }
        }

        const auto ids = index_->Search(term);
        if (ids.empty()) {
            return {false, false, "", "404:no_index_match"};
        }

        const auto payload = storage_->Read(ids.front());
        if (!payload.has_value()) {
            return {false, false, "", "404:storage_miss"};
        }

        audit_->Record({"query", "hit", query_label});
        ++success_count_;

        {
            std::lock_guard<std::mutex> lock(cache_mutex_);
            cache_[query_label] = *payload;
        }
        return {true, false, *payload, ""};
    }

    [[nodiscard]] size_t SuccessCount() const { return success_count_.load(); }

private:
    std::shared_ptr<MockPipelineAuth> auth_;
    std::shared_ptr<MockPipelineIndex> index_;
    std::shared_ptr<InMemoryPipelineStorage> storage_;
    std::shared_ptr<PipelineAuditLog> audit_;
    mutable std::mutex cache_mutex_;
    std::unordered_map<std::string, std::string> cache_;
    std::atomic_size_t success_count_{0};
};

/**
 * @brief Minimal RAG facade wiring LLM + index together.
 *
 * After an IngestionFacade run, the index contains terms that this facade
 * retrieves as context for the mock LLM answer generation.
 */
class RagFacade {
public:
    RagFacade(std::shared_ptr<MockPipelineLlmBackend> llm,
              std::shared_ptr<MockPipelineIndex> index,
              std::shared_ptr<PipelineAuditLog> audit)
        : llm_(std::move(llm)),
          index_(std::move(index)),
          audit_(std::move(audit)) {}

    struct RagResult {
        bool ok{false};
        std::string answer = {};
        bool fallback_used{false};
    };

    [[nodiscard]] RagResult Ask(const std::string& question,
                                const std::string& term) {
        const auto embedding = llm_->GenerateEmbedding(question);
        if (!embedding.has_value()) {
            return {true, "fallback:no-embedding", true};
        }

        const auto hits = index_->Search(term);
        const std::string context = hits.empty() ? "empty" : hits.front();

        const auto answer = llm_->Infer(question, context);
        if (!answer.has_value()) {
            return {true, "fallback:no-inference", true};
        }

        audit_->Record({"rag", "answer", question});
        return {true, *answer, false};
    }

private:
    std::shared_ptr<MockPipelineLlmBackend> llm_;
    std::shared_ptr<MockPipelineIndex> index_;
    std::shared_ptr<PipelineAuditLog> audit_;
};

} // namespace

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------

/**
 * @brief Fixture for the Ingest → Index → Query cross-module suite.
 *
 * All pipeline components share `storage_` and `index_`, which is the key
 * invariant that makes this a cross-module test: data written by the
 * ingestion stage must be visible to the query stage without any intermediate
 * reset or copy.
 */
class CrossModuleIngestIndexQueryTest : public DeterministicIntegrationFixture {
protected:
    void SetUp() override {
        DeterministicIntegrationFixture::SetUp();

        const auto token = std::string{"valid_cross_module_token"};
        auth->AllowToken(token);
        valid_token_ = token;

        ingest_ = std::make_unique<IngestionFacade>(storage, index, audit, /*rate_limit=*/5);
        query_  = std::make_unique<QueryFacade>(auth, index, storage, audit);
        rag_    = std::make_unique<RagFacade>(llm, index, audit);
    }

    std::string valid_token_;
    std::unique_ptr<IngestionFacade> ingest_;
    std::unique_ptr<QueryFacade>     query_;
    std::unique_ptr<RagFacade>       rag_;
};

// ---------------------------------------------------------------------------
// CMX-01: Happy path — single document ingested and retrieved end-to-end
// ---------------------------------------------------------------------------

/**
 * @test CMX-01: Ingest one document, then query it by its indexed term.
 *
 * Verifies that storage contains the raw payload, the index maps the term
 * to the document id, and the query pipeline returns the correct payload.
 */
TEST_F(CrossModuleIngestIndexQueryTest, CMX01_SingleDocumentIngestAndRetrieve) {
    auto doc = data_gen->GenerateTestDocument("cross");
    doc["id"]    = "cross_001";
    doc["terms"] = nlohmann::json::array({"alpha"});

    // --- Ingestion stage ---
    const auto ingest_result = ingest_->Ingest(doc);
    ASSERT_TRUE(ingest_result.ok) << "Ingest failed: " << ingest_result.error;

    // --- Storage boundary assertion ---
    EXPECT_TRUE(storage->Contains("cross_001"))
        << "Ingested document must be present in storage";

    // --- Index boundary assertion ---
    EXPECT_FALSE(index->Search("alpha").empty())
        << "Term 'alpha' must be resolvable after ingestion";

    // --- Query stage ---
    const auto q_result = query_->Execute(valid_token_, "q_alpha", "alpha");
    ASSERT_TRUE(q_result.ok) << "Query failed: " << q_result.error;
    EXPECT_FALSE(q_result.payload.empty());
    EXPECT_FALSE(q_result.cache_hit)
        << "First query must not be a cache hit";

    // --- Audit trail ---
    EXPECT_TRUE(audit->Contains("ingest", "ok"));
    EXPECT_TRUE(audit->Contains("query", "hit"));
}

// ---------------------------------------------------------------------------
// CMX-02: Batch ingest with rate-limit → bulk query
// ---------------------------------------------------------------------------

/**
 * @test CMX-02: Batch ingest with rate-limit checkpoint, then query all docs.
 *
 * Ensures that the checkpoint mechanism correctly resumes and that every
 * ingested document is independently queryable.
 */
TEST_F(CrossModuleIngestIndexQueryTest, CMX02_BatchIngestRateLimitThenBulkQuery) {
    constexpr size_t kDocCount = 9;

    // Generate a batch with deterministic ids and distinct terms per document.
    std::vector<nlohmann::json> docs;
    docs.reserve(kDocCount);
    for (size_t i = 0; i < kDocCount; ++i) {
        auto doc = data_gen->GenerateTestDocument("batch");
        doc["id"]    = "batch_" + std::to_string(i);
        doc["terms"] = nlohmann::json::array({"batch_term_" + std::to_string(i)});
        docs.push_back(std::move(doc));
    }

    // --- Ingestion stage (rate-limit = 5) ---
    const auto cp1 = ingest_->BatchIngest(docs, 0);
    EXPECT_EQ(cp1, 5U) << "First batch should stop at rate-limit=5";

    const auto cp2 = ingest_->BatchIngest(docs, cp1);
    EXPECT_EQ(cp2, kDocCount) << "Resume should complete the remaining 4 docs";

    EXPECT_EQ(ingest_->CdcEvents().size(), kDocCount);

    // --- Query stage: every doc must be independently retrievable ---
    size_t hits = 0;
    for (size_t i = 0; i < kDocCount; ++i) {
        const auto term  = "batch_term_" + std::to_string(i);
        const auto label = "q_" + term;
        const auto result = query_->Execute(valid_token_, label, term);
        EXPECT_TRUE(result.ok)
            << "Query for term='" << term << "' must succeed after batch ingest";
        if (result.ok) {
            ++hits;
        }
    }
    EXPECT_EQ(hits, kDocCount)
        << "All " << kDocCount << " docs must be retrievable after batch ingest";
}

// ---------------------------------------------------------------------------
// CMX-03: RAG ask after ingest uses LLM + indexed context
// ---------------------------------------------------------------------------

/**
 * @test CMX-03: After ingestion, a RAG ask retrieves indexed context for LLM.
 *
 * Verifies that the index populated by IngestionFacade is used as retrieval
 * context by the RAG pipeline and that the LLM produces a non-fallback answer.
 */
TEST_F(CrossModuleIngestIndexQueryTest, CMX03_RagAskAfterIngestUsesIndexedContext) {
    auto doc = data_gen->GenerateTestDocument("rag");
    doc["id"]    = "rag_001";
    doc["terms"] = nlohmann::json::array({"rag_ctx"});

    ASSERT_TRUE(ingest_->Ingest(doc).ok);

    // --- RAG stage ---
    const auto rag_result = rag_->Ask("What is the answer?", "rag_ctx");
    EXPECT_TRUE(rag_result.ok);
    EXPECT_FALSE(rag_result.answer.empty());
    EXPECT_FALSE(rag_result.fallback_used)
        << "LLM mock must not fall back when context is available";

    // Answer format: "answer(<question>)::<context_id>"
    EXPECT_NE(rag_result.answer.find("answer("), std::string::npos)
        << "Answer must follow mock LLM answer format";

    // --- Audit trail ---
    EXPECT_TRUE(audit->Contains("rag", "answer"));
}

// ---------------------------------------------------------------------------
// CMX-04: CDC events are consistent with index and storage state
// ---------------------------------------------------------------------------

/**
 * @test CMX-04: CDC events emitted by ingest correlate 1:1 with storage entries.
 *
 * The cross-module boundary assertion: for every "cdc:<id>" token the ingest
 * stage emits, the document must also be present in both storage and index.
 */
TEST_F(CrossModuleIngestIndexQueryTest, CMX04_CdcEventsConsistentWithStorageAndIndex) {
    constexpr size_t kCount = 4;
    for (size_t i = 0; i < kCount; ++i) {
        auto doc = data_gen->GenerateTestDocument("cdc");
        doc["id"]    = "cdc_" + std::to_string(i);
        doc["terms"] = nlohmann::json::array({"cdc_term_" + std::to_string(i)});
        ASSERT_TRUE(ingest_->Ingest(doc).ok);
    }

    const auto& events = ingest_->CdcEvents();
    ASSERT_EQ(events.size(), kCount);

    for (size_t i = 0; i < kCount; ++i) {
        const std::string id = "cdc_" + std::to_string(i);
        // CDC event format is "cdc:<id>"
        EXPECT_EQ(events[i], "cdc:" + id)
            << "CDC event[" << i << "] must match doc id";
        EXPECT_TRUE(storage->Contains(id))
            << "Storage must contain doc '" << id << "'";
        EXPECT_FALSE(index->Search("cdc_term_" + std::to_string(i)).empty())
            << "Index must map term for '" << id << "'";
    }
}

// ---------------------------------------------------------------------------
// CMX-05: Content error blocks query (no partial artifact)
// ---------------------------------------------------------------------------

/**
 * @test CMX-05: A document with a content error must not be queryable.
 *
 * When ingest reports a content_error, storage and index must remain clean
 * and a subsequent query for that document's term must return 404.
 */
TEST_F(CrossModuleIngestIndexQueryTest, CMX05_ContentErrorDoesNotLeavePartialArtifact) {
    auto doc = data_gen->GenerateTestDocument("bad");
    doc["id"]    = "bad_001";
    doc["terms"] = nlohmann::json::array({"bad_term"});

    // Ingest with simulated content error.
    const auto ingest_result = ingest_->Ingest(doc, /*content_error=*/true);
    EXPECT_FALSE(ingest_result.ok);
    EXPECT_EQ(ingest_result.error, "content_error");

    // Storage boundary: document must NOT be present.
    EXPECT_FALSE(storage->Contains("bad_001"))
        << "Storage must not contain partially-ingested document";

    // Index boundary: term must NOT map to anything.
    EXPECT_TRUE(index->Search("bad_term").empty())
        << "Index must not have term for failed ingest";

    // Query stage: must return 404.
    const auto q_result = query_->Execute(valid_token_, "q_bad", "bad_term");
    EXPECT_FALSE(q_result.ok);
    EXPECT_NE(q_result.error.find("404"), std::string::npos)
        << "Query must return 404 for a term with no ingest";

    // CDC: must not have emitted any event.
    EXPECT_TRUE(ingest_->CdcEvents().empty())
        << "Content error must not emit CDC event";
}

// ---------------------------------------------------------------------------
// CMX-06: Parallel ingest + concurrent queries maintain count consistency
// ---------------------------------------------------------------------------

/**
 * @test CMX-06: Parallel ingest threads + concurrent query threads do not
 *               produce race conditions or inconsistent counts.
 *
 * Uses a deterministic document set (SeededTestDataGenerator) so the
 * expected counts are fixed regardless of execution order.
 */
TEST_F(CrossModuleIngestIndexQueryTest, CMX06_ParallelIngestAndQueryKeepCounts) {
    constexpr size_t kIngestThreads = 4;
    constexpr size_t kDocsPerThread = 5;
    constexpr size_t kTotalDocs     = kIngestThreads * kDocsPerThread;

    // Pre-generate all documents with globally-unique ids and terms.
    std::vector<nlohmann::json> all_docs;
    all_docs.reserve(kTotalDocs);
    for (size_t i = 0; i < kTotalDocs; ++i) {
        auto doc = data_gen->GenerateTestDocument("par");
        doc["id"]    = "par_" + std::to_string(i);
        doc["terms"] = nlohmann::json::array({"par_term_" + std::to_string(i)});
        all_docs.push_back(std::move(doc));
    }

    // Parallel ingest.
    std::vector<std::thread> ingest_workers;
    ingest_workers.reserve(kIngestThreads);
    for (size_t t = 0; t < kIngestThreads; ++t) {
        ingest_workers.emplace_back([&, t]() {
            for (size_t i = t * kDocsPerThread; i < (t + 1) * kDocsPerThread; ++i) {
                EXPECT_TRUE(ingest_->Ingest(all_docs[i]).ok);
            }
        });
    }
    for (auto& w : ingest_workers) {
        w.join();
    }

    // All docs must be in storage.
    EXPECT_EQ(storage->Size(), kTotalDocs);

    // Parallel queries — each query uses a per-doc unique term.
    std::atomic_size_t hit_count{0};
    std::vector<std::thread> query_workers;
    query_workers.reserve(kIngestThreads);
    for (size_t t = 0; t < kIngestThreads; ++t) {
        query_workers.emplace_back([&, t]() {
            for (size_t i = t * kDocsPerThread; i < (t + 1) * kDocsPerThread; ++i) {
                const auto term  = "par_term_" + std::to_string(i);
                const auto label = "qpar_" + std::to_string(i);
                const auto r = query_->Execute(valid_token_, label, term);
                if (r.ok) {
                    ++hit_count;
                }
            }
        });
    }
    for (auto& w : query_workers) {
        w.join();
    }

    EXPECT_EQ(hit_count.load(), kTotalDocs)
        << "Every parallel-ingested doc must be retrievable via query";
    EXPECT_EQ(query_->SuccessCount(), kTotalDocs);
}
} } // namespace themis::test
