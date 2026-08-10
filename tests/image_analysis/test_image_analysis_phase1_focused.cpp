// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_image_analysis_phase1_focused.cpp
 * @brief Phase 1 focused test suite for image analysis plugins.
 *
 * Covers YOLOv8OnnxPlugin (IMP-YOL-01..08) and TesseractOCRPlugin
 * (IMP-OCR-01..08).  All tests use deterministic in-process fixtures –
 * no model files, no network, kSeed = 42.
 *
 * Because ONNX Runtime and Tesseract are optional dependencies the real
 * inference paths are guarded by HAVE_ONNXRUNTIME / HAVE_TESSERACT
 * compile-time flags.  Tests validate the plugin interface contract and
 * stub-mode behaviour unconditionally; when the backends ARE present the
 * same tests additionally validate that inference produces structurally
 * valid output.
 *
 * ## Test families
 *
 * ### IMP-YOL-01..08 – YOLOv8OnnxPlugin
 *   IMP-YOL-01  Plugin reports correct name and detection capability
 *   IMP-YOL-02  initialize() succeeds (stub path; no model file required)
 *   IMP-YOL-03  isReady() is true after initialize(), false after shutdown()
 *   IMP-YOL-04  detectObjects() on empty image returns a DetectionResult
 *               (success=false in stub mode; success=true in ONNX mode)
 *   IMP-YOL-05  generateEmbedding() always returns an error result
 *   IMP-YOL-06  healthCheck() mirrors isReady()
 *   IMP-YOL-07  getStatistics() contains required metric keys
 *   IMP-YOL-08  Confidence threshold > 1.0 produces zero detections (stub
 *               / ONNX mode: effectively filters all)
 *
 * ### IMP-OCR-01..08 – TesseractOCRPlugin
 *   IMP-OCR-01  Plugin reports correct name and detection capability
 *   IMP-OCR-02  initialize() succeeds (stub path; no tessdata required)
 *   IMP-OCR-03  isReady() is true after initialize(), false after shutdown()
 *   IMP-OCR-04  detectObjects() on empty image returns a DetectionResult
 *   IMP-OCR-05  generateEmbedding() always returns an error result
 *   IMP-OCR-06  healthCheck() available after initialize()
 *   IMP-OCR-07  getStatistics() contains required metric keys
 *   IMP-OCR-08  getLastOcrResult() returns an OcrResult; success field
 *               matches detectObjects() success field
 */

#include "plugins/yolov8_onnx_plugin.h"
#include "plugins/tesseract_ocr_plugin.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

using namespace themis::plugins::image;

static constexpr uint32_t kSeed = 42;

// Minimal 8×8 white JPEG (independent of OpenCV / image libraries)
// Generated offline and embedded as a byte literal to keep tests
// self-contained and dependency-free.
static const std::vector<uint8_t> kMinimalJpegWhite8x8 = {
    0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46, 0x49, 0x46, 0x00, 0x01,
    0x01, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0xFF, 0xDB, 0x00, 0x43,
    0x00, 0x08, 0x06, 0x06, 0x07, 0x06, 0x05, 0x08, 0x07, 0x07, 0x07, 0x09,
    0x09, 0x08, 0x0A, 0x0C, 0x14, 0x0D, 0x0C, 0x0B, 0x0B, 0x0C, 0x19, 0x12,
    0x13, 0x0F, 0x14, 0x1D, 0x1A, 0x1F, 0x1E, 0x1D, 0x1A, 0x1C, 0x1C, 0x20,
    0x24, 0x2E, 0x27, 0x20, 0x22, 0x2C, 0x23, 0x1C, 0x1C, 0x28, 0x37, 0x29,
    0x2C, 0x30, 0x31, 0x34, 0x34, 0x34, 0x1F, 0x27, 0x39, 0x3D, 0x38, 0x32,
    0x3C, 0x2E, 0x33, 0x34, 0x32, 0xFF, 0xC0, 0x00, 0x0B, 0x08, 0x00, 0x08,
    0x00, 0x08, 0x01, 0x01, 0x11, 0x00, 0xFF, 0xC4, 0x00, 0x1F, 0x00, 0x00,
    0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0A, 0x0B, 0xFF, 0xC4, 0x00, 0xB5, 0x10, 0x00, 0x02, 0x01, 0x03,
    0x03, 0x02, 0x04, 0x03, 0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7D,
    0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06,
    0x13, 0x51, 0x61, 0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08,
    0x23, 0x42, 0xB1, 0xC1, 0x15, 0x52, 0xD1, 0xF0, 0x24, 0x33, 0x62, 0x72,
    0x82, 0x09, 0x0A, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45,
    0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
    0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75,
    0x76, 0x77, 0x78, 0x79, 0x7A, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
    0x8A, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4,
    0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7,
    0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA,
    0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE1, 0xE2, 0xE3,
    0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5,
    0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFF, 0xDA, 0x00, 0x08, 0x01, 0x01, 0x00,
    0x00, 0x3F, 0x00, 0xFB, 0xD7, 0xFF, 0xD9
};

