/**
 * @file vec_knn.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// PERF-D3: Parallel batch insertion + SIMD-accelerated distance for VectorIndex.
//
// Design goals:
//   1. Split large entity batches into sub-batches of `batch_size` (default 32).
//   2. Submit sub-batches to a thread pool (std::async / std::thread workers).
//   3. Each worker serialises its cache update and HNSW write through the
//      existing VectorIndexManager::addBatch() API, which is internally atomic.
//   4. Pairwise distance computation uses AVX-512 → AVX2 → NEON → scalar SIMD
//      for maximum throughput on any host CPU.
//   5. A DistanceCache memoises distances for primary-key pairs that recur
//      across overlapping ingestion batches, eliminating duplicate FLOPs.

#include "acceleration/vec_knn.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <future>
#include <stdexcept>
#include <thread>
#include <utility>

#include "index/vector_index.h"
#include "storage/base_entity.h"

// SIMD headers – same guard pattern as cpu_backend_tbb.cpp
#if defined(__AVX512F__)
#define THEMIS_VEC_KNN_SIMD_AVX512 1
#include <immintrin.h>
#elif defined(__AVX2__)
#define THEMIS_VEC_KNN_SIMD_AVX2 1
#include <immintrin.h>
#elif defined(__ARM_NEON) || defined(__aarch64__)
#define THEMIS_VEC_KNN_SIMD_NEON 1
#include <arm_neon.h>
#endif

// MSVC uses /arch:AVX2 to enable intrinsics; expose the same gate.
#if defined(_MSC_VER) && (defined(__AVX2__) || defined(__AVX512F__))
#include <immintrin.h>
#ifndef THEMIS_VEC_KNN_SIMD_AVX2
#define THEMIS_VEC_KNN_SIMD_AVX2 1
#endif
#endif

namespace themis {
namespace acceleration {

namespace {
std::mutex &addBatchBridgeMutex() {
    static std::mutex m;
    return m;
}

VecKnnInsertPipeline::AddBatchBridgeFn &addBatchBridgeStorage() {
    static VecKnnInsertPipeline::AddBatchBridgeFn fn;
    return fn;
}

std::mutex &extractVectorBridgeMutex() {
    static std::mutex m;
    return m;
}

VecKnnInsertPipeline::ExtractVectorBridgeFn &extractVectorBridgeStorage() {
    static VecKnnInsertPipeline::ExtractVectorBridgeFn fn;
    return fn;
}
} // namespace

void VecKnnInsertPipeline::setAddBatchBridgeFn(AddBatchBridgeFn fn) {
    std::lock_guard<std::mutex> lk(addBatchBridgeMutex());
    addBatchBridgeStorage() = std::move(fn);
}

void VecKnnInsertPipeline::clearAddBatchBridgeFn() {
    std::lock_guard<std::mutex> lk(addBatchBridgeMutex());
    addBatchBridgeStorage() = {};
}

void VecKnnInsertPipeline::setExtractVectorBridgeFn(ExtractVectorBridgeFn fn) {
    std::lock_guard<std::mutex> lk(extractVectorBridgeMutex());
    extractVectorBridgeStorage() = std::move(fn);
}

void VecKnnInsertPipeline::clearExtractVectorBridgeFn() {
    std::lock_guard<std::mutex> lk(extractVectorBridgeMutex());
    extractVectorBridgeStorage() = {};
}

// ============================================================================
// SIMD helpers
// ============================================================================

namespace {

// ---------------------------------------------------------------------------
// Scalar fallback
// ---------------------------------------------------------------------------
inline float scalar_l2_sq(const float *a, const float *b, std::size_t dim) noexcept {
    float acc = 0.0f;
    for (std::size_t i = 0; i < dim; ++i) {
        const float d = a[i] - b[i];
        acc += d * d;
    }
    return acc;
}

#if defined(THEMIS_VEC_KNN_SIMD_AVX512)
// ---------------------------------------------------------------------------
// AVX-512 path  (16 floats per iteration)
// ---------------------------------------------------------------------------
inline float avx512_l2_sq(const float *a, const float *b, std::size_t dim) noexcept {
    __m512 acc0   = _mm512_setzero_ps();
    __m512 acc1   = _mm512_setzero_ps();
    std::size_t i = 0;

    // Unrolled 2× for instruction-level parallelism
    for (; i + 31 < dim; i += 32) {
        __m512 d0 = _mm512_sub_ps(_mm512_loadu_ps(a + i), _mm512_loadu_ps(b + i));
        __m512 d1 = _mm512_sub_ps(_mm512_loadu_ps(a + i + 16), _mm512_loadu_ps(b + i + 16));
        acc0      = _mm512_fmadd_ps(d0, d0, acc0);
        acc1      = _mm512_fmadd_ps(d1, d1, acc1);
    }
    acc0 = _mm512_add_ps(acc0, acc1);

    for (; i + 15 < dim; i += 16) {
        __m512 d = _mm512_sub_ps(_mm512_loadu_ps(a + i), _mm512_loadu_ps(b + i));
        acc0     = _mm512_fmadd_ps(d, d, acc0);
    }

    float result = _mm512_reduce_add_ps(acc0);

    // Scalar tail
    for (; i < dim; ++i) {
        const float d = a[i] - b[i];
        result += d * d;
    }
    return result;
}

inline void avx512_batch_l2_sq(const float *q, const float *db, std::size_t n, std::size_t dim, float *out) noexcept {
    for (std::size_t v = 0; v < n; ++v)
        out[v] = avx512_l2_sq(q, db + v * dim, dim);
}

#elif defined(THEMIS_VEC_KNN_SIMD_AVX2)
// ---------------------------------------------------------------------------
// AVX2 path  (8 floats per iteration)
// ---------------------------------------------------------------------------
inline float avx2_hsum(__m256 v) noexcept {
    __m128 hi  = _mm256_extractf128_ps(v, 1);
    __m128 lo  = _mm256_castps256_ps128(v);
    __m128 sum = _mm_add_ps(lo, hi);
    sum        = _mm_hadd_ps(sum, sum);
    sum        = _mm_hadd_ps(sum, sum);
    return _mm_cvtss_f32(sum);
}

inline float avx2_l2_sq(const float *a, const float *b, std::size_t dim) noexcept {
    __m256 acc0   = _mm256_setzero_ps();
    __m256 acc1   = _mm256_setzero_ps();
    std::size_t i = 0;

    // Unrolled 2× for ILP
    for (; i + 15 < dim; i += 16) {
        __m256 d0 = _mm256_sub_ps(_mm256_loadu_ps(a + i), _mm256_loadu_ps(b + i));
        __m256 d1 = _mm256_sub_ps(_mm256_loadu_ps(a + i + 8), _mm256_loadu_ps(b + i + 8));
        acc0      = _mm256_fmadd_ps(d0, d0, acc0);
        acc1      = _mm256_fmadd_ps(d1, d1, acc1);
    }
    acc0 = _mm256_add_ps(acc0, acc1);

    for (; i + 7 < dim; i += 8) {
        __m256 d = _mm256_sub_ps(_mm256_loadu_ps(a + i), _mm256_loadu_ps(b + i));
        acc0     = _mm256_fmadd_ps(d, d, acc0);
    }

    float result = avx2_hsum(acc0);

    for (; i < dim; ++i) {
        const float d = a[i] - b[i];
        result += d * d;
    }
    return result;
}

inline void avx2_batch_l2_sq(const float *q, const float *db, std::size_t n, std::size_t dim, float *out) noexcept {
    for (std::size_t v = 0; v < n; ++v)
        out[v] = avx2_l2_sq(q, db + v * dim, dim);
}

#elif defined(THEMIS_VEC_KNN_SIMD_NEON)
// ---------------------------------------------------------------------------
// ARM NEON path  (4 floats per iteration)
// ---------------------------------------------------------------------------
inline float neon_l2_sq(const float *a, const float *b, std::size_t dim) noexcept {
    float32x4_t acc0 = vdupq_n_f32(0.f);
    float32x4_t acc1 = vdupq_n_f32(0.f);
    std::size_t i    = 0;

    for (; i + 7 < dim; i += 8) {
        float32x4_t d0 = vsubq_f32(vld1q_f32(a + i), vld1q_f32(b + i));
        float32x4_t d1 = vsubq_f32(vld1q_f32(a + i + 4), vld1q_f32(b + i + 4));
        acc0           = vmlaq_f32(acc0, d0, d0);
        acc1           = vmlaq_f32(acc1, d1, d1);
    }
    acc0 = vaddq_f32(acc0, acc1);

    for (; i + 3 < dim; i += 4) {
        float32x4_t d = vsubq_f32(vld1q_f32(a + i), vld1q_f32(b + i));
        acc0          = vmlaq_f32(acc0, d, d);
    }

    float result = vaddvq_f32(acc0);
    for (; i < dim; ++i) {
        const float d = a[i] - b[i];
        result += d * d;
    }
    return result;
}

inline void neon_batch_l2_sq(const float *q, const float *db, std::size_t n, std::size_t dim, float *out) noexcept {
    for (std::size_t v = 0; v < n; ++v)
        out[v] = neon_l2_sq(q, db + v * dim, dim);
}
#endif

} // anonymous namespace

// ============================================================================
// Public SIMD dispatch
// ============================================================================

float simd_l2_sq(const float *a, const float *b, std::size_t dim) noexcept {
#if defined(THEMIS_VEC_KNN_SIMD_AVX512)
    return avx512_l2_sq(a, b, dim);
#elif defined(THEMIS_VEC_KNN_SIMD_AVX2)
    return avx2_l2_sq(a, b, dim);
#elif defined(THEMIS_VEC_KNN_SIMD_NEON)
    return neon_l2_sq(a, b, dim);
#else
    return scalar_l2_sq(a, b, dim);
#endif
}

void simd_batch_l2_sq(const float *query, const float *database, std::size_t n, std::size_t dim, float *out) noexcept {
#if defined(THEMIS_VEC_KNN_SIMD_AVX512)
    avx512_batch_l2_sq(query, database, n, dim, out);
#elif defined(THEMIS_VEC_KNN_SIMD_AVX2)
    avx2_batch_l2_sq(query, database, n, dim, out);
#elif defined(THEMIS_VEC_KNN_SIMD_NEON)
    neon_batch_l2_sq(query, database, n, dim, out);
#else
    for (std::size_t v = 0; v < n; ++v) {
        out[v] = scalar_l2_sq(query, database + v * dim, dim);
    }
#endif
}

// ============================================================================
// DistanceCache
// ============================================================================

DistanceCache::DistanceCache(std::size_t max_entries) : max_entries_(max_entries > 0 ? max_entries : 1) {}

DistanceCache::DistanceCache(DistanceCache &&other) noexcept {
    std::lock_guard<std::mutex> lk(other.mtx_);
    max_entries_ = other.max_entries_;
    map_         = std::move(other.map_);
    order_       = std::move(other.order_);
    hits_.store(other.hits_.load());
    misses_.store(other.misses_.load());
}

DistanceCache &DistanceCache::operator=(DistanceCache &&other) noexcept {
    if (this != &other) {
        std::scoped_lock lk(mtx_, other.mtx_);
        max_entries_ = other.max_entries_;
        map_         = std::move(other.map_);
        order_       = std::move(other.order_);
        hits_.store(other.hits_.load());
        misses_.store(other.misses_.load());
    }
    return *this;
}

/*static*/ std::string DistanceCache::makeKey(const std::string &a, const std::string &b) {
    // Canonical ordering so (a,b) and (b,a) share the same slot
    if (a <= b) {
        return a + '\0' + b;
    }
    return b + '\0' + a;
}

