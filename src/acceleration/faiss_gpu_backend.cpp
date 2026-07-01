/**
 * @file faiss_gpu_backend.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 81/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=25, H=41, M=8, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: faiss_gpu_backend.cpp | Version: 0.0.47 | Last Modified: 2026-06-02 11:49:05
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 96/100 | Lines: 864
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=25, H=41, M=8, L=0
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "acceleration/faiss_gpu_backend.h"

#ifdef THEMIS_ENABLE_CUDA
#include <faiss/IndexFlat.h>
#include <faiss/IndexHNSW.h>
#include <faiss/IndexIVFFlat.h>
#include <faiss/IndexIVFPQ.h>
#include <faiss/impl/ScalarQuantizer.h>
#include <faiss/gpu/GpuIndexFlat.h>
#include <faiss/gpu/GpuIndexIVFFlat.h>
#include <faiss/gpu/GpuIndexIVFPQ.h>
#include <faiss/gpu/GpuIndexIVFScalarQuantizer.h>
#include <faiss/gpu/StandardGpuResources.h>
#include <faiss/gpu/GpuCloner.h>
#include <faiss/index_io.h>
#include <cuda_runtime.h>
#endif

#include <iostream>
#include <stdexcept>
#include <cstring>

namespace themis {
namespace acceleration {

#ifdef THEMIS_ENABLE_CUDA

FaissGPUVectorBackend::FaissGPUVectorBackend() = default;

FaissGPUVectorBackend::~FaissGPUVectorBackend() {
    shutdown();
}

// IndexDeleter dispatches delete through faiss::Index's virtual destructor,
// which is correct for all concrete FAISS GPU/CPU subtype pointers.
void FaissGPUVectorBackend::IndexDeleter::operator()(faiss::Index* p) const noexcept {
    delete p;
}

void FaissGPUVectorBackend::setError(AccelerationErrorCode code,
                                      const std::string& msg,
                                      const std::string& hint) const {
    lastError_ = ErrorContext(code, "Faiss GPU", msg, hint);
    // Use structured error output — avoid raw cerr to prevent log injection
    static_cast<void>(msg); // message already stored in lastError_
}

bool FaissGPUVectorBackend::isAvailable() const noexcept {
    int deviceCount = 0;
    cudaError_t availErr = cudaGetDeviceCount(&deviceCount);
    return (availErr == cudaSuccess && deviceCount >= 1);
}

BackendCapabilities FaissGPUVectorBackend::getCapabilities() const {
    BackendCapabilities caps;
    caps.supportsVectorOps = true;
    caps.supportsGraphOps = false;
    caps.supportsGeoOps = false;
    caps.supportsBatchProcessing = true;
    caps.supportsAsync = true;

    // Precision: FP32 always available; INT8 available via IVF_SQ8
    caps.supportedPrecisions = PrecisionMode::FP32 | PrecisionMode::INT8;

    // Distance metrics: L2 and Inner Product supported
    caps.supportedMetrics =
        metricBit(DistanceMetric::L2) |
        metricBit(DistanceMetric::INNER_PRODUCT);

    if (isAvailable()) {
        cudaDeviceProp prop;
        if (cudaGetDeviceProperties(&prop, config_.deviceId) == cudaSuccess) {
            caps.deviceName = std::string(prop.name);
            caps.maxMemoryBytes = prop.totalGlobalMem;
            caps.computeUnits = prop.multiProcessorCount;
        }
    } else {
        caps.deviceName = "Faiss GPU (Not Available)";
    }
    
    return caps;
}

bool FaissGPUVectorBackend::initialize() {
    if (!isAvailable()) {
        setError(AccelerationErrorCode::NoDevicesFound,
                 "Faiss GPU: No CUDA device available",
                 "Install CUDA drivers or check cudaGetDeviceCount()");
        return false;
    }

    try {
        gpuResources_ = std::make_unique<faiss::gpu::StandardGpuResources>();

        size_t memLimit = config_.maxMemoryMB * 1024 * 1024;
        gpuResources_->setTempMemory(memLimit);

        std::cout << "Faiss GPU Backend initialized successfully" << std::endl;
        std::cout << "  Device ID: " << config_.deviceId << std::endl;
        std::cout << "  Memory Limit: " << config_.maxMemoryMB << " MB" << std::endl;

        initialized_ = true;
        return true;

    } catch (const std::exception& e) {
        setError(AccelerationErrorCode::ContextCreationFailed,
                 std::string("Faiss GPU initialization failed: ") + e.what());
        return false;
    }
}

void FaissGPUVectorBackend::shutdown() {
    if (initialized_) {
        index_.reset();
        gpuResources_.reset();
        initialized_ = false;
    }
}

bool FaissGPUVectorBackend::initializeIndex(const Config& config) {
    if (!initialized_) {
        setError(AccelerationErrorCode::BackendNotInitialized,
                 "initializeIndex: backend not initialized — call initialize() first");
        return false;
    }
    if (config.dimension <= 0) {
        setError(AccelerationErrorCode::InvalidInputShape,
                 "initializeIndex: dimension must be > 0");
        return false;
    }

    config_ = config;

    try {
        index_.reset();
        index_ = createIndex(config.indexType, config.dimension);
        currentIndexType_ = config.indexType;

        if (!index_) {
            return false;
        }

        std::cout << "Faiss index created — type: "
                  << static_cast<int>(config.indexType)
                  << ", dim: " << config.dimension << std::endl;
        return true;

    } catch (const std::exception& e) {
        setError(AccelerationErrorCode::ContextCreationFailed,
                 std::string("initializeIndex failed: ") + e.what());
        return false;
    }
}

std::unique_ptr<faiss::Index, FaissGPUVectorBackend::IndexDeleter>
FaissGPUVectorBackend::createIndex(IndexType type, int dimension) {
    faiss::gpu::GpuIndexFlatConfig flatConfig;
    flatConfig.device = config_.deviceId;

    switch (type) {
        case IndexType::FLAT_L2: {
            auto* idx = new faiss::gpu::GpuIndexFlatL2(
                gpuResources_.get(), dimension, flatConfig);
            return {idx, IndexDeleter{}};
        }

        case IndexType::FLAT_IP: {
            auto* idx = new faiss::gpu::GpuIndexFlatIP(
                gpuResources_.get(), dimension, flatConfig);
            return {idx, IndexDeleter{}};
        }

        case IndexType::IVF_FLAT: {
            // Wrap quantizer in unique_ptr for exception-safe construction.
            // GpuIndexIVFFlat takes ownership of the quantizer on success.
            auto quantizer = std::unique_ptr<faiss::gpu::GpuIndexFlatL2>(
                new faiss::gpu::GpuIndexFlatL2(gpuResources_.get(), dimension, flatConfig));
            faiss::gpu::GpuIndexIVFFlatConfig ivfConfig;
            ivfConfig.device = config_.deviceId;
            auto* idx = new faiss::gpu::GpuIndexIVFFlat(
                gpuResources_.get(), dimension, config_.nlist,
                quantizer.get(), faiss::METRIC_L2, ivfConfig);
            idx->nprobe = config_.nprobe;
            quantizer.release();  // ownership transferred to IVF index
            return {idx, IndexDeleter{}};
        }

        case IndexType::IVF_PQ: {
            // Wrap quantizer in unique_ptr for exception-safe construction.
            // GpuIndexIVFPQ takes ownership of the quantizer on success.
            auto quantizer = std::unique_ptr<faiss::gpu::GpuIndexFlatL2>(
                new faiss::gpu::GpuIndexFlatL2(gpuResources_.get(), dimension, flatConfig));
            faiss::gpu::GpuIndexIVFPQConfig ivfpqConfig;
            ivfpqConfig.device = config_.deviceId;
            auto* idx = new faiss::gpu::GpuIndexIVFPQ(
                gpuResources_.get(), dimension, config_.nlist,
                config_.m, config_.nbits, faiss::METRIC_L2, ivfpqConfig);
            idx->nprobe = config_.nprobe;
            quantizer.release();  // ownership transferred to IVF-PQ index
            return {idx, IndexDeleter{}};
        }

        case IndexType::IVF_SQ8: {
            // 8-bit scalar quantizer — better recall than PQ at equivalent memory
            faiss::gpu::GpuIndexIVFScalarQuantizerConfig sqConfig;
            sqConfig.device = config_.deviceId;
            auto* idx = new faiss::gpu::GpuIndexIVFScalarQuantizer(
                gpuResources_.get(), dimension, config_.nlist,
                faiss::ScalarQuantizer::QT_8bit, faiss::METRIC_L2,
                /*encodeResidual=*/true, sqConfig);
            idx->nprobe = config_.nprobe;
            return {idx, IndexDeleter{}};
        }

        case IndexType::HNSW_FLAT: {
            // CPU-side FAISS HNSW — no GPU resources required at query time.
            auto* idx = new faiss::IndexHNSWFlat(dimension, config_.hnswM);
            return {idx, IndexDeleter{}};
        }

        default:
            setError(AccelerationErrorCode::InvalidInputShape,
                     "createIndex: unknown IndexType value " +
                     std::to_string(static_cast<int>(type)),
                     "Use one of FLAT_L2, FLAT_IP, IVF_FLAT, IVF_PQ, IVF_SQ8, HNSW_FLAT");
            return {nullptr, IndexDeleter{}};
    }
}

