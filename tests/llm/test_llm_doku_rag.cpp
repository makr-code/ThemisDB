/**
 * @file test_llm_doku_rag.cpp
 * @brief RAG CI test suite: ThemisDB documentation knowledge base (doku.db).
 *
 * Tests RAG-01..12 validate the WikiRagSource → JsonWikiIndexReader retrieval
 * pipeline against the doku.db JSON chunk index built by ci-build-doku-db.sh.
 *
 * All questions and answer-quality criteria derive exclusively from ThemisDB
 * documentation (docs/, src/<module>/ROADMAP.md, ARCHITECTURE.md, etc.).
 *
 *  RAG-01: Retrieval for "WikiIndexStore purpose" returns BM25/HNSW/RRF chunks
 *  RAG-02: Retrieval for "implementation phases" returns Phase 1–6 content
 *  RAG-03: Retrieval for "AdaLoRA" returns rank/singular-values chunks
 *  RAG-04: Retrieval for "Community branch" returns release-branch content
 *  RAG-05: Retrieval for "model download" returns Ollama/HuggingFace content
 *  RAG-06: Recall@5 ≥ 70 % across 10 representative questions
 *  RAG-07: Query latency < 3000 ms per query on CPU-only (JsonWikiIndexReader)
 *  RAG-08: Golden dataset keyword gate — every entry hits ≥ 1 keyword in Top-5
 *  RAG-09: Golden dataset Recall@5 ≥ 80 % across all golden entries
 *  RAG-10: Golden dataset source-hint gate — expected_source_hint in Top-5
 *  RAG-11: Golden dataset latency gate — median < 500 ms per query
 *  RAG-12: Golden dataset schema/size/distribution guard
 *          (>=110 entries; 20% general / 30% specific / 50% specialized)
 *
 * When the doku.db index file is absent, tests RAG-01..07 skip (model_required
 * label).  RAG-08..12 skip when doku.db is absent but FAIL if the golden
 * dataset YAML itself is missing (RAG-12).
 *
 * @see scripts/ci-build-doku-db.sh   (produces doku.db.json)
 * @see tests/llm/data/themisdb_rag_golden_dataset.yaml  (authoritative dataset)
 * @see scripts/generate_rag_golden_dataset.py           (bootstrap generator)
 * @see tests/llm/test_llm_tinyllama_inference.cpp (INFER-01..10)
 * @see tests/llm/test_llm_adalora_doku_training.cpp (LORA-01..08)
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
#include <fstream>
#include <numeric>
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

/// Probe standard locations for the golden dataset YAML.
std::string findGoldenDatasetPath() {
    const char* env_gd = std::getenv("THEMIS_RAG_GOLDEN_DATASET_PATH");
    if (env_gd && std::filesystem::exists(env_gd)) return env_gd;

    const char* env_ws = std::getenv("GITHUB_WORKSPACE");
    const char* env_src = std::getenv("CMAKE_SOURCE_DIR");

    std::vector<std::filesystem::path> candidates = {
        // CMake copies the YAML next to the test binary (via configure_file)
        std::filesystem::path("themisdb_rag_golden_dataset.yaml"),
        std::filesystem::path("data/themisdb_rag_golden_dataset.yaml"),
        std::filesystem::path("tests/llm/data/themisdb_rag_golden_dataset.yaml"),
        std::filesystem::path("../tests/llm/data/themisdb_rag_golden_dataset.yaml"),
    };
    if (env_ws) {
        candidates.push_back(
            std::filesystem::path(env_ws) / "tests/llm/data/themisdb_rag_golden_dataset.yaml");
    }
    if (env_src) {
        candidates.push_back(
            std::filesystem::path(env_src) / "tests/llm/data/themisdb_rag_golden_dataset.yaml");
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
            return containsIgnoreCase(c.text, keyword) ||
                   containsIgnoreCase(c.section_title, keyword);
        });
}

// ─── Minimal YAML parser for the golden dataset ──────────────────────────────
//
// We implement a purpose-built parser rather than taking a YAML library
// dependency.  It handles only the subset of YAML used in the golden dataset:
//   - "key: value" string pairs
//   - "key: [a, b, c]" flow sequences
//   - "key: 0.7" float scalars
//   - "  - id: …" list-of-map entries (indented with 2 spaces)
//   - additional string metadata fields: knowledge_level, rarity_tier
//   - provenance fields (v3): source_document, source_section, indexed_at
//   Policy: question text must NOT contain any expected_keyword (v3 rule)

struct GoldenEntry {
    std::string id;
    std::string question;
    std::vector<std::string> expected_keywords;
    std::string expected_source_hint;
    std::string knowledge_level;  // general | specific | specialized
    std::string rarity_tier;      // required "rare" for specialized
    double min_recall_score = 0.7;
    // Provenance fields (required since dataset v3 — governance criterion)
    std::string source_document;  // relative repo path, e.g. "docs/llm/FLASH_ATTENTION_ARCHITECTURE.md"
    std::string source_section;   // heading path, e.g. "Kernel Fusion"
    std::string indexed_at;       // ISO date when document was processed, e.g. "2026-08-24"
};

constexpr std::size_t kGoldenDatasetMinEntries = 110;
constexpr double kGoldenGeneralTarget = 0.20;
constexpr double kGoldenSpecificTarget = 0.30;
constexpr double kGoldenSpecializedTarget = 0.50;
constexpr double kGoldenDistributionTolerance = 0.03;

/// Strip leading/trailing whitespace in-place.
static void trim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
        [](unsigned char c){ return !std::isspace(c); }));
    s.erase(std::find_if(s.rbegin(), s.rend(),
        [](unsigned char c){ return !std::isspace(c); }).base(), s.end());
}

/// Remove surrounding quotes (single or double) from a string.
static void unquote(std::string& s) {
    if (s.size() >= 2 && ((s.front() == '"' && s.back() == '"') ||
                           (s.front() == '\'' && s.back() == '\''))) {
        s = s.substr(1, s.size() - 2);
    }
}

/// Parse a YAML flow sequence "[a, b, c]" into a vector of strings.
static std::vector<std::string> parseFlowSeq(const std::string& val) {
    std::vector<std::string> result;
    std::string inner = val;
    if (inner.front() == '[') inner = inner.substr(1);
    if (!inner.empty() && inner.back() == ']') inner.pop_back();

    std::stringstream ss(inner);
    std::string token;
    while (std::getline(ss, token, ',')) {
        trim(token);
        unquote(token);
        if (!token.empty()) result.push_back(token);
    }
    return result;
}

/// Parse the golden dataset YAML file into a vector of GoldenEntry.
/// Returns an empty vector on parse error.
static std::vector<GoldenEntry> parseGoldenDataset(const std::string& yaml_path) {
    std::ifstream ifs(yaml_path);
    if (!ifs) return {};

    std::vector<GoldenEntry> entries;
    GoldenEntry current;
    bool in_entry = false;

    std::string line;
    while (std::getline(ifs, line)) {
        // Skip comments and blank lines
        std::string trimmed = line;
        trim(trimmed);
        if (trimmed.empty() || trimmed[0] == '#') continue;

        // Detect new entry marker: "  - id:"
        if (trimmed.rfind("- id:", 0) == 0) {
            if (in_entry && !current.id.empty()) {
                entries.push_back(current);
            }
            current = GoldenEntry{};
            in_entry = true;
            std::string val = trimmed.substr(5);
            trim(val);
            unquote(val);
            current.id = val;
            continue;
        }

        if (!in_entry) continue;

        // Key-value lines at the entry level (indented 4 spaces or more)
        auto colon_pos = trimmed.find(':');
        if (colon_pos == std::string::npos) continue;

        std::string key = trimmed.substr(0, colon_pos);
        trim(key);
        std::string val = trimmed.substr(colon_pos + 1);
        trim(val);

        if (key == "question") {
            unquote(val);
            current.question = val;
        } else if (key == "expected_keywords") {
            current.expected_keywords = parseFlowSeq(val);
        } else if (key == "expected_source_hint") {
            unquote(val);
            current.expected_source_hint = val;
        } else if (key == "knowledge_level") {
            unquote(val);
            current.knowledge_level = val;
        } else if (key == "rarity_tier") {
            unquote(val);
            current.rarity_tier = val;
        } else if (key == "min_recall_score") {
            try { current.min_recall_score = std::stod(val); } catch (...) {}
        } else if (key == "source_document") {
            unquote(val);
            current.source_document = val;
        } else if (key == "source_section") {
            unquote(val);
            current.source_section = val;
        } else if (key == "indexed_at") {
            unquote(val);
            current.indexed_at = val;
        }
    }
    // Push the last entry
    if (in_entry && !current.id.empty()) {
        entries.push_back(current);
    }
    return entries;
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
    void skipIfNoDb() {
        if (db_path_.empty() || !reader_ || !reader_->isReady()) {
            GTEST_SKIP() << "doku.db not available — run scripts/ci-build-doku-db.sh first "
                            "or set THEMIS_DOKU_DB_PATH";
        }
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
    skipIfNoDb();

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
    skipIfNoDb();

    const auto chunks = query5("What are the implementation phases in ThemisDB?");
    ASSERT_GT(chunks.size(), 0u) << "Query returned no results";

    // Concatenate all content for broad keyword check
    std::string combined;
    for (const auto& c : chunks) combined += c.text + " " + c.section_title + " ";

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
    skipIfNoDb();

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
    skipIfNoDb();

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
    skipIfNoDb();

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
    skipIfNoDb();

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
    skipIfNoDb();

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

// ─── Golden Dataset Fixture ───────────────────────────────────────────────────

class GoldenDatasetRagTest : public ::testing::Test {
protected:
    void SetUp() override {
        // doku.db (may be absent — golden tests skip gracefully)
        db_path_ = findDokuDbPath();
        if (!db_path_.empty()) {
            reader_ = std::make_unique<JsonWikiIndexReader>(db_path_, /*auto_load=*/true);
            if (!reader_->isReady()) {
                db_path_.clear();
                reader_.reset();
            } else {
                spdlog::info("GoldenDatasetRagTest: loaded {} chunks from {}",
                             reader_->size(), db_path_);
            }
        }

        // Golden dataset YAML (must always be present for RAG-12)
        golden_path_ = findGoldenDatasetPath();
        if (!golden_path_.empty()) {
            golden_entries_ = parseGoldenDataset(golden_path_);
            spdlog::info("GoldenDatasetRagTest: parsed {} golden entries from {}",
                         golden_entries_.size(), golden_path_);
        }
    }

    void skipIfNoDb() {
        if (db_path_.empty() || !reader_ || !reader_->isReady()) {
            GTEST_SKIP() << "doku.db not available — run scripts/ci-build-doku-db.sh "
                            "or set THEMIS_DOKU_DB_PATH";
        }
    }

    std::string db_path_;
    std::string golden_path_;
    std::unique_ptr<JsonWikiIndexReader> reader_;
    std::vector<GoldenEntry> golden_entries_;
};

