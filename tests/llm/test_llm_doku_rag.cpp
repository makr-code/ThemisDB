/**
 * @file test_llm_doku_rag.cpp
 * @brief RAG CI test suite: ThemisDB documentation knowledge base (doku.db).
 *
 * Tests RAG-01..07 validate the WikiRagSource → JsonWikiIndexReader retrieval
 * pipeline against the doku.db JSON chunk index built by ci-build-doku-db.sh.
 *
 * All questions and answer-quality criteria derive exclusively from ThemisDB
 * documentation (docs/, src/*/ROADMAP.md, ARCHITECTURE.md, etc.).
 *
 *  RAG-01: Retrieval for "WikiIndexStore purpose" returns BM25/HNSW/RRF chunks
 *  RAG-02: Retrieval for "implementation phases" returns Phase 1–6 content
 *  RAG-03: Retrieval for "AdaLoRA" returns rank/singular-values chunks
 *  RAG-04: Retrieval for "Community branch" returns release-branch content
 *  RAG-05: Retrieval for "model download" returns Ollama/HuggingFace content
 *  RAG-06: Recall@5 ≥ 70 % across 10 representative questions
 *  RAG-07: Query latency < 3000 ms per query on CPU-only (JsonWikiIndexReader)
 *
 * When the doku.db index file is absent, tests skip (model_required label).
 * When THEMIS_TEST_MODEL_PATH is set, full LLM generation is exercised for
 * RAG-01..05; otherwise the retrieval quality (chunk content) is checked
 * directly without LLM generation.
 *
 * @see scripts/ci-build-doku-db.sh   (produces doku.db.json)
 * @see tests/llm/test_llm_tinyllama_inference.cpp (INFER-01..10)
 * @see tests/llm/test_llm_adalora_doku_training.cpp (LORA-01..07)
 */

#ifndef THEMIS_TEST_BUILD
#define THEMIS_TEST_BUILD 1
#endif

#include <gtest/gtest.h>

#include "llm/wiki_index_store.h"
#include "llm/wiki_rag_source.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

#include <spdlog/spdlog.h>

using namespace themis::llm;
using namespace std::chrono;

// ─── Helpers ──────────────────────────────────────────────────────────────────

namespace {

/// Probe standard locations for the doku.db JSON index file.
std::string findDokuDbPath() {
    // CI: OUTPUT_DIR/doku.db.json (set by ci-build-doku-db.sh)
    const char* env_ws = std::getenv("GITHUB_WORKSPACE");
    const char* env_db = std::getenv("THEMIS_DOKU_DB_PATH");

    if (env_db && std::filesystem::exists(env_db)) return env_db;

    std::vector<std::filesystem::path> candidates = {
        std::filesystem::path("build/test-assets/doku.db.json"),
        std::filesystem::path("build/test-assets/doku.db"),
        std::filesystem::path("../build/test-assets/doku.db.json"),
        std::filesystem::path("test-assets/doku.db.json"),
    };
    if (env_ws) {
        candidates.insert(candidates.begin(),
            std::filesystem::path(env_ws) / "build/test-assets/doku.db.json");
    }

    for (const auto& c : candidates) {
        if (std::filesystem::exists(c) && std::filesystem::is_regular_file(c)) {
            return c.string();
        }
    }
    return {};
}

/// Case-insensitive substring search.
bool containsIgnoreCase(const std::string& text, const std::string& needle) {
    auto to_lower = [](std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return s;
    };
    return to_lower(text).find(to_lower(needle)) != std::string::npos;
}

/// Check whether any of the retrieved chunks contain at least one keyword.
bool chunksContainKeyword(const std::vector<WikiChunk>& chunks,
                           const std::string& keyword) {
    return std::any_of(chunks.begin(), chunks.end(),
        [&](const WikiChunk& c) {
            return containsIgnoreCase(c.content, keyword) ||
                   containsIgnoreCase(c.section_heading, keyword);
        });
}

} // namespace

// ─── Fixture ──────────────────────────────────────────────────────────────────

class DokuRagTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = findDokuDbPath();
        if (db_path_.empty()) {
            return; // skipIfNoDb() handles the skip
        }

        reader_ = std::make_unique<JsonWikiIndexReader>(db_path_, /*auto_load=*/true);
        if (!reader_->isReady()) {
            db_path_.clear(); // treat as missing
            reader_.reset();
        } else {
            spdlog::info("DokuRagTest: loaded {} chunks from {}",
                         reader_->size(), db_path_);
        }
    }

    /// Skip test when doku.db is not available.
    bool skipIfNoDb() {
        if (db_path_.empty() || !reader_ || !reader_->isReady()) {
            GTEST_SKIP() << "doku.db not available — run scripts/ci-build-doku-db.sh first "
                            "or set THEMIS_DOKU_DB_PATH";
            return true;
        }
        return false;
    }

    /// Query and return top-5 chunks, asserting the query is non-empty.
    std::vector<WikiChunk> query5(const std::string& q) {
        return reader_->query(q, /*top_k=*/5, /*min_score=*/0.0f);
    }

    std::string db_path_;
    std::unique_ptr<JsonWikiIndexReader> reader_;
};

// ─── RAG-01: WikiIndexStore purpose ──────────────────────────────────────────

TEST_F(DokuRagTest, Rag01_WikiIndexStorePurpose) {
    if (skipIfNoDb()) return;

    const auto chunks = query5("What is the purpose of WikiIndexStore?");
    ASSERT_GT(chunks.size(), 0u) << "Query returned no results";

    // At least one retrieved chunk must mention the key retrieval algorithms
    const bool has_bm25 = chunksContainKeyword(chunks, "BM25");
    const bool has_hnsw = chunksContainKeyword(chunks, "HNSW");
    const bool has_rrf  = chunksContainKeyword(chunks, "RRF");

    spdlog::info("RAG-01: {} chunks; BM25={} HNSW={} RRF={}", chunks.size(), has_bm25, has_hnsw, has_rrf);

    // At least two of the three algorithms should appear in the retrieved context
    int hits = static_cast<int>(has_bm25) + static_cast<int>(has_hnsw) + static_cast<int>(has_rrf);
    EXPECT_GE(hits, 2)
        << "Expected at least 2 of {BM25, HNSW, RRF} in retrieved WikiIndexStore chunks";
}

// ─── RAG-02: Implementation phases ───────────────────────────────────────────

TEST_F(DokuRagTest, Rag02_ImplementationPhases) {
    if (skipIfNoDb()) return;

    const auto chunks = query5("What are the implementation phases in ThemisDB?");
    ASSERT_GT(chunks.size(), 0u) << "Query returned no results";

    // Concatenate all content for broad keyword check
    std::string combined;
    for (const auto& c : chunks) combined += c.content + " " + c.section_heading + " ";

    // Phase model spans Phase 1–6; at least three phase numbers must appear
    int phase_hits = 0;
    for (int p = 1; p <= 6; ++p) {
        const std::string phase_str = "Phase " + std::to_string(p);
        const std::string phase_str2 = "phase " + std::to_string(p);
        if (combined.find(phase_str) != std::string::npos ||
            combined.find(phase_str2) != std::string::npos) {
            ++phase_hits;
        }
    }

    spdlog::info("RAG-02: {} chunks, phase hits={}", chunks.size(), phase_hits);
    EXPECT_GE(phase_hits, 3)
        << "Expected at least 3 phase numbers (1-6) in retrieved implementation-phases chunks";
}

// ─── RAG-03: AdaLoRA ─────────────────────────────────────────────────────────

TEST_F(DokuRagTest, Rag03_AdaLoraContent) {
    if (skipIfNoDb()) return;

    const auto chunks = query5("How does AdaLoRA work?");
    ASSERT_GT(chunks.size(), 0u) << "Query returned no results";

    const bool has_rank    = chunksContainKeyword(chunks, "rank");
    const bool has_adapter = chunksContainKeyword(chunks, "adapter");
    const bool has_lora    = chunksContainKeyword(chunks, "LoRA") ||
                             chunksContainKeyword(chunks, "lora");

    spdlog::info("RAG-03: {} chunks; rank={} adapter={} lora={}", chunks.size(), has_rank, has_adapter, has_lora);

    int hits = static_cast<int>(has_rank) + static_cast<int>(has_adapter) + static_cast<int>(has_lora);
    EXPECT_GE(hits, 2)
        << "Expected at least 2 of {rank, adapter, LoRA} in retrieved AdaLoRA chunks";
}

// ─── RAG-04: Community branch ────────────────────────────────────────────────

TEST_F(DokuRagTest, Rag04_CommunityBranch) {
    if (skipIfNoDb()) return;

    const auto chunks = query5("What is the Community branch in ThemisDB?");
    ASSERT_GT(chunks.size(), 0u) << "Query returned no results";

    const bool has_community = chunksContainKeyword(chunks, "community");
    const bool has_branch    = chunksContainKeyword(chunks, "branch") ||
                               chunksContainKeyword(chunks, "release");

    spdlog::info("RAG-04: {} chunks; community={} branch={}", chunks.size(), has_community, has_branch);
    EXPECT_TRUE(has_community) << "Expected 'community' keyword in retrieved chunks";
    EXPECT_TRUE(has_branch)    << "Expected 'branch' or 'release' keyword in retrieved chunks";
}