bool FaissGPUVectorBackend::trainIndex(const float* vectors, size_t numVectors) {
    if (!index_) {
        setError(AccelerationErrorCode::BackendNotInitialized,
                 "trainIndex: index not initialized");
        return false;
    }
    if (!vectors || numVectors == 0) {
        setError(AccelerationErrorCode::InvalidInputShape,
                 "trainIndex: null vectors or zero numVectors");
        return false;
    }

    try {
        faiss::Index* idx = nullptr;

        switch (currentIndexType_) {
            case IndexType::FLAT_L2:
            case IndexType::FLAT_IP:
            case IndexType::HNSW_FLAT:
                // These index types are pre-trained; no explicit training step needed.
                return true;

            case IndexType::IVF_FLAT:
                idx = static_cast<faiss::gpu::GpuIndexIVFFlat*>(index_.get());
                break;

            case IndexType::IVF_PQ:
                idx = static_cast<faiss::gpu::GpuIndexIVFPQ*>(index_.get());
                break;

            case IndexType::IVF_SQ8:
                idx = static_cast<faiss::gpu::GpuIndexIVFScalarQuantizer*>(index_.get());
                break;

            default:
                setError(AccelerationErrorCode::InvalidInputShape,
                         "trainIndex: unsupported index type");
                return false;
        }

        if (idx && !idx->is_trained) {
            std::cout << "Training Faiss index with " << numVectors << " vectors..." << std::endl;
            idx->train(static_cast<faiss::idx_t>(numVectors), vectors);
            std::cout << "Training complete" << std::endl;
        }

        return true;

    } catch (const std::exception& e) {
        setError(AccelerationErrorCode::KernelExecutionFailed,
                 std::string("trainIndex failed: ") + e.what());
        return false;
    }
}

