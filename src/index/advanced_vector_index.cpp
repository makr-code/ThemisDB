#include "index/advanced_vector_index.h"
#include "utils/logger.h"
#include <algorithm>
#include <cmath>

// FAISS includes (conditional on FAISS availability)
#ifdef THEMIS_HAS_FAISS
    #include <faiss/IndexFlat.h>
    #include <faiss/IndexIVFPQ.h>
    #include <faiss/IndexIVFFlat.h>
    #include <faiss/IndexHNSW.h>
    #ifdef THEMIS_ENABLE_CUDA
        #include <faiss/gpu/GpuIndexIVFPQ.h>
        #include <faiss/gpu/GpuIndexIVFFlat.h>
        #include <faiss/gpu/StandardGpuResources.h>
    #endif
    #include <faiss/index_io.h>
#else
    // Stub definitions for non-FAISS builds
    namespace faiss {
        class Index {};
        class IndexIVFPQ : public Index {};
        class IndexIVFFlat : public Index {};
    }
#endif

namespace themis {

AdvancedVectorIndex::AdvancedVectorIndex(size_t dimension, const Config& config)
    : dimension_(dimension), config_(config), index_(nullptr) {
    
    THEMIS_INFO("AdvancedVectorIndex created: dim={}, nlist={}, nprobe={}, pq_m={}, type={}",
                dimension_, config_.nlist, config_.nprobe, config_.pq_m, 
                static_cast<int>(config_.index_type));
    
    initializeIndex();
}

AdvancedVectorIndex::~AdvancedVectorIndex() {
#ifdef THEMIS_HAS_FAISS
    if (index_) {
        delete static_cast<faiss::Index*>(index_);
        index_ = nullptr;
    }
#endif
}

bool AdvancedVectorIndex::initializeIndex() {
#ifdef THEMIS_HAS_FAISS
    try {
        faiss::Index* idx = nullptr;
        
        switch (config_.index_type) {
            case Config::Type::IVF_PQ: {
                // IVF + Product Quantization (10-100x compression)
                // Need a quantizer (flat L2 index for clustering)
                auto* quantizer = new faiss::IndexFlat(dimension_, faiss::METRIC_L2);
                auto* ivf_pq = new faiss::IndexIVFPQ(
                    quantizer,
                    dimension_,
                    config_.nlist,
                    config_.pq_m,
                    config_.pq_nbits,
                    faiss::METRIC_L2
                );
                ivf_pq->own_fields = true; // FAISS will delete the quantizer
                ivf_pq->nprobe = config_.nprobe;
                idx = ivf_pq;
                THEMIS_INFO("Created IVF+PQ index: nlist={}, m={}, nbits={}",
                           config_.nlist, config_.pq_m, config_.pq_nbits);
                break;
            }
            
            case Config::Type::IVF_FLAT: {
                // IVF without compression (faster, more memory)
                auto* quantizer = new faiss::IndexFlat(dimension_, faiss::METRIC_L2);
                auto* ivf_flat = new faiss::IndexIVFFlat(
                    quantizer,
                    dimension_,
                    config_.nlist,
                    faiss::METRIC_L2
                );
                ivf_flat->own_fields = true; // FAISS will delete the quantizer
                ivf_flat->nprobe = config_.nprobe;
                idx = ivf_flat;
                THEMIS_INFO("Created IVF Flat index: nlist={}", config_.nlist);
                break;
            }
            
            case Config::Type::HNSW_FLAT: {
                // HNSW without IVF (best accuracy)
                auto* hnsw = new faiss::IndexHNSWFlat(dimension_, 32);
                idx = hnsw;
                THEMIS_INFO("Created HNSW Flat index");
                break;
            }
            
            default:
                THEMIS_ERROR("Unsupported index type");
                return false;
        }
        
        index_ = static_cast<void*>(idx);
        return true;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to initialize FAISS index: {}", e.what());
        return false;
    }
#else
    THEMIS_WARN("FAISS not available - using stub implementation");
    return false;
#endif
}

bool AdvancedVectorIndex::train(const float* vectors, size_t count) {
#ifdef THEMIS_HAS_FAISS
    if (!index_) {
        THEMIS_ERROR("Index not initialized");
        return false;
    }
    
    if (is_trained_) {
        THEMIS_WARN("Index already trained");
        return true;
    }
    
    try {
        auto* idx = static_cast<faiss::Index*>(index_);
        
        // Check if training is needed
        if (!idx->is_trained) {
            size_t train_count = std::min(count, config_.train_size);
            THEMIS_INFO("Training index on {} vectors...", train_count);
            
            idx->train(train_count, vectors);
            is_trained_ = true;
            
            THEMIS_INFO("Index trained successfully");
        } else {
            is_trained_ = true;
            THEMIS_INFO("Index does not require training");
        }
        
        return true;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Training failed: {}", e.what());
        return false;
    }
#else
    (void)vectors;
    (void)count;
    THEMIS_WARN("FAISS not available");
    return false;
#endif
}

bool AdvancedVectorIndex::add(const float* vectors, size_t count) {
#ifdef THEMIS_HAS_FAISS
    if (!index_) {
        THEMIS_ERROR("Index not initialized");
        return false;
    }
    
    if (!is_trained_) {
        THEMIS_ERROR("Index not trained - call train() first");
        return false;
    }
    
    try {
        auto* idx = static_cast<faiss::Index*>(index_);
        idx->add(count, vectors);
        
        THEMIS_INFO("Added {} vectors to index (total: {})", count, idx->ntotal);
        return true;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Add failed: {}", e.what());
        return false;
    }
#else
    (void)vectors;
    (void)count;
    return false;
#endif
}

bool AdvancedVectorIndex::addWithIds(const float* vectors, const int64_t* ids, size_t count) {
#ifdef THEMIS_HAS_FAISS
    if (!index_) {
        THEMIS_ERROR("Index not initialized");
        return false;
    }
    
    if (!is_trained_) {
        THEMIS_ERROR("Index not trained - call train() first");
        return false;
    }
    
    try {
        auto* idx = static_cast<faiss::Index*>(index_);
        idx->add_with_ids(count, vectors, ids);
        
        THEMIS_INFO("Added {} vectors with IDs to index", count);
        return true;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Add with IDs failed: {}", e.what());
        return false;
    }
#else
    (void)vectors;
    (void)ids;
    (void)count;
    return false;
#endif
}

AdvancedVectorIndex::SearchResult AdvancedVectorIndex::search(const float* query, size_t k) {
    SearchResult result;
    
#ifdef THEMIS_HAS_FAISS
    if (!index_) {
        THEMIS_ERROR("Index not initialized");
        return result;
    }
    
    try {
        auto* idx = static_cast<faiss::Index*>(index_);
        
        result.ids.resize(k);
        result.distances.resize(k);
        
        idx->search(1, query, k, result.distances.data(), result.ids.data());
        
        return result;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Search failed: {}", e.what());
        return result;
    }
#else
    (void)query;
    (void)k;
    return result;
#endif
}

std::vector<AdvancedVectorIndex::SearchResult> AdvancedVectorIndex::searchBatch(
    const float* queries,
    size_t num_queries,
    size_t k
) {
    std::vector<SearchResult> results(num_queries);
    
#ifdef THEMIS_HAS_FAISS
    if (!index_) {
        THEMIS_ERROR("Index not initialized");
        return results;
    }
    
    try {
        auto* idx = static_cast<faiss::Index*>(index_);
        
        std::vector<int64_t> all_ids(num_queries * k);
        std::vector<float> all_distances(num_queries * k);
        
        idx->search(num_queries, queries, k, all_distances.data(), all_ids.data());
        
        // Split results
        for (size_t i = 0; i < num_queries; ++i) {
            results[i].ids.assign(
                all_ids.begin() + i * k,
                all_ids.begin() + (i + 1) * k
            );
            results[i].distances.assign(
                all_distances.begin() + i * k,
                all_distances.begin() + (i + 1) * k
            );
        }
        
        THEMIS_INFO("Batch search: {} queries, k={}", num_queries, k);
        return results;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Batch search failed: {}", e.what());
        return results;
    }
#else
    (void)queries;
    (void)num_queries;
    (void)k;
    return results;
#endif
}

AdvancedVectorIndex::Stats AdvancedVectorIndex::getStats() const {
    Stats stats;
    
#ifdef THEMIS_HAS_FAISS
    if (index_) {
        auto* idx = static_cast<faiss::Index*>(index_);
        stats.total_vectors = idx->ntotal;
        stats.is_trained = is_trained_;
        stats.is_gpu = config_.use_gpu;
        
        // Estimate compression ratio for PQ
        if (config_.index_type == Config::Type::IVF_PQ) {
            // PQ compresses each vector from d*4 bytes to m*nbits/8 bytes
            double flat_size = dimension_ * sizeof(float);
            double pq_size = config_.pq_m * config_.pq_nbits / 8.0;
            stats.compression_ratio = flat_size / pq_size;
        }
        
        stats.memory_usage_bytes = stats.total_vectors * dimension_ * sizeof(float) / stats.compression_ratio;
    }
#endif
    
    return stats;
}

bool AdvancedVectorIndex::save(const std::string& path) {
#ifdef THEMIS_HAS_FAISS
    if (!index_) {
        THEMIS_ERROR("Index not initialized");
        return false;
    }
    
    try {
        auto* idx = static_cast<faiss::Index*>(index_);
        faiss::write_index(idx, path.c_str());
        
        THEMIS_INFO("Index saved to {}", path);
        return true;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Save failed: {}", e.what());
        return false;
    }
#else
    (void)path;
    return false;
#endif
}

bool AdvancedVectorIndex::load(const std::string& path) {
#ifdef THEMIS_HAS_FAISS
    try {
        if (index_) {
            delete static_cast<faiss::Index*>(index_);
        }
        
        auto* idx = faiss::read_index(path.c_str());
        index_ = static_cast<void*>(idx);
        is_trained_ = idx->is_trained;
        
        THEMIS_INFO("Index loaded from {}", path);
        return true;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Load failed: {}", e.what());
        return false;
    }
#else
    (void)path;
    return false;
#endif
}

} // namespace themis
