/**
 * @file test_prompt_evaluator_embedding.cpp
 * @brief Tests for the pluggable IEmbeddingProvider interface (issue 2.4)
 */

#include <gtest/gtest.h>
#include "prompt_engineering/prompt_evaluator.h"
#include <cmath>

using namespace themis::prompt_engineering;

// ============================================================================
// Stub embedding providers
// ============================================================================

/// Returns a constant unit vector (all identical texts have cosine = 1.0)
class ConstantEmbeddingProvider : public IEmbeddingProvider {
public:
    explicit ConstantEmbeddingProvider(std::vector<double> vec)
        : vec_(std::move(vec)) {}
    std::vector<double> embed(const std::string&) const override { return vec_; }
    std::string name() const override { return "ConstantEmbed"; }
private:
    std::vector<double> vec_;
};

/// Returns a fixed 3-D vector based on the first character of the input text
class SimpleEmbeddingProvider : public IEmbeddingProvider {
public:
    std::vector<double> embed(const std::string& text) const override {
        if (text.empty()) return {0.0, 0.0, 0.0};
        double v = static_cast<double>(text[0]);
    // Normalize so the vector has unit length
        double norm = std::sqrt(v * v + 1.0 + 1.0);
        return {v / norm, 1.0 / norm, 1.0 / norm};
    }
    std::string name() const override { return "SimpleEmbed"; }
};

/// Always throws
class ThrowingEmbeddingProvider : public IEmbeddingProvider {
public:
    std::vector<double> embed(const std::string&) const override {
        throw std::runtime_error("embed backend error");
    }
    std::string name() const override { return "ThrowingEmbed"; }
};

/// Always returns empty
class EmptyEmbeddingProvider : public IEmbeddingProvider {
public:
    std::vector<double> embed(const std::string&) const override { return {}; }
    std::string name() const override { return "EmptyEmbed"; }
};

// ============================================================================
// computeCosineSimilarity (static helper)
// ============================================================================

TEST(EmbeddingProviderTest, CosineSimilarity_Identical) {
    std::vector<double> v = {1.0, 0.0, 0.0};
    EXPECT_NEAR(PromptEvaluator::computeCosineSimilarity(v, v), 1.0, 1e-9);
}

TEST(EmbeddingProviderTest, CosineSimilarity_Orthogonal) {
    std::vector<double> v1 = {1.0, 0.0};
    std::vector<double> v2 = {0.0, 1.0};
    EXPECT_NEAR(PromptEvaluator::computeCosineSimilarity(v1, v2), 0.0, 1e-9);
}

TEST(EmbeddingProviderTest, CosineSimilarity_EmptyVectors) {
    EXPECT_DOUBLE_EQ(PromptEvaluator::computeCosineSimilarity({}, {}), 0.0);
}

TEST(EmbeddingProviderTest, CosineSimilarity_DimMismatch) {
    EXPECT_DOUBLE_EQ(PromptEvaluator::computeCosineSimilarity({1.0}, {1.0, 2.0}), 0.0);
}

TEST(EmbeddingProviderTest, CosineSimilarity_ClampedToOne) {
    // Numerical drift can push > 1.0; must be clamped
    std::vector<double> v = {1.0, 0.0};
    // Should never exceed 1.0
    EXPECT_LE(PromptEvaluator::computeCosineSimilarity(v, v), 1.0);
}

// ============================================================================
// computeEmbeddingSimilarity (instance method)
// ============================================================================

TEST(EmbeddingProviderTest, NoProvider_ReturnsNegativeOne) {
    PromptEvaluator eval;
    EXPECT_DOUBLE_EQ(eval.computeEmbeddingSimilarity("hello", "world"), -1.0);
}

TEST(EmbeddingProviderTest, WithConstantProvider_IdenticalVectors_ReturnsOne) {
    PromptEvaluator eval;
    eval.setEmbeddingProvider(
        std::make_shared<ConstantEmbeddingProvider>(std::vector<double>{1.0, 0.0, 0.0})
    );
    EXPECT_NEAR(eval.computeEmbeddingSimilarity("foo", "bar"), 1.0, 1e-9);
}

