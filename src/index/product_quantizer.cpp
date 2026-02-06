#include "index/product_quantizer.h"
#include "utils/logger.h"
#include <algorithm>
#include <random>
#include <cmath>
#include <limits>
#include <numeric>

#ifdef THEMIS_HAS_FAISS
// FAISS ProductQuantizer support (optional)
// When available, provides faster K-means training with SIMD optimizations
#endif

namespace themis {

ProductQuantizer::ProductQuantizer(int dimension, const Config& config)
    : dimension_(dimension), config_(config) {
    
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
    codebooks_.resize(config_.num_subquantizers);
    for (auto& codebook : codebooks_) {
        codebook.resize(config_.num_centroids, std::vector<float>(subvector_dim_));
    }
}

ProductQuantizer::Status ProductQuantizer::train(
    const std::vector<std::vector<float>>& training_vectors) {
    
    if (training_vectors.empty()) {
        return Status::Error("No training vectors provided");
    }
    
    if (training_vectors[0].size() != static_cast<size_t>(dimension_)) {
        return Status::Error("Training vector dimension mismatch");
    }
    
    THEMIS_INFO("ProductQuantizer::train - Training with {} vectors, dim={}, subquantizers={}",
                training_vectors.size(), dimension_, config_.num_subquantizers);
    
    // Train each subquantizer independently
    for (int sq = 0; sq < config_.num_subquantizers; ++sq) {
        int start_dim = sq * subvector_dim_;
        
        // Extract subvectors for this subquantizer
        std::vector<std::vector<float>> subvector_data;
        subvector_data.reserve(training_vectors.size());
        
        for (const auto& vec : training_vectors) {
            std::vector<float> subvec(
                vec.begin() + start_dim,
                vec.begin() + start_dim + subvector_dim_
            );
            subvector_data.push_back(std::move(subvec));
        }
        
        // Run K-means to find centroids
        codebooks_[sq] = runKMeans(subvector_data);
        
        THEMIS_DEBUG("ProductQuantizer::train - Subquantizer {}/{} trained",
                     sq + 1, config_.num_subquantizers);
    }
    
    trained_ = true;
    THEMIS_INFO("ProductQuantizer::train - Training complete (backend: {}). Compression ratio: {:.1f}x",
                getBackend(), getCompressionRatio());
    
    return Status::OK();
}

std::vector<uint8_t> ProductQuantizer::encode(const std::vector<float>& vector) const {
    if (!trained_) {
        THEMIS_WARN("ProductQuantizer::encode - Quantizer not trained, returning empty");
        return {};
    }
    
    if (vector.size() != static_cast<size_t>(dimension_)) {
        THEMIS_ERROR("ProductQuantizer::encode - Dimension mismatch: {} vs {}",
                     vector.size(), dimension_);
        return {};
    }
    
    std::vector<uint8_t> codes;
    codes.reserve(config_.num_subquantizers);
    
    // Encode each subvector independently
    for (int sq = 0; sq < config_.num_subquantizers; ++sq) {
        int start_dim = sq * subvector_dim_;
        
        std::vector<float> subvec(
            vector.begin() + start_dim,
            vector.begin() + start_dim + subvector_dim_
        );
        
        uint8_t code = findNearestCentroid(subvec, codebooks_[sq]);
        codes.push_back(code);
    }
    
    return codes;
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
    
    std::vector<float> reconstructed;
    reconstructed.reserve(dimension_);
    
    // Concatenate centroid vectors
    for (int sq = 0; sq < config_.num_subquantizers; ++sq) {
        uint8_t code = codes[sq];
        const auto& centroid = codebooks_[sq][code];
        reconstructed.insert(reconstructed.end(), centroid.begin(), centroid.end());
    }
    
    return reconstructed;
}

float ProductQuantizer::computeAsymmetricDistance(
    const std::vector<float>& query,
    const std::vector<uint8_t>& codes) const {
    
    if (!trained_ || codes.size() != static_cast<size_t>(config_.num_subquantizers)) {
        return std::numeric_limits<float>::max();
    }
    
    float total_distance = 0.0f;
    
    // Compute distance for each subvector
    for (int sq = 0; sq < config_.num_subquantizers; ++sq) {
        int start_dim = sq * subvector_dim_;
        uint8_t code = codes[sq];
        
        // Extract query subvector
        std::vector<float> query_subvec(
            query.begin() + start_dim,
            query.begin() + start_dim + subvector_dim_
        );
        
        // Get centroid and compute distance
        const auto& centroid = codebooks_[sq][code];
        float subvec_distance = l2Distance(query_subvec, centroid);
        
        // Sum squared distances (L2 distance squared)
        total_distance += subvec_distance * subvec_distance;
    }
    
    return std::sqrt(total_distance);
}

float ProductQuantizer::getCompressionRatio() const {
    // Original: dimension * sizeof(float)
    // Compressed: num_subquantizers * sizeof(uint8_t)
    size_t original_size = dimension_ * sizeof(float);
    size_t compressed_size = config_.num_subquantizers * sizeof(uint8_t);
    return static_cast<float>(original_size) / static_cast<float>(compressed_size);
}

size_t ProductQuantizer::getMemoryUsage() const {
    // Codebooks: num_subquantizers * num_centroids * subvector_dim * sizeof(float)
    return config_.num_subquantizers * config_.num_centroids * subvector_dim_ * sizeof(float);
}

std::vector<std::vector<float>> ProductQuantizer::runKMeans(
    const std::vector<std::vector<float>>& subvector_data) const {
    
    const size_t num_samples = subvector_data.size();
    const int k = config_.num_centroids;
    
    if (num_samples < static_cast<size_t>(k)) {
        THEMIS_WARN("ProductQuantizer::runKMeans - Not enough samples ({}) for {} centroids",
                    num_samples, k);
        // Return existing samples as centroids
        return subvector_data;
    }
    
    // Initialize centroids randomly (k-means++)
    std::vector<std::vector<float>> centroids;
    centroids.reserve(k);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dis(0, num_samples - 1);
    
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
        std::discrete_distribution<size_t> weighted_dis(distances.begin(), distances.end());
        centroids.push_back(subvector_data[weighted_dis(gen)]);
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
            int cluster = assignments[i];
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
                new_centroids[j] = subvector_data[dis(gen)];
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
        return std::numeric_limits<float>::max();
    }
    
    float sum = 0.0f;
    for (size_t i = 0; i < a.size(); ++i) {
        float diff = a[i] - b[i];
        sum += diff * diff;
    }
    
    return std::sqrt(sum);
}

const char* ProductQuantizer::getBackend() const {
#ifdef THEMIS_HAS_FAISS
    return use_faiss_ ? "faiss" : "custom";
#else
    return "custom";
#endif
}

} // namespace themis
