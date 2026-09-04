/**
 * @file advanced_vector_index.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=6; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=1, Debt=0, C=5, H=4, M=11, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "index/advanced_vector_index.h"
#include <stdexcept>
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
        #include <faiss/gpu/GpuIndexFlat.h>
        #include <faiss/gpu/GpuCloner.h>
        #include <faiss/gpu/StandardGpuResources.h>
    #endif
    #include <faiss/index_io.h>
#else
    // STUB/SIMULATION NOTE:
    // Purpose: Provide empty faiss:: type stubs so the AdvancedVectorIndex
    //   class can be compiled and linked on systems without FAISS installed.
    //   All method bodies guarded by `#ifdef THEMIS_HAS_FAISS` fall back to
    //   warn-and-return-false, so callers get a clear build-time error signal.
    // Activation: THEMIS_HAS_FAISS is not defined (default); set via vcpkg
    //   feature 'faiss' or -DTHEMIS_HAS_FAISS=ON in CMake.
    // Production Delta: Vector search is fully disabled; initializeIndex()
    //   returns false; train/add/search all log WARN and return false/empty.
    // Removal Plan: Install FAISS via vcpkg and set THEMIS_HAS_FAISS=ON.
    //   GPU path additionally requires -DTHEMIS_HAS_FAISS_GPU=ON.
    // Roadmap ref: src/index/FUTURE_ENHANCEMENTS.md § "FAISS Integration (v1.5.0)"
    namespace faiss {
        /** @brief Index structure. */
        class Index {
        public:
            virtual ~Index() = default;
        };
        /** @brief Index ivfpq. */
        class IndexIVFPQ : public Index {};
        /** @brief Index ivf flat. */
        class IndexIVFFlat : public Index {};
    }
#endif

namespace themis {

#if defined(THEMIS_HAS_FAISS) && defined(THEMIS_ENABLE_CUDA) && defined(THEMIS_ENABLE_CUVS)
namespace {
bool trySearchWithCudaCuvsGate(faiss::Index *cpu_index, int gpu_device, size_t num_queries, const float *queries, size_t k,
                               std::vector<float> &distances, std::vector<int64_t> &ids, std::string &error) {
    try {
        faiss::gpu::StandardGpuResources gpu_resources;
        faiss::gpu::GpuClonerOptions cloner_options;
        cloner_options.useFloat16 = false;

        std::unique_ptr<faiss::Index> gpu_index(
            faiss::gpu::index_cpu_to_gpu(&gpu_resources, gpu_device, cpu_index, &cloner_options));
        if (!gpu_index) {
            error = "index_cpu_to_gpu returned null";
            return false;
        }

        gpu_index->search(static_cast<faiss::idx_t>(num_queries), queries, static_cast<faiss::idx_t>(k), distances.data(),
                          ids.data());
        return true;
    } catch (const std::exception &e) {
        error = e.what();
        return false;
    }
}
} // namespace
#endif

AdvancedVectorIndex::AdvancedVectorIndex(size_t dimension, const Config& config)
    : dimension_(dimension), config_(config), index_(nullptr) {
    
    THEMIS_INFO("AdvancedVectorIndex created: dim={}, nlist={}, nprobe={}, pq_m={}, type={}",
                dimension_, config_.nlist, config_.nprobe, config_.pq_m, 
                static_cast<int>(config_.index_type));
    
    initializeIndex();
}

AdvancedVectorIndex::~AdvancedVectorIndex() noexcept {
#ifdef THEMIS_HAS_FAISS
    // Gap: exception_in_destructor + delete_no_nullptr — guard delete with noexcept try/catch
    if (index_) {
        try {
            delete static_cast<faiss::Index*>(index_);
        } catch (const std::exception& e) {
            THEMIS_ERROR("AdvancedVectorIndex::~AdvancedVectorIndex: exception deleting index (ignored): {}",
                         e.what());
        } catch (...) {
            THEMIS_ERROR("AdvancedVectorIndex::~AdvancedVectorIndex: unknown exception deleting index (ignored)");
        }
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
                auto quantizer_owner = std::make_unique<faiss::IndexFlat>(dimension_, faiss::METRIC_L2);
                auto ivf_pq_owner = std::make_unique<faiss::IndexIVFPQ>(
                    quantizer_owner.get(),
                    dimension_,
                    config_.nlist,
                    config_.pq_m,
                    config_.pq_nbits,
                    faiss::METRIC_L2
                );
                auto* ivf_pq = ivf_pq_owner.get();
                ivf_pq->own_fields = true; // FAISS will delete the quantizer
                quantizer_owner.release(); // ownership transferred to ivf_pq
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
                
                idx = ivf_pq_owner.release();
                THEMIS_INFO("Created IVF+PQ index: nlist={}, m={}, nbits={}, adc={}", 
                           config_.nlist, config_.pq_m, config_.pq_nbits, 
                           config_.use_adc_tables);
                break;
            }
            
            case Config::Type::IVF_FLAT: {
                // IVF without compression (faster, more memory)
                auto quantizer_owner = std::make_unique<faiss::IndexFlat>(dimension_, faiss::METRIC_L2);
                auto ivf_flat_owner = std::make_unique<faiss::IndexIVFFlat>(
                    quantizer_owner.get(),
                    dimension_,
                    config_.nlist,
                    faiss::METRIC_L2
                );
                auto* ivf_flat = ivf_flat_owner.get();
                ivf_flat->own_fields = true; // FAISS will delete the quantizer
                quantizer_owner.release(); // ownership transferred to ivf_flat
                ivf_flat->nprobe = config_.nprobe;
                idx = ivf_flat_owner.release();
                THEMIS_INFO("Created IVF Flat index: nlist={}", config_.nlist);
                break;
            }
            
            case Config::Type::HNSW_FLAT: {
                // HNSW without IVF (best accuracy)
                auto hnsw_owner = std::make_unique<faiss::IndexHNSWFlat>(static_cast<int>(dimension_), 32);
                idx = hnsw_owner.release();
                THEMIS_INFO("Created HNSW Flat index");
                break;
            }
            
            default:
                THEMIS_ERROR("Unsupported index type");
                return false;
        }
        
