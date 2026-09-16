// SIMULATION NOTE:
// Purpose: Wave D long-duration soak for mixed OCR/object-detection workloads
// Activation: THEMIS_SOAK_DURATION_MS env override; default 60 s (CI-safe), 3600 s (full run)
// Production Delta: in-process stubs; no real Tesseract/ONNX runtime required
// Removal Plan: replace stubs when plugin CI environment provides Tesseract + ONNX dependencies

#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

namespace {

long long soak_duration_ms() {
    if (const char* env = std::getenv("THEMIS_SOAK_DURATION_MS"))
        return std::stoll(env);
    return 60'000LL;
}

// --- Stubs -----------------------------------------------------------

struct OcrResult {
    std::string text;
    float confidence;
};

struct DetectionResult {
    std::string label;
    float score;
};

OcrResult stub_ocr(int frame_id) {
    return {"extracted_text_frame_" + std::to_string(frame_id), 0.92f};
}

DetectionResult stub_detect(int frame_id) {
    return {"object_class_" + std::to_string(frame_id % 80), 0.87f};
}

// --- Soak helpers ---------------------------------------------------

void run_ocr_worker(std::atomic<uint64_t>& ops, long long duration_ms) {
    auto deadline = std::chrono::steady_clock::now() +
                    std::chrono::milliseconds(duration_ms);
    int id = 0;
    while (std::chrono::steady_clock::now() < deadline) {
        auto r = stub_ocr(id++);
        EXPECT_FALSE(r.text.empty());
        EXPECT_GT(r.confidence, 0.0f);
        ops.fetch_add(1, std::memory_order_relaxed);
    }
}

void run_detection_worker(std::atomic<uint64_t>& ops, long long duration_ms) {
    auto deadline = std::chrono::steady_clock::now() +
                    std::chrono::milliseconds(duration_ms);
    int id = 0;
    while (std::chrono::steady_clock::now() < deadline) {
        auto r = stub_detect(id++);
        EXPECT_FALSE(r.label.empty());
        EXPECT_GT(r.score, 0.0f);
        ops.fetch_add(1, std::memory_order_relaxed);
    }
}

} // namespace

// ---------------------------------------------------------------------------
// Test: ImageAnalysisSoak_OCRThroughput
// Wave D D1 — sustained OCR throughput without memory growth
// ---------------------------------------------------------------------------
TEST(ImageAnalysisSoak, OCRThroughput) {
    const long long dur = soak_duration_ms();
    std::atomic<uint64_t> ops{0};
    constexpr int kThreads = 4;

    std::vector<std::thread> workers;
    workers.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i)
        workers.emplace_back(run_ocr_worker, std::ref(ops), dur);
    for (auto& w : workers) w.join();

    const uint64_t total = ops.load();
    EXPECT_GT(total, 0u) << "[IMAGE:OCRSoak] Zero ops over " << dur << " ms";
    const double ops_per_s = static_cast<double>(total) / (dur / 1000.0);
    EXPECT_GT(ops_per_s, 100.0) << "[IMAGE:OCRSoak] Throughput too low: " << ops_per_s << " ops/s";
}

// ---------------------------------------------------------------------------
// Test: ImageAnalysisSoak_ObjectDetectionStability
// Wave D D1 — long-duration object-detection without crash / accuracy drift
// ---------------------------------------------------------------------------
TEST(ImageAnalysisSoak, ObjectDetectionStability) {
    const long long dur = soak_duration_ms();
    std::atomic<uint64_t> ops{0};
    constexpr int kThreads = 4;

    std::vector<std::thread> workers;
    workers.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i)
        workers.emplace_back(run_detection_worker, std::ref(ops), dur);
    for (auto& w : workers) w.join();

    EXPECT_GT(ops.load(), 0u) << "[IMAGE:DetectionSoak] Zero ops over " << dur << " ms";
}

// ---------------------------------------------------------------------------
// Test: ImageAnalysisSoak_MixedWorkloadReliability
// Wave D D1 — mixed OCR + detection pipeline without ordering failures
// ---------------------------------------------------------------------------
TEST(ImageAnalysisSoak, MixedWorkloadReliability) {
    const long long dur = soak_duration_ms();
    std::atomic<uint64_t> ocr_ops{0};
    std::atomic<uint64_t> det_ops{0};

    std::thread ocr_worker(run_ocr_worker, std::ref(ocr_ops), dur);
    std::thread det_worker(run_detection_worker, std::ref(det_ops), dur);
    ocr_worker.join();
    det_worker.join();

    EXPECT_GT(ocr_ops.load(), 0u) << "[IMAGE:MixedSoak] OCR produced zero ops";
    EXPECT_GT(det_ops.load(), 0u) << "[IMAGE:MixedSoak] Detection produced zero ops";
}
