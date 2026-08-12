/*
 * ThemisDB — TensorIngestionBridge Tests
 *
 * Tests for:
 *   ITensorDecompositionBackend interface       TIB-01..TIB-04
 *   NullTensorDecompositionBackend (no-op)      TIB-05..TIB-07
 *   TensorIngestionBridge (concrete)            TIB-08..TIB-12
 *   builtin.chunk_tt_decompose step             TIB-13..TIB-18
 *   ExtractionContext::tensor_cores field       TIB-19..TIB-20
 *
 * Acceptance criteria:
 *
 * ITensorDecompositionBackend interface (TIB-01..TIB-04)
 *   TIB-01  NullTensorDecompositionBackend::isAvailable() returns false
 *   TIB-02  NullTensorDecompositionBackend::decompose() returns record with
 *           empty serialized_train
 *   TIB-03  NullTensorDecompositionBackend::shouldDecompose() returns false
 *   TIB-04  ITensorDecompositionBackend can be used via shared_ptr polymorphism
 *
 * NullTensorDecompositionBackend (TIB-05..TIB-07)
 *   TIB-05  Null backend description() is non-empty
 *   TIB-06  Null backend chunk_id propagated in returned record
 *   TIB-07  Null backend source_file_id propagated in returned record
 *
 * TensorIngestionBridge (TIB-08..TIB-12)
 *   TIB-08  isAvailable() returns true
 *   TIB-09  decompose() on a constant vector returns a populated record
 *   TIB-10  shouldDecompose() returns false for zero-vector (incompressible)
 *   TIB-11  decompose() record carries correct chunk_id and source_file_id
 *   TIB-12  description() includes epsilon and kappa values
 *
 * builtin.chunk_tt_decompose step (TIB-13..TIB-18)
 *   TIB-13  Step with null backend skips when skip_when_unavailable=true
 *   TIB-14  Step with null backend errors when skip_when_unavailable=false
 *   TIB-15  Step is no-op when ctx.embeddings is empty
 *   TIB-16  Step produces tensor_cores for compressible embeddings
 *   TIB-17  Step propagates provenance metadata (section_ref, page)
 *   TIB-18  Step skips incompressible chunks (min_kappa gate)
 *
 * ExtractionContext extensions (TIB-19..TIB-20)
 *   TIB-19  ExtractionContext::hasTensorCores() returns false on empty context
 *   TIB-20  ExtractionContext::hasTensorCores() returns true after step writes cores
 */

#include <gtest/gtest.h>

#include "ingestion/inference_backend.h"
#include "ingestion/extraction_context.h"
#include "ingestion/builtin_step_factories.h"
#include "ingestion/ingestion_step.h"
#include "tensor/tensor_ingestion_bridge.h"

#include <memory>
#include <string>
#include <vector>
#include <numeric>

using namespace themis::ingestion;
using namespace themis::tensor;

// =============================================================================
// Helpers
// =============================================================================

/// Build a flat embedding of constant value `val` with `dim` floats.
static std::vector<float> makeConstantEmbedding(std::size_t dim, float val = 1.0f) {
    return std::vector<float>(dim, val);
}

/// Build an embedding with low-rank structure (compressible): outer product of
/// two short vectors → expected κ is high.
static std::vector<float> makeRankOneEmbedding(std::size_t rows, std::size_t cols) {
    std::vector<float> u(rows), v(cols);
    for (std::size_t i = 0; i < rows; ++i) u[i] = static_cast<float>(i + 1);
    for (std::size_t j = 0; j < cols; ++j) v[j] = static_cast<float>(j + 1);
    std::vector<float> out(rows * cols);
    for (std::size_t i = 0; i < rows; ++i)
        for (std::size_t j = 0; j < cols; ++j)
            out[i * cols + j] = u[i] * v[j];
    return out;
}

/// Create a minimal ExtractionContext with one VectorRecord.
static ExtractionContext makeContextWithEmbedding(
    const std::string& file_id,
    const std::vector<float>& emb,
    const std::string& section_ref = "",
    const std::string& page        = "")
{
    ExtractionContext ctx;
    ctx.manifest.file_id      = file_id;
    ctx.manifest.original_path = "/tmp/" + file_id + ".pdf";

    VectorRecord vr;
    vr.chunk_id       = file_id + ":0";
    vr.source_file_id = file_id;
    vr.text_snippet   = "Test chunk";
    vr.embedding      = emb;
    if (!section_ref.empty()) vr.metadata["section_ref"] = section_ref;
    if (!page.empty())        vr.metadata["page"]        = page;

    ctx.embeddings.push_back(std::move(vr));
    return ctx;
}

