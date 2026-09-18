/**
 * @file tensor_rag_cost_model.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "query/optimizer_cost_model.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>

namespace themis {
namespace query {

// ============================================================================
// RagRetrievalPath — which index backend handles the retrieval
// ============================================================================

enum class RagRetrievalPath : uint8_t {
    HNSW_FLAT    = 0,  ///< Standard HNSW, flat float32 storage
    FAISS_IVF_PQ = 1,  ///< FAISS IVF-PQ, compressed flat storage
    TT_HYBRID    = 2,  ///< HNSW on TT-core sketches + TT-domain distance
    TT_ZERO_COPY = 3,  ///< Full TT-index + mmap injection (GgmlTensorBridge)
};

inline const char* to_cstr(RagRetrievalPath p) noexcept {
    switch (p) {
        case RagRetrievalPath::HNSW_FLAT:    return "HNSW_FLAT";
        case RagRetrievalPath::FAISS_IVF_PQ: return "FAISS_IVF_PQ";
        case RagRetrievalPath::TT_HYBRID:    return "TT_HYBRID";
        case RagRetrievalPath::TT_ZERO_COPY: return "TT_ZERO_COPY";
    }
    return "UNKNOWN";
}

// ============================================================================
// TensorRagParams — data-dependent input parameters for cost estimation
// ============================================================================

struct TensorRagParams {
    // ---- Index / corpus parameters ----

    std::size_t N = 1'000'000;

    std::size_t d = 768;

    std::size_t k = 10;

    // ---- HNSW parameters (paths A and B) ----

    std::size_t hnsw_M = 16;

    std::size_t hnsw_ef = 64;

    // ---- FAISS IVF-PQ parameters (path FAISS_IVF_PQ) ----

    std::size_t faiss_nlist = 4096;

    std::size_t faiss_nprobe = 64;

    std::size_t faiss_m = 64;

    // ---- TT parameters (paths B and C) ----

    std::size_t tt_rank = 32;

    std::size_t tt_modes = 2;

    std::size_t tt_n = 28;  // 28² = 784 ≈ 768 for BERT embeddings

    // ---- Payload / context parameters ----

    std::size_t payload_bytes = 2048;

    std::size_t context_tokens = 512;

    bool gpu_available = false;

    bool ggml_bridge_available = false;
};

// ============================================================================
// RagCostBreakdown — cost breakdown for one retrieval path
// ============================================================================

struct RagCostBreakdown {
    RagRetrievalPath path = RagRetrievalPath::HNSW_FLAT;

    // ---- Phase costs (NCU) ----

    double cost_index_traversal  = 0.0;

    double cost_dist_verification = 0.0;

    double cost_storage_fetch    = 0.0;

    double cost_decode           = 0.0;

    double cost_inject           = 0.0;

    // ---- Totals ----

    double total_ncu() const noexcept {
        return cost_index_traversal + cost_dist_verification
             + cost_storage_fetch   + cost_decode + cost_inject;
    }

    double ttft_ms() const noexcept { return total_ncu() / 1000.0; }

    WorkloadType workloadType() const noexcept {
        return WorkloadType::VECTOR_SEARCH;  // TENSOR_RAG maps to VECTOR_SEARCH for now
    }

    /**
     * @brief Summary.
     * @return Return value.
     */
    std::string summary() const;
};

// ============================================================================
// TensorRagCostModel
// ============================================================================

class TensorRagCostModel {
public:
    // -----------------------------------------------------------------------
    // Calibration constants (tuned to OptimizerCostModel::CostConstants)
    // -----------------------------------------------------------------------

    struct Constants {
        // CPU throughput
        double simd_floats_per_ns      = 16.0;  ///< AVX-512: 16 float32/ns
        double gflops_single_thread    = 1.0;   ///< ~1 GFlops single-thread CPU
        double gflops_gpu              = 20.0;  ///< ~20 GFlops for distance kernels (RTX 4090)

        // Memory / I/O
        double ssd_bandwidth_bytes_per_ns = 0.5; ///< ~500 MB/s NVMe
        double json_parse_bytes_per_ns = 0.5;    ///< ~500 MB/s simdjson
        double mmap_page_fault_ns      = 10000;  ///< ~10 µs per page fault
        double mmap_pages_per_tt_core  = 4.0;   ///< ~4 OS pages per TT-core

        // LLM injection
        double tokenise_ns_per_token   = 200.0;  ///< ~200 ns/token (BPE)
        double prefill_ns_per_token    = 50000.0;///< ~50 µs/token prefill (7B CPU)
        double tt_contraction_ns_per_flop = 1.0; ///< 1/GFLOPS
    };

