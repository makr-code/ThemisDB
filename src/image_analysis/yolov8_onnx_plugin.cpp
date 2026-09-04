// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file yolov8_onnx_plugin.cpp
 * @brief YOLOv8 object detection plugin – ONNX Runtime backend.
 *
 * The implementation is divided into two compilation paths:
 *
 *  1. **With ONNX Runtime** (`HAVE_ONNXRUNTIME` defined): a real ONNX Runtime
 *     session is created and YOLOv8 post-processing (box decoding + NMS) is
 *     performed in this file.
 *
 *  2. **Without ONNX Runtime** (default): every `detectObjects()` call returns
 *     a well-formed `DetectionResult` with `success = false` and an explanatory
 *     `error_message`.  This path is used in unit-test builds and environments
 *     where ONNX Runtime is not installed.
 *
 * ## YOLOv8 output tensor layout (when HAVE_ONNXRUNTIME)
 * The standard YOLOv8 detection export produces a single output tensor of shape
 * `[1, 84, 8400]` (for 640×640 input):
 * - Axis 1 index 0-3: cx, cy, w, h (centre-format, in pixels)
 * - Axis 1 index 4-83: class confidence scores (80 COCO classes)
 *
 * Post-processing steps:
 * 1. Transpose to `[8400, 84]` for cache-friendly iteration.
 * 2. For each anchor: objectness = max(class_confs); skip if < threshold.
 * 3. Convert cx/cy/w/h → x1/y1/x2/y2 and normalise to [0, 1].
 * 4. Apply greedy NMS per class (IoU threshold from config).
 * 5. Cap total detections at `max_detections`.
 */

#include "plugins/yolov8_onnx_plugin.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <fstream>
#include <mutex>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef HAVE_ONNXRUNTIME
#include <onnxruntime_cxx_api.h>
#endif

#ifdef HAVE_OPENCV
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#endif

namespace themis {
namespace plugins {
namespace image {

// ---------------------------------------------------------------------------
// COCO 80-class label table (default)
// ---------------------------------------------------------------------------
namespace {

static const std::vector<std::string> kCocoLabels = {
    "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train",
    "truck", "boat", "traffic light", "fire hydrant", "stop sign",
    "parking meter", "bench", "bird", "cat", "dog", "horse", "sheep", "cow",
    "elephant", "bear", "zebra", "giraffe", "backpack", "umbrella", "handbag",
    "tie", "suitcase", "frisbee", "skis", "snowboard", "sports ball", "kite",
    "baseball bat", "baseball glove", "skateboard", "surfboard",
    "tennis racket", "bottle", "wine glass", "cup", "fork", "knife", "spoon",
    "bowl", "banana", "apple", "sandwich", "orange", "broccoli", "carrot",
    "hot dog", "pizza", "donut", "cake", "chair", "couch", "potted plant",
    "bed", "dining table", "toilet", "tv", "laptop", "mouse", "remote",
    "keyboard", "cell phone", "microwave", "oven", "toaster", "sink",
    "refrigerator", "book", "clock", "vase", "scissors", "teddy bear",
    "hair drier", "toothbrush"};

/// Load one label per line from a text file.
static std::vector<std::string> loadLabels(const std::string& path) {
    std::vector<std::string> labels;
    std::ifstream in(path);
    if (!in.is_open()) {
        return {};
    }
    std::string line = {};
    while (std::getline(in, line)) {
        if (!line.empty()) {
            labels.push_back(line);
        }
    }
    return labels;
}

/// Compute Intersection-over-Union for two XYXY boxes (normalised).
static float iouXYXY(float x1a, float y1a, float x2a, float y2a,
                     float x1b, float y1b, float x2b, float y2b) {
    const float ix1 = std::max(x1a, x1b);
    const float iy1 = std::max(y1a, y1b);
    const float ix2 = std::min(x2a, x2b);
    const float iy2 = std::min(y2a, y2b);
    const float inter = std::max(0.0f, ix2 - ix1) * std::max(0.0f, iy2 - iy1);
    if (inter == 0.0f) {
      return 0.0f;
    }
    const float areaA = (x2a - x1a) * (y2a - y1a);
    const float areaB = (x2b - x1b) * (y2b - y1b);
    return inter / (areaA + areaB - inter);
}

} // namespace

// ---------------------------------------------------------------------------
// Implementation struct
// ---------------------------------------------------------------------------

struct YOLOv8OnnxPlugin::Impl {
    // Configuration
    std::string model_path;
    std::string labels_path;
    int input_width  = 640;
    int input_height = 640;
    float conf_threshold  = 0.25f;
    float nms_iou_threshold = 0.45f;
    int max_detections = 100;
    BackendType backend = BackendType::CPU;