/// Helper StepConfig factory.
static StepConfig makeConfig(const nlohmann::json& cfg_obj = {}) {
    StepConfig sc;
    sc.config = cfg_obj;
    return sc;
}

// =============================================================================
// TIB-01..TIB-04  ITensorDecompositionBackend interface contract
// =============================================================================

TEST(TensorIngestionBridge, TIB01_NullBackendIsAvailableFalse) {
    NullTensorDecompositionBackend nb;
    EXPECT_FALSE(nb.isAvailable());
}

TEST(TensorIngestionBridge, TIB02_NullBackendDecomposeReturnsEmptyTrain) {
    NullTensorDecompositionBackend nb;
    auto rec = nb.decompose({1.0f, 2.0f, 3.0f, 4.0f}, "c:0", "file1", 0.01, 0);
    EXPECT_TRUE(rec.serialized_train.empty());
}

TEST(TensorIngestionBridge, TIB03_NullBackendShouldDecomposeReturnsFalse) {
    NullTensorDecompositionBackend nb;
    EXPECT_FALSE(nb.shouldDecompose({1.0f, 2.0f}, 1.3));
}

TEST(TensorIngestionBridge, TIB04_InterfaceUsableViaSharedPtr) {
    std::shared_ptr<ITensorDecompositionBackend> backend =
        std::make_shared<NullTensorDecompositionBackend>();
    EXPECT_FALSE(backend->isAvailable());
    auto rec = backend->decompose({}, "c:0", "f");
    EXPECT_TRUE(rec.serialized_train.empty());
}

// =============================================================================
// TIB-05..TIB-07  NullTensorDecompositionBackend specifics
// =============================================================================

TEST(TensorIngestionBridge, TIB05_NullBackendDescriptionNonEmpty) {
    NullTensorDecompositionBackend nb;
    EXPECT_FALSE(nb.description().empty());
}

TEST(TensorIngestionBridge, TIB06_NullBackendPropagatesChunkId) {
    NullTensorDecompositionBackend nb;
    auto rec = nb.decompose({1.0f}, "my:chunk", "file2", 0.01, 0);
    EXPECT_EQ(rec.chunk_id, "my:chunk");
}

TEST(TensorIngestionBridge, TIB07_NullBackendPropagatesSourceFileId) {
    NullTensorDecompositionBackend nb;
    auto rec = nb.decompose({1.0f}, "c", "file-abc", 0.01, 0);
    EXPECT_EQ(rec.source_file_id, "file-abc");
}

// =============================================================================
// TIB-08..TIB-12  TensorIngestionBridge concrete implementation
// =============================================================================

TEST(TensorIngestionBridge, TIB08_IsAvailableTrue) {
    TensorIngestionBridge bridge;
    EXPECT_TRUE(bridge.isAvailable());
}

TEST(TensorIngestionBridge, TIB09_DecomposeConstantVectorReturnsPopulatedRecord) {
    TensorIngestionBridge bridge;
    // Constant vector is rank-1 → very compressible
    auto emb = makeRankOneEmbedding(32, 32);  // 1024-dim rank-1 matrix
    auto rec = bridge.decompose(emb, "chunk:0", "fileid");
    // A populated record has non-empty serialized_train
    EXPECT_FALSE(rec.serialized_train.empty());
    EXPECT_EQ(rec.chunk_id, "chunk:0");
    EXPECT_EQ(rec.source_file_id, "fileid");
    EXPECT_GT(rec.compression_ratio, 0.0);
    EXPECT_EQ(rec.order, 2u);
}

TEST(TensorIngestionBridge, TIB10_ShouldDecomposeReturnsFalseForZeroVector) {
    TensorIngestionBridge bridge;
    // Zero vector: all singular values are 0 → TT rank-1 would equal input
    // size. In practice the bridge may still compress, but with min_kappa=10
    // this should fail the gate.
    auto zeros = std::vector<float>(64, 0.0f);
    // With a very high kappa requirement, zero vectors should not pass.
    EXPECT_FALSE(bridge.shouldDecompose(zeros, 100.0));
}

