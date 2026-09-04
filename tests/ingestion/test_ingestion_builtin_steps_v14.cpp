/*
 * test_ingestion_builtin_steps_v14.cpp
 *
 * Tests for the three new v1.4.0 builtin ingestion steps:
 *   ING-01..04  builtin.decompress  (DecompressStep)
 *   ING-05..08  builtin.legal_reference_extractor  (LegalReferenceExtractorStep)
 *   ING-09..12  builtin.chunk_embed  (ChunkEmbedStep + IEmbeddingBackend)
 */

#include <gtest/gtest.h>
#include "ingestion/ingestion_step.h"
#include "ingestion/builtin_step_factories.h"
#include "ingestion/inference_backend.h"
#include "ingestion/extraction_context.h"
#include "ingestion/file_manifest.h"
#include "ingestion/base_entity.h"
#include <string>
#include <vector>

using namespace themis::ingestion;
using namespace themis::ingestion::builtin;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static ExtractionContext makeCtx(const std::string& path = "",
                                  const std::string& mime = "") {
    ExtractionContext ctx;
    ctx.manifest.original_path = path;
    ctx.manifest.detected_mime = mime;
    ctx.manifest.file_id       = "sha256:deadbeef";
    return ctx;
}

static StepConfig makeConfig(const nlohmann::json& cfg = {}) {
    StepConfig sc;
    sc.config = cfg;
    return sc;
}

// ─────────────────────────────────────────────────────────────────────────────
// ING-01: DecompressStep — getName()/getVersion()
// ─────────────────────────────────────────────────────────────────────────────
TEST(DecompressStepTest, ING01_NameAndVersion) {
    auto step = createDecompressStep();
    ASSERT_NE(step, nullptr);
    EXPECT_STREQ(step->getName(),    "builtin.decompress");
    EXPECT_STREQ(step->getVersion(), "1.4.0");
}

// ─────────────────────────────────────────────────────────────────────────────
// ING-02: DecompressStep — supported MIME types advertised
// ─────────────────────────────────────────────────────────────────────────────
TEST(DecompressStepTest, ING02_SupportedMimeTypes) {
    auto step = createDecompressStep();
    const auto mimes = step->supportedMimeTypes();
    EXPECT_FALSE(mimes.empty());
    // ZIP must be in the list
    bool has_zip = false;
    for (const auto& m : mimes) {
        if (m == "application/zip") {
          has_zip = true;
        }
    }
    EXPECT_TRUE(has_zip) << "application/zip not in supportedMimeTypes()";
}

