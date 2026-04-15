/*
 * ThemisDB — Ingestion / AI Separation Tests
 *
 * Tests for:
 *   ITextGenerationBackend         (interface contract)   IB-01..IB-04
 *   NullTextGenerationBackend      (always-unavailable)   IB-05..IB-07
 *   LegalLlmAdapter with injected backend                 IB-08..IB-12
 *
 * These tests verify that the ingestion module's LLM integration layer is
 * fully decoupled from any concrete LLM implementation (SoC / DIP).
 * All backend interaction goes through the ITextGenerationBackend interface.
 *
 * Acceptance criteria:
 *
 * ITextGenerationBackend interface (IB-01..IB-04)
 *   IB-01  NullTextGenerationBackend::isAvailable() returns false
 *   IB-02  NullTextGenerationBackend::generate() returns empty string
 *   IB-03  NullTextGenerationBackend::description() is non-empty
 *   IB-04  ITextGenerationBackend can be used via shared_ptr polymorphism
 *
 * NullTextGenerationBackend (IB-05..IB-07)
 *   IB-05  Default-constructed LegalLlmAdapter uses NullTextGenerationBackend
 *   IB-06  LegalLlmAdapter::isLlmAvailable() returns false with null backend
 *   IB-07  LegalLlmAdapter::buildExtractorFn() returns empty fn with null backend
 *
 * LegalLlmAdapter with injected backend (IB-08..IB-12)
 *   IB-08  Injected available backend makes isLlmAvailable() return true
 *   IB-09  Injected available backend produces a non-empty ExtractorFn
 *   IB-10  ExtractorFn from available backend calls backend::generate()
 *   IB-11  nullptr backend injection falls back to NullTextGenerationBackend
 *   IB-12  backend exceptions in generate() are caught; result has warning
 */

#include <gtest/gtest.h>

#include "ingestion/inference_backend.h"
#include "ingestion/llm_adapter.h"

#include <string>
#include <atomic>

using namespace themis::ingestion;

// =============================================================================
// Stub backends used by tests
// =============================================================================

/// An always-available stub backend that echoes a fixed JSON response.
class StubAvailableBackend : public ITextGenerationBackend {
public:
    mutable std::atomic<int> call_count{0};
    std::string fixed_response;

    explicit StubAvailableBackend(const std::string& response = R"(
        {"deontic_category":"obligation","confidence":0.9,"entities":[],"obligations":[]})") 
        : fixed_response(response) {}

    std::string generate(const std::string& /*prompt*/,
                         int    /*max_tokens*/,
                         double /*temperature*/,
                         const std::string& /*lora_adapter*/) override {
        ++call_count;
        return fixed_response;
    }

    bool        isAvailable() const override { return true; }
    std::string description() const override { return "StubAvailableBackend"; }
};

/// A backend that always throws on generate().
class ThrowingBackend : public ITextGenerationBackend {
public:
    std::string generate(const std::string&, int, double, const std::string&) override {
        throw std::runtime_error("backend exploded");
    }
    bool        isAvailable() const override { return true; }
    std::string description() const override { return "ThrowingBackend"; }
};

// =============================================================================
// IB-01..IB-04  ITextGenerationBackend interface tests
// =============================================================================

TEST(InferenceBackend, IB01_Null_IsAvailable_ReturnsFalse) {
    NullTextGenerationBackend nb;
    EXPECT_FALSE(nb.isAvailable());
}

TEST(InferenceBackend, IB02_Null_Generate_ReturnsEmptyString) {
    NullTextGenerationBackend nb;
    EXPECT_EQ(nb.generate("hello", 512, 0.1, ""), "");
}

TEST(InferenceBackend, IB03_Null_Description_NonEmpty) {
    NullTextGenerationBackend nb;
    EXPECT_FALSE(nb.description().empty());
}

TEST(InferenceBackend, IB04_Polymorphism_ViaSharedPtr) {
    std::shared_ptr<ITextGenerationBackend> ptr =
        std::make_shared<NullTextGenerationBackend>();
    EXPECT_FALSE(ptr->isAvailable());
    EXPECT_TRUE(ptr->generate("x", 1, 0.1, "").empty());
}

// =============================================================================
// IB-05..IB-07  Default-constructed LegalLlmAdapter uses NullTextGenerationBackend
// =============================================================================

TEST(InferenceBackend, IB05_DefaultAdapter_UsesNullBackend) {
    LegalLlmAdapter adapter;
    // Default adapter → NullTextGenerationBackend → unavailable
    EXPECT_FALSE(adapter.isLlmAvailable());
}

TEST(InferenceBackend, IB06_DefaultAdapter_IsLlmAvailable_False) {
    LegalLlmAdapter adapter;
    EXPECT_FALSE(adapter.isLlmAvailable());
}

TEST(InferenceBackend, IB07_DefaultAdapter_BuildExtractorFn_Empty) {
    LegalLlmAdapter adapter;
    auto fn = adapter.buildExtractorFn();
    // Empty function — DeonticExtractor will use built-in regex
    EXPECT_FALSE(static_cast<bool>(fn));
}

// =============================================================================
// IB-08..IB-12  LegalLlmAdapter with injected backend
// =============================================================================

TEST(InferenceBackend, IB08_InjectedAvailableBackend_IsLlmAvailableTrue) {
    auto backend = std::make_shared<StubAvailableBackend>();
    LegalLlmAdapter adapter(backend);
    EXPECT_TRUE(adapter.isLlmAvailable());
}

TEST(InferenceBackend, IB09_InjectedAvailableBackend_NonEmptyExtractorFn) {
    auto backend = std::make_shared<StubAvailableBackend>();
    LegalLlmAdapter adapter(backend);
    auto fn = adapter.buildExtractorFn();
    EXPECT_TRUE(static_cast<bool>(fn));
}

TEST(InferenceBackend, IB10_ExtractorFn_CallsBackendGenerate) {
    auto backend = std::make_shared<StubAvailableBackend>();
    LegalLlmAdapter adapter(backend);
    auto fn = adapter.buildExtractorFn();
    ASSERT_TRUE(static_cast<bool>(fn));

    // Execute the extractor function — it should call backend->generate()
    auto result = fn("§ 4 Abs. 1 BImSchG verpflichtet den Betreiber ...");
    EXPECT_GE(backend->call_count.load(), 1);
    // JSON response contains "obligation" → deontic_categories should be populated
    EXPECT_FALSE(result.deontic_categories.empty());
}

TEST(InferenceBackend, IB11_NullptrBackendInjection_FallsBackToNull) {
    // Passing nullptr should not crash — adapter uses NullTextGenerationBackend
    LegalLlmAdapter adapter(nullptr);
    EXPECT_FALSE(adapter.isLlmAvailable());
    auto fn = adapter.buildExtractorFn();
    EXPECT_FALSE(static_cast<bool>(fn));
}

TEST(InferenceBackend, IB12_ThrowingBackend_ExceptionCaughtInExtractorFn) {
    auto backend = std::make_shared<ThrowingBackend>();
    LegalLlmAdapter adapter(backend);
    auto fn = adapter.buildExtractorFn();
    ASSERT_TRUE(static_cast<bool>(fn));

    // Should NOT throw — exception must be caught inside the lambda
    DeonticExtraction result;
    EXPECT_NO_THROW(result = fn("some text"));
    // Warning should mention the exception
    EXPECT_FALSE(result.warnings.empty());
}
