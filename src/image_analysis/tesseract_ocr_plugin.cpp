// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file tesseract_ocr_plugin.cpp
 * @brief Tesseract OCR plugin – Tesseract C API backend.
 *
 * Two compilation paths:
 *
 *  1. **With Tesseract** (`HAVE_TESSERACT` defined): a real `TessBaseAPI`
 *     instance is created and OCR is performed per call.  Word-level
 *     bounding boxes and confidence scores are extracted and normalised.
 *
 *  2. **Without Tesseract** (default): every `detectObjects()` call returns
 *     a well-formed `DetectionResult` with `success = false` and an
 *     explanatory error message, and `getLastOcrResult()` returns a matching
 *     `OcrResult`.  This path is used in unit-test builds and environments
 *     where Tesseract is not installed.
 *
 * ## Input image decoding
 * Tesseract requires raw pixel data.  When OpenCV is available the image bytes
 * are decoded with `cv::imdecode`.  Without OpenCV the image bytes are passed
 * directly to `SetImage` as a raw byte buffer — useful only when the caller
 * already provides unpacked pixel data (grayscale or RGB).
 *
 * ## OCR → DetectionResult mapping
 * Each recognised word is returned as a @ref DetectionResult::BoundingBox:
 * - `label`      = word text (UTF-8)
 * - `confidence` = Tesseract word confidence / 100  (normalised to 0–1)
 * - `x`, `y`, `width`, `height` = normalised to [0, 1] in image coordinates
 */

#include "plugins/tesseract_ocr_plugin.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#ifdef HAVE_TESSERACT
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#endif

#ifdef HAVE_OPENCV
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#endif

namespace themis {
namespace plugins {
namespace image {

// ---------------------------------------------------------------------------
// Implementation struct
// ---------------------------------------------------------------------------

struct TesseractOCRPlugin::Impl {
    // Configuration
    std::string tessdata_path;
    std::string language     = "eng";
    int  page_seg_mode       = 3; // PSM_AUTO
    int  dpi                 = 70;
    std::string whitelist;
    float min_confidence_cfg = 0.0f;  // 0–100 Tesseract scale stored as 0–1

    BackendType backend = BackendType::CPU;
    std::atomic<bool> ready{false};

    // Metrics
    std::atomic<int64_t> inference_total{0};
    std::atomic<int64_t> inference_errors{0};
    std::atomic<int64_t> latency_ms_sum{0};
    std::atomic<int64_t> latency_ms_count{0};
    std::atomic<int64_t> words_total{0};

#ifdef HAVE_TESSERACT
    std::unique_ptr<tesseract::TessBaseAPI> api;
    std::mutex tess_mtx; // Tesseract API is not thread-safe per instance
#endif

    // -----------------------------------------------------------------------
    // Initialise Tesseract
    // -----------------------------------------------------------------------
    bool init(const PluginConfig& config, BackendType /*requested*/) {
        tessdata_path    = config.get<std::string>("tessdata_path", "");
        language         = config.get<std::string>("language", "eng");
        page_seg_mode    = config.get<int>("page_seg_mode", 3);
        dpi              = config.get<int>("dpi", 70);
        whitelist        = config.get<std::string>("whitelist_chars", "");
        min_confidence_cfg = config.get<float>("min_confidence", 0.0f);
        backend = BackendType::CPU; // OCR is always CPU

#ifdef HAVE_TESSERACT
        api = std::make_unique<tesseract::TessBaseAPI>();
        const char* data_dir =
            tessdata_path.empty() ? nullptr : tessdata_path.c_str();
        if (api->Init(data_dir, language.c_str()) != 0) {
            api.reset();
            return false;
        }
        api->SetPageSegMode(
            static_cast<tesseract::PageSegMode>(page_seg_mode));
        if (!whitelist.empty()) {
            api->SetVariable("tessedit_char_whitelist", whitelist.c_str());
        }
        ready.store(true);
        return true;
#else
        // No Tesseract: initialise "ready" so callers can query the plugin
        // state without errors; inference returns explanatory error.
        ready.store(true);
        return true;
#endif
    }