bool DistanceCache::get(const std::string &pk_a, const std::string &pk_b, float &out) const {
    std::string key = makeKey(pk_a, pk_b);
    std::lock_guard<std::mutex> lk(mtx_);
    auto it = map_.find(key);
    if (it == map_.end()) {
        misses_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    out = it->second;
    hits_.fetch_add(1, std::memory_order_relaxed);
    return true;
}

void DistanceCache::put(const std::string &pk_a, const std::string &pk_b, float value) {
    std::string key = makeKey(pk_a, pk_b);
    std::lock_guard<std::mutex> lk(mtx_);
    if (map_.count(key)) {
        map_[key] = value; // update existing – no size growth
        return;
    }
    // Evict oldest entry if at capacity (O(1) with std::deque)
    if (map_.size() >= max_entries_ && !order_.empty()) {
        map_.erase(order_.front());
        order_.pop_front(); // O(1) for std::deque
    }
    map_[key] = value;
    order_.push_back(key);
}

void DistanceCache::invalidate(const std::string &pk) {
    std::lock_guard<std::mutex> lk(mtx_);
    // Erase all keys that contain pk (either side of the '\0' separator)
    for (auto it = map_.begin(); it != map_.end();) {
        const std::string &k = it->first;
        auto sep             = k.find('\0');
        bool matches         = ((sep != std::string::npos) && (k.substr(0, sep) == pk || k.substr(sep + 1) == pk));
        if (matches) {
            auto oi = std::find(order_.begin(), order_.end(), k);
            if (oi != order_.end()) {
                order_.erase(oi);
            }
            it = map_.erase(it);
        } else {
            ++it;
        }
    }
}

void DistanceCache::clear() {
    std::lock_guard<std::mutex> lk(mtx_);
    map_.clear();
    order_.clear();
}

std::size_t DistanceCache::size() const {
    std::lock_guard<std::mutex> lk(mtx_);
    return static_cast<int>(map_.size());
}

// ============================================================================
// VecKnnInsertPipeline
// ============================================================================

VecKnnInsertPipeline::VecKnnInsertPipeline(VecKnnPipelineConfig config)
    : config_(std::move(config)),
      cache_(std::make_unique<DistanceCache>(config_.enable_cache ? config_.cache_entries : 0)) {
    if (config_.batch_size == 0) {
        config_.batch_size = 32;
    }
    if (config_.num_threads == 0) {
        config_.num_threads = std::max<std::size_t>(1, static_cast<std::size_t>(std::thread::hardware_concurrency()));
    }
}

VecKnnInsertPipeline::~VecKnnInsertPipeline() = default;

void VecKnnInsertPipeline::setBatchSize(std::size_t sz) {
    config_.batch_size = (sz > 0) ? sz : 32;
}

void VecKnnInsertPipeline::setThreadCount(std::size_t n) {
    config_.num_threads = (n > 0) ? n : std::max<std::size_t>(1, static_cast<std::size_t>(std::thread::hardware_concurrency()));
}

void VecKnnInsertPipeline::enableDistanceCache(bool enable) {
    config_.enable_cache = enable;
    if (!enable) {
        cache_->clear();
    }
}

// ---------------------------------------------------------------------------
// computeDistances – standalone SIMD pairwise distance query
// ---------------------------------------------------------------------------
std::vector<float> VecKnnInsertPipeline::computeDistances(const float *query_vectors, std::size_t numQueries,
                                                          const float *db_vectors, std::size_t numDB,
                                                          std::size_t dim) const {
    if (!query_vectors || !db_vectors || numQueries == 0 || numDB == 0 || dim == 0) {
        return {};
    }

    std::vector<float> result(numQueries * numDB);
    for (std::size_t q = 0; q < numQueries; ++q) {
        simd_batch_l2_sq(query_vectors + q * dim, db_vectors, numDB, dim, result.data() + q * numDB);
    }
    return result;
}

// ---------------------------------------------------------------------------
// insertBatch – the main parallel insertion entry point
// ---------------------------------------------------------------------------
VecKnnInsertResult VecKnnInsertPipeline::insertBatch(VectorIndexManager &index, const std::vector<BaseEntity> &entities,
                                                     std::string_view vectorField) {
    VecKnnInsertResult result = {};
    if (entities.empty()) {
        return result;
    }

    AddBatchBridgeFn addBatchBridge;
    {
        std::lock_guard<std::mutex> lk(addBatchBridgeMutex());
        addBatchBridge = addBatchBridgeStorage();
    }
    if (!addBatchBridge) {
        result.ok      = false;
        result.failed  = entities.size();
        result.message = "VecKnnInsertPipeline addBatch bridge not configured";
        total_failed_.fetch_add(result.failed, std::memory_order_relaxed);
        return result;
    }

    ExtractVectorBridgeFn extractVectorBridge;
    {
        std::lock_guard<std::mutex> lk(extractVectorBridgeMutex());
        extractVectorBridge = extractVectorBridgeStorage();
    }
    if (!extractVectorBridge) {
        result.ok      = false;
        result.failed  = entities.size();
        result.message = "VecKnnInsertPipeline vector extraction bridge not configured";
        total_failed_.fetch_add(result.failed, std::memory_order_relaxed);
        return result;
    }

    std::string field = vectorField.empty() ? config_.vector_field : std::string(vectorField);
    if (field.empty()) {
        field = "embedding";
    }

    if (!config_.enable_cache) {
        cache_->clear();
    }

    const std::size_t batchSize  = std::max<std::size_t>(1, config_.batch_size);
    const std::size_t maxWorkers = std::max<std::size_t>(1, config_.num_threads);

    std::atomic<std::size_t> inserted{0};
    std::atomic<std::size_t> failed{0};
    std::mutex msgMutex = {};
    std::string firstError = {};

    const auto prewarmCache = [this, &field, &extractVectorBridge](const std::vector<BaseEntity> &batch) {
        if (!config_.enable_cache) {
            return;
        }

        std::vector<std::string> pks;
        std::vector<std::vector<float>> vectors;
        pks.reserve(batch.size());
        vectors.reserve(batch.size());

        for (const auto &e : batch) {
            auto vec = extractVectorBridge(e, field);
            if (!vec.has_value() || vec->empty()) {
                continue;
            }
            pks.push_back(e.getPrimaryKey());
            vectors.push_back(std::move(*vec));
        }

        for (std::size_t i = 0; i  < vectors.size(); ++i) {
            for (std::size_t j = i + 1; j  < vectors.size(); ++j) {
                if (vectors[i].size() != vectors[j].size()) {
                    continue;
                }

                float cached = 0.0f;
                if (cache_->get(pks[i], pks[j], cached)) {
                    continue;
                }

                const float dist = simd_l2_sq(vectors[i].data(), vectors[j].data(), vectors[i].size());
                cache_->put(pks[i], pks[j], dist);
            }
        }
    };

    std::vector<std::future<void>> futures;
    futures.reserve((entities.size() + batchSize - 1) / batchSize);

    for (std::size_t begin = 0; begin  < entities.size(); begin += batchSize) {
        const std::size_t end = std::min(begin + batchSize, entities.size());

        auto worker = [&, begin, end]() {
            std::vector<BaseEntity> batch;
            batch.reserve(end - begin);
            for (std::size_t i = begin; i < end; ++i) {
                batch.push_back(entities[i]);
            }

            prewarmCache(batch);

            VecKnnInsertResult batchResult;
            {
                std::lock_guard<std::mutex> lk(index_mtx_);
                batchResult = addBatchBridge(index, batch, field);
            }

            if (batchResult.ok) {
                const std::size_t okCount = batchResult.inserted > 0 ? batchResult.inserted : batch.size();
                inserted.fetch_add(okCount, std::memory_order_relaxed);
            } else {
                const std::size_t failedCount = batchResult.failed > 0 ? batchResult.failed : batch.size();
                failed.fetch_add(failedCount, std::memory_order_relaxed);
                std::lock_guard<std::mutex> lk(msgMutex);
                if (firstError.empty()) {
                    firstError = batchResult.message;
                }
            }
        };

        futures.emplace_back(std::async(std::launch::async, worker));
        if (futures.size() >= maxWorkers) {
            futures.front().get();
            futures.erase(futures.begin());
        }
    }

    for (auto &f : futures) {
        f.get();
    }

    result.inserted = inserted.load(std::memory_order_relaxed);
    result.failed   = failed.load(std::memory_order_relaxed);
    result.ok       = (result.failed == 0);
    if (!result.ok) {
        result.message = firstError.empty() ? "VecKnnInsertPipeline batch insert failed" : firstError;
    }

    total_inserted_.fetch_add(result.inserted, std::memory_order_relaxed);
    total_failed_.fetch_add(result.failed, std::memory_order_relaxed);
    return result;
}

} // namespace acceleration
} // namespace themis