        index_ = static_cast<void*>(idx);
        is_trained_ = idx->is_trained;
        return true;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to initialize FAISS index: {}", e.what());
        return false;
    }
#else
    StubCallbacks callbacks = StubCallbacks();
    {
        std::lock_guard<std::mutex> lk(AdvancedVectorIndex::stubCallbacksMutex());
        callbacks = AdvancedVectorIndex::stubCallbacksStorage();
    }
    if (callbacks.initialize) {
        try {
            return callbacks.initialize(dimension_, config_);
        } catch (const std::exception& e) {
            THEMIS_ERROR("AdvancedVectorIndex::initializeIndex callback failed: {}", e.what());
            return false;
        } catch (...) {
            THEMIS_ERROR("AdvancedVectorIndex::initializeIndex callback failed");
            return false;
        }
    }
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
    StubCallbacks callbacks = StubCallbacks();
    {
        std::lock_guard<std::mutex> lk(AdvancedVectorIndex::stubCallbacksMutex());
        callbacks = AdvancedVectorIndex::stubCallbacksStorage();
    }
    if (callbacks.train) {
        try {
            const bool ok = callbacks.train(vectors, count);
            is_trained_ = ok;
            return ok;
        } catch (const std::exception& e) {
            THEMIS_ERROR("AdvancedVectorIndex::train callback failed: {}", e.what());
            return false;
        } catch (...) {
            THEMIS_ERROR("AdvancedVectorIndex::train callback failed");
            return false;
        }
    }
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

    auto* idx = static_cast<faiss::Index*>(index_);
    if (idx->is_trained) {
        is_trained_ = true;
    }
    
    if (!is_trained_) {
        THEMIS_ERROR("Index not trained - call train() first");
        return false;
    }
    
    try {
        idx->add(count, vectors);
        
        THEMIS_INFO("Added {} vectors to index (total: {})", count, idx->ntotal);
        return true;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Add failed: {}", e.what());
        return false;
    }