    // -----------------------------------------------------------------------
    // Perform OCR on raw image bytes
    // -----------------------------------------------------------------------
    OcrResult runOcr(const std::vector<uint8_t>& image_data,
                     float effective_conf_01) {
        OcrResult ocr;

#ifdef HAVE_TESSERACT
        if (!api) {
            ocr.success       = false;
            ocr.error_message = "Tesseract API not initialised";
            return ocr;
        }

        std::lock_guard<std::mutex> lk(tess_mtx);

#ifdef HAVE_OPENCV
        cv::Mat encoded(1, static_cast<int>(image_data.size()), CV_8UC1,
                        const_cast<uint8_t*>(image_data.data()));
        cv::Mat img = cv::imdecode(encoded, cv::IMREAD_GRAYSCALE);
        if (img.empty()) {
            ocr.success       = false;
            ocr.error_message = "Image decoding failed";
            return ocr;
        }
        api->SetImage(img.data, img.cols, img.rows, 1, img.step);
        api->SetSourceResolution(dpi);
        const int img_w = img.cols;
        const int img_h = img.rows;
#else
        // Without OpenCV: interpret raw bytes as a flat grayscale buffer.
        // Only useful when the caller guarantees raw pixel data.
        const int est_side = static_cast<int>(
            std::sqrt(static_cast<double>(image_data.size())));
        api->SetImage(image_data.data(), est_side, est_side, 1, est_side);
        api->SetSourceResolution(dpi);
        const int img_w = est_side;
        const int img_h = est_side;
#endif

        // Extract full text
        std::unique_ptr<char[]> text_ptr(api->GetUTF8Text());
        ocr.full_text = text_ptr ? std::string(text_ptr.get()) : std::string{};

        // Extract per-word detail
        tesseract::ResultIterator* ri = api->GetIterator();
        tesseract::PageIteratorLevel level = tesseract::RIL_WORD;
        if (ri != nullptr) {
            do {
                const float word_conf = ri->Confidence(level);
                if ((word_conf / 100.0f) < effective_conf_01) {
                    continue;
                }
                std::unique_ptr<char[]> word_ptr(ri->GetUTF8Text(level));
                if (!word_ptr) {
                  continue;
                }

                int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
                ri->BoundingBox(level, &x1, &y1, &x2, &y2);

                OcrWord w;
                w.text       = std::string(word_ptr.get());
                w.confidence = word_conf;
                w.x          = static_cast<float>(x1) / img_w;
                w.y          = static_cast<float>(y1) / img_h;
                w.width      = static_cast<float>(x2 - x1) / img_w;
                w.height     = static_cast<float>(y2 - y1) / img_h;
                ocr.words.push_back(std::move(w));
            } while (ri->Next(level));
            delete ri;
        }

        api->Clear();
        ocr.success = true;
        return ocr;
#else
        (void)image_data;
        (void)effective_conf_01;
        ocr.success       = false;
        ocr.error_message =
            "TesseractOCRPlugin: built without Tesseract support "
            "(recompile with HAVE_TESSERACT)";
        return ocr;
#endif
    }
};

// ---------------------------------------------------------------------------
// TesseractOCRPlugin public implementation
// ---------------------------------------------------------------------------

TesseractOCRPlugin::TesseractOCRPlugin()
    : impl_(std::make_unique<Impl>()) {}

TesseractOCRPlugin::~TesseractOCRPlugin() {
    if (impl_ && impl_->ready.load()) {
        shutdown();
    }
}

PluginInfo TesseractOCRPlugin::getInfo() const {
    PluginInfo info;
    info.name        = "TesseractOCRPlugin";
    info.version     = "1.0.0";
    info.description = "OCR via Tesseract C API – extracts text and layout regions";
    info.author      = "ThemisDB Contributors";
    info.license     = "Apache-2.0";
    info.model_name  = "tesseract";
    info.model_version = "4.x / 5.x";
    info.supported_formats = {"jpeg", "png", "bmp", "tiff", "webp"};
    info.capabilities.supports_detection        = true;  // OCR regions as boxes
    info.capabilities.supports_batch_processing = false;
    info.capabilities.thread_safe               = false; // one TessBaseAPI per instance
    info.capabilities.supported_backends        = {BackendType::CPU};
    info.capabilities.min_memory_mb             = 128;
    info.capabilities.recommended_memory_mb     = 512;
    return info;
}

bool TesseractOCRPlugin::initialize(const PluginConfig& config,
                                     BackendType backend) {
    return impl_->init(config, backend);
}

void TesseractOCRPlugin::shutdown() {
    impl_->ready.store(false);
#ifdef HAVE_TESSERACT
    if (impl_->api) {
        std::lock_guard<std::mutex> lk(impl_->tess_mtx);
        impl_->api->End();
        impl_->api.reset();
    }
#endif
}

bool TesseractOCRPlugin::isReady() const {
    return impl_->ready.load();
}

BackendType TesseractOCRPlugin::getBackend() const {
    return BackendType::CPU;
}

DetectionResult TesseractOCRPlugin::detectObjects(
    const std::vector<uint8_t>& image_data,
    const ImageMetadata* /*metadata*/,
    float confidence_threshold) {
    if (!impl_->ready.load()) {
        DetectionResult err;
        err.success = false;
        err.error_message = "TesseractOCRPlugin not initialised";
        return err;
    }

    const float eff_conf = (confidence_threshold > 0.0f)
                               ? confidence_threshold
                               : impl_->min_confidence_cfg;

    const auto t0 = std::chrono::steady_clock::now();
    OcrResult ocr = impl_->runOcr(image_data, eff_conf);
    const auto t1 = std::chrono::steady_clock::now();
    ocr.inference_time_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    // Update metrics
    impl_->inference_total.fetch_add(1, std::memory_order_relaxed);
    if (!ocr.success) {
        impl_->inference_errors.fetch_add(1, std::memory_order_relaxed);
    } else {
        impl_->words_total.fetch_add(
            static_cast<int64_t>(ocr.words.size()),
            std::memory_order_relaxed);
    }
    impl_->latency_ms_sum.fetch_add(ocr.inference_time_ms,
                                    std::memory_order_relaxed);
    impl_->latency_ms_count.fetch_add(1, std::memory_order_relaxed);

    // Store last OCR result for getLastOcrResult()
    {
        std::lock_guard<std::mutex> lk(last_result_mtx_);
        last_ocr_result_ = ocr;
    }

    // Map OcrResult → DetectionResult (word boxes)
    DetectionResult det;
    det.success    = ocr.success;
    det.error_message = ocr.error_message;
    det.model_name = "tesseract";
    det.inference_time_ms = ocr.inference_time_ms;

    if (ocr.success) {
        for (const auto& w : ocr.words) {
            DetectionResult::BoundingBox box;
            box.x          = w.x;
            box.y          = w.y;
            box.width      = w.width;
            box.height     = w.height;
            box.label      = w.text;
            box.confidence = w.confidence / 100.0f; // normalise to [0,1]
            det.detections.push_back(box);
        }
    }

    return det;
}

EmbeddingResult TesseractOCRPlugin::generateEmbedding(
    const std::vector<uint8_t>& /*image_data*/,
    const ImageMetadata* /*metadata*/) {
    EmbeddingResult r;
    r.success = false;
    r.error_message = "TesseractOCRPlugin does not support embedding generation";
    return r;
}

bool TesseractOCRPlugin::healthCheck() const {
#ifdef HAVE_TESSERACT
    return impl_->ready.load() && impl_->api != nullptr;
#else
    return impl_->ready.load();
#endif
}

nlohmann::json TesseractOCRPlugin::getStatistics() const {
    const int64_t count =
        impl_->latency_ms_count.load(std::memory_order_relaxed);
    const double avg_ms = (count > 0)
        ? static_cast<double>(impl_->latency_ms_sum.load()) / count
        : 0.0;

    return {
        {"inference_total",  impl_->inference_total.load()},
        {"inference_errors", impl_->inference_errors.load()},
        {"latency_ms_avg",   avg_ms},
        {"words_total",      impl_->words_total.load()},
        {"language",         impl_->language},
        {"tessdata_path",    impl_->tessdata_path}
    };
}

OcrResult TesseractOCRPlugin::getLastOcrResult() const {
    std::lock_guard<std::mutex> lk(last_result_mtx_);
    return last_ocr_result_;
}

} // namespace image
} // namespace plugins
} // namespace themis
