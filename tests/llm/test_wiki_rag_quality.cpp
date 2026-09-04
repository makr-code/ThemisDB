/**
 * @file test_wiki_rag_quality.cpp
 * @brief Quality gate tests for the wiki RAG retrieval path.
 *
 * Tests:
 *  WISQ-01: Recall@5 on an inline synthetic reference collection
 *  WISQ-02: Latency gate — query 100-chunk JsonWikiIndexReader in < 200 ms
 *  WISQ-03: Score monotonicity — results are sorted descending
 *  WISQ-04: min_score threshold filters low-relevance results
 *  WISQ-05: WikiRagSource pipeline integration with JsonWikiIndexReader
 *
 * All tests use JsonWikiIndexReader (no RocksDB dependency).
 */

#include <gtest/gtest.h>

#include "llm/wiki_index_store.h"
#include "llm/wiki_rag_source.h"
#include "rag/modular_rag_pipeline.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace themis::llm;
using namespace themis::rag;

// ─── Helpers ─────────────────────────────────────────────────────────────────

namespace {

/// Write content to /tmp and return path.
std::string writeJson(const std::string& json, const std::string& suffix = "") {
    const std::string path = "/tmp/wiki_quality_" + suffix + "_" +
        std::to_string(std::hash<std::string>{}(json)) + ".json";
    std::ofstream f(path);
    f << json;
    return path;
}

/// Build a synthetic 20-doc reference corpus with known relevant pairs.
/// Each document has a distinct topic keyword.
struct RefDoc {
    std::string chunk_id;
    std::string topic;    ///< Primary keyword (appears 5x)
    std::string text;
};

std::vector<RefDoc> buildReferenceCorpus() {
    // 20 topics, each with a short document
    const std::vector<std::pair<std::string,std::string>> topics = {
        {"hnsw",       "HNSW is a hierarchical navigable small world graph algorithm for ANN search."},
        {"rocksdb",    "RocksDB provides durable key-value storage using a log-structured merge tree."},
        {"bm25",       "BM25 is a probabilistic ranking function used in full-text information retrieval."},
        {"embedding",  "Embeddings are dense vector representations of text used in semantic search."},
        {"cosine",     "Cosine similarity measures the angle between two vectors in high-dimensional space."},
        {"lora",       "LoRA enables parameter-efficient fine-tuning by injecting low-rank matrices."},
        {"transformer","The transformer architecture uses self-attention to capture long-range dependencies."},
        {"quantize",   "Quantization reduces model size by representing weights with fewer bits."},
        {"raft",       "Raft is a consensus algorithm for distributed log replication."},
        {"grpc",       "gRPC is a high-performance RPC framework using Protocol Buffers."},
        {"llama",      "LLaMA is an open-weights large language model trained by Meta."},
        {"faiss",      "FAISS provides efficient similarity search and clustering for dense vectors."},
        {"tfidf",      "TF-IDF weights term frequency by inverse document frequency in IR."},
        {"kmeans",     "K-means partitions data into k clusters by minimising within-cluster variance."},
        {"attention",  "Attention mechanisms allow models to focus on relevant parts of the input."},
        {"pipeline",   "A RAG pipeline combines retrieval with generation to ground LLM responses."},
        {"token",      "Tokenization splits text into subword units for model input processing."},
        {"vector",     "Vector search retrieves documents by semantic similarity using dense embeddings."},
        {"index",      "An inverted index maps terms to the documents in which they appear."},
        {"rerank",     "Reranking re-scores initial retrieval candidates with a cross-encoder model."},
    };

    std::vector<RefDoc> corpus;
    int seq = 0;
    for (const auto& [kw, desc] : topics) {
        RefDoc d;
        d.chunk_id = "ref" + std::to_string(seq++);
        d.topic    = kw;
        // Repeat the keyword several times to ensure BM25 can find it
        d.text = desc + " " + kw + " " + kw + " " + kw + " " + kw + " " + kw;
        corpus.push_back(d);
    }
    return corpus;
}

/// Serialise a RefDoc corpus to JSON index format.
std::string corpusToJson(const std::vector<RefDoc>& corpus) {
    std::ostringstream oss;
    oss << "[\n";
    for (std::size_t i = 0; i < corpus.size(); ++i) {
        oss << "  {\"chunk_id\":\"" << corpus[i].chunk_id
            << "\",\"file_path\":\"docs/ref.md\""
            << ",\"section_title\":\"" << corpus[i].topic << "\""
            << ",\"line_start\":" << (i * 5 + 1)
            << ",\"line_end\":"   << (i * 5 + 5)
            << ",\"text\":\""     << corpus[i].text << "\"}";
        if (i + 1 < corpus.size()) {
          oss << ',';
        }
        oss << '\n';
    }
    oss << "]";
    return oss.str();
}

/// Build 100-chunk synthetic corpus for latency test.
std::string build100ChunkJson() {
    std::ostringstream oss;
    oss << "[\n";
    for (int i = 0; i < 100; ++i) {
        oss << "  {\"chunk_id\":\"lat" << i
            << "\",\"file_path\":\"docs/lat.md\""
            << ",\"section_title\":\"Section " << i << "\""
            << ",\"line_start\":" << (i * 10 + 1)
            << ",\"line_end\":"   << (i * 10 + 10)
            << ",\"text\":\"Chunk " << i
            << " contains information about topic " << (i % 20)
            << " storage vector index retrieval embedding cosine\"}";
        if (i + 1 < 100) {
          oss << ',';
        }
        oss << '\n';
    }
    oss << "]";
    return oss.str();
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// WISQ-01: Recall@5 on synthetic reference collection
//
// For each of 5 probe queries, the correct document should appear in top-5
// results.  Target: Recall@5 >= 4/5 (80 %).
// ─────────────────────────────────────────────────────────────────────────────
TEST(WikiRagQuality, WISQ01_RecallAt5) {
    auto corpus = buildReferenceCorpus();
    const std::string path = writeJson(corpusToJson(corpus), "recall");
    JsonWikiIndexReader reader(path, /*auto_load=*/true);
    ASSERT_TRUE(reader.isReady());

    // 5 probe queries with expected top chunk_id
    struct Probe { std::string query; std::string expected_id; };
    const std::vector<Probe> probes = {
        {"HNSW hierarchical navigable small world",      "ref0"},
        {"RocksDB log-structured merge tree storage",    "ref1"},
        {"BM25 probabilistic ranking full-text",         "ref2"},
        {"embedding dense vector semantic search",       "ref3"},
        {"cosine similarity angle high-dimensional",     "ref4"},
    };

    int hits = 0;
    for (const auto& p : probes) {
        auto results = reader.query(p.query, 5, 0.0f);
        bool found = std::any_of(results.begin(), results.end(),
            [&](const WikiChunk& c){ return c.chunk_id == p.expected_id; });
        if (found) {
          ++hits;
        }
        EXPECT_TRUE(found) << "Expected '" << p.expected_id
                           << "' in top-5 for query: " << p.query;
    }

    const double recall = static_cast<double>(hits) / static_cast<double>(probes.size());
    EXPECT_GE(recall, 0.8) << "Recall@5 below 80%: " << hits << "/5";
}

// ─────────────────────────────────────────────────────────────────────────────
// WISQ-02: Latency gate — 100-chunk query in < 200 ms
// ─────────────────────────────────────────────────────────────────────────────
TEST(WikiRagQuality, WISQ02_LatencyUnder200ms) {
    const std::string path = writeJson(build100ChunkJson(), "latency");
    JsonWikiIndexReader reader(path, /*auto_load=*/true);
    ASSERT_TRUE(reader.isReady());
    ASSERT_EQ(reader.size(), 100u);

    const auto t0 = std::chrono::steady_clock::now();

    // Run 10 queries to get a representative sample
    for (int i = 0; i < 10; ++i) {
        auto results = reader.query("vector index storage embedding retrieval cosine", 5, 0.0f);
        (void)results;
    }

    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0).count();

    // 10 queries on 100 chunks must complete in < 200 ms total
    EXPECT_LT(elapsed_ms, 200)
        << "10 queries on 100 chunks took " << elapsed_ms << " ms (limit 200 ms)";
}

// ─────────────────────────────────────────────────────────────────────────────
// WISQ-03: Score monotonicity
// ─────────────────────────────────────────────────────────────────────────────
TEST(WikiRagQuality, WISQ03_ScoreMonotonicity) {
    auto corpus = buildReferenceCorpus();
    const std::string path = writeJson(corpusToJson(corpus), "mono");
    JsonWikiIndexReader reader(path, /*auto_load=*/true);

    auto results = reader.query("vector embedding cosine similarity search HNSW", 10, 0.0f);
    ASSERT_GE(results.size(), 2u);

    for (std::size_t i = 1; i < results.size(); ++i) {
        EXPECT_GE(results[i-1].score, results[i].score)
            << "Score at index " << (i-1) << " (" << results[i-1].score
            << ") < score at " << i << " (" << results[i].score << ")";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// WISQ-04: min_score threshold filters low-relevance results
// ─────────────────────────────────────────────────────────────────────────────
TEST(WikiRagQuality, WISQ04_MinScoreFiltering) {
    auto corpus = buildReferenceCorpus();
    const std::string path = writeJson(corpusToJson(corpus), "minscore");
    JsonWikiIndexReader reader(path, /*auto_load=*/true);

    // Very high threshold should return fewer (or zero) results
    auto all     = reader.query("vector search", 20, 0.0f);
    auto high_th = reader.query("vector search", 20, 99999.0f); // impossible threshold

    EXPECT_TRUE(high_th.empty()) << "No results should pass threshold of 99999";
    EXPECT_FALSE(all.empty())    << "Some results expected with threshold 0";
}

// ─────────────────────────────────────────────────────────────────────────────
// WISQ-05: WikiRagSource pipeline integration with JsonWikiIndexReader
// ─────────────────────────────────────────────────────────────────────────────
TEST(WikiRagQuality, WISQ05_PipelineIntegration) {
    auto corpus = buildReferenceCorpus();
    const std::string path = writeJson(corpusToJson(corpus), "pipeline");
    JsonWikiIndexReader reader(path, /*auto_load=*/true);

    WikiRagSourceConfig cfg;
    cfg.top_k    = 5;
    cfg.min_score = 0.0f;

    WikiRagSource source(reader, cfg);

    ModularRAGContext ctx;
    ctx.query = "BM25 full-text ranking information retrieval";

    auto stage_result = source.retrieveFromWiki(ctx);
    EXPECT_EQ(stage_result.status, StageStatus::Success);
    EXPECT_FALSE(stage_result.candidates.empty());
    EXPECT_LE(stage_result.candidates.size(), 5u);

    // All candidates must have source_namespace = "wiki"
    for (const auto& cand : stage_result.candidates) {
        EXPECT_EQ(cand.source_namespace, "wiki");
        EXPECT_FALSE(cand.provenance_tags.empty());
    }

    // Top candidate should relate to BM25 (chunk_id "ref2")
    EXPECT_EQ(stage_result.candidates[0].doc_id, "ref2")
        << "BM25 document should rank first for BM25 query";
}