    explicit TensorRagCostModel(Constants c = {}) : c_(c) {}

    // -----------------------------------------------------------------------
    // estimate() — cost breakdown for a given path
    // -----------------------------------------------------------------------

    RagCostBreakdown estimate(const TensorRagParams& p,
                              RagRetrievalPath        path) const noexcept
    {
        RagCostBreakdown out;
        out.path = path;

        switch (path) {
            /**
             * @brief Estimate Hnsw Flat.
             * @param[in] p Input parameter.
             * @param[in] out Input parameter.
             * @return Return value.
             */
            case RagRetrievalPath::HNSW_FLAT:    estimateHnswFlat(p, out);    break;
            /**
             * @brief Estimate Faiss Ivf Pq.
             * @param[in] p Input parameter.
             * @param[in] out Input parameter.
             * @return Return value.
             */
            case RagRetrievalPath::FAISS_IVF_PQ: estimateFaissIvfPq(p, out); break;
            /**
             * @brief Estimate Tt Hybrid.
             * @param[in] p Input parameter.
             * @param[in] out Input parameter.
             * @return Return value.
             */
            case RagRetrievalPath::TT_HYBRID:    estimateTtHybrid(p, out);    break;
            /**
             * @brief Estimate Tt Zero Copy.
             * @param[in] p Input parameter.
             * @param[in] out Input parameter.
             * @return Return value.
             */
            case RagRetrievalPath::TT_ZERO_COPY: estimateTtZeroCopy(p, out);  break;
        }
        return out;
    }

    RagRetrievalPath cheapestPath(const TensorRagParams& p) const noexcept {
        double best_cost  = 1e18;
        auto   best_path  = RagRetrievalPath::HNSW_FLAT;

        for (auto path : {RagRetrievalPath::HNSW_FLAT,
                          RagRetrievalPath::FAISS_IVF_PQ,
                          RagRetrievalPath::TT_HYBRID,
                          RagRetrievalPath::TT_ZERO_COPY})
        {
            if (path == RagRetrievalPath::TT_ZERO_COPY && !p.ggml_bridge_available)
                continue;

            auto bd = estimate(p, path);
            if (bd.total_ncu() < best_cost) {
                best_cost = bd.total_ncu();
                best_path = path;
            }
        }
        return best_path;
    }

    double speedup(const TensorRagParams& p,
                   RagRetrievalPath        baseline,
                   RagRetrievalPath        faster) const noexcept
    {
        double t_base  = estimate(p, baseline).total_ncu();
        double t_fast  = estimate(p, faster  ).total_ncu();
        if (t_fast < 1.0) {
          return 1.0;
        }
        return t_base / t_fast;
    }

    const Constants& constants() const noexcept { return c_; }

private:
    Constants c_;

    // -----------------------------------------------------------------------
    // Phase helpers — all return costs in NCU (≈ µs)
    // -----------------------------------------------------------------------

    double tIdxHnsw(const TensorRagParams& p) const noexcept {
        double dist_ops   = static_cast<double>(p.hnsw_ef * p.hnsw_M * p.d);
        double ns         = dist_ops / c_.simd_floats_per_ns;
        return ns / 1000.0;  // NCU (≈ µs)
    }

    double tIdxHnswTT(const TensorRagParams& p) const noexcept {
        double sketch_ops = static_cast<double>(p.hnsw_ef * p.hnsw_M
                                               * p.tt_modes * p.tt_rank * p.tt_rank);
        return (sketch_ops / c_.simd_floats_per_ns) / 1000.0;
    }

    double tIdxFaiss(const TensorRagParams& p) const noexcept {
        double ops = static_cast<double>(p.faiss_nprobe)
                   * (static_cast<double>(p.N) / static_cast<double>(p.faiss_nlist))
                   * static_cast<double>(p.faiss_m);
        return (ops / c_.simd_floats_per_ns) / 1000.0;
    }

    double tDistFlat(const TensorRagParams& p) const noexcept {
        double flops = static_cast<double>(p.k * p.d * 2);  // multiply + add
        return (flops / (c_.gflops_single_thread * 1000.0));  // NCU
    }

    double tDistTT(const TensorRagParams& p) const noexcept {
        double r3   = static_cast<double>(p.tt_rank)
                    * static_cast<double>(p.tt_rank)
                    * static_cast<double>(p.tt_rank);
        double flops = static_cast<double>(p.k)
                     * static_cast<double>(p.tt_modes)
                     * static_cast<double>(p.tt_n)
                     * r3;
        return (flops / (c_.gflops_single_thread * 1000.0));
    }