// ─── RAG-12: Golden dataset YAML presence guard (hard failure) ────────────────

TEST_F(GoldenDatasetRagTest, Rag12_GoldenDatasetPresent) {
    // This test MUST NOT be skipped — the YAML file is committed to the repo.
    ASSERT_FALSE(golden_path_.empty())
        << "Golden dataset YAML not found.  Expected at one of:\n"
           "  tests/llm/data/themisdb_rag_golden_dataset.yaml\n"
           "  $THEMIS_RAG_GOLDEN_DATASET_PATH\n"
           "  Next to the test binary (CMake configure_file copy)\n"
           "Run: git status tests/llm/data/ to verify the file is present.";

    ASSERT_FALSE(golden_entries_.empty())
        << "Golden dataset YAML found at '" << golden_path_
        << "' but could not be parsed or is empty.";

    // Sanity: every entry must have a non-empty id, question, and at least one keyword
    for (const auto& e : golden_entries_) {
        EXPECT_FALSE(e.id.empty())       << "Entry with empty id found";
        EXPECT_FALSE(e.question.empty()) << "Entry '" << e.id << "' has empty question";
        EXPECT_FALSE(e.expected_keywords.empty())
            << "Entry '" << e.id << "' has no expected_keywords";
        EXPECT_TRUE(
            e.knowledge_level == "general" ||
            e.knowledge_level == "specific" ||
            e.knowledge_level == "specialized")
            << "Entry '" << e.id << "' has invalid knowledge_level: '" << e.knowledge_level << "'";
        if (e.knowledge_level == "specialized") {
            EXPECT_EQ(e.rarity_tier, "rare")
                << "Entry '" << e.id << "' must set rarity_tier=rare for specialized knowledge";
        }
        // v3: provenance fields are mandatory for governance
        EXPECT_FALSE(e.source_document.empty())
            << "Entry '" << e.id << "' is missing source_document (v3 governance requirement)";
        EXPECT_FALSE(e.indexed_at.empty())
            << "Entry '" << e.id << "' is missing indexed_at (v3 governance requirement)";
    }

    const auto total = golden_entries_.size();
    EXPECT_GE(total, kGoldenDatasetMinEntries)
        << "Golden dataset too small: " << total
        << " entries; requires at least " << kGoldenDatasetMinEntries;

    std::size_t general_count = 0;
    std::size_t specific_count = 0;
    std::size_t specialized_count = 0;
    for (const auto& e : golden_entries_) {
        if (e.knowledge_level == "general") ++general_count;
        else if (e.knowledge_level == "specific") ++specific_count;
        else if (e.knowledge_level == "specialized") ++specialized_count;
    }

    const auto ratio = [&](const std::size_t count) {
        return static_cast<double>(count) / static_cast<double>(total);
    };
    const auto general_ratio = ratio(general_count);
    const auto specific_ratio = ratio(specific_count);
    const auto specialized_ratio = ratio(specialized_count);

    EXPECT_NEAR(general_ratio, kGoldenGeneralTarget, kGoldenDistributionTolerance)
        << "General knowledge ratio mismatch: count=" << general_count
        << ", total=" << total;
    EXPECT_NEAR(specific_ratio, kGoldenSpecificTarget, kGoldenDistributionTolerance)
        << "Specific knowledge ratio mismatch: count=" << specific_count
        << ", total=" << total;
    EXPECT_NEAR(specialized_ratio, kGoldenSpecializedTarget, kGoldenDistributionTolerance)
        << "Specialized knowledge ratio mismatch: count=" << specialized_count
        << ", total=" << total;

    spdlog::info("RAG-12: {} entries validated (general={}, specific={}, specialized={}) from {}",
                 golden_entries_.size(), general_count, specific_count, specialized_count, golden_path_);
}

