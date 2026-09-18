/**
 * @file ann_index.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

// ANN (Approximate Nearest Neighbor) Index Backends for the ThemisDB Index Module
//
// Provides a unified IAnnIndex interface and two alternative ANN backends:
//   1. ScaNN  – Scalable Nearest Neighbors (tree-AH, Google ICML'20) – pure C++ implementation
//   2. DiskAnnAdapter – thin adapter over DiskANNIndex from performance/phase3 (requires
//      THEMIS_ENABLE_DISKANN compile-time flag)
//
// Integration with VectorIndexManager is through AdvancedIndexConfig::Type::SCANN /
// AdvancedIndexConfig::Type::DISKANN.
//
// References:
//   ScaNN : "Accelerating Large-Scale Inference with Anisotropic Vector Quantization"
//           Guo et al., ICML 2020  https://arxiv.org/abs/1908.10396
//   DiskANN: "DiskANN: Fast Accurate Billion-point Nearest Neighbor Search on a Single Node"
//            Subramanya et al., NeurIPS 2019

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <limits>

#ifdef THEMIS_ENABLE_DISKANN
#include "performance/phase3/diskann.h"
#endif

namespace themis {
namespace index {

// ---------------------------------------------------------------------------
// IAnnIndex – common interface for ANN backends used by VectorIndexManager
// ---------------------------------------------------------------------------

struct AnnSearchResult {
    int64_t id;       ///< User-visible vector ID (label)
    float   distance; ///< Distance from query (smaller = closer)
};

class IAnnIndex {
public:
    /**
     * @brief IAnn Index.
     * @return Return value.
     */
    virtual ~IAnnIndex() = default;

    /**
     * @brief Build.
     * @param[in] vectors Input parameter.
     * @param[in] ids Input parameter.
     * @param[in] count Input parameter.
     * @param[in] dim Input parameter.
     * @return True when the operation succeeds.
     */
    virtual bool build(const float* vectors, const int64_t* ids,
                       size_t count, size_t dim) = 0;

    [[nodiscard]] virtual bool add(int64_t id, const float* vector, size_t dim) = 0;

    /**
     * @brief Search.
     * @param[in] query Input parameter.
     * @param[in] dim Input parameter.
     * @param[in] k Input parameter.
     * @return Return value.
     */
    virtual std::vector<AnnSearchResult> search(const float* query, size_t dim,
                                                 int k) const = 0;

    virtual bool save(const std::string& /*path*/) const { return false; }

    /**
     * @brief Load.
     * @param[in] param Input parameter.
     * @return True when the operation succeeds.
     * @details Implements load without additional internal calls.
     */
    virtual bool load(const std::string& /*path*/) { return false; }

    [[nodiscard]] virtual size_t size() const = 0;
};

// ---------------------------------------------------------------------------
// ScaNN – Scalable Nearest Neighbors (Google ICML'20) – pure C++ backend
//
// Algorithm outline
//   Phase 1  K-means partitioning: divide the dataset into `num_leaves` Voronoi
//            cells whose centroids are computed with Lloyd's algorithm.
//   Phase 2  Asymmetric Hashing (AH): residual vectors inside every cell are
//            compressed using product quantization (PQ) so that distance
//            estimation is cheap.
//   Search   (a) Score all centroids, keep top `num_leaves_to_search` cells.
//            (b) Scan compressed vectors in those cells with AH distance.
//            (c) Re-rank the raw top-`reorder_num_neighbors` candidates
//                using exact float distances.
//
// This implementation is a pure C++17 self-contained version that does not
// depend on TensorFlow or any external library, making it suitable for the
// ThemisDB build system.
// ---------------------------------------------------------------------------

struct ScaNNConfig {
    // Partitioning
    size_t num_leaves            = 1000;  ///< Target number of Voronoi cells
    size_t num_leaves_to_search  = 100;   ///< Cells probed per query
    size_t kmeans_iters          = 10;    ///< K-means training iterations

    // Asymmetric hashing (product quantization)
    bool   enable_ah             = true;  ///< Enable AH compression
    size_t pq_num_subspaces      = 8;     ///< Number of PQ sub-spaces
    size_t pq_bits_per_subspace  = 8;     ///< Bits per PQ centroid (→ 256 centroids)

    // Re-ranking
    size_t reorder_num_neighbors = 200;   ///< Candidates re-scored with exact dist

    // Distance metric (L2 only in this implementation)
    enum class Metric { L2 } metric = Metric::L2;
};

class ScaNN final : public IAnnIndex {
public:
    explicit ScaNN(ScaNNConfig cfg = {});
    ~ScaNN() override = default;

    bool build(const float* vectors, const int64_t* ids,
               size_t count, size_t dim) override;

    bool add(int64_t id, const float* vector, size_t dim) override;

    std::vector<AnnSearchResult> search(const float* query, size_t dim,
                                        int k) const override;

    bool save(const std::string& path) const override;
    bool load(const std::string& path) override;

    size_t size() const override;

    const ScaNNConfig& config() const { return cfg_; }

private:
    // ---- internal types ----
    struct PQCodebook {
        size_t num_subspaces = 0;
        size_t sub_dim       = 0;      // dim / num_subspaces
        size_t num_centroids = 256;    // 2^pq_bits_per_subspace
        // centroids[s][c][d]  s=subspace, c=centroid, d=sub_dim
        std::vector<std::vector<std::vector<float>>> centroids;