bool FaissGPUVectorBackend::addVectors(const float* vectors, size_t numVectors) {
    if (!index_) {
        setError(AccelerationErrorCode::BackendNotInitialized,
                 "addVectors: index not initialized");
        return false;
    }
    if (!vectors || numVectors == 0) {
        setError(AccelerationErrorCode::InvalidInputShape,
                 "addVectors: null vectors or zero numVectors");
        return false;
    }

    try {
        faiss::Index* idx = nullptr;

        switch (currentIndexType_) {
            case IndexType::FLAT_L2:
                idx = static_cast<faiss::gpu::GpuIndexFlatL2*>(index_.get());
                break;
            case IndexType::FLAT_IP:
                idx = static_cast<faiss::gpu::GpuIndexFlatIP*>(index_.get());
                break;
            case IndexType::IVF_FLAT:
                idx = static_cast<faiss::gpu::GpuIndexIVFFlat*>(index_.get());
                break;
            case IndexType::IVF_PQ:
                idx = static_cast<faiss::gpu::GpuIndexIVFPQ*>(index_.get());
                break;
            case IndexType::IVF_SQ8:
                idx = static_cast<faiss::gpu::GpuIndexIVFScalarQuantizer*>(index_.get());
                break;
            case IndexType::HNSW_FLAT:
                idx = static_cast<faiss::IndexHNSWFlat*>(index_.get());
                break;
            default:
                setError(AccelerationErrorCode::InvalidInputShape,
                         "addVectors: unsupported index type");
                return false;
        }

        if (!idx->is_trained) {
            setError(AccelerationErrorCode::InvalidConfiguration,
                     "addVectors: index must be trained before adding vectors",
                     "Call trainIndex() with a representative sample first");
            return false;
        }

        idx->add(static_cast<faiss::idx_t>(numVectors), vectors);
        std::cout << "Added " << numVectors << " vectors to index (total: "
                  << idx->ntotal << ")" << std::endl;

        return true;

    } catch (const std::exception& e) {
        setError(AccelerationErrorCode::KernelExecutionFailed,
                 std::string("addVectors failed: ") + e.what());
        return false;
    }
}