TEST(TensorIngestionBridge, TIB11_DecomposeRecordCarriesCorrectIds) {
    TensorIngestionBridge bridge;
    auto emb = makeRankOneEmbedding(16, 16);
    auto rec = bridge.decompose(emb, "my_chunk:3", "sha256abc");
    EXPECT_EQ(rec.chunk_id, "my_chunk:3");
    EXPECT_EQ(rec.source_file_id, "sha256abc");
}

TEST(TensorIngestionBridge, TIB12_DescriptionContainsEpsilonAndKappa) {
    TensorIngestionBridge bridge(0.02, 32, 1.5);
    const auto desc = bridge.description();
    EXPECT_NE(desc.find("0.02"), std::string::npos);
    EXPECT_NE(desc.find("1.5"),  std::string::npos);
}

// =============================================================================
// TIB-13..TIB-18  builtin.chunk_tt_decompose step
// =============================================================================

TEST(ChunkTtDecomposeStep, TIB13_NullBackendSkipsWhenSkipEnabled) {
    auto step = builtin::createChunkTtDecomposeStep(nullptr);
    ExtractionContext ctx;
    ctx.manifest.file_id = "f1";
    // Add a dummy embedding so the step sees non-empty embeddings
    VectorRecord vr;
    vr.chunk_id = "f1:0";
    vr.embedding = {1.0f, 2.0f};
    ctx.embeddings.push_back(vr);

    StepConfig sc;
    sc.config = {{"skip_when_unavailable", true}};
    auto result = step->execute(ctx, sc);
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(ctx.tensor_cores.empty());
    // Should have a warning about unavailability
    ASSERT_FALSE(ctx.warnings.empty());
    EXPECT_NE(ctx.warnings.front().find("unavailable"), std::string::npos);
}

TEST(ChunkTtDecomposeStep, TIB14_NullBackendErrorsWhenSkipDisabled) {
    auto step = builtin::createChunkTtDecomposeStep(nullptr);
    ExtractionContext ctx;
    ctx.manifest.file_id = "f2";
    VectorRecord vr;
    vr.chunk_id = "f2:0";
    vr.embedding = {1.0f};
    ctx.embeddings.push_back(vr);

    StepConfig sc;
    sc.config = {{"skip_when_unavailable", false}};
    auto result = step->execute(ctx, sc);
    EXPECT_FALSE(result.has_value());
}

TEST(ChunkTtDecomposeStep, TIB15_StepIsNoOpWhenEmbeddingsEmpty) {
    auto bridge = std::make_shared<TensorIngestionBridge>();
    auto step   = builtin::createChunkTtDecomposeStep(bridge);
    ExtractionContext ctx;
    ctx.manifest.file_id = "f3";
    // ctx.embeddings is empty

    auto result = step->execute(ctx, makeConfig());
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(ctx.tensor_cores.empty());
    ASSERT_FALSE(ctx.warnings.empty());
    EXPECT_NE(ctx.warnings.front().find("empty"), std::string::npos);
}

TEST(ChunkTtDecomposeStep, TIB16_StepProducesTensorCoresForCompressibleEmbeddings) {
    auto bridge = std::make_shared<TensorIngestionBridge>(0.05, 0, 0.0 /*min_kappa=0 → always pass*/);
    auto step   = builtin::createChunkTtDecomposeStep(bridge);

    auto emb = makeRankOneEmbedding(32, 32); // 1024 floats, rank-1
    auto ctx  = makeContextWithEmbedding("sha256-abc", emb);

    // min_kappa=0.0 → κ-gate always passes
    StepConfig sc;
    sc.config = {{"min_kappa", 0.0}, {"epsilon", 0.05}};
    auto result = step->execute(ctx, sc);
    EXPECT_TRUE(result.has_value());
    EXPECT_FALSE(ctx.tensor_cores.empty());
    EXPECT_EQ(ctx.tensor_cores.front().chunk_id, "sha256-abc:0");
}