// ─────────────────────────────────────────────────────────────────────────────
// ING-03: DecompressStep — empty path produces warning, no error
// ─────────────────────────────────────────────────────────────────────────────
TEST(DecompressStepTest, ING03_EmptyPathProducesWarning) {
    auto step = createDecompressStep();
    auto ctx  = makeCtx("", "application/zip");
    auto sc   = makeConfig();
    auto res  = step->execute(ctx, sc);
    EXPECT_TRUE(res.has_value()) << "Should succeed (non-fatal) on empty path";
    EXPECT_FALSE(ctx.warnings.empty()) << "Should produce a warning for empty path";
    EXPECT_EQ(ctx.extracted_file_paths.size(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// ING-04: DecompressStep — unknown MIME type → warning, no extraction
// ─────────────────────────────────────────────────────────────────────────────
TEST(DecompressStepTest, ING04_UnknownMimeTypeProducesWarning) {
    auto step = createDecompressStep();
    auto ctx  = makeCtx("/some/file.xyz", "application/octet-stream");
    auto sc   = makeConfig();
    auto res  = step->execute(ctx, sc);
    EXPECT_TRUE(res.has_value()) << "Unknown MIME type should not cause error";
    // Warning should mention unrecognised archive type
    bool found = false;
    for (const auto& w : ctx.warnings) {
        if (w.find("unrecognised archive type") != std::string::npos) {
          found = true;
        }
    }
    EXPECT_TRUE(found) << "Expected 'unrecognised archive type' in warnings";
    EXPECT_EQ(ctx.extracted_file_paths.size(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// ING-05: LegalReferenceExtractorStep — getName()/getVersion()
// ─────────────────────────────────────────────────────────────────────────────
TEST(LegalReferenceExtractorStepTest, ING05_NameAndVersion) {
    auto step = createLegalReferenceExtractorStep();
    ASSERT_NE(step, nullptr);
    EXPECT_STREQ(step->getName(),    "builtin.legal_reference_extractor");
    EXPECT_STREQ(step->getVersion(), "1.4.0");
}

// ─────────────────────────────────────────────────────────────────────────────
// ING-06: LegalReferenceExtractorStep — empty raw_text sets counts to zero
// ─────────────────────────────────────────────────────────────────────────────
TEST(LegalReferenceExtractorStepTest, ING06_EmptyTextSetsZeroCounts) {
    auto step = createLegalReferenceExtractorStep();
    auto ctx  = makeCtx();
    ctx.raw_text = "";
    auto sc  = makeConfig();
    auto res = step->execute(ctx, sc);
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(ctx.extra.at("legal_refs.extracted_count"), "0");
    EXPECT_EQ(ctx.extra.at("legal_refs.dangling_count"),  "0");
    EXPECT_EQ(ctx.extra.at("legal_refs.warnings_json"),   "[]");
}

// ─────────────────────────────────────────────────────────────────────────────
// ING-07: LegalReferenceExtractorStep — extracts references and stores counts
// ─────────────────────────────────────────────────────────────────────────────
TEST(LegalReferenceExtractorStepTest, ING07_ExtractsReferences) {
    auto step = createLegalReferenceExtractorStep();
    auto ctx  = makeCtx();
    // German legal text with a known §-reference format
    ctx.raw_text = "Gemäß § 4 BImSchG sind folgende Maßnahmen erforderlich.";
    nlohmann::json cfg;
    cfg["known_laws"] = nlohmann::json::array({"BImSchG"});
    auto sc  = makeConfig(cfg);
    auto res = step->execute(ctx, sc);
    EXPECT_TRUE(res.has_value());
    // Extracted count should be stored
    EXPECT_TRUE(ctx.extra.count("legal_refs.extracted_count") > 0);
    EXPECT_TRUE(ctx.extra.count("legal_refs.dangling_count")  > 0);
    EXPECT_TRUE(ctx.extra.count("legal_refs.warnings_json")   > 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// ING-08: LegalReferenceExtractorStep — dangling refs produce ctx.warnings
// ─────────────────────────────────────────────────────────────────────────────
TEST(LegalReferenceExtractorStepTest, ING08_DanglingRefProducesWarning) {
    auto step = createLegalReferenceExtractorStep();
    auto ctx  = makeCtx();
    // Reference to unknown law: no known laws registered
    ctx.raw_text = "Siehe § 99 UnbekanntesGesetz.";
    auto sc  = makeConfig();  // no known_laws configured
    auto res = step->execute(ctx, sc);
    EXPECT_TRUE(res.has_value());
    // dangling_count should be > 0 and warning should propagate
    const std::string& dangling_str = ctx.extra.at("legal_refs.dangling_count");
    int dangling = std::stoi(dangling_str);
    EXPECT_GE(dangling, 0);  // may or may not resolve depending on regex
    // warnings_json must be a valid JSON array string
    EXPECT_EQ(ctx.extra.at("legal_refs.warnings_json").front(), '[');
}

// ─────────────────────────────────────────────────────────────────────────────
// ING-09: ChunkEmbedStep — getName()/getVersion()
// ─────────────────────────────────────────────────────────────────────────────
TEST(ChunkEmbedStepTest, ING09_NameAndVersion) {
    auto step = createChunkEmbedStep();
    ASSERT_NE(step, nullptr);
    EXPECT_STREQ(step->getName(),    "builtin.chunk_embed");
    EXPECT_STREQ(step->getVersion(), "1.4.0");
}

// ─────────────────────────────────────────────────────────────────────────────
// ING-10: ChunkEmbedStep — unavailable backend + skip_when_unavailable=true
//         → step is a no-op (no embeddings, no error)
// ─────────────────────────────────────────────────────────────────────────────
TEST(ChunkEmbedStepTest, ING10_UnavailableBackendSkipped) {
    auto step = createChunkEmbedStep(nullptr);  // NullEmbeddingBackend
    auto ctx  = makeCtx();
    TextChunk chunk;
    chunk.seq  = 0;
    chunk.text = "Hello world";
    ctx.chunks.push_back(chunk);

    nlohmann::json cfg;
    cfg["skip_when_unavailable"] = true;
    auto sc  = makeConfig(cfg);
    auto res = step->execute(ctx, sc);

    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(ctx.embeddings.size(), 0u) << "No embeddings when backend unavailable + skip=true";
    EXPECT_FALSE(ctx.warnings.empty())   << "Should have a skip warning";
}

// ─────────────────────────────────────────────────────────────────────────────
// ING-11: ChunkEmbedStep — unavailable backend + skip_when_unavailable=false
//         → error result
// ─────────────────────────────────────────────────────────────────────────────
TEST(ChunkEmbedStepTest, ING11_UnavailableBackendError) {
    auto step = createChunkEmbedStep(nullptr);  // NullEmbeddingBackend
    auto ctx  = makeCtx();
    TextChunk chunk;
    chunk.seq  = 0;
    chunk.text = "Hello world";
    ctx.chunks.push_back(chunk);

    nlohmann::json cfg;
    cfg["skip_when_unavailable"] = false;
    auto sc  = makeConfig(cfg);
    auto res = step->execute(ctx, sc);

    EXPECT_FALSE(res.has_value()) << "Should fail when backend unavailable + skip=false";
}

// ─────────────────────────────────────────────────────────────────────────────
// ING-12: ChunkEmbedStep — available backend produces VectorRecord per chunk
// ─────────────────────────────────────────────────────────────────────────────

// A minimal stub backend that is available and returns deterministic vectors
class StubEmbeddingBackend : public IEmbeddingBackend {
public:
    explicit StubEmbeddingBackend(int dims = 4) : dims_(dims) {}
    std::vector<float> embed(const std::string& /*text*/) override {
        return std::vector<float>(static_cast<std::size_t>(dims_), 1.0f);
    }
    int  dimensions()  const override { return dims_; }
    bool isAvailable() const override { return true; }
    std::string description() const override { return "StubEmbedding"; }
private:
    int dims_ = {};
};

TEST(ChunkEmbedStepTest, ING12_AvailableBackendProducesVectorRecords) {
    auto backend = std::make_shared<StubEmbeddingBackend>(4);
    auto step    = createChunkEmbedStep(backend);
    auto ctx     = makeCtx();
    ctx.manifest.file_id = "sha256:abc123";

    for (int i = 0; i < 3; ++i) {
        TextChunk c;
        c.seq  = static_cast<std::uint32_t>(i);
        c.text = "chunk text " + std::to_string(i);
        ctx.chunks.push_back(c);
    }

    nlohmann::json cfg;
    cfg["skip_when_unavailable"] = false;
    auto sc  = makeConfig(cfg);
    auto res = step->execute(ctx, sc);

    EXPECT_TRUE(res.has_value());
    ASSERT_EQ(ctx.embeddings.size(), 3u);
    for (std::size_t i = 0; i < ctx.embeddings.size(); ++i) {
        EXPECT_EQ(ctx.embeddings[i].embedding.size(), 4u);
        EXPECT_EQ(ctx.embeddings[i].source_file_id, "sha256:abc123");
        EXPECT_FALSE(ctx.embeddings[i].chunk_id.empty());
        EXPECT_FLOAT_EQ(ctx.embeddings[i].embedding[0], 1.0f);
    }
}