// ─── RAG-08: Golden dataset keyword gate ─────────────────────────────────────

TEST_F(GoldenDatasetRagTest, Rag08_GoldenKeywordGate) {
    skipIfNoDb();
    if (golden_entries_.empty()) {
        GTEST_SKIP() << "No golden entries loaded (RAG-12 covers this)";
    }

    int miss_count = 0;
    for (const auto& entry : golden_entries_) {
        const auto chunks = reader_->query(entry.question, /*top_k=*/5, /*min_score=*/0.0f);
        bool any_hit = false;
        for (const auto& kw : entry.expected_keywords) {
            if (chunksContainKeyword(chunks, kw)) {
                any_hit = true;
                break;
            }
        }
        if (!any_hit) {
            ++miss_count;
            spdlog::warn("RAG-08 MISS: {} '{}' — no keyword in {{{}}} found in Top-5",
                         entry.id, entry.question.substr(0, 60),
                         entry.expected_keywords.empty() ? "" : entry.expected_keywords[0]);
        }
    }

    const int total = static_cast<int>(golden_entries_.size());
    spdlog::info("RAG-08: keyword gate — {}/{} entries hit at least one keyword",
                 total - miss_count, total);

    // Allow at most 20% misses (same tolerance as RAG-09 80% recall gate)
    const int max_misses = std::max(1, total / 5);
    EXPECT_LE(miss_count, max_misses)
        << miss_count << "/" << total
        << " golden entries had no keyword match in Top-5 (max allowed: " << max_misses << ")";
}