    std::vector<std::string> labels = kCocoLabels;

    // State
    std::atomic<bool> ready{false};

    // Metrics (atomic for lock-free updates in detectObjects)
    std::atomic<int64_t> inference_total{0};
    std::atomic<int64_t> inference_errors{0};
    std::atomic<int64_t> latency_ms_sum{0};
    std::atomic<int64_t> latency_ms_count{0};
    std::atomic<int64_t> detections_total{0};

#ifdef HAVE_ONNXRUNTIME
    Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "yolov8"};
    std::unique_ptr<Ort::Session> session;
    Ort::SessionOptions session_opts;
    std::string input_name;
    std::string output_name;
#endif

    // -----------------------------------------------------------------------
    // Initialise ONNX session
    // -----------------------------------------------------------------------
    bool init(const PluginConfig& config, BackendType requested_backend) {
        model_path  = config.get<std::string>("model_path", "");
        labels_path = config.get<std::string>("labels_path", "");
        input_width  = config.get<int>("input_width", 640);
        input_height = config.get<int>("input_height", 640);
        conf_threshold    = config.get<float>("confidence_threshold", 0.25f);
        nms_iou_threshold = config.get<float>("nms_iou_threshold", 0.45f);
        max_detections    = config.get<int>("max_detections", 100);
        backend = requested_backend;

        if (!labels_path.empty()) {
            auto loaded = loadLabels(labels_path);
            if (!loaded.empty()) {
                labels = std::move(loaded);
            }
        }

#ifdef HAVE_ONNXRUNTIME
        if (model_path.empty()) {
            return false;
        }

        session_opts.SetGraphOptimizationLevel(
            GraphOptimizationLevel::ORT_ENABLE_ALL);
        session_opts.SetIntraOpNumThreads(1);

        if (requested_backend == BackendType::CUDA ||
            requested_backend == BackendType::AUTO) {
            // Try CUDA EP; silently continue on CPU if unavailable.
            try {
                OrtCUDAProviderOptions cuda_opts{};
                session_opts.AppendExecutionProvider_CUDA(cuda_opts);
                backend = BackendType::CUDA;
            } catch (...) {
                backend = BackendType::CPU;
            }
        }

        try {
#ifdef _WIN32
            std::wstring wpath(model_path.begin(), model_path.end());
            session = std::make_unique<Ort::Session>(env, wpath.c_str(),
                                                     session_opts);
#else
            session = std::make_unique<Ort::Session>(env, model_path.c_str(),
                                                     session_opts);
#endif
        } catch (const Ort::Exception& e) {
            (void)e; // logged via error result in detectObjects
            return false;
        }

        Ort::AllocatorWithDefaultOptions alloc;
        input_name  = session->GetInputNameAllocated(0, alloc).get();
        output_name = session->GetOutputNameAllocated(0, alloc).get();
        ready.store(true);
        return true;
#else
        // Without ONNX Runtime: mark ready so callers can proceed; actual
        // inference returns an informative error.
        (void)model_path;
        ready.store(true);
        return true;
#endif
    }