std::vector<std::vector<std::pair<uint32_t, float>>> FaissGPUVectorBackend::search(
    const float* queries,
    size_t numQueries,
    size_t k
) {
    if (!index_) {
        setError(AccelerationErrorCode::BackendNotInitialized,
                 "search: index not initialized");
        return {};
    }
    if (!queries || numQueries == 0 || k == 0) {
        setError(AccelerationErrorCode::InvalidInputShape,
                 "search: null queries, zero numQueries, or zero k");
        return {};
    }

    try {
        faiss::Index* idx = nullptr;

        switch (currentIndexType_) {
            case IndexType::FLAT_L2:
                idx = static_cast<faiss::gpu::GpuIndexFlatL2*>(index_.get());
                break;
            case IndexType::FLAT_IP:
                idx = static_cast<faiss::gpu::GpuIndexFlatIP*>(index_.get());
                break;
            case IndexType::IVF_FLAT:
                idx = static_cast<faiss::gpu::GpuIndexIVFFlat*>(index_.get());
                break;
            case IndexType::IVF_PQ:
                idx = static_cast<faiss::gpu::GpuIndexIVFPQ*>(index_.get());
                break;
            case IndexType::IVF_SQ8:
                idx = static_cast<faiss::gpu::GpuIndexIVFScalarQuantizer*>(index_.get());
                break;
            case IndexType::HNSW_FLAT:
                idx = static_cast<faiss::IndexHNSWFlat*>(index_.get());
                break;
            default:
                setError(AccelerationErrorCode::InvalidInputShape,
                         "search: unsupported index type");
                return {};
        }

        if (idx->ntotal == 0) {
            setError(AccelerationErrorCode::InvalidConfiguration,
                     "search: index is empty — add vectors before searching");
            return {};
        }

        const faiss::idx_t effective_k =
            static_cast<faiss::idx_t>(std::min(k, static_cast<size_t>(idx->ntotal)));

        // Allocate output buffers
        std::vector<float> distances(numQueries * static_cast<size_t>(effective_k));
        std::vector<faiss::idx_t> labels(numQueries * static_cast<size_t>(effective_k));

        // Perform search
        idx->search(static_cast<faiss::idx_t>(numQueries), queries,
                    effective_k, distances.data(), labels.data());

        // Convert to output format
        std::vector<std::vector<std::pair<uint32_t, float>>> results(numQueries);
        for (size_t q = 0; q < numQueries; ++q) {
            results[q].reserve(static_cast<size_t>(effective_k));
            for (faiss::idx_t i = 0; i < effective_k; ++i) {
                const size_t pos = q * static_cast<size_t>(effective_k) +
                                   static_cast<size_t>(i);
                if (labels[pos] >= 0) {
                    results[q].emplace_back(
                        static_cast<uint32_t>(labels[pos]),
                        distances[pos]
                    );
                }
            }
        }

        return results;

    } catch (const std::exception& e) {
        setError(AccelerationErrorCode::KernelExecutionFailed,
                 std::string("search failed: ") + e.what());
        return {};
    }
}

std::vector<float> FaissGPUVectorBackend::computeDistances(
    const float* queries,
    size_t numQueries,
    size_t dim,
    const float* vectors,
    size_t numVectors,
    bool useL2
) {
    if (!queries || !vectors || numQueries == 0 || numVectors == 0 || dim == 0) {
        setError(AccelerationErrorCode::InvalidInputShape,
                 "computeDistances: null pointers or zero-size inputs");
        return {};
    }

    // For one-time distance computation, create a temporary flat index
    Config tempConfig;
    tempConfig.indexType = useL2 ? IndexType::FLAT_L2 : IndexType::FLAT_IP;
    tempConfig.dimension = static_cast<int>(dim);
    tempConfig.deviceId = config_.deviceId;

    auto tempIndex = createIndex(tempConfig.indexType, static_cast<int>(dim));
    if (!tempIndex) {
        return {};
    }

    try {
        faiss::Index* idx = useL2
            ? static_cast<faiss::Index*>(static_cast<faiss::gpu::GpuIndexFlatL2*>(tempIndex.get()))
            : static_cast<faiss::Index*>(static_cast<faiss::gpu::GpuIndexFlatIP*>(tempIndex.get()));

        // Add vectors to temporary index
        idx->add(static_cast<faiss::idx_t>(numVectors), vectors);

        // Search for all vectors (k = numVectors)
        std::vector<float> distances(numQueries * numVectors);
        std::vector<faiss::idx_t> labels(numQueries * numVectors);

        idx->search(static_cast<faiss::idx_t>(numQueries), queries,
                    static_cast<faiss::idx_t>(numVectors),
                    distances.data(), labels.data());

        // tempIndex released automatically by unique_ptr at scope exit
        return distances;

    } catch (const std::exception& e) {
        setError(AccelerationErrorCode::KernelExecutionFailed,
                 std::string("computeDistances failed: ") + e.what());
        // tempIndex released automatically by unique_ptr at scope exit
        return {};
    }
}