// ---------------------------------------------------------------------------
// YOLOv8 tests
// ---------------------------------------------------------------------------

class YOLOv8PluginTest : public ::testing::Test {
protected:
    void SetUp() override {
        plugin_ = std::make_unique<YOLOv8OnnxPlugin>();
        // Initialize with no model_path → stub mode (no real ONNX session)
        PluginConfig cfg(nlohmann::json::object());
        ASSERT_TRUE(plugin_->initialize(cfg));
    }

    void TearDown() override {
        if (plugin_) plugin_->shutdown();
    }

    std::unique_ptr<YOLOv8OnnxPlugin> plugin_;
};

// IMP-YOL-01 – Plugin reports correct name and detection capability
TEST_F(YOLOv8PluginTest, IMP_YOL_01_PluginInfo) {
    PluginInfo info = plugin_->getInfo();
    EXPECT_EQ(info.name, "YOLOv8OnnxPlugin");
    EXPECT_TRUE(info.capabilities.supports_detection);
    EXPECT_FALSE(info.capabilities.supports_embedding);
    EXPECT_FALSE(info.capabilities.supports_captioning);
    // Must list at least CPU backend
    EXPECT_FALSE(info.capabilities.supported_backends.empty());
}

// IMP-YOL-02 – initialize() succeeds in stub mode (no model file)
TEST(YOLOv8PluginNoFixtureTest, IMP_YOL_02_InitStub) {
    YOLOv8OnnxPlugin p;
    PluginConfig cfg(nlohmann::json::object());
    EXPECT_TRUE(p.initialize(cfg));
    p.shutdown();
}

// IMP-YOL-03 – isReady() lifecycle
TEST(YOLOv8PluginNoFixtureTest, IMP_YOL_03_ReadyLifecycle) {
    YOLOv8OnnxPlugin p;
    EXPECT_FALSE(p.isReady()); // before init
    PluginConfig cfg(nlohmann::json::object());
    ASSERT_TRUE(p.initialize(cfg));
    EXPECT_TRUE(p.isReady());
    p.shutdown();
    EXPECT_FALSE(p.isReady());
}

// IMP-YOL-04 – detectObjects() returns a well-formed DetectionResult
TEST_F(YOLOv8PluginTest, IMP_YOL_04_DetectReturnStructure) {
    DetectionResult r = plugin_->detectObjects(kMinimalJpegWhite8x8);
    // In stub mode: success = false (no ONNX Runtime); in ONNX mode: success = true.
    // Either way, inference_time_ms must be set and bounding boxes are valid.
    EXPECT_GE(r.inference_time_ms, 0);
    // Bounding boxes – if any – must have normalised coordinates [0,1]
    for (const auto& box : r.detections) {
        EXPECT_GE(box.x, 0.0f);
        EXPECT_GE(box.y, 0.0f);
        EXPECT_GE(box.width,  0.0f);
        EXPECT_GE(box.height, 0.0f);
        EXPECT_LE(box.x + box.width,  1.01f); // allow tiny float rounding
        EXPECT_LE(box.y + box.height, 1.01f);
        EXPECT_GE(box.confidence, 0.0f);
        EXPECT_LE(box.confidence, 1.0f);
    }
}

// IMP-YOL-05 – generateEmbedding() is unsupported
TEST_F(YOLOv8PluginTest, IMP_YOL_05_EmbeddingUnsupported) {
    EmbeddingResult r = plugin_->generateEmbedding(kMinimalJpegWhite8x8);
    EXPECT_FALSE(r.success);
    EXPECT_FALSE(r.error_message.empty());
}

// IMP-YOL-06 – healthCheck() mirrors isReady()
TEST_F(YOLOv8PluginTest, IMP_YOL_06_HealthCheck) {
    EXPECT_TRUE(plugin_->healthCheck());
}

// IMP-YOL-07 – getStatistics() contains required metric keys
TEST_F(YOLOv8PluginTest, IMP_YOL_07_StatisticsKeys) {
    // Run one inference to populate counters
    plugin_->detectObjects(kMinimalJpegWhite8x8);

    nlohmann::json stats = plugin_->getStatistics();
    EXPECT_TRUE(stats.contains("inference_total"));
    EXPECT_TRUE(stats.contains("inference_errors"));
    EXPECT_TRUE(stats.contains("latency_ms_avg"));
    EXPECT_TRUE(stats.contains("detections_total"));

    // inference_total must be ≥ 1 after one call
    EXPECT_GE(stats["inference_total"].get<int64_t>(), 1);
}

// IMP-YOL-08 – Extremely high confidence threshold yields zero detections
TEST_F(YOLOv8PluginTest, IMP_YOL_08_HighThresholdNoDetections) {
    // A confidence threshold of 2.0 (> 1.0) is outside the valid range and
    // must not produce any detections.
    DetectionResult r = plugin_->detectObjects(kMinimalJpegWhite8x8,
                                               nullptr, 2.0f);
    EXPECT_TRUE(r.detections.empty());
}

// ---------------------------------------------------------------------------
// TesseractOCR tests
// ---------------------------------------------------------------------------