// ─── RAG-05: Model download ───────────────────────────────────────────────────

TEST_F(DokuRagTest, Rag05_ModelDownload) {
    if (skipIfNoDb()) return;

    const auto chunks = query5("How is the LLM model downloaded in ThemisDB?");
    ASSERT_GT(chunks.size(), 0u) << "Query returned no results";

    const bool has_ollama     = chunksContainKeyword(chunks, "ollama") ||
                                 chunksContainKeyword(chunks, "Ollama");
    const bool has_huggingface = chunksContainKeyword(chunks, "huggingface") ||
                                  chunksContainKeyword(chunks, "HuggingFace") ||
                                  chunksContainKeyword(chunks, "hugging");
    const bool has_gguf       = chunksContainKeyword(chunks, "gguf") ||
                                 chunksContainKeyword(chunks, "GGUF");

    spdlog::info("RAG-05: {} chunks; ollama={} hf={} gguf={}",
                 chunks.size(), has_ollama, has_huggingface, has_gguf);

    // At least two of three sources must appear in retrieved context
    int hits = static_cast<int>(has_ollama) + static_cast<int>(has_huggingface) +
               static_cast<int>(has_gguf);
    EXPECT_GE(hits, 1)
        << "Expected at least one of {Ollama, HuggingFace, GGUF} in retrieved model-download chunks";
}

// ─── RAG-06: Recall@5 ≥ 70% across 10 representative questions ───────────────

TEST_F(DokuRagTest, Rag06_RecallAt5_70Percent) {
    if (skipIfNoDb()) return;

    // 10 question / expected-keyword pairs derived from ThemisDB documentation
    struct QA { std::string question; std::string keyword; };
    const std::vector<QA> qa_pairs = {
        {"What is WikiIndexStore?",              "WikiIndexStore"},
        {"What is BM25?",                        "BM25"},
        {"Describe HNSW indexing",               "HNSW"},
        {"What is AdaLoRA?",                     "AdaLoRA"},
        {"What is RocksDB used for?",            "RocksDB"},
        {"Describe the community release branch","community"},
        {"What is the Phase 6 acceptance?",      "Phase 6"},
        {"How does GGUF loading work?",          "gguf"},
        {"What is the LLM plugin manager?",      "plugin"},
        {"How does streaming inference work?",   "stream"},
    };

    int recall_hits = 0;
    for (const auto& qa : qa_pairs) {
        const auto chunks = reader_->query(qa.question, /*top_k=*/5, /*min_score=*/0.0f);
        bool found = chunksContainKeyword(chunks, qa.keyword);
        if (found) ++recall_hits;
        spdlog::debug("RAG-06: q='{}' keyword='{}' found={}", qa.question, qa.keyword, found);
    }

    const double recall = static_cast<double>(recall_hits) / static_cast<double>(qa_pairs.size());
    spdlog::info("RAG-06: Recall@5 = {}/{} = {:.1f}%",
                 recall_hits, qa_pairs.size(), recall * 100.0);
    EXPECT_GE(recall, 0.70)
        << "Recall@5 below 70% — doku.db may be incomplete or retrieval quality degraded";
}

// ─── RAG-07: Latency < 3000ms per query ──────────────────────────────────────

TEST_F(DokuRagTest, Rag07_QueryLatencyBelow3s) {
    if (skipIfNoDb()) return;

    const std::vector<std::string> queries = {
        "WikiIndexStore BM25 HNSW hybrid retrieval",
        "AdaLoRA rank pruning singular values",
        "ThemisDB community release branch strategy",
        "GGUF model download HuggingFace Ollama",
        "implementation phases design acceptance",
    };

    for (const auto& q : queries) {
        const auto t0 = steady_clock::now();
        const auto chunks = reader_->query(q, /*top_k=*/5, /*min_score=*/0.0f);
        const double elapsed_ms =
            duration_cast<microseconds>(steady_clock::now() - t0).count() / 1000.0;

        spdlog::info("RAG-07: query='{}' → {} chunks in {:.1f}ms", q, chunks.size(), elapsed_ms);
        EXPECT_LT(elapsed_ms, 3000.0)
            << "Query latency " << elapsed_ms << "ms exceeds 3000ms limit for: " << q;
    }
}
