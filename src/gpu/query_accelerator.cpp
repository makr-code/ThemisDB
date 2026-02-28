#include "themis/gpu/query_accelerator.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <unordered_map>

#ifdef THEMIS_ENABLE_CUDA
#  include <cuda_runtime.h>
#endif
#ifdef THEMIS_ENABLE_CUVS
#  include <raft/core/device_resources.hpp>
#  include <raft/core/device_mdarray.hpp>
#  include <raft/core/copy.hpp>
#  include <cuvs/neighbors/ivf_flat.hpp>
#  include <cuvs/distance/distance_types.hpp>
#endif

namespace themis {
namespace gpu {

// ---------------------------------------------------------------------------
// FP16 / BF16 quantisation helpers (CPU simulation of Tensor Core precision)
// ---------------------------------------------------------------------------

/// Encode a float32 to IEEE 754 FP16 bits (uint16_t).
static uint16_t fp32_to_fp16(float f) noexcept {
    uint32_t bits;
    std::memcpy(&bits, &f, 4);
    const uint32_t sign     = (bits >> 31) & 0x1u;
    const int32_t  exp32    = static_cast<int32_t>((bits >> 23) & 0xFFu) - 127;
    const uint32_t mant32   = bits & 0x7FFFFFu;

    // Special cases
    if (exp32 == 128) {
        // Inf or NaN
        return static_cast<uint16_t>((sign << 15) | 0x7C00u |
               (mant32 ? 0x0200u : 0u));  // preserve NaN signal
    }
    if (exp32 < -24) {
        // Too small: flush to ±0
        return static_cast<uint16_t>(sign << 15);
    }
    if (exp32 < -14) {
        // Subnormal FP16
        uint32_t shift = static_cast<uint32_t>(-14 - exp32);
        uint32_t mant16 = (mant32 | 0x800000u) >> (shift + 13);
        return static_cast<uint16_t>((sign << 15) | mant16);
    }
    if (exp32 > 15) {
        // Overflow: return ±Inf
        return static_cast<uint16_t>((sign << 15) | 0x7C00u);
    }
    // Normalised
    uint32_t exp16  = static_cast<uint32_t>(exp32 + 15);
    uint32_t mant16 = mant32 >> 13;
    // Round to nearest even
    uint32_t round  = mant32 & 0x1FFFu;
    if (round > 0x1000u || (round == 0x1000u && (mant16 & 1u))) ++mant16;
    if (mant16 >= 0x400u) { ++exp16; mant16 = 0; }
    return static_cast<uint16_t>((sign << 15) | (exp16 << 10) | (mant16 & 0x3FFu));
}

/// Decode IEEE 754 FP16 bits back to float32.
static float fp16_to_fp32(uint16_t h) noexcept {
    const uint32_t sign  = static_cast<uint32_t>((h >> 15) & 0x1u);
    const uint32_t exp16 = (h >> 10) & 0x1Fu;
    const uint32_t mant16 = h & 0x3FFu;

    uint32_t bits;
    if (exp16 == 0x1F) {
        // Inf or NaN
        bits = (sign << 31) | 0x7F800000u | (mant16 << 13);
    } else if (exp16 == 0) {
        // Zero or subnormal
        if (mant16 == 0) {
            bits = sign << 31;
        } else {
            // Normalise the subnormal
            uint32_t m = mant16;
            int32_t  e = -14;
            while ((m & 0x400u) == 0) { m <<= 1; --e; }
            m &= 0x3FFu;
            bits = (sign << 31) | (static_cast<uint32_t>(e + 127) << 23) | (m << 13);
        }
    } else {
        bits = (sign << 31) | ((exp16 + 112u) << 23) | (mant16 << 13);
    }
    float f;
    std::memcpy(&f, &bits, 4);
    return f;
}

/// Round-trip a float through FP16 to simulate Tensor Core precision loss.
static float quantise_fp16(float f) noexcept {
    return fp16_to_fp32(fp32_to_fp16(f));
}

/// Encode a float32 to BF16 bits (uint16_t) — top 16 bits of FP32 with RNE.
static uint16_t fp32_to_bf16(float f) noexcept {
    uint32_t bits;
    std::memcpy(&bits, &f, 4);
    // Round to nearest even
    bits += 0x7FFFu + ((bits >> 16) & 1u);
    return static_cast<uint16_t>(bits >> 16);
}

/// Decode BF16 bits back to float32 — restore the truncated mantissa bits as 0.
static float bf16_to_fp32(uint16_t b) noexcept {
    uint32_t bits = static_cast<uint32_t>(b) << 16;
    float f;
    std::memcpy(&f, &bits, 4);
    return f;
}

/// Round-trip a float through BF16 to simulate Tensor Core precision loss.
static float quantise_bf16(float f) noexcept {
    return bf16_to_fp32(fp32_to_bf16(f));
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

GPUQueryAccelerator::GPUQueryAccelerator()
    : GPUQueryAccelerator(Config{}) {}

GPUQueryAccelerator::GPUQueryAccelerator(const Config& config)
    : config_(config)
    , graph_cache_enabled_(config.enable_graph_cache) {}

// ---------------------------------------------------------------------------
// Graph cache control
// ---------------------------------------------------------------------------

void GPUQueryAccelerator::enableGraphCache() {
    graph_cache_enabled_.store(true, std::memory_order_relaxed);
}

void GPUQueryAccelerator::disableGraphCache() {
    graph_cache_enabled_.store(false, std::memory_order_relaxed);
}

GPUGraphCache::Stats GPUQueryAccelerator::getGraphCacheStats() const {
    return graph_cache_.getStats();
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

bool GPUQueryAccelerator::shouldUseGPU(size_t num_rows) const noexcept {
    if (config_.force_cpu) return false;
    return num_rows >= config_.gpu_threshold_rows;
}

void GPUQueryAccelerator::recordOp(size_t rows, uint64_t bytes, bool gpu_used) {
    stats_.rows_processed += rows;
    stats_.bytes_scanned  += bytes;
    if (gpu_used) ++stats_.gpu_ops;
    else          ++stats_.cpu_fallback_ops;
}

// static
QueryShape GPUQueryAccelerator::makeShape(QueryShape::OpType op,
                                          size_t             row_count,
                                          uint64_t           param_hash) noexcept {
    QueryShape s;
    s.op         = op;
    s.row_count  = row_count;
    s.param_hash = param_hash;
    return s;
}

// ---------------------------------------------------------------------------
// scan
// ---------------------------------------------------------------------------

GPUQueryAccelerator::ScanResult
GPUQueryAccelerator::scan(const std::vector<Row>& rows, FilterFn filter) {
    ScanResult result;
    result.rows_scanned = rows.size();

    // Determine path ---------------------------------------------------------
    bool use_gpu = shouldUseGPU(rows.size());
    result.used_gpu = use_gpu;

    // Graph cache check ------------------------------------------------------
    // For recurring scans with the same row count: on a cache hit we replay the
    // captured graph (CPU simulation: execute normally but note the cache hit).
    // On a miss we capture the new shape for future replays.
    if (graph_cache_enabled_) {
        QueryShape shape = makeShape(QueryShape::OpType::SCAN, rows.size());
        if (graph_cache_.lookup(shape)) {
            std::lock_guard<std::mutex> lk(mutex_);
            ++stats_.graph_cache_hits;
        } else {
            graph_cache_.capture(shape);
            std::lock_guard<std::mutex> lk(mutex_);
            ++stats_.graph_cache_misses;
        }
    }

    // GPU path stub: when THEMIS_ENABLE_CUDA / THEMIS_ENABLE_HIP is defined,
    // copy rows to device, run a Thrust::copy_if / cub::DeviceSelect kernel,
    // copy results back.  For now we fall through to the CPU implementation.
    (void)use_gpu;

    // CPU sequential scan ----------------------------------------------------
    for (const auto& row : rows) {
        if (!filter || filter(row)) {
            result.rows.push_back(row);
        }
    }
    result.rows_passed = result.rows.size();

    // Stats ------------------------------------------------------------------
    uint64_t bytes = 0;
    for (const auto& r : rows) bytes += r.data.size();
    std::lock_guard<std::mutex> lk(mutex_);
    ++stats_.total_scans;
    recordOp(rows.size(), bytes, result.used_gpu);

    return result;
}

// ---------------------------------------------------------------------------
// sort
// ---------------------------------------------------------------------------

GPUQueryAccelerator::SortResult
GPUQueryAccelerator::sort(std::vector<Row> rows, KeyFn key_fn, SortOrder order) {
    SortResult result;
    bool use_gpu = shouldUseGPU(rows.size());
    result.used_gpu = use_gpu;

    // Graph cache check — include sort order in the param hash ---------------
    if (graph_cache_enabled_) {
        uint64_t param = static_cast<uint64_t>(order);
        QueryShape shape = makeShape(QueryShape::OpType::SORT, rows.size(), param);
        if (graph_cache_.lookup(shape)) {
            std::lock_guard<std::mutex> lk(mutex_);
            ++stats_.graph_cache_hits;
        } else {
            graph_cache_.capture(shape);
            std::lock_guard<std::mutex> lk(mutex_);
            ++stats_.graph_cache_misses;
        }
    }

    // GPU stub: would copy IDs + keys to device, run Thrust stable_sort_by_key,
    // gather rows back.  CPU path:
    std::stable_sort(rows.begin(), rows.end(),
        [&](const Row& a, const Row& b) {
            double ka = key_fn(a);
            double kb = key_fn(b);
            return (order == SortOrder::ASC) ? ka < kb : ka > kb;
        });
    result.rows = std::move(rows);

    uint64_t bytes = 0;
    for (const auto& r : result.rows) bytes += r.data.size();
    std::lock_guard<std::mutex> lk(mutex_);
    ++stats_.total_sorts;
    recordOp(result.rows.size(), bytes, result.used_gpu);

    return result;
}

// ---------------------------------------------------------------------------
// aggregate
// ---------------------------------------------------------------------------

GPUQueryAccelerator::AggResult
GPUQueryAccelerator::aggregate(const std::vector<Row>& rows,
                               AggFunc                  func,
                               KeyFn                    value_fn) {
    AggResult result;
    if (rows.empty()) return result;

    bool use_gpu = shouldUseGPU(rows.size());
    result.used_gpu = use_gpu;
    result.count    = rows.size();

    // Graph cache check — include AggFunc in the param hash ------------------
    if (graph_cache_enabled_) {
        uint64_t param = static_cast<uint64_t>(func);
        QueryShape shape = makeShape(QueryShape::OpType::AGGREGATE, rows.size(), param);
        if (graph_cache_.lookup(shape)) {
            std::lock_guard<std::mutex> lk(mutex_);
            ++stats_.graph_cache_hits;
        } else {
            graph_cache_.capture(shape);
            std::lock_guard<std::mutex> lk(mutex_);
            ++stats_.graph_cache_misses;
        }
    }

    // GPU stub: would use cub::DeviceReduce.  CPU sequential path:
    double sum = 0.0;
    double mn  = std::numeric_limits<double>::max();
    double mx  = std::numeric_limits<double>::lowest();

    for (const auto& row : rows) {
        double v = value_fn(row);
        sum += v;
        if (v < mn) mn = v;
        if (v > mx) mx = v;
    }

    switch (func) {
        case AggFunc::SUM:   result.value = sum;                         break;
        case AggFunc::COUNT: result.value = static_cast<double>(rows.size()); break;
        case AggFunc::MIN:   result.value = mn;                          break;
        case AggFunc::MAX:   result.value = mx;                          break;
        case AggFunc::AVG:   result.value = sum / static_cast<double>(rows.size()); break;
    }

    uint64_t bytes = 0;
    for (const auto& r : rows) bytes += r.data.size();
    std::lock_guard<std::mutex> lk(mutex_);
    ++stats_.total_aggregates;
    recordOp(rows.size(), bytes, result.used_gpu);

    return result;
}

// ---------------------------------------------------------------------------
// hashJoin
// ---------------------------------------------------------------------------

GPUQueryAccelerator::JoinResult
GPUQueryAccelerator::hashJoin(const std::vector<Row>& left,
                               const std::vector<Row>& right,
                               JoinKeyFn               left_key,
                               JoinKeyFn               right_key) {
    JoinResult result;
    if (left.empty() || right.empty()) return result;

    bool use_gpu = shouldUseGPU(left.size() + right.size());
    result.used_gpu = use_gpu;

    // Graph cache check — key on total row count -----------------------------
    if (graph_cache_enabled_) {
        size_t total = left.size() + right.size();
        QueryShape shape = makeShape(QueryShape::OpType::JOIN, total);
        if (graph_cache_.lookup(shape)) {
            std::lock_guard<std::mutex> lk(mutex_);
            ++stats_.graph_cache_hits;
        } else {
            graph_cache_.capture(shape);
            std::lock_guard<std::mutex> lk(mutex_);
            ++stats_.graph_cache_misses;
        }
    }

    // GPU stub: would use a parallel hash join kernel.  CPU path uses
    // an unordered_multimap on the smaller side:
    const std::vector<Row>* build_side  = &left;
    const std::vector<Row>* probe_side  = &right;
    JoinKeyFn                build_key  = left_key;
    JoinKeyFn                probe_key  = right_key;
    bool swapped = false;

    if (right.size() < left.size()) {
        std::swap(build_side, probe_side);
        std::swap(build_key,  probe_key);
        swapped = true;
    }

    std::unordered_multimap<uint64_t, const Row*> ht;
    ht.reserve(build_side->size());
    for (const auto& row : *build_side) {
        ht.emplace(build_key(row), &row);
    }

    for (const auto& row : *probe_side) {
        uint64_t k = probe_key(row);
        auto [beg, end] = ht.equal_range(k);
        for (auto it = beg; it != end; ++it) {
            if (!swapped)
                result.pairs.emplace_back(*it->second, row);
            else
                result.pairs.emplace_back(row, *it->second);
        }
    }

    uint64_t bytes = 0;
    for (const auto& r : left)  bytes += r.data.size();
    for (const auto& r : right) bytes += r.data.size();
    std::lock_guard<std::mutex> lk(mutex_);
    ++stats_.total_joins;
    recordOp(left.size() + right.size(), bytes, result.used_gpu);

    return result;
}

// ---------------------------------------------------------------------------
// dotProduct
// ---------------------------------------------------------------------------

GPUQueryAccelerator::DotProductResult
GPUQueryAccelerator::dotProduct(const std::vector<float>& a,
                                const std::vector<float>& b)
{
    DotProductResult result;
    result.precision_used = config_.precision_mode;

    if (a.empty() || a.size() != b.size()) {
        std::lock_guard<std::mutex> lk(mutex_);
        ++stats_.total_dot_products;
        recordOp(0, 0, false);
        return result;
    }

    bool use_gpu = shouldUseGPU(a.size());
    result.used_gpu = use_gpu;

    // GPU stub: would dispatch to cublasSgemv (FP32), cublasHgemm (FP16), or
    // cublasGemmEx with CUDA_R_16BF (BF16).  CPU simulation path below.

    double sum = 0.0;
    switch (config_.precision_mode) {
        case PrecisionMode::FP16:
            for (size_t i = 0; i < a.size(); ++i) {
                sum += static_cast<double>(quantise_fp16(a[i]) *
                                           quantise_fp16(b[i]));
            }
            break;
        case PrecisionMode::BF16:
            for (size_t i = 0; i < a.size(); ++i) {
                sum += static_cast<double>(quantise_bf16(a[i]) *
                                           quantise_bf16(b[i]));
            }
            break;
        case PrecisionMode::FP32:
        default:
            for (size_t i = 0; i < a.size(); ++i) {
                sum += static_cast<double>(a[i]) * static_cast<double>(b[i]);
            }
            break;
    }
    result.value = sum;

    std::lock_guard<std::mutex> lk(mutex_);
    ++stats_.total_dot_products;
    if (config_.precision_mode == PrecisionMode::FP16) ++stats_.fp16_ops;
    else if (config_.precision_mode == PrecisionMode::BF16) ++stats_.bf16_ops;
    recordOp(a.size(), a.size() * sizeof(float) * 2, result.used_gpu);

    return result;
}

// ---------------------------------------------------------------------------
// annSearch  (GPU-accelerated ANN via cuVS/RAFT; CPU brute-force fallback)
// ---------------------------------------------------------------------------

GPUQueryAccelerator::AnnResult
GPUQueryAccelerator::annSearch(const std::vector<float>& queries,
                               size_t                    numQueries,
                               size_t                    dim,
                               const std::vector<float>& database,
                               size_t                    numVectors,
                               size_t                    k,
                               bool                      useL2)
{
    AnnResult result;

    // Validate inputs --------------------------------------------------------
    if (dim == 0 || k == 0 || numQueries == 0 || numVectors == 0 ||
        queries.size() != numQueries * dim ||
        database.size() != numVectors * dim) {
        std::lock_guard<std::mutex> lk(mutex_);
        ++stats_.total_ann_searches;
        recordOp(0, 0, false);
        return result;
    }

    bool use_gpu = shouldUseGPU(numVectors);
    result.used_gpu = false;  // set true only if GPU path executes successfully

    // Graph cache check — key on (numQueries * dim) as row count and pack
    // k + useL2 flag into the parameter hash.  Salt constants ensure that
    // L2 and inner-product shapes with the same k are kept as distinct entries.
    static constexpr uint64_t kAnnL2Salt = 0xABCDEF01ULL;  // salt for L2 metric
    static constexpr uint64_t kAnnIPSalt = 0x12345678ULL;   // salt for inner-product metric
    if (graph_cache_enabled_) {
        uint64_t param = static_cast<uint64_t>(k) ^ (useL2 ? kAnnL2Salt : kAnnIPSalt);
        QueryShape shape = makeShape(QueryShape::OpType::ANN_SEARCH,
                                     numQueries * dim, param);
        if (graph_cache_.lookup(shape)) {
            std::lock_guard<std::mutex> lk(mutex_);
            ++stats_.graph_cache_hits;
        } else {
            graph_cache_.capture(shape);
            std::lock_guard<std::mutex> lk(mutex_);
            ++stats_.graph_cache_misses;
        }
    }

#ifdef THEMIS_ENABLE_CUDA
    // -------------------------------------------------------------------------
    // GPU path — cuVS/RAFT IVF-Flat ANN search.
    //
    // Activated when THEMIS_ENABLE_CUDA is defined and the database is large
    // enough to exceed the GPU dispatch threshold (shouldUseGPU returns true).
    // Falls through to the CPU brute-force path below on any failure.
    // -------------------------------------------------------------------------
    if (use_gpu) {
        bool gpu_done = false;
#ifdef THEMIS_ENABLE_CUVS
        try {
            raft::device_resources handle;

            // 1. Copy database to device
            auto db_dev = raft::make_device_matrix<float>(handle, numVectors, dim);
            raft::copy(db_dev.data_handle(), database.data(),
                       numVectors * dim, handle.get_stream());

            // 2. Build IVF-Flat index
            cuvs::neighbors::ivf_flat::index_params idx_params;
            idx_params.metric = useL2
                ? cuvs::distance::DistanceType::L2Unexpanded
                : cuvs::distance::DistanceType::InnerProduct;
            auto index = cuvs::neighbors::ivf_flat::build(
                handle, idx_params, db_dev.view());

            // 3. Copy queries to device and run search
            auto q_dev = raft::make_device_matrix<float>(handle, numQueries, dim);
            raft::copy(q_dev.data_handle(), queries.data(),
                       numQueries * dim, handle.get_stream());
            auto neighbors_dev =
                raft::make_device_matrix<uint32_t>(handle, numQueries, k);
            auto distances_dev =
                raft::make_device_matrix<float>(handle, numQueries, k);
            cuvs::neighbors::ivf_flat::search_params search_params;
            cuvs::neighbors::ivf_flat::search(handle, search_params, index,
                q_dev.view(), neighbors_dev.view(), distances_dev.view());

            // 4. Copy results back to host and populate AnnResult
            std::vector<uint32_t> neighbor_idx(numQueries * k);
            std::vector<float>    host_distances(numQueries * k);
            raft::copy(neighbor_idx.data(), neighbors_dev.data_handle(),
                       numQueries * k, handle.get_stream());
            raft::copy(host_distances.data(), distances_dev.data_handle(),
                       numQueries * k, handle.get_stream());
            handle.sync_stream();

            result.results.resize(numQueries);
            for (size_t qi = 0; qi < numQueries; ++qi) {
                result.results[qi].resize(k);
                for (size_t ni = 0; ni < k; ++ni) {
                    result.results[qi][ni].index    = neighbor_idx[qi * k + ni];
                    result.results[qi][ni].distance = host_distances[qi * k + ni];
                }
            }
            gpu_done = true;
        } catch (...) {
            // cudaMalloc failure or cuVS error — fall through to CPU path.
            gpu_done = false;
        }
#endif // THEMIS_ENABLE_CUVS

        if (gpu_done) {
            result.used_gpu = true;
            uint64_t bytes = static_cast<uint64_t>((numQueries + numVectors) * dim * sizeof(float));
            std::lock_guard<std::mutex> lk(mutex_);
            ++stats_.total_ann_searches;
            recordOp(numVectors, bytes, true);
            return result;
        }
        // GPU path did not complete — fall through to CPU brute-force below.
    }
#endif // THEMIS_ENABLE_CUDA

    // CPU brute-force exact k-NN (used when GPU unavailable or below threshold):

    const size_t actual_k = std::min(k, numVectors);
    result.results.resize(numQueries);

    for (size_t qi = 0; qi < numQueries; ++qi) {
        const float* q = queries.data() + qi * dim;

        // Compute distances from this query to all database vectors
        // using a max-heap of size k to track the k-nearest so far.
        // Heap element: (distance, index)
        using Pair = std::pair<float, size_t>;
        std::vector<Pair> heap;
        heap.reserve(actual_k + 1);

        for (size_t vi = 0; vi < numVectors; ++vi) {
            const float* v = database.data() + vi * dim;

            float dist = 0.0f;
            if (useL2) {
                for (size_t d = 0; d < dim; ++d) {
                    float diff = q[d] - v[d];
                    dist += diff * diff;
                }
            } else {
                // Negative inner product used as a distance (lower = more similar).
                // For unit-normalized vectors this equals cosine distance, but works
                // for unnormalized vectors as well: the result may be negative when
                // dot(q, v) > 0 (high similarity).  Callers performing maximum inner
                // product search (MIPS) should normalize their vectors beforehand to
                // obtain the standard cosine-distance interpretation.
                float dot = 0.0f;
                for (size_t d = 0; d < dim; ++d) dot += q[d] * v[d];
                dist = -dot;
            }

            if (heap.size() < actual_k) {
                heap.emplace_back(dist, vi);
                if (heap.size() == actual_k) {
                    std::make_heap(heap.begin(), heap.end());
                }
            } else if (dist < heap.front().first) {
                std::pop_heap(heap.begin(), heap.end());
                heap.back() = {dist, vi};
                std::push_heap(heap.begin(), heap.end());
            }
        }

        // Sort heap ascending by distance
        std::sort_heap(heap.begin(), heap.end());

        result.results[qi].resize(heap.size());
        for (size_t ni = 0; ni < heap.size(); ++ni) {
            result.results[qi][ni].index    = heap[ni].second;
            result.results[qi][ni].distance = heap[ni].first;
        }
    }

    uint64_t bytes = static_cast<uint64_t>((numQueries + numVectors) * dim * sizeof(float));
    std::lock_guard<std::mutex> lk(mutex_);
    ++stats_.total_ann_searches;
    recordOp(numVectors, bytes, result.used_gpu);

    return result;
}



GPUQueryAccelerator::Stats GPUQueryAccelerator::getStats() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return stats_;
}

void GPUQueryAccelerator::resetStats() {
    std::lock_guard<std::mutex> lk(mutex_);
    stats_ = Stats{};
}

} // namespace gpu
} // namespace themis