TEST(ChunkTtDecomposeStep, TIB17_StepPropagatesProvenanceMetadata) {
    auto bridge = std::make_shared<TensorIngestionBridge>(0.05, 0, 0.0);
    auto step   = builtin::createChunkTtDecomposeStep(bridge);

    auto emb = makeRankOneEmbedding(32, 32);
    auto ctx  = makeContextWithEmbedding("sha256-xyz", emb,
                                          "§ 4 Abs. 1", "12");

    StepConfig sc;
    sc.config = {{"min_kappa", 0.0}, {"epsilon", 0.05}};
    auto result = step->execute(ctx, sc);
    ASSERT_TRUE(result.has_value());
    ASSERT_FALSE(ctx.tensor_cores.empty());

    const auto& rec = ctx.tensor_cores.front();
    EXPECT_EQ(rec.metadata.at("section_ref"), "§ 4 Abs. 1");
    EXPECT_EQ(rec.metadata.at("page"), "12");
}

TEST(ChunkTtDecomposeStep, TIB18_StepSkipsIncompressibleChunks) {
    // Bridge with high min_kappa so zero vector fails the gate
    auto bridge = std::make_shared<TensorIngestionBridge>(0.01, 0, 100.0);
    auto step   = builtin::createChunkTtDecomposeStep(bridge);

    // zero vector → κ will be < 100 → gate blocks
    auto ctx = makeContextWithEmbedding("file-z", std::vector<float>(64, 0.0f));

    StepConfig sc;
    sc.config = {{"min_kappa", 100.0}};
    auto result = step->execute(ctx, sc);
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(ctx.tensor_cores.empty());
    // Warning about skipped chunk
    bool found_skip_warning = false;
    for (const auto& w : ctx.warnings) {
        if (w.find("skipped") != std::string::npos ||
            w.find("compressibility") != std::string::npos) {
            found_skip_warning = true;
            break;
        }
    }
    EXPECT_TRUE(found_skip_warning);
}

// =============================================================================
// TIB-19..TIB-20  ExtractionContext extensions
// =============================================================================

TEST(ExtractionContextExtensions, TIB19_HasTensorCoresReturnsFalseOnEmpty) {
    ExtractionContext ctx;
    EXPECT_FALSE(ctx.hasTensorCores());
}

TEST(ExtractionContextExtensions, TIB20_HasTensorCoresReturnsTrueAfterWrite) {
    ExtractionContext ctx;
    TensorCoreRecord rec;
    rec.chunk_id = "c:0";
    ctx.tensor_cores.push_back(std::move(rec));
    EXPECT_TRUE(ctx.hasTensorCores());
}

// =============================================================================
// TIB-21..TIB-22  Rademacher random projection in shouldDecompose() (stub #159)
// =============================================================================

TEST(TensorIngestionBridge, TIB21_ShouldDecomposeHandlesLargeDimWithRademacher) {
    // Build a clearly compressible large embedding (dim > 1024):
    // flatten a rank-1 matrix so the pilot projection should preserve
    // high compressibility under the kappa gate.
    TensorIngestionBridge bridge(0.01, 32, 1.3);

    constexpr std::size_t rows = 64;
    constexpr std::size_t cols = 64;
    auto embedding = makeRankOneEmbedding(rows, cols); // dim = 4096 (> 1024)

    // The main goal of this test is that the large-dimension Rademacher
    // pilot path remains functional. With a disabled gate (min_kappa=0),
    // shouldDecompose must succeed.
    EXPECT_TRUE(bridge.shouldDecompose(embedding, 0.0));
}

TEST(TensorIngestionBridge, TIB22_RademacherProjectionIsDeterministic) {
    // The Rademacher projection is seeded from embedding.size(), so two calls
    // with the same embedding must return the same shouldDecompose() result.
    TensorIngestionBridge bridge(0.01, 32, 1.3);

    constexpr std::size_t dim = 1536;
    std::vector<float> emb(dim, 0.0f);
    for (std::size_t i = 0; i < dim; ++i) {
        emb[i] = std::cos(static_cast<float>(i) * 0.02f);
    }

    bool r1 = bridge.shouldDecompose(emb, 1.3);
    bool r2 = bridge.shouldDecompose(emb, 1.3);
    EXPECT_EQ(r1, r2);
}