#else
    StubCallbacks callbacks = StubCallbacks();
    {
        std::lock_guard<std::mutex> lk(AdvancedVectorIndex::stubCallbacksMutex());
        callbacks = AdvancedVectorIndex::stubCallbacksStorage();
    }
    if (callbacks.add) {
        try {
            return callbacks.add(vectors, count);
        } catch (const std::exception& e) {
            THEMIS_ERROR("AdvancedVectorIndex::add callback failed: {}", e.what());
            return false;
        } catch (...) {
            THEMIS_ERROR("AdvancedVectorIndex::add callback failed");
            return false;
        }
    }
    return false;
#endif
}

bool AdvancedVectorIndex::addWithIds(const float* vectors, const int64_t* ids, size_t count) {
#ifdef THEMIS_HAS_FAISS
    if (!index_) {
        THEMIS_ERROR("Index not initialized");
        return false;
    }

    auto* idx = static_cast<faiss::Index*>(index_);
    if (idx->is_trained) {
        is_trained_ = true;
    }
    
    if (!is_trained_) {
        THEMIS_ERROR("Index not trained - call train() first");
        return false;
    }
    
    try {
        idx->add_with_ids(count, vectors, ids);
        
        THEMIS_INFO("Added {} vectors with IDs to index", count);
        return true;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Add with IDs failed: {}", e.what());
        return false;
    }
#else
    StubCallbacks callbacks = StubCallbacks();
    {
        std::lock_guard<std::mutex> lk(AdvancedVectorIndex::stubCallbacksMutex());
        callbacks = AdvancedVectorIndex::stubCallbacksStorage();
    }
    if (callbacks.add_with_ids) {
        try {
            return callbacks.add_with_ids(vectors, ids, count);
        } catch (const std::exception& e) {
            THEMIS_ERROR("AdvancedVectorIndex::addWithIds callback failed: {}", e.what());
            return false;
        } catch (...) {
            THEMIS_ERROR("AdvancedVectorIndex::addWithIds callback failed");
            return false;
        }
    }
    return false;
#endif
}

AdvancedVectorIndex::SearchResult AdvancedVectorIndex::search(const float* query, size_t k) {
    SearchResult result = SearchResult{};
    
#ifdef THEMIS_HAS_FAISS
    if (!index_) {
        THEMIS_ERROR("Index not initialized");
        return result;
    }
    
    try {
        auto* idx = static_cast<faiss::Index*>(index_);
        result.ids.resize(k);
        result.distances.resize(k);

#if defined(THEMIS_ENABLE_CUDA) && defined(THEMIS_ENABLE_CUVS)
        if (config_.use_gpu) {
            std::string gpu_error = {};
            if (trySearchWithCudaCuvsGate(idx, config_.gpu_device, 1, query, k, result.distances, result.ids, gpu_error)) {
                return result;
            }
            THEMIS_WARN("AdvancedVectorIndex CUDA/cuVS gate search failed (device={}): {} — falling back to CPU index search",
                        config_.gpu_device, gpu_error);
        }
#endif

        idx->search(1, query, k, result.distances.data(), result.ids.data());
        
        return result;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Search failed: {}", e.what());
        return result;
    }
#else
    StubCallbacks callbacks = StubCallbacks();
    {
        std::lock_guard<std::mutex> lk(AdvancedVectorIndex::stubCallbacksMutex());
        callbacks = AdvancedVectorIndex::stubCallbacksStorage();
    }
    if (callbacks.search) {
        try {
            return callbacks.search(query, k);
        } catch (const std::exception& e) {
            THEMIS_ERROR("AdvancedVectorIndex::search callback failed: {}", e.what());
            return result;
        } catch (...) {
            THEMIS_ERROR("AdvancedVectorIndex::search callback failed");
            return result;
        }
    }
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

#if defined(THEMIS_ENABLE_CUDA) && defined(THEMIS_ENABLE_CUVS)
        if (config_.use_gpu) {
            std::string gpu_error = {};
            if (!trySearchWithCudaCuvsGate(idx, config_.gpu_device, num_queries, queries, k, all_distances, all_ids,
                                           gpu_error)) {
                THEMIS_WARN("AdvancedVectorIndex CUDA/cuVS gate batch search failed (device={}): {} — falling back to CPU index search",
                            config_.gpu_device, gpu_error);
                idx->search(num_queries, queries, k, all_distances.data(), all_ids.data());
            }
        } else {
            idx->search(num_queries, queries, k, all_distances.data(), all_ids.data());
        }
#else
        idx->search(num_queries, queries, k, all_distances.data(), all_ids.data());
#endif
        
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
    StubCallbacks callbacks = StubCallbacks();
    {
        std::lock_guard<std::mutex> lk(AdvancedVectorIndex::stubCallbacksMutex());
        callbacks = AdvancedVectorIndex::stubCallbacksStorage();
    }
    if (callbacks.search_batch) {
        try {
            return callbacks.search_batch(queries, num_queries, k);
        } catch (const std::exception& e) {
            THEMIS_ERROR("AdvancedVectorIndex::searchBatch callback failed: {}", e.what());
            return results;
        } catch (...) {
            THEMIS_ERROR("AdvancedVectorIndex::searchBatch callback failed");
            return results;
        }
    }
    return results;
#endif
}