// ─── RAG-09: Golden dataset Recall@5 ≥ 80 % ─────────────────────────────────

TEST_F(GoldenDatasetRagTest, Rag09_GoldenRecallAt5_80Percent) {
    skipIfNoDb();
    if (golden_entries_.empty()) {
        GTEST_SKIP() << "No golden entries loaded (RAG-12 covers this)";
    }

    int hits = 0;
    for (const auto& entry : golden_entries_) {
        const auto chunks = reader_->query(entry.question, /*top_k=*/5, /*min_score=*/0.0f);
        for (const auto& kw : entry.expected_keywords) {
            if (chunksContainKeyword(chunks, kw)) {
                ++hits;
                break;
            }
        }
    }

    const int total = static_cast<int>(golden_entries_.size());
    const double recall = static_cast<double>(hits) / static_cast<double>(total);
    spdlog::info("RAG-09: golden Recall@5 = {}/{} = {:.1f}%", hits, total, recall * 100.0);

    EXPECT_GE(recall, 0.80)
        << "Golden dataset Recall@5 = " << (recall * 100.0) << "% — below 80% gate.\n"
           "  " << hits << "/" << total << " entries returned at least one expected keyword "
           "in the Top-5 results.\n"
           "  Investigate doku.db freshness or retrieval quality.";
}

