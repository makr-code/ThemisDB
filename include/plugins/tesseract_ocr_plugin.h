// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file tesseract_ocr_plugin.h
 * @brief Tesseract OCR plugin implementing IImageAnalysisBackend.
 *
 * Wraps the Tesseract C API to extract text and layout regions from raw image
 * bytes.  The plugin maps onto the standard ThemisDB image analysis interface:
 * text content is returned via a custom @ref OcrResult structure (exposed
 * through `getStatistics()`), while the primary @ref detectObjects override
 * returns layout bounding boxes so that OCR regions are visible to consumers
 * that only know the generic detection API.
 *
 * ## Configuration keys (PluginConfig)
 * - `tessdata_path`  (string, optional) – path to `tessdata/` directory.
 *                    Defaults to the TESSDATA_PREFIX environment variable or
 *                    `/usr/share/tesseract-ocr/4.00/tessdata`.
 * - `language`       (string, default: "eng") – Tesseract language string,
 *                    e.g. `"eng"`, `"eng+deu"`.
 * - `page_seg_mode`  (int, default: 3) – Tesseract PSM constant (0–13).
 * - `dpi`            (int, default: 70) – source DPI hint for Tesseract.
 * - `whitelist_chars` (string, optional) – restrict recognised characters.
 * - `min_confidence` (float, default: 0.0) – drop word results below this
 *                    confidence (0–100 scale as returned by Tesseract).
 *
 * ## OCR result access
 * After a `detectObjects()` call the full OCR transcript is available via
 * `getLastOcrResult()`.  The result includes per-word bounding boxes, word
 * confidence scores, and the full page text.
 *
 * ## Thread-Safety
 * Each plugin instance holds its own `TessBaseAPI` object; instances may be
 * used concurrently across threads.  Do **not** share a single instance across
 * threads.
 *
 * ## Metrics (via getStatistics())
 * - `ocr_inference_total`   – total OCR calls
 * - `ocr_inference_errors`  – failed calls
 * - `ocr_latency_ms_avg`    – rolling average latency
 * - `ocr_words_total`       – total words recognised
 */

#pragma once

#include "plugins/image_analysis_interface.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace themis {
namespace plugins {
namespace image {

/**
 * @brief Per-word OCR result with confidence and bounding box.
 */
struct OcrWord {
    std::string text;        ///< Recognised word text (UTF-8)
    float confidence = 0.0f; ///< Tesseract confidence 0–100
    /// Normalised bounding box [0, 1] in image coordinates
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

/**
 * @brief Full OCR result for one image.
 */
struct OcrResult {
    bool success = false;
    std::string error_message;
    std::string full_text;          ///< Complete page text
    std::vector<OcrWord> words;     ///< Per-word detail
    int64_t inference_time_ms = 0;
};

/**
 * @brief Tesseract OCR plugin.
 *
 * Implements @ref IImageAnalysisBackend using the Tesseract C API.  When
 * Tesseract is not present at compile-time (HAVE_TESSERACT undefined) the
 * plugin initialises successfully but returns informative error results
 * instead of crashing.
 */
class TesseractOCRPlugin : public IImageAnalysisBackend {
public:
    TesseractOCRPlugin();
    ~TesseractOCRPlugin() override;

    // -------------------------------------------------------------------------
    // IImageAnalysisBackend interface
    // -------------------------------------------------------------------------

    /// @brief Returns plugin metadata.
    PluginInfo getInfo() const override;

    /**
     * @brief Initialise the plugin and create the Tesseract engine.
     *
     * @param config  Plugin configuration (see class-level doc for keys).
     * @param backend Ignored (Tesseract is CPU-only); stored for reporting.
     * @return true on success; false if Tesseract initialisation fails.
     */
    bool initialize(const PluginConfig& config,
                    BackendType backend = BackendType::AUTO) override;

    /// @brief Destroy Tesseract API handle and release resources.
    void shutdown() override;

    /// @brief Returns true after successful `initialize()`.
    bool isReady() const override;

    /// @brief Always returns BackendType::CPU.
    BackendType getBackend() const override;

    /**
     * @brief Perform OCR and return text regions as bounding boxes.
     *
     * Each recognised word is returned as a @ref DetectionResult::BoundingBox
     * with `label` set to the word text and `confidence` set to the Tesseract
     * word confidence (normalised 0–1).  The full transcript is accessible via
     * `getLastOcrResult()`.
     *
     * @param image_data           Raw image bytes (JPEG / PNG / BMP / TIFF).
     * @param metadata             Optional image metadata; may be nullptr.
     * @param confidence_threshold Minimum word confidence (0–1); defaults to
     *                             the `min_confidence` config value when 0.
     * @return DetectionResult where each "detection" is an OCR word box.
     */
    DetectionResult detectObjects(
        const std::vector<uint8_t>& image_data,
        const ImageMetadata* metadata = nullptr,
        float confidence_threshold = 0.0f) override;

    /**
     * @brief generateEmbedding – not supported by this plugin.
     *
     * Returns an error result; use the ONNX CLIP plugin for embeddings.
     */
    EmbeddingResult generateEmbedding(
        const std::vector<uint8_t>& image_data,
        const ImageMetadata* metadata = nullptr) override;

    /// @brief Runs a trivial sanity check on the Tesseract handle.
    bool healthCheck() const override;

    /**
     * @brief Returns JSON statistics.
     *
     * Keys: `inference_total`, `inference_errors`, `latency_ms_avg`,
     *       `words_total`, `language`, `tessdata_path`.
     */
    nlohmann::json getStatistics() const override;

    // -------------------------------------------------------------------------
    // OCR-specific accessors
    // -------------------------------------------------------------------------

    /**
     * @brief Return the full OCR result from the most recent `detectObjects()`
     *        call on this instance.
     *
     * @note Thread-Safety: caller must not call `detectObjects()` concurrently
     *       on the same instance; use separate plugin instances per thread.
     */
    OcrResult getLastOcrResult() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    mutable std::mutex last_result_mtx_;
    OcrResult last_ocr_result_;
};

} // namespace image
} // namespace plugins
} // namespace themis

#ifndef THEMIS_IMAGE_PLUGIN_DISABLE_EXPORT
THEMIS_IMAGE_PLUGIN(themis::plugins::image::TesseractOCRPlugin)
#endif