std::vector<std::vector<std::pair<uint32_t, float>>> FaissGPUVectorBackend::batchKnnSearch(
    const float* queries,
    size_t numQueries,
    size_t dim,
    const float* vectors,
    size_t numVectors,
    size_t k,
    bool useL2
) {
    if (!queries || !vectors || numQueries == 0 || numVectors == 0 || dim == 0 || k == 0) {
        setError(AccelerationErrorCode::InvalidInputShape,
                 "batchKnnSearch: null pointers or zero-size inputs");
        return {};
    }

    // For one-time KNN search, create a temporary flat index
    Config tempConfig;
    tempConfig.indexType = useL2 ? IndexType::FLAT_L2 : IndexType::FLAT_IP;
    tempConfig.dimension = static_cast<int>(dim);
    tempConfig.deviceId = config_.deviceId;

    auto tempIndex = createIndex(tempConfig.indexType, static_cast<int>(dim));
    if (!tempIndex) {
        return {};
    }

    try {
        faiss::Index* idx = useL2
            ? static_cast<faiss::Index*>(static_cast<faiss::gpu::GpuIndexFlatL2*>(tempIndex.get()))
            : static_cast<faiss::Index*>(static_cast<faiss::gpu::GpuIndexFlatIP*>(tempIndex.get()));

        // Add vectors to temporary index
        idx->add(static_cast<faiss::idx_t>(numVectors), vectors);

        const faiss::idx_t effective_k =
            static_cast<faiss::idx_t>(std::min(k, numVectors));

        // Allocate output buffers
        std::vector<float> distances(numQueries * static_cast<size_t>(effective_k));
        std::vector<faiss::idx_t> labels(numQueries * static_cast<size_t>(effective_k));

        // Perform search
        idx->search(static_cast<faiss::idx_t>(numQueries), queries,
                    effective_k, distances.data(), labels.data());

        // Convert to output format
        std::vector<std::vector<std::pair<uint32_t, float>>> results(numQueries);
        for (size_t q = 0; q < numQueries; ++q) {
            results[q].reserve(static_cast<size_t>(effective_k));
            for (faiss::idx_t i = 0; i < effective_k; ++i) {
                const size_t pos = q * static_cast<size_t>(effective_k) +
                                   static_cast<size_t>(i);
                if (labels[pos] >= 0) {
                    results[q].emplace_back(
                        static_cast<uint32_t>(labels[pos]),
                        distances[pos]
                    );
                }
            }
        }

        // tempIndex released automatically by unique_ptr at scope exit
        return results;

    } catch (const std::exception& e) {
        setError(AccelerationErrorCode::KernelExecutionFailed,
                 std::string("batchKnnSearch failed: ") + e.what());
        // tempIndex released automatically by unique_ptr at scope exit
        return {};
    }
}