AdvancedVectorIndex::Stats AdvancedVectorIndex::getStats() const {
    Stats stats = Stats{};
    
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
#else
    StubCallbacks callbacks = StubCallbacks();
    {
        std::lock_guard<std::mutex> lk(AdvancedVectorIndex::stubCallbacksMutex());
        callbacks = AdvancedVectorIndex::stubCallbacksStorage();
    }
    if (callbacks.stats) {
        try {
            return callbacks.stats();
        } catch (const std::exception& e) {
            THEMIS_ERROR("AdvancedVectorIndex::getStats callback failed: {}", e.what());
        } catch (...) {
            THEMIS_ERROR("AdvancedVectorIndex::getStats callback failed");
        }
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
    StubCallbacks callbacks = StubCallbacks();
    {
        std::lock_guard<std::mutex> lk(AdvancedVectorIndex::stubCallbacksMutex());
        callbacks = AdvancedVectorIndex::stubCallbacksStorage();
    }
    if (callbacks.save) {
        try {
            return callbacks.save(path);
        } catch (const std::exception& e) {
            THEMIS_ERROR("AdvancedVectorIndex::save callback failed: {}", e.what());
            return false;
        } catch (...) {
            THEMIS_ERROR("AdvancedVectorIndex::save callback failed");
            return false;
        }
    }
    return false;
#endif
}

bool AdvancedVectorIndex::load(const std::string& path) {
#ifdef THEMIS_HAS_FAISS
    try {
        if (index_) {
            // Gap: delete_no_nullptr — guard faiss::Index dtor exceptions; set nullptr first
            auto* old = static_cast<faiss::Index*>(index_);
            index_ = nullptr;  // prevent dangling pointer regardless of dtor outcome
            try {
                delete old;
            } catch (...) {
                THEMIS_ERROR("AdvancedVectorIndex::load: exception deleting old index (ignored)");
            }
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
    StubCallbacks callbacks = StubCallbacks();
    {
        std::lock_guard<std::mutex> lk(AdvancedVectorIndex::stubCallbacksMutex());
        callbacks = AdvancedVectorIndex::stubCallbacksStorage();
    }
    if (callbacks.load) {
        try {
            return callbacks.load(path);
        } catch (const std::exception& e) {
            THEMIS_ERROR("AdvancedVectorIndex::load callback failed: {}", e.what());
            return false;
        } catch (...) {
            THEMIS_ERROR("AdvancedVectorIndex::load callback failed");
            return false;
        }
    }
    return false;
#endif
}

AdvancedVectorIndex::Config AdvancedVectorIndex::getWorkloadOptimizedConfig(
    size_t dataset_size,
    size_t dimension,
    WorkloadType workload) {
    
    Config config = Config{};
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
        [[fallthrough]];
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