    // -----------------------------------------------------------------------
    // Preprocess: decode image bytes → float32 CHW tensor [1,3,H,W]
    // -----------------------------------------------------------------------
    bool preprocess(const std::vector<uint8_t>& image_data,
                    std::vector<float>& tensor_out,
                    int& orig_w, int& orig_h) const {
#ifdef HAVE_OPENCV
        cv::Mat encoded(1, static_cast<int>(image_data.size()), CV_8UC1,
                        const_cast<uint8_t*>(image_data.data()));
        cv::Mat img = cv::imdecode(encoded, cv::IMREAD_COLOR);
        if (img.empty()) {
          return false;
        }

        orig_w = img.cols;
        orig_h = img.rows;

        cv::Mat resized;
        cv::resize(img, resized, cv::Size(input_width, input_height));
        cv::Mat rgb;
        cv::cvtColor(resized, rgb, cv::COLOR_BGR2RGB);

        // Normalise to [0, 1] and convert to planar CHW layout
        tensor_out.resize(static_cast<std::size_t>(3) * static_cast<std::size_t>(input_height) * static_cast<std::size_t>(input_width));
        for (int c = 0; c < 3; ++c) {
            for (int y = 0; y < input_height; ++y) {
                for (int x = 0; x < input_width; ++x) {
                    tensor_out[static_cast<std::size_t>(c) * static_cast<std::size_t>(input_height) * static_cast<std::size_t>(input_width) +
                               static_cast<std::size_t>(y) * static_cast<std::size_t>(input_width) + static_cast<std::size_t>(x)] =
                        rgb.at<cv::Vec3b>(y, x)[c] / 255.0f;
                }
            }
        }
        return true;
#else
        // Without OpenCV: fill a black tensor so the session can still run.
        (void)image_data;
        orig_w = input_width;
        orig_h = input_height;
        tensor_out.assign(static_cast<std::size_t>(3) * static_cast<std::size_t>(input_height) * static_cast<std::size_t>(input_width), 0.0f);
        return true;
#endif
    }

    // -----------------------------------------------------------------------
    // Post-process YOLOv8 output tensor → DetectionResult
    // Shape: [1, 84, 8400]  (cx, cy, w, h, 80 class scores)
    // -----------------------------------------------------------------------
    DetectionResult postprocess(const std::vector<float>& raw,
                                int orig_w, int orig_h,
                                float effective_conf) const {
        DetectionResult result;
        result.success = true;
        result.model_name = "yolov8";

        const int num_classes = static_cast<int>(labels.size());
        const int num_anchors = 8400; // standard for 640×640
        // raw layout: channel-major [84, 8400] – stride = num_anchors
        if (static_cast<int>(raw.size()) < static_cast<size_t>(84) * num_anchors) {
            result.success = false;
            result.error_message = "Unexpected output tensor size";
            return result;
        }

        struct Candidate {
            float x1, y1, x2, y2;
            float conf = {};
            int   cls = {};
        };
        std::vector<Candidate> candidates;
        candidates.reserve(256);

        const float scale_x = static_cast<float>(orig_w);
        const float scale_y = static_cast<float>(orig_h);

        for (int a = 0; a < num_anchors; ++a) {
            const float cx = raw[0 * num_anchors + a] / input_width;
            const float cy = raw[1 * num_anchors + a] / input_height;
            const float bw = raw[2 * num_anchors + a] / input_width;
            const float bh = raw[3 * num_anchors + a] / input_height;

            // Find best class
            float best_conf = 0.0f;
            int   best_cls  = 0;
            for (int c = 0; c < num_classes && c < 80; ++c) {
                const float s = raw[(4 + c) * num_anchors + a];
                if (s > best_conf) {
                    best_conf = s;
                    best_cls  = c;
                }
            }

            if (best_conf < effective_conf) {
              continue;
            }

            Candidate cand;
            cand.x1   = std::max(0.0f, cx - bw / 2.0f);
            cand.y1   = std::max(0.0f, cy - bh / 2.0f);
            cand.x2   = std::min(1.0f, cx + bw / 2.0f);
            cand.y2   = std::min(1.0f, cy + bh / 2.0f);
            cand.conf = best_conf;
            cand.cls  = best_cls;
            candidates.push_back(cand);
        }

        // Greedy NMS per class
        std::sort(candidates.begin(), candidates.end(),
                  [](const Candidate& a, const Candidate& b) {
                      return a.conf > b.conf;
                  });

        std::vector<bool> suppressed(candidates.size(), false);
        for (size_t i = 0; i < candidates.size(); ++i) {
            if (suppressed[i]) {
              continue;
            }
            auto& ci = candidates[i];

            DetectionResult::BoundingBox box;
            box.x          = ci.x1;
            box.y          = ci.y1;
            box.width      = ci.x2 - ci.x1;
            box.height     = ci.y2 - ci.y1;
            box.confidence = ci.conf;
            box.label = (ci.cls < static_cast<int>(labels.size()))
                            ? labels[ci.cls]
                            : std::to_string(ci.cls);
            result.detections.push_back(box);

            if (static_cast<int>(result.detections.size()) >=
                max_detections) {
                break;
            }

            for (size_t j = i + 1; j < candidates.size(); ++j) {
                if (suppressed[j]) {
                  continue;
                }
                if (candidates[j].cls != ci.cls) {
                  continue;
                }
                auto& cj = candidates[j];
                if (iouXYXY(ci.x1, ci.y1, ci.x2, ci.y2,
                             cj.x1, cj.y1, cj.x2, cj.y2) >
                    nms_iou_threshold) {
                    suppressed[j] = true;
                }
            }
        }

        (void)scale_x;
        (void)scale_y;
        return result;
    }