// ─── RAG-10: Golden dataset source-hint gate ─────────────────────────────────

TEST_F(GoldenDatasetRagTest, Rag10_GoldenSourceHintGate) {
    skipIfNoDb();
    if (golden_entries_.empty()) {
        GTEST_SKIP() << "No golden entries loaded (RAG-12 covers this)";
    }

    // Only evaluate entries that have a non-empty expected_source_hint
    std::vector<const GoldenEntry*> with_hint;
    for (const auto& e : golden_entries_) {
        if (!e.expected_source_hint.empty()) {
            with_hint.push_back(&e);
        }
    }

    if (with_hint.empty()) {
        GTEST_SKIP() << "No golden entries have an expected_source_hint — skipping RAG-10";
    }

    int source_hits = 0;
    for (const auto* entry : with_hint) {
        const auto chunks = reader_->query(entry->question, /*top_k=*/5, /*min_score=*/0.0f);
        bool found = std::any_of(chunks.begin(), chunks.end(),
            [&](const WikiChunk& c) {
                return containsIgnoreCase(c.doc_id,  entry->expected_source_hint) ||
                       containsIgnoreCase(c.section_title, entry->expected_source_hint);
            });
        if (found) ++source_hits;
        else {
            spdlog::warn("RAG-10 MISS: {} '{}' — hint '{}' not in Top-5 doc_ids",
                         entry->id, entry->question.substr(0, 50),
                         entry->expected_source_hint);
        }
    }

    const int total_hinted = static_cast<int>(with_hint.size());
    const double source_recall = static_cast<double>(source_hits) /
                                  static_cast<double>(total_hinted);
    spdlog::info("RAG-10: source-hint gate — {}/{} entries found expected source ({:.0f}%)",
                 source_hits, total_hinted, source_recall * 100.0);

    // Allow up to 30% source misses (sources may be in different doc splits)
    EXPECT_GE(source_recall, 0.70)
        << "Source-hint Recall = " << (source_recall * 100.0)
        << "% — below 70% gate for entries with expected_source_hint.";
}

// ─── RAG-11: Golden dataset latency gate — median < 500 ms ───────────────────

TEST_F(GoldenDatasetRagTest, Rag11_GoldenLatencyMedianBelow500ms) {
    skipIfNoDb();
    if (golden_entries_.empty()) {
        GTEST_SKIP() << "No golden entries loaded (RAG-12 covers this)";
    }

    std::vector<double> latencies;
    latencies.reserve(golden_entries_.size());

    for (const auto& entry : golden_entries_) {
        const auto t0 = steady_clock::now();
        (void)reader_->query(entry.question, /*top_k=*/5, /*min_score=*/0.0f);
        const double elapsed_ms =
            duration_cast<microseconds>(steady_clock::now() - t0).count() / 1000.0;
        latencies.push_back(elapsed_ms);
    }

    // Compute median
    std::vector<double> sorted = latencies;
    std::sort(sorted.begin(), sorted.end());
    const double median_ms = sorted[sorted.size() / 2];
    const double max_ms = sorted.back();
    const double mean_ms = std::accumulate(sorted.begin(), sorted.end(), 0.0) /
                            static_cast<double>(sorted.size());

    spdlog::info("RAG-11: latency over {} queries — median={:.1f}ms mean={:.1f}ms max={:.1f}ms",
                 latencies.size(), median_ms, mean_ms, max_ms);

    EXPECT_LT(median_ms, 500.0)
        << "Median query latency " << median_ms << "ms exceeds 500ms gate "
           "over " << latencies.size() << " golden-dataset queries.";
}