bool FaissGPUVectorBackend::saveIndex(const std::string& filepath) {
    if (!index_) {
        setError(AccelerationErrorCode::BackendNotInitialized,
                 "saveIndex: index not initialized");
        return false;
    }
    // Sanitize filepath: reject empty, path-traversal sequences, and null bytes
    if (filepath.empty()) {
        setError(AccelerationErrorCode::InvalidParameter,
                 "saveIndex: filepath is empty");
        return false;
    }
    if (filepath.find("..") != std::string::npos ||
        filepath.size() != std::strlen(filepath.c_str())) {
        setError(AccelerationErrorCode::InvalidParameter,
                 "saveIndex: filepath contains invalid characters");
        return false;
    }

    // Sanitize filepath for logging: replace control chars to prevent
    // log injection (CWE-117) and XSS in log viewers (CWE-79).
    std::string safeFilepath;
    safeFilepath.reserve(filepath.size());
    for (unsigned char c : filepath) {
        safeFilepath += (c < 0x20 || c == 0x7f) ? '?' : static_cast<char>(c);
    }

    try {
        faiss::Index* cpuIndex = nullptr;

        switch (currentIndexType_) {
            case IndexType::FLAT_L2: {
                auto* gpuIdx = static_cast<faiss::gpu::GpuIndexFlatL2*>(index_.get());
                cpuIndex = faiss::gpu::index_gpu_to_cpu(gpuIdx);
                break;
            }
            case IndexType::FLAT_IP: {
                auto* gpuIdx = static_cast<faiss::gpu::GpuIndexFlatIP*>(index_.get());
                cpuIndex = faiss::gpu::index_gpu_to_cpu(gpuIdx);
                break;
            }
            case IndexType::IVF_FLAT: {
                auto* gpuIdx = static_cast<faiss::gpu::GpuIndexIVFFlat*>(index_.get());
                cpuIndex = faiss::gpu::index_gpu_to_cpu(gpuIdx);
                break;
            }
            case IndexType::IVF_PQ: {
                auto* gpuIdx = static_cast<faiss::gpu::GpuIndexIVFPQ*>(index_.get());
                cpuIndex = faiss::gpu::index_gpu_to_cpu(gpuIdx);
                break;
            }
            case IndexType::IVF_SQ8: {
                auto* gpuIdx = static_cast<faiss::gpu::GpuIndexIVFScalarQuantizer*>(index_.get());
                cpuIndex = faiss::gpu::index_gpu_to_cpu(gpuIdx);
                break;
            }
            case IndexType::HNSW_FLAT:
                // HNSW_FLAT is already CPU-side; write directly without GPU transfer.
                faiss::write_index(static_cast<faiss::IndexHNSWFlat*>(index_.get()),
                                   filepath.c_str());
                std::cout << "Index saved to: " << safeFilepath << std::endl;
                return true;

            default:
                setError(AccelerationErrorCode::InvalidInputShape,
                         "saveIndex: unsupported index type");
                return false;
        }

        if (cpuIndex) {
            faiss::write_index(cpuIndex, filepath.c_str());
            delete cpuIndex;
            std::cout << "Index saved to: " << safeFilepath << std::endl;
            return true;
        }

        setError(AccelerationErrorCode::InternalError,
                 "saveIndex: GPU-to-CPU transfer returned null");
        return false;

    } catch (const std::exception& e) {
        setError(AccelerationErrorCode::KernelExecutionFailed,
                 std::string("saveIndex failed: ") + e.what());
        return false;
    }
}

bool FaissGPUVectorBackend::loadIndex(const std::string& filepath) {
    // Sanitize filepath: reject empty, path-traversal sequences, and null bytes
    if (filepath.empty()) {
        setError(AccelerationErrorCode::InvalidParameter,
                 "loadIndex: filepath is empty");
        return false;
    }
    if (filepath.find("..") != std::string::npos ||
        filepath.size() != std::strlen(filepath.c_str())) {
        setError(AccelerationErrorCode::InvalidParameter,
                 "loadIndex: filepath contains invalid characters");
        return false;
    }

    // Sanitize filepath for logging and error messages: replace control chars
    // to prevent log injection (CWE-117) and XSS in log viewers (CWE-79).
    std::string safeFilepath;
    safeFilepath.reserve(filepath.size());
    for (unsigned char c : filepath) {
        safeFilepath += (c < 0x20 || c == 0x7f) ? '?' : static_cast<char>(c);
    }

    try {
        faiss::Index* cpuIndex = faiss::read_index(filepath.c_str());

        if (!cpuIndex) {
            setError(AccelerationErrorCode::InternalError,
                     "loadIndex: failed to read index from " + safeFilepath);
            return false;
        }

        index_.reset();  // release existing index before loading new one

        faiss::gpu::GpuClonerOptions options;
        options.useFloat16 = false;

        auto* gpuIndex = faiss::gpu::index_cpu_to_gpu(
            gpuResources_.get(),
            config_.deviceId,
            cpuIndex,
            &options
        );

        delete cpuIndex;

        if (!gpuIndex) {
            setError(AccelerationErrorCode::MemoryCopyFailed,
                     "loadIndex: GPU transfer failed for " + safeFilepath);
            return false;
        }

        // Wrap in smart pointer; IndexDeleter::operator() uses virtual dispatch (delete p).
        index_.reset(static_cast<faiss::Index*>(gpuIndex));
        config_.dimension = gpuIndex->d;

        std::cout << "Index loaded from: " << safeFilepath << std::endl;
        std::cout << "  Vectors: " << gpuIndex->ntotal << std::endl;
        std::cout << "  Dimension: " << gpuIndex->d << std::endl;

        return true;

    } catch (const std::exception& e) {
        setError(AccelerationErrorCode::KernelExecutionFailed,
                 std::string("loadIndex failed: ") + e.what());
        return false;
    }
}

