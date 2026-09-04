/**
 * @file product_quantizer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "index/product_quantizer.h"
#include "utils/logger.h"

// FAISS includes (conditional)
#ifdef THEMIS_HAS_FAISS
    #include <faiss/impl/ProductQuantizer.h>
#endif

#include <algorithm>
#include <random>
#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <cstring>

#ifdef THEMIS_HAS_FAISS
#include <faiss/Clustering.h>
#include <faiss/IndexFlat.h>
// FAISS ProductQuantizer support: provides faster K-means training with SIMD optimizations
#endif

namespace themis {

namespace {

bool checkedMultiply(size_t lhs, size_t rhs, size_t& out) {
    if (lhs == 0 || rhs == 0) {
        out = 0;
        return true;
    }

    if (lhs > std::numeric_limits<size_t>::max() / rhs) {
        return false;
    }

    out = lhs * rhs;
    return true;
}

bool isValidSlice(const std::vector<float>& values, size_t offset, size_t length) {
    return offset <= values.size() && length <= values.size() - offset;
}

} // namespace

ProductQuantizer::ProductQuantizer(int dimension, const Config& config)
    : dimension_(dimension), config_(config), trained_(false) {

    if (dimension_ <= 0) {
        throw std::invalid_argument("Dimension must be positive");
    }

    if (config_.num_subquantizers <= 0) {
        throw std::invalid_argument("num_subquantizers must be positive");
    }

    if (config_.num_centroids <= 0 || config_.num_centroids > 256) {
        throw std::invalid_argument("num_centroids must be in range [1, 256]");
    }

    if (config_.max_iterations <= 0) {
        throw std::invalid_argument("max_iterations must be positive");
    }

    if (dimension_ % config_.num_subquantizers != 0) {
        throw std::invalid_argument(
            "Dimension must be divisible by num_subquantizers");
    }
    
    subvector_dim_ = dimension_ / config_.num_subquantizers;
    
#ifdef THEMIS_HAS_FAISS
    use_faiss_ = config_.prefer_faiss;
    THEMIS_INFO("ProductQuantizer: Initialized with {} acceleration (dimension={}, subquantizers={})",
                use_faiss_ ? "FAISS" : "custom", dimension_, config_.num_subquantizers);
#else
    use_faiss_ = false;
    THEMIS_INFO("ProductQuantizer: Initialized with custom implementation (dimension={}, subquantizers={}) - FAISS not available",
                dimension_, config_.num_subquantizers);
#endif
    
    // Pre-allocate codebooks
#ifdef THEMIS_HAS_FAISS
    // Use FAISS ProductQuantizer
    // Parameters: dimension, M (num_subquantizers), nbits (8 for 256 centroids)
    faiss_pq_ = std::make_unique<faiss::ProductQuantizer>(
        static_cast<size_t>(dimension_),
        static_cast<size_t>(config_.num_subquantizers),
        8  // 8 bits = 256 centroids
    );
    
    THEMIS_INFO("ProductQuantizer created with FAISS backend: dim={}, M={}, dsub={}",
                dimension_, config_.num_subquantizers, subvector_dim_);
#else
    // Fallback: Pre-allocate codebooks for custom implementation
    codebooks_.resize(config_.num_subquantizers);
    for (auto& codebook : codebooks_) {
        codebook.resize(config_.num_centroids, std::vector<float>(subvector_dim_));
    }
    
    THEMIS_INFO("ProductQuantizer created with fallback implementation: dim={}, M={}, dsub={}",
                dimension_, config_.num_subquantizers, subvector_dim_);
#endif
}

ProductQuantizer::~ProductQuantizer() = default;

ProductQuantizer::ProductQuantizer(ProductQuantizer&&) noexcept = default;
ProductQuantizer& ProductQuantizer::operator=(ProductQuantizer&&) noexcept = default;

ProductQuantizer::Status ProductQuantizer::train(
    const std::vector<std::vector<float>>& training_vectors) {

    if (training_vectors.empty()) {
        return Status::Error("No training vectors provided");
    }

    const size_t expected_dimension = static_cast<size_t>(dimension_);
    for (const auto& vec : training_vectors) {
        if (vec.size() != expected_dimension) {
            return Status::Error("Training vector dimension mismatch");
        }
    }

    THEMIS_INFO("ProductQuantizer::train - Training with {} vectors, dim={}, M={}",
                training_vectors.size(), dimension_, config_.num_subquantizers);
    
#ifdef THEMIS_HAS_FAISS
    try {
        // Convert training vectors to contiguous array for FAISS
        std::vector<float> training_data;
        size_t expected_values = 0;
        if (!checkedMultiply(training_vectors.size(), expected_dimension, expected_values)) {
            return Status::Error("Training data size overflow");
        }
        training_data.reserve(expected_values);
        
        for (const auto& vec : training_vectors) {
            training_data.insert(training_data.end(), vec.begin(), vec.end());
        }
        
        // Validate data size (development safety check)
        if (training_data.size() != expected_values) {
            return Status::Error("Training data size mismatch during conversion");
        }
        
        // Train FAISS ProductQuantizer
        THEMIS_DEBUG("ProductQuantizer::train - Starting FAISS training");
        faiss_pq_->train(training_vectors.size(), training_data.data());
        
        trained_ = true;
        
        THEMIS_INFO("ProductQuantizer::train (FAISS) - Training complete. Compression: {:.1f}x",
                    getCompressionRatio());
        
        return Status::OK();
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("ProductQuantizer::train (FAISS) - Training failed: {}", e.what());
        return Status::Error(std::string("FAISS training failed: ") + e.what());
    }
#else
    // Fallback: Custom K-means training
    // Train each subquantizer independently
    for (int sq = 0; sq < config_.num_subquantizers; ++sq) {
        const size_t sq_index = static_cast<size_t>(sq);
        if (sq_index >= codebooks_.size()) {
            return Status::Error("Subquantizer index out of range");
        }

        const size_t start_dim = sq_index * static_cast<size_t>(subvector_dim_);
        
        // Extract subvectors for this subquantizer
        std::vector<std::vector<float>> subvector_data;
        subvector_data.reserve(training_vectors.size());
        
        for (const auto& vec : training_vectors) {
            if (!isValidSlice(vec, start_dim, static_cast<size_t>(subvector_dim_))) {
                return Status::Error("Training subvector slice out of range");
            }
            std::vector<float> subvec(
                vec.begin() + start_dim,
                vec.begin() + start_dim + subvector_dim_
            );
            subvector_data.push_back(std::move(subvec));
        }
        
        // Run K-means to find centroids
        codebooks_[sq_index] = runKMeans(subvector_data);
        
        THEMIS_DEBUG("ProductQuantizer::train - Subquantizer {}/{} trained",
                     sq + 1, config_.num_subquantizers);
    }
    
    trained_ = true;
    THEMIS_INFO("ProductQuantizer::train - Training complete (backend: {}). Compression ratio: {:.1f}x",
                getBackend(), getCompressionRatio());
    THEMIS_INFO("ProductQuantizer::train (fallback) - Training complete. Compression: {:.1f}x",
                getCompressionRatio());
    
    return Status::OK();
#endif
}

std::vector<uint8_t> ProductQuantizer::encode(const std::vector<float>& vector) const {
    if (!trained_) {
        THEMIS_WARN("ProductQuantizer::encode - Quantizer not trained");
        return {};
    }
    
    if (vector.size() != static_cast<size_t>(dimension_)) {
        THEMIS_ERROR("ProductQuantizer::encode - Dimension mismatch: {} vs {}",
                     vector.size(), dimension_);
        return {};
    }
    
#ifdef THEMIS_HAS_FAISS
    std::vector<uint8_t> codes(config_.num_subquantizers);
    
    try {
        if (!faiss_pq_ || vector.empty() || codes.empty()) {
            THEMIS_ERROR("ProductQuantizer::encode (FAISS) - Invalid encoder state");
            return {};
        }
        // FAISS compute_codes: (input data, output codes, num_vectors)
        faiss_pq_->compute_codes(vector.data(), codes.data(), 1);
        return codes;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("ProductQuantizer::encode (FAISS) - Encoding failed for vector size {}: {}", 
                     vector.size(), e.what());
        return {};
    }
#else
    // Fallback: Custom encoding
    std::vector<uint8_t> codes;
    codes.reserve(config_.num_subquantizers);
    
    // Encode each subvector independently
    for (int sq = 0; sq < config_.num_subquantizers; ++sq) {
        const size_t sq_index = static_cast<size_t>(sq);
        if (sq_index >= codebooks_.size()) {
            THEMIS_ERROR("ProductQuantizer::encode - Subquantizer index out of range");
            return {};
        }

        const size_t start_dim = sq_index * static_cast<size_t>(subvector_dim_);
        if (!isValidSlice(vector, start_dim, static_cast<size_t>(subvector_dim_))) {
            THEMIS_ERROR("ProductQuantizer::encode - Subvector slice out of range");
            return {};
        }
        
        std::vector<float> subvec(
            vector.begin() + start_dim,
            vector.begin() + start_dim + subvector_dim_
        );
        
        uint8_t code = findNearestCentroid(subvec, codebooks_[sq_index]);
        codes.push_back(code);
    }
    
    return codes;
#endif
}

std::vector<float> ProductQuantizer::decode(const std::vector<uint8_t>& codes) const {
    if (!trained_) {
        THEMIS_WARN("ProductQuantizer::decode - Quantizer not trained");
        return {};
    }
    
    if (codes.size() != static_cast<size_t>(config_.num_subquantizers)) {
        THEMIS_ERROR("ProductQuantizer::decode - Code size mismatch");
        return {};
    }
    
#ifdef THEMIS_HAS_FAISS
    std::vector<float> decoded(dimension_);
    
    try {
        if (!faiss_pq_ || codes.empty() || decoded.empty()) {
            THEMIS_ERROR("ProductQuantizer::decode (FAISS) - Invalid decoder state");
            return {};
        }
        // FAISS decode: (input codes, output data, num_vectors)
        faiss_pq_->decode(codes.data(), decoded.data(), 1);
        return decoded;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("ProductQuantizer::decode (FAISS) - Decoding failed for {} codes: {}", 
                     codes.size(), e.what());
        return {};
    }
#else
    // Fallback: Custom decoding
    std::vector<float> reconstructed;
    reconstructed.reserve(dimension_);
    
    // Concatenate centroid vectors
    for (int sq = 0; sq < config_.num_subquantizers; ++sq) {
        const size_t sq_index = static_cast<size_t>(sq);
        if (sq_index >= codebooks_.size()) {
            THEMIS_ERROR("ProductQuantizer::decode - Subquantizer index out of range");
            return {};
        }

        const uint8_t code = codes[sq_index];
        if (code >= codebooks_[sq_index].size()) {
            THEMIS_ERROR("ProductQuantizer::decode - Code {} out of range for subquantizer {}", static_cast<int>(code), sq);
            return {};
        }

        const auto& centroid = codebooks_[sq_index][code];
        reconstructed.insert(reconstructed.end(), centroid.begin(), centroid.end());
    }
    
    return reconstructed;
#endif
}

float ProductQuantizer::computeAsymmetricDistance(
    const std::vector<float>& query,
    const std::vector<uint8_t>& codes) const {
    
    if (!trained_) {
        THEMIS_WARN("ProductQuantizer::computeAsymmetricDistance - Quantizer not trained");
        return std::numeric_limits<float>::max();
    }
    
    if (query.size() != static_cast<size_t>(dimension_)) {
        THEMIS_ERROR("ProductQuantizer::computeAsymmetricDistance - Query dimension mismatch");
        return std::numeric_limits<float>::max();
    }
    
    if (codes.size() != static_cast<size_t>(config_.num_subquantizers)) {
        THEMIS_ERROR("ProductQuantizer::computeAsymmetricDistance - Code size mismatch");
        return std::numeric_limits<float>::max();
    }
    
#ifdef THEMIS_HAS_FAISS
    // Use FAISS optimized ADC (Asymmetric Distance Computation) for better performance
    try {
        // Compute distance table for query
        size_t distance_table_size = 0;
        if (!checkedMultiply(static_cast<size_t>(config_.num_subquantizers),
                             static_cast<size_t>(config_.num_centroids),
                             distance_table_size) ||
            distance_table_size == 0) {
            THEMIS_ERROR("ProductQuantizer::computeAsymmetricDistance - Distance table size overflow");
            return std::numeric_limits<float>::max();
        }

        for (uint8_t code : codes) {
            if (code >= config_.num_centroids) {
                THEMIS_ERROR("ProductQuantizer::computeAsymmetricDistance - Code {} out of range", static_cast<int>(code));
                return std::numeric_limits<float>::max();
            }
        }

        std::vector<float> dis_table(distance_table_size);
        if (!faiss_pq_ || query.empty()) {
            THEMIS_ERROR("ProductQuantizer::computeAsymmetricDistance - Invalid FAISS state");
            return std::numeric_limits<float>::max();
        }
        faiss_pq_->compute_distance_table(query.data(), dis_table.data());
        
        // Compute distance using precomputed table
        float distance = 0.0f;
        for (int i = 0; i < config_.num_subquantizers; ++i) {
            const size_t table_index =
                static_cast<size_t>(i) * static_cast<size_t>(config_.num_centroids) + codes[static_cast<size_t>(i)];
            if (table_index >= dis_table.size()) {
                THEMIS_ERROR("ProductQuantizer::computeAsymmetricDistance - Distance table index out of range");
                return std::numeric_limits<float>::max();
            }
            distance += dis_table[table_index];
        }
        
        return std::sqrt(distance);
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("ProductQuantizer::computeAsymmetricDistance (FAISS ADC) - Failed: {}", e.what());
        THEMIS_DEBUG("Falling back to decode-based distance computation");
        // Fallthrough to decode method
    }
#endif
    
    // Fallback: decode and compute L2 distance
    // This path is used when: FAISS unavailable, FAISS ADC fails, or explicit fallback mode
    auto decoded = decode(codes);
    if (decoded.empty()) {
        THEMIS_DEBUG("ProductQuantizer::computeAsymmetricDistance - decode returned empty, cannot compute distance");
        return std::numeric_limits<float>::max();
    }
    
    // Compute L2 distance
    float distance = 0.0f;
    for (size_t i = 0; i < query.size(); ++i) {
        float diff = query[i] - decoded[i];
        distance += diff * diff;
    }
    
    return std::sqrt(distance);
}

float ProductQuantizer::getCompressionRatio() const {
    // Original: dimension * sizeof(float)
    // Compressed: num_subquantizers * sizeof([[maybe_unused]] uint8_t)
    if (dimension_ <= 0 || config_.num_subquantizers <= 0) {
        return 0.0f;
    }

    size_t original_size = 0;
    size_t compressed_size = 0;
    if (!checkedMultiply(static_cast<size_t>(dimension_), sizeof(float), original_size) ||
        !checkedMultiply(static_cast<size_t>(config_.num_subquantizers), sizeof(std::vector<uint8_t>::value_type), compressed_size) ||
        compressed_size == 0) {
        return 0.0f;
    }
    return static_cast<float>(original_size) / static_cast<float>(compressed_size);
}

size_t ProductQuantizer::getMemoryUsage() const {
    // Returns theoretical codebook memory usage
    // Note: When using FAISS backend, actual memory may differ due to
    // FAISS internal structures, alignment, and metadata
    // Codebooks: num_subquantizers * num_centroids * subvector_dim * sizeof(float)
    return config_.num_subquantizers * config_.num_centroids * subvector_dim_ * sizeof(float);
}

#ifndef THEMIS_HAS_FAISS
// Fallback implementations used when FAISS is not available

std::vector<std::vector<float>> ProductQuantizer::runKMeans(
    const std::vector<std::vector<float>>& subvector_data) const {
    
    const size_t num_samples = subvector_data.size();
    const int k = config_.num_centroids;

    if (k <= 0 || subvector_dim_ <= 0) {
        return {};
    }

    for (const auto& subvector : subvector_data) {
        if (subvector.size() != static_cast<size_t>(subvector_dim_)) {
            THEMIS_WARN("ProductQuantizer::runKMeans - Subvector dimension mismatch");
            return {};
        }
    }
    
    if (num_samples < static_cast<size_t>(k)) {
        THEMIS_WARN("ProductQuantizer::runKMeans - Not enough samples ({}) for {} centroids",
                    num_samples, k);
        // Return existing samples as centroids
        return subvector_data;
    }

#ifdef THEMIS_HAS_FAISS
    // Use FAISS K-means when available and preferred
    if (use_faiss_) {
        try {
            THEMIS_DEBUG("ProductQuantizer::runKMeans - Using FAISS K-means (faster)");
            
            // Convert data to FAISS format (flat array)
            std::vector<float> flat_data;
            flat_data.reserve(num_samples * subvector_dim_);
            for (const auto& vec : subvector_data) {
                flat_data.insert(flat_data.end(), vec.begin(), vec.end());
            }
            
            // Create FAISS clustering object
            faiss::Clustering clustering(subvector_dim_, k);
            clustering.niter = config_.max_iterations;
            clustering.verbose = false;
            
            // Create index for clustering
            faiss::IndexFlatL2 index(subvector_dim_);
            
            // Run FAISS K-means
            clustering.train(num_samples, flat_data.data(), index);
            
            // Extract centroids from FAISS result
            std::vector<std::vector<float>> centroids;
            centroids.reserve(k);
            const float* centroid_data = clustering.centroids.data();
            if (centroid_data == nullptr) {
                THEMIS_WARN("ProductQuantizer::runKMeans - FAISS returned null centroid buffer");
                return {};
            }
            
            for (int i = 0; i < k; ++i) {
                std::vector<float> centroid(subvector_dim_);
                for (int d = 0; d < subvector_dim_; ++d) {
                    const size_t centroid_index =
                        static_cast<size_t>(i) * static_cast<size_t>(subvector_dim_) + static_cast<size_t>(d);
                    centroid[d] = centroid_data[centroid_index];
                }
                centroids.push_back(std::move(centroid));
            }
            
            THEMIS_DEBUG("ProductQuantizer::runKMeans - FAISS K-means completed successfully");
            return centroids;
            
        } catch (const std::exception& e) {
            THEMIS_WARN("ProductQuantizer::runKMeans - FAISS K-means failed: {}, falling back to custom", e.what());
            // Fall through to custom implementation
        }
    }
#endif
    
    // Custom K-means implementation (fallback)
    THEMIS_DEBUG("ProductQuantizer::runKMeans - Using custom K-means implementation");
    
    // Initialize centroids randomly (k-means++)
    std::vector<std::vector<float>> centroids;
    centroids.reserve(k);
    
    std::random_device rd = {};
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint64_t> dis(0, num_samples - 1);
    
    // Pick first centroid randomly
    centroids.push_back(subvector_data[dis(gen)]);
    
    // Pick remaining centroids using k-means++ initialization
    for (int i = 1; i < k; ++i) {
        std::vector<float> distances(num_samples);
        
        // Compute distance to nearest centroid for each sample
        for (size_t j = 0; j < num_samples; ++j) {
            float min_dist = std::numeric_limits<float>::max();
            for (const auto& centroid : centroids) {
                float dist = l2Distance(subvector_data[j], centroid);
                min_dist = std::min(min_dist, dist);
            }
            distances[j] = min_dist * min_dist;  // D^2 weighting
        }
        
        // Sample next centroid with probability proportional to D^2
        std::discrete_distribution<uint64_t> weighted_dis(distances.begin(), distances.end());
        const size_t selected_index = static_cast<size_t>(weighted_dis(gen));
        if (selected_index >= subvector_data.size()) {
            THEMIS_WARN("ProductQuantizer::runKMeans - Weighted centroid index out of range");
            return {};
        }
        centroids.push_back(subvector_data[selected_index]);
    }
    
    // Run k-means iterations
    std::vector<int> assignments(num_samples);
    
    for (int iter = 0; iter < config_.max_iterations; ++iter) {
        // Assignment step
        for (size_t i = 0; i < num_samples; ++i) {
            float min_dist = std::numeric_limits<float>::max();
            int best_cluster = 0;
            
            for (int j = 0; j < k; ++j) {
                float dist = l2Distance(subvector_data[i], centroids[j]);
                if (dist < min_dist) {
                    min_dist = dist;
                    best_cluster = j;
                }
            }
            
            assignments[i] = best_cluster;
        }
        
        // Update step
        std::vector<std::vector<float>> new_centroids(k, std::vector<float>(subvector_dim_, 0.0f));
        std::vector<int> counts(k, 0);
        
        for (size_t i = 0; i < num_samples; ++i) {
            const int cluster = assignments[i];
            if (cluster < 0 || cluster >= k) {
                THEMIS_WARN("ProductQuantizer::runKMeans - Cluster assignment out of range");
                return {};
            }
            counts[cluster]++;
            
            for (int d = 0; d < subvector_dim_; ++d) {
                new_centroids[cluster][d] += subvector_data[i][d];
            }
        }
        
        // Compute mean
        float max_change = 0.0f;
        for (int j = 0; j < k; ++j) {
            if (counts[j] > 0) {
                for (int d = 0; d < subvector_dim_; ++d) {
                    new_centroids[j][d] /= counts[j];
                }
                
                // Check convergence
                float change = l2Distance(centroids[j], new_centroids[j]);
                max_change = std::max(max_change, change);
            } else {
                // Handle empty cluster: reinitialize
                const size_t reset_index = dis(gen);
                if (reset_index >= subvector_data.size()) {
                    THEMIS_WARN("ProductQuantizer::runKMeans - Reinitialization index out of range");
                    return {};
                }
                new_centroids[j] = subvector_data[reset_index];
            }
        }
        
        centroids = std::move(new_centroids);
        
        // Check convergence
        if (max_change < config_.convergence_threshold) {
            THEMIS_DEBUG("ProductQuantizer::runKMeans - Converged after {} iterations", iter + 1);
            break;
        }
    }
    
    return centroids;
}

uint8_t ProductQuantizer::findNearestCentroid(
    const std::vector<float>& subvector,
    const std::vector<std::vector<float>>& centroids) const {
    
    float min_dist = std::numeric_limits<float>::max();
    uint8_t best_idx = 0;
    
    for (size_t i = 0; i < centroids.size(); ++i) {
        float dist = l2Distance(subvector, centroids[i]);
        if (dist < min_dist) {
            min_dist = dist;
            best_idx = static_cast<uint8_t>(i);
        }
    }
    
    return best_idx;
}

float ProductQuantizer::l2Distance(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.size() != b.size()) {
        THEMIS_DEBUG("ProductQuantizer::l2Distance - vector size mismatch ({} != {})", a.size(), b.size());
        return std::numeric_limits<float>::max();
    }
    
    float sum = 0.0f;
    for (size_t i = 0; i < a.size(); ++i) {
        float diff = a[i] - b[i];
        sum += diff * diff;
    }
    
    return std::sqrt(sum);
}

#endif // !THEMIS_HAS_FAISS

const char* ProductQuantizer::getBackend() const {
    // Reports which backend is actually being used for training
#ifdef THEMIS_HAS_FAISS
    return use_faiss_ ? "faiss" : "custom";
#else
    return "custom";
#endif
}

} // namespace themis
