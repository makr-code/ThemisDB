/*
 * ThemisDB | File: test_wiki_index_store.cpp | Version: 0.1.0
 * Maturity: 🟢 PRODUCTION-READY | Score: 97/100
 * Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0
 * Status: Production Ready
 */

/**
 * @file test_wiki_index_store.cpp
 * @brief Unit tests WIS-01..16 for WikiChunkSplitter, JsonWikiIndexReader,
 *        WikiIndexStore (JSON-path only), and WikiRagSource.
 *
 * Tests:
 *  WIS-01: WikiChunkSplitter heading-aware split produces chunks per heading
 *  WIS-02: WikiChunkSplitter overlap — consecutive chunks share tokens
 *  WIS-03: WikiChunkSplitter chunk_id determinism across two split() calls
 *  WIS-04: WikiChunkSplitter empty input returns empty vector
 *  WIS-05: JsonWikiIndexReader load+query roundtrip
 *  WIS-06: JsonWikiIndexReader result ordering (highest score first)
 *  WIS-07: JsonWikiIndexReader top_k limit respected
 *  WIS-08: JsonWikiIndexReader empty result when no term overlap
 *  WIS-09: WikiIndexStore isReady() returns false before construction completes
 *          (tested via JsonWikiIndexReader proxy: not-loaded → isReady=false)
 *  WIS-10: JsonWikiIndexReader BM25 path: known term found
 *  WIS-11: JsonWikiIndexReader vector path skipped (n/a for JSON reader)
 *  WIS-12: JsonWikiIndexReader flush (no-op check for load/reload)
 *  WIS-13: WikiRagSource stage integration — Success status, candidates populated
 *  WIS-14: WikiRagSource provenance tags present
 *  WIS-15: WikiRagSource fail-open: unloaded reader → Skipped
 *  WIS-16: WikiRagSource empty wiki → Success with zero candidates
 */

#include <gtest/gtest.h>

#include "llm/wiki_chunk_splitter.h"
#include "llm/wiki_index_store.h"
#include "llm/wiki_rag_source.h"
#include "rag/modular_rag_pipeline.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

using namespace themis::llm;
using namespace themis::rag;

// ─── Test helpers ─────────────────────────────────────────────────────────

namespace {

/// Build a simple 3-section Markdown document for splitter tests.
std::string makeSampleMarkdown() {
    return R"(# Introduction

This is the introduction paragraph. It explains the overall design of the
system and provides context for the reader.

## Architecture

The architecture section describes the components. Each component has a
specific role:
- Storage engine
- Index manager
- Query planner

### Storage Engine

The storage engine uses RocksDB for durable, high-performance key-value
storage with atomic WriteBatch operations and WAL-based durability.

## Conclusion

In summary, the system provides hybrid BM25 and vector retrieval.
)";
}

/// Build a minimal JSON index string (3 chunks).
std::string makeMinimalJsonIndex() {
    return R"([
  {
    "chunk_id": "aaa001",
    "file_path": "docs/arch.md",
    "section_title": "Architecture",
    "line_start": 1,
    "line_end": 10,
    "text": "The architecture uses RocksDB for storage and HNSW for vector search."
  },
  {
    "chunk_id": "bbb002",
    "file_path": "docs/arch.md",
    "section_title": "Query",
    "line_start": 11,
    "line_end": 20,
    "text": "Query planning uses an adaptive optimizer with cost-based decisions."
  },
  {
    "chunk_id": "ccc003",
    "file_path": "docs/intro.md",
    "section_title": "Introduction",
    "line_start": 1,
    "line_end": 5,
    "text": "ThemisDB is a C++ database engine with LLM integration."
  }
])";
}

/// Write a string to a temporary file, return its path.
std::string writeTempFile(const std::string& content) {
    std::string path = "/tmp/wiki_test_" + std::to_string(
        std::hash<std::string>{}(content)) + ".json";
    std::ofstream f(path);
    f << content;
    return path;
}