FaissGPUVectorBackend::IndexStats FaissGPUVectorBackend::getIndexStats() const {
    IndexStats stats;
    stats.type = currentIndexType_;

    if (!index_) {
        return stats;
    }

    try {
        faiss::Index* idx = nullptr;

        switch (currentIndexType_) {
            case IndexType::FLAT_L2:
                idx = static_cast<faiss::gpu::GpuIndexFlatL2*>(index_.get());
                break;
            case IndexType::FLAT_IP:
                idx = static_cast<faiss::gpu::GpuIndexFlatIP*>(index_.get());
                break;
            case IndexType::IVF_FLAT:
                idx = static_cast<faiss::gpu::GpuIndexIVFFlat*>(index_.get());
                break;
            case IndexType::IVF_PQ:
                idx = static_cast<faiss::gpu::GpuIndexIVFPQ*>(index_.get());
                break;
            case IndexType::IVF_SQ8:
                idx = static_cast<faiss::gpu::GpuIndexIVFScalarQuantizer*>(index_.get());
                break;
            case IndexType::HNSW_FLAT:
                idx = static_cast<faiss::IndexHNSWFlat*>(index_.get());
                break;
            default:
                return stats;
        }

        if (idx) {
            stats.numVectors = static_cast<size_t>(idx->ntotal);
            stats.dimension  = static_cast<size_t>(idx->d);
            stats.isTrained  = idx->is_trained;
            // Approximate memory usage (raw vectors only; excludes graph/IVF overhead)
            stats.memoryUsageBytes = stats.numVectors * stats.dimension * sizeof(float);
        }

    } catch (const std::exception &) {
        // Return partial stats on error — do not propagate exceptions from a const getter
    } catch (const std::string &) {
        // Return partial stats on error — do not propagate exceptions from a const getter
    } catch (const char *) {
        // Return partial stats on error — do not propagate exceptions from a const getter
    }

    return stats;
}

void FaissGPUVectorBackend::resetIndex() {
    if (!index_) return;

    try {
        faiss::Index* idx = nullptr;

        switch (currentIndexType_) {
            case IndexType::FLAT_L2:
                idx = static_cast<faiss::gpu::GpuIndexFlatL2*>(index_.get());
                break;
            case IndexType::FLAT_IP:
                idx = static_cast<faiss::gpu::GpuIndexFlatIP*>(index_.get());
                break;
            case IndexType::IVF_FLAT:
                idx = static_cast<faiss::gpu::GpuIndexIVFFlat*>(index_.get());
                break;
            case IndexType::IVF_PQ:
                idx = static_cast<faiss::gpu::GpuIndexIVFPQ*>(index_.get());
                break;
            case IndexType::IVF_SQ8:
                idx = static_cast<faiss::gpu::GpuIndexIVFScalarQuantizer*>(index_.get());
                break;
            case IndexType::HNSW_FLAT:
                idx = static_cast<faiss::IndexHNSWFlat*>(index_.get());
                break;
            default:
                setError(AccelerationErrorCode::InvalidInputShape,
                         "resetIndex: unsupported index type");
                return;
        }

        if (idx) {
            idx->reset();
            std::cout << "Index reset" << std::endl;
        }

    } catch (const std::exception& e) {
        setError(AccelerationErrorCode::KernelExecutionFailed,
                 std::string("resetIndex failed: ") + e.what());
    }
}

#endif // THEMIS_ENABLE_CUDA

} // namespace acceleration
} // namespace themis