        /**
         * @brief Train.
         * @param[in] data Input parameter.
         * @param[in] n Input parameter.
         * @param[in] d Input parameter.
         * @param[in] nss Input parameter.
         * @param[in] bits Input parameter.
         * @param[in] iters Input parameter.
         */
        void train(const float* data, size_t n, size_t d,
                   size_t nss, size_t bits, size_t iters);
        /**
         * @brief Encode.
         * @param[in] vec Input parameter.
         * @param[in] d Input parameter.
         * @return Return value.
         */
        std::vector<uint8_t> encode(const float* vec, size_t d) const;
        /**
         * @brief Decode distance.
         * @param[in] query Input parameter.
         * @param[in] code Input parameter.
         * @return Return value.
         */
        float decode_distance(const float* query, const std::vector<uint8_t>& code) const;
    };

    struct Leaf {
        std::vector<float>             centroid;
        std::vector<int64_t>           ids;
        std::vector<std::vector<float>> vectors; // full (for rerank)
        std::vector<std::vector<uint8_t>> codes; // PQ-compressed (optional)
    };

    /**
     * @brief ---- helpers ----
     * @param[in] a Input parameter.
     * @param[in] b Input parameter.
     * @param[in] d Input parameter.
     * @return Return value.
     */
    static float l2sq(const float* a, const float* b, size_t d);
    /**
     * @brief Kmeans.
     * @param[in] data Input parameter.
     * @param[in] n Input parameter.
     * @param[in] d Input parameter.
     * @param[in] k Input parameter.
     * @param[in] iters Input parameter.
     * @param[in,out] centroids Input/output parameter.
     * @param[in,out] assignments Input/output parameter.
     */
    static void  kmeans(const float* data, size_t n, size_t d,
                        size_t k, size_t iters,
                        std::vector<std::vector<float>>& centroids,
                        std::vector<size_t>& assignments);

    // ---- state ----
    ScaNNConfig cfg_;
    size_t dim_ = 0;
    std::vector<Leaf>     leaves_;
    PQCodebook            codebook_;
    bool                  trained_ = false;

    // Fallback flat storage before training (accumulates vectors via add())
    std::vector<int64_t>           flat_ids_;
    std::vector<std::vector<float>> flat_vectors_;
};


// ---------------------------------------------------------------------------
// DiskAnnAdapter – wraps DiskANNIndex (performance/phase3) as IAnnIndex
//   Only available when THEMIS_ENABLE_DISKANN is defined.
// ---------------------------------------------------------------------------

#ifdef THEMIS_ENABLE_DISKANN

class DiskAnnAdapter final : public IAnnIndex {
public:
    explicit DiskAnnAdapter(const std::string& index_path, size_t cache_mb = 1024)
        : index_path_(index_path), cache_mb_(cache_mb) {}

    bool build(const float* vectors, const int64_t* ids,
               size_t count, size_t dim) override {
        dim_ = dim;
        impl_ = std::make_unique<performance::phase3::DiskANNIndex>(dim, index_path_, cache_mb_);
        std::vector<std::pair<performance::phase3::VectorID, std::vector<float>>> vecs;
        vecs.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            int64_t label = ids ? ids[i] : static_cast<int64_t>(i);
            std::vector<float> v(vectors + i * dim, vectors + i * dim + dim);
            vecs.emplace_back(static_cast<performance::phase3::VectorID>(label), std::move(v));
        }
        impl_->build(vecs);
        count_ = count;
        return true;
    }

    bool add(int64_t id, const float* vector, size_t dim) override {
        if (!impl_) {
            dim_ = dim;
            impl_ = std::make_unique<performance::phase3::DiskANNIndex>(dim, index_path_, cache_mb_);
        }
        std::vector<float> v(vector, vector + dim);
        impl_->add(static_cast<performance::phase3::VectorID>(id), v);
        ++count_;
        return true;
    }

    std::vector<AnnSearchResult> search(const float* query, size_t dim,
                                        int k) const override {
        if (!impl_) return {};
        std::vector<float> q(query, query + dim);
        auto raw = impl_->search(q, k);
        std::vector<AnnSearchResult> out = {};

        out.reserve(raw.size());
        for (auto& r : raw)
            out.push_back({static_cast<int64_t>(r.id), r.distance});
        return out;
    }

    bool save(const std::string& path) const override {
        if (!impl_) {
          return false;
        }
        impl_->flush();
        return impl_->save(path);
    }

    bool load(const std::string& path) override {
        // Peek the dimension stored in the metadata sidecar so we create the
        // DiskANNIndex with the correct dimension even when build() was never called.
        const std::string meta_path = path + ".meta";
        if (dim_ == 0) {
            /**
             * @brief Peek.
             * @param[in] meta_path Path to the meta.
             * @param[in] binary Input parameter.
             * @return Return value.
             */
            std::ifstream peek(meta_path, std::ios::binary);
            if (peek) {
                size_t stored_dim = 0;
                peek.read(reinterpret_cast<char*>(&stored_dim), sizeof(stored_dim));
                if (peek && stored_dim > 0)
                    dim_ = stored_dim;
            }
        }
        if (!impl_) {
            impl_ = std::make_unique<performance::phase3::DiskANNIndex>(
                dim_ > 0 ? dim_ : 1, index_path_, cache_mb_);
        }
        if (!impl_->load(path)) {
          return false;
        }
        count_ = impl_->get_stats().num_vectors;
        return true;
    }

    size_t size() const override { return count_; }

private:
    std::string index_path_;
    size_t cache_mb_;
    size_t dim_ = 0;
    size_t count_ = 0;
    std::unique_ptr<performance::phase3::DiskANNIndex> impl_;
};
#endif // THEMIS_ENABLE_DISKANN

} // namespace index
} // namespace themis