    // -----------------------------------------------------------------------
    // Run single-image inference
    // -----------------------------------------------------------------------
    DetectionResult runInference(const std::vector<uint8_t>& image_data,
                                 float effective_conf) {
        DetectionResult result;

#ifdef HAVE_ONNXRUNTIME
        if (!session) {
            result.success = false;
            result.error_message = "ONNX session not initialised";
            return result;
        }

        int orig_w = 0, orig_h = 0;
        std::vector<float> input_tensor = {};

        if (!preprocess(image_data, input_tensor, orig_w, orig_h)) {
            result.success = false;
            result.error_message = "Image preprocessing failed";
            return result;
        }

        std::array<int64_t, 4> input_shape = {1, 3,
            static_cast<int64_t>(input_height),
            static_cast<int64_t>(input_width)};
        auto mem_info = Ort::MemoryInfo::CreateCpu(
            OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);
        Ort::Value input_val = Ort::Value::CreateTensor<float>(
            mem_info, input_tensor.data(), input_tensor.size(),
            input_shape.data(), input_shape.size());

        const char* in_names[]  = {input_name.c_str()};
        const char* out_names[] = {output_name.c_str()};

        std::vector<Ort::Value> outputs;
        try {
            outputs = session->Run(Ort::RunOptions{nullptr}, in_names,
                                   &input_val, 1, out_names, 1);
        } catch (const Ort::Exception& e) {
            result.success = false;
            result.error_message = std::string("ONNX inference error: ") + e.what();
            return result;
        }

        const float* raw = outputs[0].GetTensorData<float>();
        const auto& shape = outputs[0].GetTensorTypeAndShapeInfo().GetShape();
        size_t total = 1;
        for (auto d : shape) {
          total *= static_cast<size_t>(d);
        }
        std::vector<float> raw_vec(raw, raw + total);

        result = postprocess(raw_vec, orig_w, orig_h, effective_conf);
#else
        result.success = false;
        result.error_message =
            "YOLOv8OnnxPlugin: built without ONNX Runtime support "
            "(recompile with HAVE_ONNXRUNTIME)";
#endif
        return result;
    }
};

// ---------------------------------------------------------------------------
// YOLOv8OnnxPlugin public implementation
// ---------------------------------------------------------------------------

YOLOv8OnnxPlugin::YOLOv8OnnxPlugin()
    : impl_(std::make_shared<Impl>()) {}

YOLOv8OnnxPlugin::~YOLOv8OnnxPlugin() {
    if (impl_ && impl_->ready.load()) {
        shutdown();
    }
}

PluginInfo YOLOv8OnnxPlugin::getInfo() const {
    PluginInfo info;
    info.name        = "YOLOv8OnnxPlugin";
    info.version     = "1.0.0";
    info.description = "YOLOv8 object detection via ONNX Runtime";
    info.author      = "ThemisDB Contributors";
    info.license     = "Apache-2.0";
    info.model_name  = "yolov8";
    info.model_version = "8.x";
    info.supported_formats = {"jpeg", "png", "bmp", "webp"};
    info.capabilities.supports_detection       = true;
    info.capabilities.supports_batch_processing = false;
    info.capabilities.thread_safe              = false; // one session per instance
    info.capabilities.supported_backends =
        {BackendType::CPU, BackendType::CUDA, BackendType::AUTO};
    info.capabilities.min_memory_mb         = 256;
    info.capabilities.recommended_memory_mb = 1024;
    return info;
}

bool YOLOv8OnnxPlugin::initialize(const PluginConfig& config,
                                   BackendType backend) {
    std::lock_guard<std::mutex> lk(impl_swap_mtx_);
    auto new_impl = std::make_shared<Impl>();
    if (!new_impl->init(config, backend)) {
        return false;
    }
    impl_ = std::move(new_impl);
    return true;
}

void YOLOv8OnnxPlugin::shutdown() {
    std::lock_guard<std::mutex> lk(impl_swap_mtx_);
    if (impl_) {
        impl_->ready.store(false);
#ifdef HAVE_ONNXRUNTIME
        impl_->session.reset();
#endif
    }
}

bool YOLOv8OnnxPlugin::isReady() const {
    std::lock_guard<std::mutex> lk(impl_swap_mtx_);
    return impl_ && impl_->ready.load();
}

BackendType YOLOv8OnnxPlugin::getBackend() const {
    std::lock_guard<std::mutex> lk(impl_swap_mtx_);
    return impl_ ? impl_->backend : BackendType::CPU;
}

DetectionResult YOLOv8OnnxPlugin::detectObjects(
    const std::vector<uint8_t>& image_data,
    const ImageMetadata* /*metadata*/,
    float confidence_threshold) {
    std::shared_ptr<Impl> local;
    {
        std::lock_guard<std::mutex> lk(impl_swap_mtx_);
        local = impl_;
    }

    if (!local || !local->ready.load()) {
        DetectionResult err;
        err.success = false;
        err.error_message = "YOLOv8OnnxPlugin not initialised";
        return err;
    }

    const float eff_conf = (confidence_threshold > 0.0f)
                               ? confidence_threshold
                               : local->conf_threshold;

    const auto t0 = std::chrono::steady_clock::now();
    DetectionResult result = local->runInference(image_data, eff_conf);
    const auto t1 = std::chrono::steady_clock::now();
    result.inference_time_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    // Update metrics
    local->inference_total.fetch_add(1, std::memory_order_relaxed);
    if (!result.success) {
        local->inference_errors.fetch_add(1, std::memory_order_relaxed);
    } else {
        local->detections_total.fetch_add(
            static_cast<int64_t>(result.detections.size()),
            std::memory_order_relaxed);
    }
    local->latency_ms_sum.fetch_add(result.inference_time_ms,
                                    std::memory_order_relaxed);
    local->latency_ms_count.fetch_add(1, std::memory_order_relaxed);

    result.model_name = "yolov8";
    return result;
}

EmbeddingResult YOLOv8OnnxPlugin::generateEmbedding(
    const std::vector<uint8_t>& /*image_data*/,
    const ImageMetadata* /*metadata*/) {
    EmbeddingResult r;
    r.success = false;
    r.error_message = "YOLOv8OnnxPlugin does not support embedding generation";
    return r;
}

bool YOLOv8OnnxPlugin::healthCheck() const {
    std::lock_guard<std::mutex> lk(impl_swap_mtx_);
    return impl_ && impl_->ready.load();
}

nlohmann::json YOLOv8OnnxPlugin::getStatistics() const {
    std::shared_ptr<Impl> local;
    {
        std::lock_guard<std::mutex> lk(impl_swap_mtx_);
        local = impl_;
    }
    if (!local) {
      return nlohmann::json::object();
    }

    const int64_t count = local->latency_ms_count.load(std::memory_order_relaxed);
    const double avg_ms = (count > 0)
        ? static_cast<double>(local->latency_ms_sum.load()) / count
        : 0.0;

    return {
        {"inference_total",   local->inference_total.load()},
        {"inference_errors",  local->inference_errors.load()},
        {"latency_ms_avg",    avg_ms},
        {"detections_total",  local->detections_total.load()},
        {"backend",           static_cast<int>(local->backend)},
        {"model_path",        local->model_path}
    };
}

bool YOLOv8OnnxPlugin::reloadModel(const PluginConfig& new_config) {
    auto new_impl = std::make_shared<Impl>();
    if (!new_impl->init(new_config, impl_ ? impl_->backend : BackendType::AUTO)) {
        return false;
    }
    std::lock_guard<std::mutex> lk(impl_swap_mtx_);
    impl_ = std::move(new_impl);
    return true;
}

} // namespace image
} // namespace plugins
} // namespace themis