    double tFetchSSD(const TensorRagParams& p) const noexcept {
        double bytes = static_cast<double>(p.k * p.payload_bytes);
        return (bytes / c_.ssd_bandwidth_bytes_per_ns) / 1000.0;
    }

    double tFetchTTCores(const TensorRagParams& p) const noexcept {
        // Each TT-train: Σ r_k·n_k·r_{k+1} ≈ d_modes · tt_rank² · tt_n floats
        double bytes_per_train = static_cast<double>(p.tt_modes)
                               * static_cast<double>(p.tt_rank * p.tt_rank)
                               * static_cast<double>(p.tt_n)
                               * 4.0;  // float32
        double bytes = static_cast<double>(p.k) * bytes_per_train;
        return (bytes / c_.ssd_bandwidth_bytes_per_ns) / 1000.0;
    }

    double tFetchMmap(const TensorRagParams& p) const noexcept {
        double faults = static_cast<double>(p.k) * c_.mmap_pages_per_tt_core
                      * static_cast<double>(p.tt_modes);
        return (faults * c_.mmap_page_fault_ns) / (1000.0 * 1000.0);  // NCU
    }

    double tDecJson(const TensorRagParams& p) const noexcept {
        double bytes = static_cast<double>(p.k * p.payload_bytes);
        return (bytes / c_.json_parse_bytes_per_ns) / 1000.0;
    }

    double tInjectTokens(const TensorRagParams& p) const noexcept {
        double tok_ns    = static_cast<double>(p.context_tokens) * c_.tokenise_ns_per_token;
        double prefill_ns = static_cast<double>(p.context_tokens) * c_.prefill_ns_per_token;
        return (tok_ns + prefill_ns) / 1000.0;
    }

    double tInjectTTContraction(const TensorRagParams& p) const noexcept {
        return tDistTT(p);  // same transfer-matrix cost; no tokenisation overhead
    }

    // -----------------------------------------------------------------------
    // Path implementations
    // -----------------------------------------------------------------------

    void estimateHnswFlat(const TensorRagParams& p, RagCostBreakdown& out) const noexcept {
        out.cost_index_traversal   = tIdxHnsw(p);
        out.cost_dist_verification = tDistFlat(p);
        out.cost_storage_fetch     = tFetchSSD(p);
        out.cost_decode            = tDecJson(p);
        out.cost_inject            = tInjectTokens(p);
    }

    void estimateFaissIvfPq(const TensorRagParams& p, RagCostBreakdown& out) const noexcept {
        double gflops = p.gpu_available ? c_.gflops_gpu : c_.gflops_single_thread;
        out.cost_index_traversal   = tIdxFaiss(p) / (p.gpu_available ? 10.0 : 1.0);
        out.cost_dist_verification = (static_cast<double>(p.k * p.d * 2))
                                   / (gflops * 1000.0);
        out.cost_storage_fetch     = tFetchSSD(p);
        out.cost_decode            = tDecJson(p);
        out.cost_inject            = tInjectTokens(p);
    }

    void estimateTtHybrid(const TensorRagParams& p, RagCostBreakdown& out) const noexcept {
        out.cost_index_traversal   = tIdxHnswTT(p);
        out.cost_dist_verification = tDistTT(p);
        out.cost_storage_fetch     = tFetchTTCores(p);
        out.cost_decode            = 0.0;   // TT-cores are already float32
        out.cost_inject            = tInjectTokens(p);  // no mmap bridge yet
    }

    void estimateTtZeroCopy(const TensorRagParams& p, RagCostBreakdown& out) const noexcept {
        out.cost_index_traversal   = tIdxHnswTT(p);
        out.cost_dist_verification = tDistTT(p);
        out.cost_storage_fetch     = tFetchMmap(p);   // only page faults
        out.cost_decode            = 0.0;              // no decode; mmap is live pointer
        out.cost_inject            = tInjectTTContraction(p); // replaces tokenise+prefill
    }
};

// ============================================================================
// RagCostBreakdown::summary()
// ============================================================================

inline std::string RagCostBreakdown::summary() const {
    char buf[512];
    std::snprintf(buf, sizeof(buf),
        "[%s] idx=%.2f dist=%.2f fetch=%.2f dec=%.2f inject=%.2f → total=%.2f NCU (%.1f ms TTFT)",
        to_cstr(path),
        cost_index_traversal, cost_dist_verification,
        cost_storage_fetch, cost_decode, cost_inject,
        total_ncu(), ttft_ms());
    return std::string(buf);
}

} // namespace query
} // namespace themis
