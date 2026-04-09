/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            vec_knn.cpp                                        ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-04-09                                         ║
  Author:          copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Issue:           PERF-D3 – Vector Insert: Parallel Insertion,       ║
                   SIMD Distance for 600k/s Target                    ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
#include "index/vector_index.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <future>
#include <stdexcept>
#include <thread>
#include <utility>

// SIMD headers – same guard pattern as cpu_backend_tbb.cpp
#if defined(__AVX512F__)
#  define THEMIS_VEC_KNN_SIMD_AVX512 1
#  include <immintrin.h>
#elif defined(__AVX2__)
#  define THEMIS_VEC_KNN_SIMD_AVX2 1
#  include <immintrin.h>
#elif defined(__ARM_NEON) || defined(__aarch64__)
#  define THEMIS_VEC_KNN_SIMD_NEON 1
#  include <arm_neon.h>
#endif

// MSVC uses /arch:AVX2 to enable intrinsics; expose the same gate.
#if defined(_MSC_VER) && (defined(__AVX2__) || defined(__AVX512F__))
#  include <immintrin.h>
#  ifndef THEMIS_VEC_KNN_SIMD_AVX2
#    define THEMIS_VEC_KNN_SIMD_AVX2 1
#  endif
#endif

namespace themis {
namespace acceleration {

// ============================================================================
// SIMD helpers
// ============================================================================

namespace {

// ---------------------------------------------------------------------------
// Scalar fallback
// ---------------------------------------------------------------------------
inline float scalar_l2_sq(const float* a, const float* b, std::size_t dim) noexcept {
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
inline float avx512_l2_sq(const float* a, const float* b, std::size_t dim) noexcept {
    __m512 acc0 = _mm512_setzero_ps();
    __m512 acc1 = _mm512_setzero_ps();
    std::size_t i = 0;

    // Unrolled 2× for instruction-level parallelism
    for (; i + 31 < dim; i += 32) {
        __m512 d0 = _mm512_sub_ps(_mm512_loadu_ps(a + i),      _mm512_loadu_ps(b + i));
        __m512 d1 = _mm512_sub_ps(_mm512_loadu_ps(a + i + 16), _mm512_loadu_ps(b + i + 16));
        acc0 = _mm512_fmadd_ps(d0, d0, acc0);
        acc1 = _mm512_fmadd_ps(d1, d1, acc1);
    }
    acc0 = _mm512_add_ps(acc0, acc1);

    for (; i + 15 < dim; i += 16) {
        __m512 d = _mm512_sub_ps(_mm512_loadu_ps(a + i), _mm512_loadu_ps(b + i));
        acc0 = _mm512_fmadd_ps(d, d, acc0);
    }

    float result = _mm512_reduce_add_ps(acc0);

    // Scalar tail
    for (; i < dim; ++i) {
        const float d = a[i] - b[i];
        result += d * d;
    }
    return result;
}

inline void avx512_batch_l2_sq(const float* q, const float* db,
                                std::size_t n, std::size_t dim,
                                float* out) noexcept {
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
    sum = _mm_hadd_ps(sum, sum);
    sum = _mm_hadd_ps(sum, sum);
    return _mm_cvtss_f32(sum);
}

inline float avx2_l2_sq(const float* a, const float* b, std::size_t dim) noexcept {
    __m256 acc0 = _mm256_setzero_ps();
    __m256 acc1 = _mm256_setzero_ps();
    std::size_t i = 0;

    // Unrolled 2× for ILP
    for (; i + 15 < dim; i += 16) {
        __m256 d0 = _mm256_sub_ps(_mm256_loadu_ps(a + i),     _mm256_loadu_ps(b + i));
        __m256 d1 = _mm256_sub_ps(_mm256_loadu_ps(a + i + 8), _mm256_loadu_ps(b + i + 8));
        acc0 = _mm256_fmadd_ps(d0, d0, acc0);
        acc1 = _mm256_fmadd_ps(d1, d1, acc1);
    }
    acc0 = _mm256_add_ps(acc0, acc1);

    for (; i + 7 < dim; i += 8) {
        __m256 d = _mm256_sub_ps(_mm256_loadu_ps(a + i), _mm256_loadu_ps(b + i));
        acc0 = _mm256_fmadd_ps(d, d, acc0);
    }

    float result = avx2_hsum(acc0);

    for (; i < dim; ++i) {
        const float d = a[i] - b[i];
        result += d * d;
    }
    return result;
}

inline void avx2_batch_l2_sq(const float* q, const float* db,
                               std::size_t n, std::size_t dim,
                               float* out) noexcept {
    for (std::size_t v = 0; v < n; ++v)
        out[v] = avx2_l2_sq(q, db + v * dim, dim);
}

#elif defined(THEMIS_VEC_KNN_SIMD_NEON)
// ---------------------------------------------------------------------------
// ARM NEON path  (4 floats per iteration)
// ---------------------------------------------------------------------------
inline float neon_l2_sq(const float* a, const float* b, std::size_t dim) noexcept {
    float32x4_t acc0 = vdupq_n_f32(0.f);
    float32x4_t acc1 = vdupq_n_f32(0.f);
    std::size_t i = 0;

    for (; i + 7 < dim; i += 8) {
        float32x4_t d0 = vsubq_f32(vld1q_f32(a + i),     vld1q_f32(b + i));
        float32x4_t d1 = vsubq_f32(vld1q_f32(a + i + 4), vld1q_f32(b + i + 4));
        acc0 = vmlaq_f32(acc0, d0, d0);
        acc1 = vmlaq_f32(acc1, d1, d1);
    }
    acc0 = vaddq_f32(acc0, acc1);

    for (; i + 3 < dim; i += 4) {
        float32x4_t d = vsubq_f32(vld1q_f32(a + i), vld1q_f32(b + i));
        acc0 = vmlaq_f32(acc0, d, d);
    }

    float result = vaddvq_f32(acc0);
    for (; i < dim; ++i) {
        const float d = a[i] - b[i];
        result += d * d;
    }
    return result;
}

inline void neon_batch_l2_sq(const float* q, const float* db,
                               std::size_t n, std::size_t dim,
                               float* out) noexcept {
    for (std::size_t v = 0; v < n; ++v)
        out[v] = neon_l2_sq(q, db + v * dim, dim);
}
#endif

} // anonymous namespace

// ============================================================================
// Public SIMD dispatch
// ============================================================================

float simd_l2_sq(const float* a, const float* b, std::size_t dim) noexcept {
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

void simd_batch_l2_sq(const float* query,
                      const float* database,
                      std::size_t  n,
                      std::size_t  dim,
                      float*       out) noexcept {
#if defined(THEMIS_VEC_KNN_SIMD_AVX512)
    avx512_batch_l2_sq(query, database, n, dim, out);
#elif defined(THEMIS_VEC_KNN_SIMD_AVX2)
    avx2_batch_l2_sq(query, database, n, dim, out);
#elif defined(THEMIS_VEC_KNN_SIMD_NEON)
    neon_batch_l2_sq(query, database, n, dim, out);
#else
    for (std::size_t v = 0; v < n; ++v)
        out[v] = scalar_l2_sq(query, database + v * dim, dim);
#endif
}

// ============================================================================
// DistanceCache
// ============================================================================

DistanceCache::DistanceCache(std::size_t max_entries)
    : max_entries_(max_entries > 0 ? max_entries : 1) {}

DistanceCache::DistanceCache(DistanceCache&& other) noexcept {
    std::lock_guard<std::mutex> lk(other.mtx_);
    max_entries_ = other.max_entries_;
    map_         = std::move(other.map_);
    order_       = std::move(other.order_);
    hits_.store(other.hits_.load());
    misses_.store(other.misses_.load());
}

DistanceCache& DistanceCache::operator=(DistanceCache&& other) noexcept {
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

/*static*/ std::string DistanceCache::makeKey(const std::string& a, const std::string& b) {
    // Canonical ordering so (a,b) and (b,a) share the same slot
    if (a <= b) return a + '\0' + b;
    return b + '\0' + a;
}

bool DistanceCache::get(const std::string& pk_a, const std::string& pk_b, float& out) const {
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

void DistanceCache::put(const std::string& pk_a, const std::string& pk_b, float value) {
    std::string key = makeKey(pk_a, pk_b);
    std::lock_guard<std::mutex> lk(mtx_);
    if (map_.count(key)) {
        map_[key] = value; // update existing – no size growth
        return;
    }
    // Evict oldest entry if at capacity
    if (map_.size() >= max_entries_ && !order_.empty()) {
        map_.erase(order_.front());
        order_.erase(order_.begin()); // O(n) but order_ is bounded by max_entries_
    }
    map_[key] = value;
    order_.push_back(key);
}

void DistanceCache::invalidate(const std::string& pk) {
    std::lock_guard<std::mutex> lk(mtx_);
    // Erase all keys that contain pk (either side of the '\0' separator)
    for (auto it = map_.begin(); it != map_.end(); ) {
        const std::string& k = it->first;
        auto sep = k.find('\0');
        bool matches = (sep != std::string::npos)
                     && (k.substr(0, sep) == pk || k.substr(sep + 1) == pk);
        if (matches) {
            auto oi = std::find(order_.begin(), order_.end(), k);
            if (oi != order_.end()) order_.erase(oi);
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
    return map_.size();
}

// ============================================================================
// VecKnnInsertPipeline
// ============================================================================

VecKnnInsertPipeline::VecKnnInsertPipeline(VecKnnPipelineConfig config)
    : config_(std::move(config))
    , cache_(std::make_unique<DistanceCache>(
          config_.enable_cache ? config_.cache_entries : 0))
{
    if (config_.batch_size == 0)   config_.batch_size   = 32;
    if (config_.num_threads == 0)  config_.num_threads  =
        std::max(1u, std::thread::hardware_concurrency());
}

VecKnnInsertPipeline::~VecKnnInsertPipeline() = default;

void VecKnnInsertPipeline::setBatchSize(std::size_t sz) {
    config_.batch_size = (sz > 0) ? sz : 32;
}

void VecKnnInsertPipeline::setThreadCount(std::size_t n) {
    config_.num_threads = (n > 0) ? n : std::max(1u, std::thread::hardware_concurrency());
}

void VecKnnInsertPipeline::enableDistanceCache(bool enable) {
    config_.enable_cache = enable;
    if (!enable) cache_->clear();
}

// ---------------------------------------------------------------------------
// computeDistances – standalone SIMD pairwise distance query
// ---------------------------------------------------------------------------
std::vector<float> VecKnnInsertPipeline::computeDistances(
    const float* query_vectors,
    std::size_t  numQueries,
    const float* db_vectors,
    std::size_t  numDB,
    std::size_t  dim) const
{
    if (!query_vectors || !db_vectors || numQueries == 0 || numDB == 0 || dim == 0)
        return {};

    std::vector<float> result(numQueries * numDB);
    for (std::size_t q = 0; q < numQueries; ++q) {
        simd_batch_l2_sq(query_vectors + q * dim,
                         db_vectors,
                         numDB,
                         dim,
                         result.data() + q * numDB);
    }
    return result;
}

// ---------------------------------------------------------------------------
// insertBatch – the main parallel insertion entry point
// ---------------------------------------------------------------------------
VecKnnInsertResult VecKnnInsertPipeline::insertBatch(
    VectorIndexManager&            index,
    const std::vector<BaseEntity>& entities,
    std::string_view               vectorField)
{
    VecKnnInsertResult result;
    if (entities.empty()) return result;

    const std::string field = vectorField.empty()
                            ? config_.vector_field
                            : std::string(vectorField);
    const std::size_t total  = entities.size();
    const std::size_t bsz    = config_.batch_size;
    const std::size_t nthrd  = config_.num_threads;

    // Split into sub-batches
    std::vector<std::pair<std::size_t, std::size_t>> ranges; // [begin, end)
    for (std::size_t begin = 0; begin < total; begin += bsz) {
        ranges.emplace_back(begin, std::min(begin + bsz, total));
    }

    // Parallel futures
    const std::size_t nranges = ranges.size();
    std::vector<std::future<VecKnnInsertResult>> futures;
    futures.reserve(nranges);

    // Semaphore-like: limit concurrency to num_threads using a simple
    // batching approach – submit at most num_threads futures at a time.
    std::size_t submitted = 0;
    while (submitted < nranges) {
        std::size_t wave_end = std::min(submitted + nthrd, nranges);

        for (std::size_t r = submitted; r < wave_end; ++r) {
            auto [begin, end] = ranges[r];

            futures.push_back(
                std::async(std::launch::async,
                [this, &index, &entities, &field, begin, end]() -> VecKnnInsertResult
                {
                    // Build sub-batch view
                    std::vector<BaseEntity> sub(entities.begin() + static_cast<std::ptrdiff_t>(begin),
                                                entities.begin() + static_cast<std::ptrdiff_t>(end));

                    // Pre-warm distance cache for this sub-batch:
                    // compute pairwise distances and store them so that any
                    // subsequent KNN query over the same entities avoids recomputation.
                    if (config_.enable_cache && sub.size() >= 2) {
                        // Flatten all vectors in the sub-batch for SIMD batch call
                        std::vector<std::vector<float>> vecs;
                        std::vector<const float*>        ptrs;
                        std::size_t dim = 0;
                        vecs.reserve(sub.size());
                        ptrs.reserve(sub.size());

                        for (const auto& e : sub) {
                            auto v = e.extractVector(field);
                            if (v) {
                                if (dim == 0) dim = v->size();
                                vecs.push_back(std::move(*v));
                                ptrs.push_back(vecs.back().data());
                            } else {
                                vecs.push_back({});
                                ptrs.push_back(nullptr);
                            }
                        }

                        if (dim > 0) {
                            // Build flat DB array
                            std::vector<float> flat_db(sub.size() * dim, 0.f);
                            for (std::size_t i = 0; i < sub.size(); ++i) {
                                if (ptrs[i])
                                    std::copy(ptrs[i], ptrs[i] + dim,
                                              flat_db.data() + i * dim);
                            }

                            std::vector<float> dist_row(sub.size());
                            for (std::size_t qi = 0; qi < sub.size(); ++qi) {
                                if (!ptrs[qi]) continue;
                                simd_batch_l2_sq(ptrs[qi],
                                                 flat_db.data(),
                                                 sub.size(),
                                                 dim,
                                                 dist_row.data());
                                for (std::size_t di = qi + 1; di < sub.size(); ++di) {
                                    if (!ptrs[di]) continue;
                                    cache_->put(sub[qi].getPrimaryKey(),
                                                sub[di].getPrimaryKey(),
                                                dist_row[di]);
                                }
                            }
                        }
                    }

                    // Serialise the actual write into the shared index.
                    // VectorIndexManager::addBatch() is internally atomic
                    // (single WriteBatch commit), so we only need the outer
                    // mutex to prevent concurrent HNSW graph mutations.
                    VecKnnInsertResult sub_result;
                    {
                        std::lock_guard<std::mutex> lk(index_mtx_);
                        auto status = index.addBatch(sub, field);
                        if (status.ok) {
                            sub_result.inserted = sub.size();
                        } else {
                            sub_result.ok      = false;
                            sub_result.message = status.message;
                            sub_result.failed  = sub.size();
                        }
                    }
                    return sub_result;
                }));
        }

        // Collect results for this wave before launching the next
        for (std::size_t r = submitted; r < wave_end; ++r) {
            auto sub_res = futures[r].get();
            result.inserted += sub_res.inserted;
            result.failed   += sub_res.failed;
            if (!sub_res.ok) {
                result.ok      = false;
                result.message = sub_res.message; // last error wins
            }
        }
        submitted = wave_end;
    }

    total_inserted_.fetch_add(result.inserted, std::memory_order_relaxed);
    total_failed_.fetch_add(result.failed,   std::memory_order_relaxed);

    return result;
}

} // namespace acceleration
} // namespace themis