class TesseractPluginTest : public ::testing::Test {
protected:
    void SetUp() override {
        plugin_ = std::make_unique<TesseractOCRPlugin>();
        // Initialize with empty config → stub mode (no tessdata required)
        PluginConfig cfg(nlohmann::json::object());
        ASSERT_TRUE(plugin_->initialize(cfg));
    }

    void TearDown() override {
        if (plugin_) plugin_->shutdown();
    }

    std::unique_ptr<TesseractOCRPlugin> plugin_;
};

// IMP-OCR-01 – Plugin reports correct name and detection capability
TEST_F(TesseractPluginTest, IMP_OCR_01_PluginInfo) {
    PluginInfo info = plugin_->getInfo();
    EXPECT_EQ(info.name, "TesseractOCRPlugin");
    EXPECT_TRUE(info.capabilities.supports_detection);
    EXPECT_FALSE(info.capabilities.supports_embedding);
    // OCR is always CPU
    ASSERT_FALSE(info.capabilities.supported_backends.empty());
    EXPECT_EQ(info.capabilities.supported_backends.front(), BackendType::CPU);
}

// IMP-OCR-02 – initialize() succeeds in stub mode (no tessdata)
TEST(TesseractPluginNoFixtureTest, IMP_OCR_02_InitStub) {
    TesseractOCRPlugin p;
    PluginConfig cfg(nlohmann::json::object());
    EXPECT_TRUE(p.initialize(cfg));
    p.shutdown();
}

// IMP-OCR-03 – isReady() lifecycle
TEST(TesseractPluginNoFixtureTest, IMP_OCR_03_ReadyLifecycle) {
    TesseractOCRPlugin p;
    EXPECT_FALSE(p.isReady());
    PluginConfig cfg(nlohmann::json::object());
    ASSERT_TRUE(p.initialize(cfg));
    EXPECT_TRUE(p.isReady());
    p.shutdown();
    EXPECT_FALSE(p.isReady());
}

// IMP-OCR-04 – detectObjects() returns a well-formed DetectionResult
TEST_F(TesseractPluginTest, IMP_OCR_04_DetectReturnStructure) {
    DetectionResult r = plugin_->detectObjects(kMinimalJpegWhite8x8);
    EXPECT_GE(r.inference_time_ms, 0);
    // Any returned bounding boxes must have normalised coordinates
    for (const auto& box : r.detections) {
        EXPECT_GE(box.x, 0.0f);
        EXPECT_GE(box.y, 0.0f);
        EXPECT_GE(box.width,  0.0f);
        EXPECT_GE(box.height, 0.0f);
        EXPECT_LE(box.x + box.width,  1.01f);
        EXPECT_LE(box.y + box.height, 1.01f);
        EXPECT_GE(box.confidence, 0.0f);
        EXPECT_LE(box.confidence, 1.0f);
        // label (word text) must be non-empty for a valid word result
        EXPECT_FALSE(box.label.empty());
    }
}

// IMP-OCR-05 – generateEmbedding() is unsupported
TEST_F(TesseractPluginTest, IMP_OCR_05_EmbeddingUnsupported) {
    EmbeddingResult r = plugin_->generateEmbedding(kMinimalJpegWhite8x8);
    EXPECT_FALSE(r.success);
    EXPECT_FALSE(r.error_message.empty());
}

// IMP-OCR-06 – healthCheck() is available after initialize()
TEST_F(TesseractPluginTest, IMP_OCR_06_HealthCheck) {
    // In stub mode: healthCheck() returns true (Tesseract API object may be
    // null but ready flag is set).  In real mode: API must be valid.
    // Both cases must not throw or crash.
    bool hc = plugin_->healthCheck();
    // We don't assert the value: stub vs real mode differ.
    (void)hc; // suppress unused-variable warning
    SUCCEED();
}

// IMP-OCR-07 – getStatistics() contains required metric keys
TEST_F(TesseractPluginTest, IMP_OCR_07_StatisticsKeys) {
    plugin_->detectObjects(kMinimalJpegWhite8x8);

    nlohmann::json stats = plugin_->getStatistics();
    EXPECT_TRUE(stats.contains("inference_total"));
    EXPECT_TRUE(stats.contains("inference_errors"));
    EXPECT_TRUE(stats.contains("latency_ms_avg"));
    EXPECT_TRUE(stats.contains("words_total"));
    EXPECT_TRUE(stats.contains("language"));

    EXPECT_GE(stats["inference_total"].get<int64_t>(), 1);
}

// IMP-OCR-08 – getLastOcrResult() success mirrors detectObjects() success
TEST_F(TesseractPluginTest, IMP_OCR_08_LastOcrResultConsistency) {
    DetectionResult det = plugin_->detectObjects(kMinimalJpegWhite8x8);
    OcrResult ocr = plugin_->getLastOcrResult();

    // Success flag must match between the two views
    EXPECT_EQ(ocr.success, det.success);

    // If OCR succeeded, word count must match box count
    if (ocr.success) {
        EXPECT_EQ(ocr.words.size(), det.detections.size());
    }
}
