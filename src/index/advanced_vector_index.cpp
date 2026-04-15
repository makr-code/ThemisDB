/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            advanced_vector_index.cpp                          ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:17:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   90.0/100                                       ║
    • Total Lines:     487                                            ║
    • Open Issues:     TODOs: 0, Stubs: 2                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • d275653619  2026-04-14  update after codefindings               ║
    • 7c2cc11ffb  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • a2d7c07202  2026-04-14  update after codefindings               ║
    • ad6e8f172c  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • ba22b5bb24  2026-03-13  fix(backpressure): stabilize overload metrics test and al... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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
                
                // v1.5.x: Enable ADC tables for ~40% faster search
                if (config_.use_adc_tables) {
                    ivf_pq->use_precomputed_table = 1; // Enable ADC distance tables
                    if (config_.polysemous_ht > 0) {
                        ivf_pq->polysemous_ht = config_.polysemous_ht;
                        THEMIS_INFO("Enabled polysemous hash tables: ht={}", 
                                   config_.polysemous_ht);
                    }
                    THEMIS_INFO("Enabled ADC tables for IVF+PQ (v1.5.x optimization)");
                }
                
                idx = ivf_pq;
                THEMIS_INFO("Created IVF+PQ index: nlist={}, m={}, nbits={}, adc={}", 
                           config_.nlist, config_.pq_m, config_.pq_nbits, 
                           config_.use_adc_tables);
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
                auto* hnsw = new faiss::IndexHNSWFlat(static_cast<int>(dimension_), 32);
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
    THEMIS_WARN("AdvancedVectorIndex::initializeIndex: FAISS not available, returning stub");
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
            double flat_size = static_cast<double>(dimension_) * sizeof(float);
            double pq_size = config_.pq_m * config_.pq_nbits / 8.0;
            stats.compression_ratio = flat_size / pq_size;
        }
        
            stats.memory_usage_bytes = static_cast<size_t>(static_cast<double>(stats.total_vectors * dimension_ * sizeof(float)) / stats.compression_ratio);
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
    return false;
#endif
}

AdvancedVectorIndex::Config AdvancedVectorIndex::getWorkloadOptimizedConfig(
    size_t dataset_size,
    size_t dimension,
    WorkloadType workload) {
    
    Config config;
    config.workload = workload;
    
    // Calculate optimal nlist (typically sqrt(N) to N/50)
    size_t base_nlist = static_cast<size_t>(std::sqrt(dataset_size));
    base_nlist = std::max(size_t(64), std::min(base_nlist, size_t(65536)));
    
    // Workload-specific nlist adjustments
    switch (workload) {
        case WorkloadType::OLTP:
            // OLTP: Fewer clusters for faster search
            config.nlist = std::max(size_t(64), base_nlist / 2);
            config.nprobe = 32;  // Lower nprobe for speed
            config.index_type = Config::Type::IVF_FLAT;  // Prefer speed over compression
            break;
            
        case WorkloadType::ANALYTICS:
            // Analytics: More clusters, higher nprobe for accuracy
            config.nlist = std::min(size_t(65536), base_nlist * 2);
            config.nprobe = 128;  // Higher nprobe for recall
            config.index_type = Config::Type::IVF_PQ;  // Use compression for large datasets
            break;
            
        case WorkloadType::RAG:
            // RAG: Balance between speed and accuracy
            config.nlist = base_nlist;
            config.nprobe = 64;
            config.index_type = Config::Type::IVF_PQ;
            break;
            
        case WorkloadType::BATCH_INSERT:
            // Batch insert: Optimize for fast training and insertion
            config.nlist = std::max(size_t(64), base_nlist / 4);
            config.nprobe = 32;
            config.index_type = Config::Type::IVF_FLAT;
            config.train_size = std::min(size_t(50000), dataset_size / 10);  // Faster training
            break;
            
        case WorkloadType::MIXED:
        default:
            config.nlist = base_nlist;
            config.nprobe = 64;
            config.index_type = Config::Type::IVF_PQ;
            break;
    }
    
    // PQ parameters based on dimension and workload (only for PQ-based indices)
    if (config.index_type == Config::Type::IVF_PQ || 
        config.index_type == Config::Type::IVF_HNSW_PQ) {
        config.use_pq = true;
        
        // pq_m should divide dimension evenly
        if (workload == WorkloadType::ANALYTICS || workload == WorkloadType::RAG) {
            // Higher compression for these workloads
            config.pq_m = 16;  // More sub-quantizers
        } else {
            config.pq_m = 8;   // Standard compression
        }
        
        // Ensure pq_m divides dimension
        while (dimension % config.pq_m != 0 && config.pq_m > 4) {
            config.pq_m--;
        }
        
        config.pq_nbits = 8;  // Standard 8 bits
    } else {
        config.use_pq = false;  // No PQ for IVF_FLAT or HNSW_FLAT
    }
    
    // Training size recommendations (only set if not already set by workload)
    if (config.train_size == 0) {
        config.train_size = std::max(size_t(30 * config.nlist), 
                                     std::min(size_t(100000), dataset_size / 10));
    }
    
    // GPU usage recommendations
    if (workload == WorkloadType::ANALYTICS || dataset_size > 1000000) {
        config.use_gpu = true;  // Recommend GPU for large/analytical workloads
    }
    
    THEMIS_INFO("Generated workload-optimized config: workload={}, nlist={}, nprobe={}, pq_m={}",
                static_cast<int>(workload), config.nlist, config.nprobe, config.pq_m);
    
    return config;
}

} // namespace themis
