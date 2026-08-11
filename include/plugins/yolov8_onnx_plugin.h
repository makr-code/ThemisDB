// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file yolov8_onnx_plugin.h
 * @brief YOLOv8 object detection plugin using ONNX Runtime.
 *
 * Implements @ref themis::plugins::image::IImageAnalysisBackend for
 * bounding-box object detection via a YOLOv8-n/s/m/l/x ONNX model.
 *
 * ## Configuration keys (PluginConfig)
 * - `model_path`         (string, required) – path to the YOLOv8 `.onnx` file.
 * - `labels_path`        (string, optional) – path to a COCO-style labels file
 *                         (one label per line).  Defaults to 80 COCO class names.
 * - `input_width`        (int, default: 640) – model input width in pixels.
 * - `input_height`       (int, default: 640) – model input height in pixels.
 * - `confidence_threshold` (float, default: 0.25) – minimum object confidence.
 * - `nms_iou_threshold`  (float, default: 0.45) – IoU threshold for NMS.
 * - `max_detections`     (int, default: 100) – cap on returned detections.
 * - `enable_mmap_loading` (bool, default: false) – memory-map model file.
 *
 * ## Thread-Safety
 * `detectObjects()` and `healthCheck()` are safe to call concurrently.
 * `initialize()` / `shutdown()` / `reloadModel()` must not overlap.
 *
 * ## Metrics (Prometheus counters exported via getStatistics())
 * - `yolov8_inference_total`      – total inference calls
 * - `yolov8_inference_errors`     – failed inference calls
 * - `yolov8_latency_ms_sum`       – cumulative latency for histogram
 * - `yolov8_latency_ms_count`     – sample count for histogram
 * - `yolov8_detections_total`     – total bounding boxes produced
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
 * @brief YOLOv8 ONNX object detection plugin.
 *
 * Wraps an ONNX Runtime session loaded from a YOLOv8 export.  When ONNX
 * Runtime is not present at compile-time the plugin initialises successfully
 * but every `detectObjects()` call returns an informative error result rather
 * than crashing.
 */
class YOLOv8OnnxPlugin : public IImageAnalysisBackend {
public:
    YOLOv8OnnxPlugin();
    ~YOLOv8OnnxPlugin() override;

    // -------------------------------------------------------------------------
    // IImageAnalysisBackend interface
    // -------------------------------------------------------------------------

    /// @brief Returns plugin metadata (name, version, capabilities).
    PluginInfo getInfo() const override;

    /**
     * @brief Initialise the plugin and load the ONNX model.
     *
     * @param config  Plugin configuration (see class-level doc for keys).
     * @param backend Preferred execution backend; falls back to CPU when the
     *                requested provider is unavailable.
     * @return true on success; false if `model_path` is missing or the ONNX
     *         session cannot be created.
     */
    bool initialize(const PluginConfig& config,
                    BackendType backend = BackendType::AUTO) override;

    /// @brief Unload model and release ONNX Runtime session.
    void shutdown() override;

    /// @brief Returns true after a successful `initialize()` call.
    bool isReady() const override;

    /// @brief Returns the currently active backend type.
    BackendType getBackend() const override;

    /**
     * @brief Detect objects in a raw image.
     *
     * Preprocesses the image to the configured input resolution, runs ONNX
     * inference, applies Non-Maximum Suppression, and returns normalised
     * bounding boxes.
     *
     * @param image_data           Raw image bytes (JPEG / PNG / BMP).
     * @param metadata             Optional image metadata; may be nullptr.
     * @param confidence_threshold Per-call confidence override; uses config
     *                             default when ≤ 0.
     * @return DetectionResult with normalised bounding boxes [0, 1].
     */
    DetectionResult detectObjects(
        const std::vector<uint8_t>& image_data,
        const ImageMetadata* metadata = nullptr,
        float confidence_threshold = 0.0f) override;

    /**
     * @brief generateEmbedding – not supported by this plugin.
     *
     * Always returns an error result; use the ONNX CLIP plugin for embeddings.
     */
    EmbeddingResult generateEmbedding(
        const std::vector<uint8_t>& image_data,
        const ImageMetadata* metadata = nullptr) override;

    /// @brief Validates ONNX session is responsive; returns false after shutdown.
    bool healthCheck() const override;

    /**
     * @brief Returns JSON statistics object.
     *
     * Keys: `inference_total`, `inference_errors`, `latency_ms_avg`,
     *       `detections_total`, `backend`, `model_path`.
     */
    nlohmann::json getStatistics() const override;

    // -------------------------------------------------------------------------
    // Hot-swap
    // -------------------------------------------------------------------------

    /**
     * @brief Reload the ONNX model without restarting ThemisDB.
     *
     * In-flight `detectObjects()` calls are allowed to finish.  The new
     * session is created before the old one is swapped out, so inference
     * remains available throughout.
     *
     * @param new_config New configuration (must include a valid `model_path`).
     * @return true on success; false on invalid config or session creation error.
     */
    bool reloadModel(const PluginConfig& new_config);

private:
    struct Impl;
    mutable std::mutex impl_swap_mtx_;
    std::shared_ptr<Impl> impl_;
};

} // namespace image
} // namespace plugins
} // namespace themis

#ifndef THEMIS_IMAGE_PLUGIN_DISABLE_EXPORT
THEMIS_IMAGE_PLUGIN(themis::plugins::image::YOLOv8OnnxPlugin)
#endif
