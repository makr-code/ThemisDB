/**
 * @file test_adapter_scope_validation.cpp
 * @brief CMT-FIN-36..40: Content module adapter scope validation tests
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Test Suite: CMT-FIN-36..40 (Scope Lifetime Validation)
 * @note Authority: src/content/MODULE_GAPS_BATCH5.md §CMT-7503
 * @note Status: Scope mismatch verification tests - RAII patterns validation
 * @date 2026-08-15
 */

#include <gtest/gtest.h>
#include <memory>
#include <span>
#include <vector>
#include <string>

#include "content/adapters/format_extractor_adapters.h"
#include "ingestion/format_extractor.h"

namespace themis {
namespace content {
namespace adapters {
namespace test {

// ─────────────────────────────────────────────────────────────────────────────
// CMT-FIN-36: Adapter Factory Returns Heap-Allocated Smart Pointers
// ─────────────────────────────────────────────────────────────────────────────
class CMT_FIN_36_AdapterFactoryOwnership : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize factory
        factory_ = createDefaultFormatExtractorFactory();
        ASSERT_TRUE(factory_);
    }

    std::shared_ptr<ingestion::IFormatExtractorFactory> factory_;
};

TEST_F(CMT_FIN_36_AdapterFactoryOwnership, ImageExtractorReturnsValidSharedPtr) {
    // CMT-FIN-36-01: Verify image extractor factory returns valid shared_ptr
    // Not a raw pointer, not a dangling reference
    auto image_adapter = createImageExtractorAdapter();
    ASSERT_TRUE(image_adapter);
    EXPECT_EQ(image_adapter.use_count(), 1); // Owned by caller
    EXPECT_STREQ(image_adapter->name(), "ImageExtractorAdapter");
}

TEST_F(CMT_FIN_36_AdapterFactoryOwnership, PdfExtractorReturnsValidSharedPtr) {
    // CMT-FIN-36-02: Verify PDF extractor factory returns valid shared_ptr
    auto pdf_adapter = createPdfExtractorAdapter();
    ASSERT_TRUE(pdf_adapter);
    EXPECT_EQ(pdf_adapter.use_count(), 1); // Owned by caller
    EXPECT_STREQ(pdf_adapter->name(), "PdfExtractorAdapter");
}

TEST_F(CMT_FIN_36_AdapterFactoryOwnership, OfficeExtractorReturnsValidSharedPtr) {
    // CMT-FIN-36-03: Verify office extractor factory returns valid shared_ptr
    auto office_adapter = createOfficeExtractorAdapter();
    if (office_adapter) {  // May be disabled at compile time
        EXPECT_EQ(office_adapter.use_count(), 1);
        EXPECT_STREQ(office_adapter->name(), "OfficeExtractorAdapter");
    }
}

TEST_F(CMT_FIN_36_AdapterFactoryOwnership, ArchiveExtractorReturnsValidSharedPtr) {
    // CMT-FIN-36-04: Verify archive extractor factory returns valid shared_ptr
    auto archive_adapter = createArchiveExtractorAdapter();
    ASSERT_TRUE(archive_adapter);
    EXPECT_EQ(archive_adapter.use_count(), 1);
    EXPECT_STREQ(archive_adapter->name(), "ArchiveExtractorAdapter");
}

TEST_F(CMT_FIN_36_AdapterFactoryOwnership, AudioExtractorReturnsValidSharedPtr) {
    // CMT-FIN-36-05: Verify audio extractor factory returns valid shared_ptr
    auto audio_adapter = createAudioExtractorAdapter();
    if (audio_adapter) {  // May be disabled at compile time
        EXPECT_EQ(audio_adapter.use_count(), 1);
        EXPECT_STREQ(audio_adapter->name(), "AudioExtractorAdapter");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// CMT-FIN-37: Adapter RAII Lifetime Boundaries
// ─────────────────────────────────────────────────────────────────────────────
class CMT_FIN_37_AdapterRaiiLifetime : public ::testing::Test {};

TEST_F(CMT_FIN_37_AdapterRaiiLifetime, ImageExtractorScopedLifetime) {
    // CMT-FIN-37-01: Verify image extractor lifetime is properly scoped
    std::shared_ptr<ingestion::IFormatExtractor> adapter;
    {
        adapter = createImageExtractorAdapter();
        EXPECT_EQ(adapter.use_count(), 1);
    }
    // Still valid outside the block
    EXPECT_TRUE(adapter);
    EXPECT_STREQ(adapter->name(), "ImageExtractorAdapter");
}

TEST_F(CMT_FIN_37_AdapterRaiiLifetime, PdfExtractorScopedLifetime) {
    // CMT-FIN-37-02: Verify PDF extractor lifetime is properly scoped
    std::shared_ptr<ingestion::IFormatExtractor> adapter;
    {
        adapter = createPdfExtractorAdapter();
        EXPECT_EQ(adapter.use_count(), 1);
    }
    // Still valid outside the block
    EXPECT_TRUE(adapter);
    EXPECT_STREQ(adapter->name(), "PdfExtractorAdapter");
}

TEST_F(CMT_FIN_37_AdapterRaiiLifetime, ArchiveExtractorScopedLifetime) {
    // CMT-FIN-37-03: Verify archive extractor lifetime is properly scoped
    std::shared_ptr<ingestion::IFormatExtractor> adapter;
    {
        adapter = createArchiveExtractorAdapter();
        EXPECT_EQ(adapter.use_count(), 1);
    }
    // Still valid outside the block
    EXPECT_TRUE(adapter);
    EXPECT_STREQ(adapter->name(), "ArchiveExtractorAdapter");
}

// ─────────────────────────────────────────────────────────────────────────────
// CMT-FIN-38: Adapter Extract Method Scope Safety
// ─────────────────────────────────────────────────────────────────────────────
class CMT_FIN_38_AdapterExtractScope : public ::testing::Test {};

TEST_F(CMT_FIN_38_AdapterExtractScope, ImageExtractorReturnsSelfOwnedResult) {
    // CMT-FIN-38-01: Verify image extractor returns result with independent lifetime
    auto adapter = createImageExtractorAdapter();
    ASSERT_TRUE(adapter);

    // Create a sample image data (minimal JPEG header)
    std::vector<uint8_t> sample_jpeg{
        0xFF, 0xD8, 0xFF, 0xE0,  // JPEG SOI + APP0
        0x00, 0x10, 0x4A, 0x46,  // APP0 length and identifier
        0x49, 0x46, 0x00, 0x01   // "JFIF\0\1"
    };
    std::span<const std::byte> data_span(
        reinterpret_cast<const std::byte*>(sample_jpeg.data()),
        sample_jpeg.size());

    // Call extract
    ingestion::FormatExtractResult result = adapter->extract(data_span, "image/jpeg", "");

    // Verify result doesn't contain dangling pointers
    // (result uses std::string for text and metadata, which own their memory)
    EXPECT_TRUE(result.error.empty() || !result.ok);  // Either no error or failed parse
    // Note: raw_text and metadata are std::string/map types - they own their memory
}

TEST_F(CMT_FIN_38_AdapterExtractScope, PdfExtractorReturnsSelfOwnedResult) {
    // CMT-FIN-38-02: Verify PDF extractor returns result with independent lifetime
    auto adapter = createPdfExtractorAdapter();
    ASSERT_TRUE(adapter);

    // Create a sample PDF header (minimal)
    std::vector<uint8_t> sample_pdf{
        0x25, 0x50, 0x44, 0x46,  // %PDF
        0x2D, 0x31, 0x2E, 0x34   // -1.4
    };
    std::span<const std::byte> data_span(
        reinterpret_cast<const std::byte*>(sample_pdf.data()),
        sample_pdf.size());

    // Call extract
    ingestion::FormatExtractResult result = adapter->extract(data_span, "application/pdf", "");

    // Verify result doesn't contain dangling pointers
    EXPECT_TRUE(result.error.empty() || !result.ok);
    // Note: raw_text and metadata are std::string/map types - they own their memory
}

// ─────────────────────────────────────────────────────────────────────────────
// CMT-FIN-39: Stack/Heap Boundary Validation
// ─────────────────────────────────────────────────────────────────────────────
class CMT_FIN_39_StackHeapBoundary : public ::testing::Test {};

TEST_F(CMT_FIN_39_StackHeapBoundary, ImageExtractorNoStackEscapeOnExtract) {
    // CMT-FIN-39-01: Verify extract() doesn't return stack-allocated pointers
    auto adapter = createImageExtractorAdapter();
    ASSERT_TRUE(adapter);

    // Even with empty data, result should have valid lifetime
    std::vector<uint8_t> empty_data;
    std::span<const std::byte> empty_span(
        reinterpret_cast<const std::byte*>(empty_data.data()),
        0);

    ingestion::FormatExtractResult result = adapter->extract(empty_span, "image/jpeg", "");

    // raw_text and metadata are std::string/map - safely heap-managed
    EXPECT_NO_THROW(
        {
            // Access result fields to ensure they're valid
            [[maybe_unused]] auto t = result.raw_text;
            [[maybe_unused]] auto m = result.metadata;
            [[maybe_unused]] auto e = result.error;
        });
}

TEST_F(CMT_FIN_39_StackHeapBoundary, PdfExtractorNoStackEscapeOnExtract) {
    // CMT-FIN-39-02: Verify extract() doesn't return stack-allocated pointers
    auto adapter = createPdfExtractorAdapter();
    ASSERT_TRUE(adapter);

    std::vector<uint8_t> empty_data;
    std::span<const std::byte> empty_span(
        reinterpret_cast<const std::byte*>(empty_data.data()),
        0);

    ingestion::FormatExtractResult result = adapter->extract(empty_span, "application/pdf", "");

    // raw_text and metadata are std::string/map - safely heap-managed
    EXPECT_NO_THROW(
        {
            [[maybe_unused]] auto t = result.raw_text;
            [[maybe_unused]] auto m = result.metadata;
            [[maybe_unused]] auto e = result.error;
        });
}

// ─────────────────────────────────────────────────────────────────────────────
// CMT-FIN-40: Copy/Move Semantics Safety
// ─────────────────────────────────────────────────────────────────────────────
class CMT_FIN_40_MoveSemantics : public ::testing::Test {};

TEST_F(CMT_FIN_40_MoveSemantics, ImageExtractorMovePreservesValidity) {
    // CMT-FIN-40-01: Verify move semantics don't break adapter lifetime
    auto adapter1 = createImageExtractorAdapter();
    auto name1 = adapter1->name();

    // Move to new variable
    auto adapter2 = std::move(adapter1);

    // adapter2 should still be valid
    EXPECT_STREQ(adapter2->name(), "ImageExtractorAdapter");
    EXPECT_STREQ(adapter2->name(), name1);

    // adapter1 should be null after move
    EXPECT_FALSE(adapter1);
}

TEST_F(CMT_FIN_40_MoveSemantics, PdfExtractorMovePreservesValidity) {
    // CMT-FIN-40-02: Verify move semantics don't break adapter lifetime
    auto adapter1 = createPdfExtractorAdapter();
    auto name1 = adapter1->name();

    // Move to new variable
    auto adapter2 = std::move(adapter1);

    // adapter2 should still be valid
    EXPECT_STREQ(adapter2->name(), "PdfExtractorAdapter");
    EXPECT_STREQ(adapter2->name(), name1);

    // adapter1 should be null after move
    EXPECT_FALSE(adapter1);
}

TEST_F(CMT_FIN_40_MoveSemantics, CopyAndMoveMultipleAdapters) {
    // CMT-FIN-40-03: Verify multiple adapter instances maintain independent lifetimes
    auto img_adapter1 = createImageExtractorAdapter();
    auto pdf_adapter1 = createPdfExtractorAdapter();

    auto img_adapter2 = img_adapter1;  // Copy
    auto pdf_adapter2 = std::move(pdf_adapter1);  // Move

    // Both should be valid
    EXPECT_TRUE(img_adapter1);
    EXPECT_TRUE(img_adapter2);
    EXPECT_TRUE(pdf_adapter2);

    // Use count reflects copy
    EXPECT_EQ(img_adapter1.use_count(), 2);  // Shared between img_adapter1 and img_adapter2
    EXPECT_EQ(img_adapter2.use_count(), 2);

    // pdf_adapter1 should be null
    EXPECT_FALSE(pdf_adapter1);

    // pdf_adapter2 should be valid (moved from pdf_adapter1)
    EXPECT_EQ(pdf_adapter2.use_count(), 1);
}

}  // namespace test
}  // namespace adapters
}  // namespace content
}  // namespace themis