TEST(EmbeddingProviderTest, WithSimpleProvider_SameText_ReturnsOne) {
    PromptEvaluator eval;
    eval.setEmbeddingProvider(std::make_shared<SimpleEmbeddingProvider>());
    double sim = eval.computeEmbeddingSimilarity("abc", "abc");
    EXPECT_NEAR(sim, 1.0, 1e-9);
}

TEST(EmbeddingProviderTest, WithSimpleProvider_DifferentText_NotOne) {
    PromptEvaluator eval;
    eval.setEmbeddingProvider(std::make_shared<SimpleEmbeddingProvider>());
    double sim = eval.computeEmbeddingSimilarity("abc", "xyz");
    EXPECT_LT(sim, 1.0);
    EXPECT_GE(sim, 0.0);
}

TEST(EmbeddingProviderTest, ThrowingProvider_FallsBackToNegativeOne) {
    PromptEvaluator eval;
    eval.setEmbeddingProvider(std::make_shared<ThrowingEmbeddingProvider>());
    // computeEmbeddingSimilarity returns -1 on error; evaluateSingle falls back to Jaccard
    EXPECT_DOUBLE_EQ(eval.computeEmbeddingSimilarity("foo", "bar"), -1.0);
}

TEST(EmbeddingProviderTest, EmptyProvider_ReturnsNegativeOne) {
    PromptEvaluator eval;
    eval.setEmbeddingProvider(std::make_shared<EmptyEmbeddingProvider>());
    EXPECT_DOUBLE_EQ(eval.computeEmbeddingSimilarity("foo", "bar"), -1.0);
}

TEST(EmbeddingProviderTest, ClearProvider) {
    PromptEvaluator eval;
    eval.setEmbeddingProvider(std::make_shared<SimpleEmbeddingProvider>());
    EXPECT_TRUE(eval.hasEmbeddingProvider());
    eval.clearEmbeddingProvider();
    EXPECT_FALSE(eval.hasEmbeddingProvider());
}

// ============================================================================
// Integration: evaluateSingle uses embedding similarity when provider is set
// ============================================================================

TEST(EmbeddingProviderTest, EvaluateSingle_UsesEmbeddingWhenProviderSet) {
    PromptEvaluator eval;
    eval.setEmbeddingProvider(
        std::make_shared<ConstantEmbeddingProvider>(std::vector<double>{1.0, 0.0})
    );

    // Constant provider: all texts map to the same vector → cosine sim = 1.0
    auto metrics = eval.evaluateSingle("completely different text", "foo bar baz");
    EXPECT_NEAR(metrics.semantic_similarity, 1.0, 1e-9);
    EXPECT_TRUE(metrics.details.contains("embedding_provider"));
}

TEST(EmbeddingProviderTest, EvaluateSingle_FallsBackToJaccardOnThrow) {
    PromptEvaluator eval;
    eval.setEmbeddingProvider(std::make_shared<ThrowingEmbeddingProvider>());

    // Should not throw; falls back to Jaccard similarity
    EXPECT_NO_THROW({
        auto metrics = eval.evaluateSingle("hello world", "hello world");
        EXPECT_GE(metrics.semantic_similarity, 0.0);
        EXPECT_LE(metrics.semantic_similarity, 1.0);
    });
}

TEST(EmbeddingProviderTest, EvaluateSingle_NoProvider_UsesJaccard) {
    PromptEvaluator eval;
    auto m1 = eval.evaluateSingle("hello world", "hello world");
    EXPECT_NEAR(m1.semantic_similarity, 1.0, 1e-9);

    auto m2 = eval.evaluateSingle("cat sat mat", "dog runs fast");
    EXPECT_NEAR(m2.semantic_similarity, 0.0, 1e-9);
}
