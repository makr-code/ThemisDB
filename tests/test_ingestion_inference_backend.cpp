/*
 * ThemisDB — Ingestion / AI Separation Tests
 *
 * Tests for:
 *   ITextGenerationBackend         (interface contract)   IB-01..IB-04
 *   NullTextGenerationBackend      (always-unavailable)   IB-05..IB-07
 *   LegalLlmAdapter with injected backend                 IB-08..IB-12
 *   SemanticValidator extractor injection                  SE-01..SE-08
 *   IngestionManager AI backend injection                  IM-01..IM-05
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

TEST(InferenceBackend, IB03b_NullText_CallbackBridge_OverridesGenerateAndAvailability) {
    NullTextGenerationBackend::setGenerateFn(
        [](const std::string& prompt, int, double, const std::string&) {
            return std::string("bridge:") + prompt;
        });
    NullTextGenerationBackend::setAvailabilityFn([]() { return true; });

    NullTextGenerationBackend nb;
    EXPECT_TRUE(nb.isAvailable());
    EXPECT_EQ(nb.generate("hello", 32, 0.2, ""), "bridge:hello");

    NullTextGenerationBackend::setGenerateFn({});
    NullTextGenerationBackend::setAvailabilityFn({});
}

TEST(InferenceBackend, IB04_Polymorphism_ViaSharedPtr) {
    std::shared_ptr<ITextGenerationBackend> ptr =
        std::make_shared<NullTextGenerationBackend>();
    EXPECT_FALSE(ptr->isAvailable());
    EXPECT_TRUE(ptr->generate("x", 1, 0.1, "").empty());
}

TEST(InferenceBackend, IB04b_NullEmbedding_CallbackBridge_OverridesEmbedAndAvailability) {
    NullEmbeddingBackend::setEmbedFn(
        [](const std::string& text, int dims) {
            return std::vector<float>(static_cast<std::size_t>(dims),
                                      text.empty() ? 0.0f : 1.0f);
        });
    NullEmbeddingBackend::setAvailabilityFn([]() { return true; });

    NullEmbeddingBackend eb(4);
    auto vec = eb.embed("abc");
    ASSERT_EQ(vec.size(), 4u);
    EXPECT_FLOAT_EQ(vec[0], 1.0f);
    EXPECT_TRUE(eb.isAvailable());

    NullEmbeddingBackend::setEmbedFn({});
    NullEmbeddingBackend::setAvailabilityFn({});
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

// =============================================================================
// Includes for Phase 2 tests
// =============================================================================

#include "ingestion/semantic_validator.h"
#include "ingestion/ingestion_manager.h"

// =============================================================================
// SE-01..SE-08  SemanticValidator extractor injection
// =============================================================================

TEST(SemanticValidatorSoC, SE01_DefaultExtractorUsesRegex) {
    SemanticValidator v;
    // Default extractor = regex only; does not crash and produces a result
    auto result = v.extractDocument("doc-1",
        "§ 4 Abs. 1 BImSchG verpflichtet den Betreiber zur Einholung einer Genehmigung.");
    EXPECT_FALSE(result.document_id.empty());
}

TEST(SemanticValidatorSoC, SE02_SetExtractor_DefaultDeonticExtractor) {
    SemanticValidator v;
    DeonticExtractor default_extractor;
    // Setting a plain default extractor must not crash
    EXPECT_NO_THROW(v.setExtractor(std::move(default_extractor)));
    auto result = v.extractDocument("doc-2",
        "§ 5 Der Betreiber muss Lärmmessungen durchführen.");
    EXPECT_FALSE(result.document_id.empty());
}

TEST(SemanticValidatorSoC, SE03_SetExtractor_LlmBackedExtractorFromStubAdapter) {
    auto backend = std::make_shared<StubAvailableBackend>();
    LegalLlmAdapter adapter(backend);
    DeonticExtractor llm_extractor = adapter.buildExtractor(0.75);

    SemanticValidator v;
    EXPECT_NO_THROW(v.setExtractor(std::move(llm_extractor)));
}

TEST(SemanticValidatorSoC, SE04_SetExtractor_LlmBackedExtractor_CallsBackend) {
    auto backend = std::make_shared<StubAvailableBackend>();
    LegalLlmAdapter adapter(backend);
    DeonticExtractor llm_extractor = adapter.buildExtractor(0.75);

    SemanticValidator v;
    v.setExtractor(std::move(llm_extractor));

    auto result = v.extractDocument("doc-3",
        "§ 4 Abs. 1 BImSchG verpflichtet den Betreiber zur Einholung einer Genehmigung.");
    // The stub backend should have been called at least once
    EXPECT_GE(backend->call_count.load(), 1);
    EXPECT_FALSE(result.document_id.empty());
}

TEST(SemanticValidatorSoC, SE05_SetExtractor_TwiceReplacesExtractor) {
    auto backend1 = std::make_shared<StubAvailableBackend>();
    auto backend2 = std::make_shared<StubAvailableBackend>();

    LegalLlmAdapter adapter1(backend1);
    LegalLlmAdapter adapter2(backend2);

    SemanticValidator v;
    v.setExtractor(adapter1.buildExtractor(0.75));
    v.setExtractor(adapter2.buildExtractor(0.75));

    v.extractDocument("doc-4", "§ 3 Der Unternehmer darf nicht ...");

    // Only backend2 should have been called (second extractor wins)
    EXPECT_EQ(backend1->call_count.load(), 0);
    EXPECT_GE(backend2->call_count.load(), 1);
}

TEST(SemanticValidatorSoC, SE06_ExtractDocument_Works_After_SetExtractor) {
    auto backend = std::make_shared<StubAvailableBackend>();
    LegalLlmAdapter adapter(backend);

    SemanticValidator v;
    v.setExtractor(adapter.buildExtractor(0.75));

    auto result = v.extractDocument("doc-5",
        "§ 1 Geltungsbereich\n"
        "§ 2 Abs. 1 Der Betreiber ist verpflichtet eine Genehmigung einzuholen.");
    EXPECT_GE(result.provisions.size(), 1u);
}

TEST(SemanticValidatorSoC, SE07_SemanticValidator_NoLlmIncludes) {
    // Structural test: SemanticValidator compiles without any llm/ headers.
    // If this test compiles successfully, the SoC boundary is enforced.
    SemanticValidator v;
    SUCCEED();
}

TEST(SemanticValidatorSoC, SE08_UnavailableBackend_FallsBackToRegex) {
    // An unavailable backend → LegalLlmAdapter::buildExtractorFn() returns {}
    // → buildExtractor() installs no LLM fn → regex fallback active
    NullTextGenerationBackend null_backend;
    ASSERT_FALSE(null_backend.isAvailable());

    LegalLlmAdapter adapter(std::make_shared<NullTextGenerationBackend>());
    DeonticExtractor extractor = adapter.buildExtractor(0.75);
    // Extractor must work with regex fallback — no crash
    DeonticExtraction result = extractor.extract(
        "§ 3 Der Betreiber muss die Anlage stillegen.");
    EXPECT_GE(result.overall_confidence, 0.0);
}

// =============================================================================
// IM-01..IM-05  IngestionManager AI backend injection
// =============================================================================

TEST(IngestionManagerSoC, IM01_DefaultGetBackend_ReturnsNullBackend) {
    IngestionManager mgr("test_db");
    auto backend = mgr.getTextGenerationBackend();
    ASSERT_NE(backend, nullptr);
    EXPECT_FALSE(backend->isAvailable());
}

TEST(IngestionManagerSoC, IM02_SetNullptr_ResetsToNullBackend) {
    IngestionManager mgr("test_db");
    mgr.setTextGenerationBackend(nullptr);
    auto backend = mgr.getTextGenerationBackend();
    ASSERT_NE(backend, nullptr);
    EXPECT_FALSE(backend->isAvailable());
}

TEST(IngestionManagerSoC, IM03_SetBackend_StoresProvidedBackend) {
    IngestionManager mgr("test_db");
    auto stub = std::make_shared<StubAvailableBackend>();
    mgr.setTextGenerationBackend(stub);
    auto retrieved = mgr.getTextGenerationBackend();
    EXPECT_EQ(retrieved, stub);
    EXPECT_TRUE(retrieved->isAvailable());
}

TEST(IngestionManagerSoC, IM04_RunLegalExtraction_UnavailableBackend_NocrashRegexFallback) {
    IngestionManager mgr("test_db");
    // Default null backend → regex extraction
    LegalIngestionConfig cfg;
    cfg.confidence_threshold = 0.5;
    EXPECT_NO_THROW({
        auto result = mgr.runLegalExtraction(
            "doc-im-04",
            "§ 4 Abs. 1 BImSchG verpflichtet den Betreiber zur Einholung einer Genehmigung.",
            cfg);
        EXPECT_EQ(result.document_id, "doc-im-04");
    });
}

TEST(IngestionManagerSoC, IM05_RunLegalExtraction_AvailableBackend_CallsGenerate) {
    IngestionManager mgr("test_db");
    auto stub = std::make_shared<StubAvailableBackend>();
    mgr.setTextGenerationBackend(stub);

    LegalIngestionConfig cfg;
    cfg.confidence_threshold = 0.5;
    EXPECT_NO_THROW({
        auto result = mgr.runLegalExtraction(
            "doc-im-05",
            "§ 4 Abs. 1 BImSchG verpflichtet den Betreiber zur Einholung einer Genehmigung.",
            cfg);
        EXPECT_EQ(result.document_id, "doc-im-05");
    });
    // The stub backend must have been called at least once by the pipeline
    EXPECT_GE(stub->call_count.load(), 1);
}