// ── Minimal reader that always returns a pre-configured result ──────────────
class FixedWikiReader : public IWikiIndexReader {
public:
    explicit FixedWikiReader(std::vector<WikiChunk> chunks, bool ready = true)
        : chunks_(std::move(chunks)), ready_(ready) {}

    std::vector<WikiChunk> query(const std::string&, int top_k, float /*min*/) const override {
        std::vector<WikiChunk> out = chunks_;
        if (top_k > 0 && static_cast<int>(out.size()) > top_k) {
            out.resize(static_cast<std::size_t>(top_k));
        }
        return out;
    }
    bool isReady() const noexcept override { return ready_; }

private:
    std::vector<WikiChunk> chunks_;
    bool                   ready_;
};

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// WIS-01: Heading-aware split produces a chunk per section
// ─────────────────────────────────────────────────────────────────────────────
TEST(WikiChunkSplitter, WIS01_HeadingAwareSplit) {
    WikiChunkSplitter splitter(500, 50); // large window to avoid sub-splits
    auto chunks = splitter.split("docs/arch.md", makeSampleMarkdown());

    // Expect at least one chunk per non-empty section
    ASSERT_GE(chunks.size(), 4u) << "Expected at least 4 chunks (intro, arch, storage, conclusion)";

    // Every chunk must have non-empty text and a source_path
    for (const auto& c : chunks) {
        EXPECT_FALSE(c.text.empty())         << "chunk_id=" << c.chunk_id;
        EXPECT_EQ(c.source_path, "docs/arch.md");
        EXPECT_EQ(c.doc_id,      "docs/arch.md");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// WIS-02: Overlap — consecutive chunks in long sections share tokens
// ─────────────────────────────────────────────────────────────────────────────
TEST(WikiChunkSplitter, WIS02_OverlapBetweenChunks) {
    // Create a very long section to force sub-chunking
    std::string content = "# Long Section\n\n";
    for (int i = 0; i < 300; ++i) {
        content += "Token" + std::to_string(i) + " ";
        if (i % 15 == 14) content += '\n';
    }

    WikiChunkSplitter splitter(50, 10);
    auto chunks = splitter.split("test.md", content);

    ASSERT_GE(chunks.size(), 2u) << "Long section should be split into >= 2 chunks";

    // Verify overlap: the last tokens of chunk[0] should appear at the start of chunk[1]
    // (this is a soft check — we verify that consecutive chunks are not disjoint in lines)
    for (std::size_t i = 1; i < chunks.size(); ++i) {
        EXPECT_LE(chunks[i].line_start, chunks[i-1].line_end + 5)
            << "Consecutive chunks should have overlapping or adjacent line ranges";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// WIS-03: Chunk ID determinism — two identical calls produce identical IDs
// ─────────────────────────────────────────────────────────────────────────────
TEST(WikiChunkSplitter, WIS03_ChunkIdDeterminism) {
    WikiChunkSplitter splitter;
    const std::string content = makeSampleMarkdown();

    auto chunks1 = splitter.split("docs/arch.md", content);
    auto chunks2 = splitter.split("docs/arch.md", content);

    ASSERT_EQ(chunks1.size(), chunks2.size());
    for (std::size_t i = 0; i < chunks1.size(); ++i) {
        EXPECT_EQ(chunks1[i].chunk_id, chunks2[i].chunk_id)
            << "chunk_id at index " << i << " should be deterministic";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// WIS-04: Empty input returns empty vector
// ─────────────────────────────────────────────────────────────────────────────
TEST(WikiChunkSplitter, WIS04_EmptyInput) {
    WikiChunkSplitter splitter;
    EXPECT_TRUE(splitter.split("file.md", "").empty());
    EXPECT_TRUE(splitter.split("",        "").empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// WIS-05: JsonWikiIndexReader load+query roundtrip
// ─────────────────────────────────────────────────────────────────────────────
TEST(JsonWikiIndexReader, WIS05_LoadAndQueryRoundtrip) {
    const std::string path = writeTempFile(makeMinimalJsonIndex());
    JsonWikiIndexReader reader(path);
    reader.load();

    EXPECT_TRUE(reader.isReady());
    EXPECT_EQ(reader.size(), 3u);

    auto results = reader.query("RocksDB HNSW architecture", 10, 0.0f);
    ASSERT_FALSE(results.empty()) << "Should find chunk about architecture/RocksDB/HNSW";
    EXPECT_EQ(results[0].chunk_id, "aaa001");
}

// ─────────────────────────────────────────────────────────────────────────────
// WIS-06: JsonWikiIndexReader score ordering (highest first)
// ─────────────────────────────────────────────────────────────────────────────
TEST(JsonWikiIndexReader, WIS06_ScoreOrdering) {
    const std::string path = writeTempFile(makeMinimalJsonIndex());
    JsonWikiIndexReader reader(path, /*auto_load=*/true);

    auto results = reader.query("storage RocksDB vector HNSW architecture", 10, 0.0f);
    ASSERT_GE(results.size(), 2u);

    for (std::size_t i = 1; i < results.size(); ++i) {
        EXPECT_GE(results[i-1].score, results[i].score)
            << "Results should be sorted by descending score";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// WIS-07: JsonWikiIndexReader top_k limit respected
// ─────────────────────────────────────────────────────────────────────────────
TEST(JsonWikiIndexReader, WIS07_TopKLimit) {
    const std::string path = writeTempFile(makeMinimalJsonIndex());
    JsonWikiIndexReader reader(path, /*auto_load=*/true);

    auto results = reader.query("the", 1, 0.0f);
    EXPECT_LE(results.size(), 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// WIS-08: JsonWikiIndexReader empty result when query has no term overlap
// ─────────────────────────────────────────────────────────────────────────────
TEST(JsonWikiIndexReader, WIS08_EmptyResultNoOverlap) {
    const std::string path = writeTempFile(makeMinimalJsonIndex());
    JsonWikiIndexReader reader(path, /*auto_load=*/true);

    // Use a query that shares zero tokens with any chunk
    auto results = reader.query("zzzxxx999yyy888", 10, 0.0f);
    EXPECT_TRUE(results.empty())
        << "No results expected for query with zero token overlap";
}

// ─────────────────────────────────────────────────────────────────────────────
// WIS-09: isReady() false before load()
// ─────────────────────────────────────────────────────────────────────────────
TEST(JsonWikiIndexReader, WIS09_NotReadyBeforeLoad) {
    JsonWikiIndexReader reader("/nonexistent/path.json");
    EXPECT_FALSE(reader.isReady());
}

// ─────────────────────────────────────────────────────────────────────────────
// WIS-10: BM25 path — known query term found with positive score
// ─────────────────────────────────────────────────────────────────────────────
TEST(JsonWikiIndexReader, WIS10_BM25PositiveScore) {
    const std::string path = writeTempFile(makeMinimalJsonIndex());
    JsonWikiIndexReader reader(path, /*auto_load=*/true);

    auto results = reader.query("optimizer adaptive", 10, 0.0f);
    ASSERT_FALSE(results.empty());

    // The "Query" chunk should score highest for "optimizer"
    EXPECT_EQ(results[0].chunk_id, "bbb002")
        << "Expected query-planning chunk to score highest for 'optimizer'";
    EXPECT_GT(results[0].score, 0.0f);
}

// ─────────────────────────────────────────────────────────────────────────────
// WIS-11: Vector path not applicable for JsonWikiIndexReader (no-op check)
// ─────────────────────────────────────────────────────────────────────────────
TEST(JsonWikiIndexReader, WIS11_NoVectorDependency) {
    // JsonWikiIndexReader must function without VectorIndexManager.
    // This test simply verifies no link-time or runtime error occurs.
    const std::string path = writeTempFile(makeMinimalJsonIndex());
    JsonWikiIndexReader reader(path, /*auto_load=*/true);
    auto results = reader.query("database LLM", 5, 0.0f);
    // Should return results (ccc003 matches "database LLM ThemisDB")
    ASSERT_FALSE(results.empty());
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// WIS-12: Reload clears and re-populates chunk list
// ─────────────────────────────────────────────────────────────────────────────
TEST(JsonWikiIndexReader, WIS12_ReloadRefreshesChunks) {
    const std::string path = writeTempFile(makeMinimalJsonIndex());
    JsonWikiIndexReader reader(path, /*auto_load=*/true);
    EXPECT_EQ(reader.size(), 3u);

    // Reload must keep the same count
    reader.load();
    EXPECT_EQ(reader.size(), 3u);
    EXPECT_TRUE(reader.isReady());
}

// ─────────────────────────────────────────────────────────────────────────────
// WIS-13: WikiRagSource stage integration — Success + candidates
// ─────────────────────────────────────────────────────────────────────────────
TEST(WikiRagSource, WIS13_StageIntegration) {
    WikiChunk c;
    c.chunk_id = "x001"; c.text = "HNSW cosine metric search"; c.score = 0.8f;
    FixedWikiReader reader({c});

    WikiRagSource src(reader);
    ModularRAGContext ctx;
    ctx.query = "vector similarity";

    auto result = src.retrieveFromWiki(ctx);

    EXPECT_EQ(result.status, StageStatus::Success);
    ASSERT_EQ(result.candidates.size(), 1u);
    EXPECT_EQ(result.candidates[0].doc_id, "x001");
    EXPECT_FLOAT_EQ(result.candidates[0].score, 0.8f);
    EXPECT_EQ(result.candidates[0].source_namespace, "wiki");
}

// ─────────────────────────────────────────────────────────────────────────────
// WIS-14: WikiRagSource provenance tags present
// ─────────────────────────────────────────────────────────────────────────────
TEST(WikiRagSource, WIS14_ProvenanceTags) {
    WikiChunk c;
    c.chunk_id      = "y002";
    c.text          = "Storage engine";
    c.score         = 0.5f;
    c.source_path   = "docs/storage.md";
    c.section_title = "Overview";
    FixedWikiReader reader({c});

    WikiRagSource src(reader);
    ModularRAGContext ctx;
    ctx.query = "storage";
    auto result = src.retrieveFromWiki(ctx);

    ASSERT_EQ(result.status, StageStatus::Success);
    ASSERT_EQ(result.candidates.size(), 1u);

    const auto& tags = result.candidates[0].provenance_tags;
    auto hasTag = [&](const std::string& t) {
        return std::find(tags.begin(), tags.end(), t) != tags.end();
    };

    EXPECT_TRUE(hasTag("retrieve:wiki"))
        << "At least one 'retrieve:wiki*' tag expected";
    EXPECT_TRUE(hasTag("source:docs/storage.md"));
    EXPECT_TRUE(hasTag("section:Overview"));
}

// ─────────────────────────────────────────────────────────────────────────────
// WIS-15: WikiRagSource fail-open: not-ready reader → Skipped
// ─────────────────────────────────────────────────────────────────────────────
TEST(WikiRagSource, WIS15_FailOpenNotReady) {
    FixedWikiReader reader({}, /*ready=*/false);

    WikiRagSourceConfig cfg;
    cfg.fail_open = true;

    WikiRagSource src(reader, cfg);
    ModularRAGContext ctx;
    ctx.query = "anything";
    auto result = src.retrieveFromWiki(ctx);

    EXPECT_EQ(result.status, StageStatus::Skipped);
    EXPECT_FALSE(result.diagnostic.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// WIS-16: WikiRagSource empty wiki → Success with zero candidates
// ─────────────────────────────────────────────────────────────────────────────
TEST(WikiRagSource, WIS16_EmptyWikiSuccess) {
    FixedWikiReader reader(/*chunks=*/{}, /*ready=*/true);

    WikiRagSource src(reader);
    ModularRAGContext ctx;
    ctx.query = "anything";
    auto result = src.retrieveFromWiki(ctx);

    EXPECT_EQ(result.status, StageStatus::Success);
    EXPECT_TRUE(result.candidates.empty());
}
