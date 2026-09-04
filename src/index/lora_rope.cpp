/**
 * @file lora_rope.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "index/lora_rope.h"
#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <numeric>
#include <random>

namespace themis {

// ============================================================================
// Constants
// ============================================================================

// Default standard deviation for random initialization of LoRA matrices
constexpr double LORA_INIT_STD_DEV = 0.01;

// ============================================================================
// LoRARopeAdapter Implementation
// ============================================================================

LoRARopeAdapter LoRARopeAdapter::createRandom(
    const std::string& name,
    const std::string& domain,
    size_t num_rotation_pairs,
    size_t rank,
    float alpha
) {
    LoRARopeAdapter adapter;
    adapter.name = name;
    adapter.domain = domain;
    adapter.rank = rank;
    adapter.alpha = alpha;
    adapter.enabled = true;
    adapter.scaling = 1.0f;
    
    // Initialize random number generator with small random values
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<double> dist(0.0, LORA_INIT_STD_DEV);
    
    // Initialize matrix B: (num_rotation_pairs, rank)
    adapter.matrix_B.resize(num_rotation_pairs);
    for (size_t i = 0; i < num_rotation_pairs; ++i) {
        adapter.matrix_B[i].resize(rank);
        for (size_t j = 0; j < rank; ++j) {
            adapter.matrix_B[i][j] = dist(gen);
        }
    }
    
    // Initialize matrix A: (rank, num_rotation_pairs)
    adapter.matrix_A.resize(rank);
    for (size_t i = 0; i < rank; ++i) {
        adapter.matrix_A[i].resize(num_rotation_pairs);
        for (size_t j = 0; j < num_rotation_pairs; ++j) {
            adapter.matrix_A[i][j] = dist(gen);
        }
    }
    
    // Initialize theta_delta to zeros (optional modifications)
    adapter.theta_delta.resize(num_rotation_pairs, 0.0);
    
    return adapter;
}

LoRARopeAdapter LoRARopeAdapter::createZero(
    const std::string& name,
    const std::string& domain,
    size_t num_rotation_pairs,
    size_t rank,
    float alpha
) {
    LoRARopeAdapter adapter;
    adapter.name = name;
    adapter.domain = domain;
    adapter.rank = rank;
    adapter.alpha = alpha;
    adapter.enabled = true;
    adapter.scaling = 1.0f;
    
    // Initialize matrix B: (num_rotation_pairs, rank) with zeros
    adapter.matrix_B.resize(num_rotation_pairs);
    for (size_t i = 0; i < num_rotation_pairs; ++i) {
        adapter.matrix_B[i].resize(rank, 0.0);
    }
    
    // Initialize matrix A: (rank, num_rotation_pairs) with zeros
    adapter.matrix_A.resize(rank);
    for (size_t i = 0; i < rank; ++i) {
        adapter.matrix_A[i].resize(num_rotation_pairs, 0.0);
    }
    
    // Initialize theta_delta to zeros
    adapter.theta_delta.resize(num_rotation_pairs, 0.0);
    
    return adapter;
}

// ============================================================================
// LoRARopeAdapterRegistry Implementation
// ============================================================================

bool LoRARopeAdapterRegistry::registerAdapter(const LoRARopeAdapter& adapter) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if adapter already exists
    if (adapters_.find(adapter.name) != adapters_.end()) {
        return false;  // Adapter already registered
    }
    
    adapters_[adapter.name] = adapter;
    return true;
}

bool LoRARopeAdapterRegistry::unregisterAdapter(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    return adapters_.erase(name) > 0;
}

std::optional<LoRARopeAdapter> LoRARopeAdapterRegistry::getAdapter(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = adapters_.find(name);
    if (it != adapters_.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool LoRARopeAdapterRegistry::hasAdapter(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return adapters_.find(name) != adapters_.end();
}

std::vector<std::string> LoRARopeAdapterRegistry::listAdapters() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> names;
    names.reserve(adapters_.size());
    
    for (const auto& [name, _] : adapters_) {
        names.push_back(name);
    }
    
    return names;
}

bool LoRARopeAdapterRegistry::setAdapterEnabled(const std::string& name, bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = adapters_.find(name);
    if (it != adapters_.end()) {
        it->second.enabled = enabled;
        return true;
    }
    return false;
}

void LoRARopeAdapterRegistry::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    adapters_.clear();
}

size_t LoRARopeAdapterRegistry::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return adapters_.size();
}

// ============================================================================
// LoRARotaryEmbedding Implementation
// ============================================================================

LoRARotaryEmbedding::LoRARotaryEmbedding(
    const RotationConfig& config,
    std::shared_ptr<LoRARopeAdapterRegistry> adapter_registry
) : RotaryEmbedding(config),
    adapter_registry_(adapter_registry ? adapter_registry : std::make_shared<LoRARopeAdapterRegistry>()) {
}

std::vector<float> LoRARotaryEmbedding::rotateWithAdapter(
    const std::vector<float>& embedding,
    size_t position,
    const std::string& adapter_name
) const {
    // First apply base rotation
    std::vector<float> rotated = rotate(embedding, position);
    
    // Get adapter from registry
    auto adapter_opt = adapter_registry_->getAdapter(adapter_name);
    if (!adapter_opt.has_value()) {
        throw std::runtime_error("Adapter not found: " + adapter_name);
    }
    
    const auto& adapter = adapter_opt.value();
    
    // Check if adapter is enabled
    if (!adapter.enabled) {
        return rotated;  // Return base rotation without LoRA modification
    }
    
    // Validate adapter dimensions
    if (!adapter.isValid(getConfig().num_rotation_pairs)) {
        throw std::runtime_error("Invalid adapter dimensions for: " + adapter_name);
    }
    
    // Extract rotation features from position
    std::vector<double> features = extractRotationFeatures(position);
    
    // Apply LoRA modification: forward pass through A then B
    // Step 1: features @ A^T -> intermediate (size = rank)
    std::vector<double> intermediate(adapter.rank, 0.0);
    for (size_t r = 0; r < adapter.rank; ++r) {
        for (size_t i = 0; i < features.size(); ++i) {
            intermediate[r] += features[i] * adapter.matrix_A[r][i];
        }
    }
    
    // Step 2: intermediate @ B^T -> delta_features (size = num_rotation_pairs)
    std::vector<double> delta_features(getConfig().num_rotation_pairs, 0.0);
    for (size_t i = 0; i < getConfig().num_rotation_pairs; ++i) {
        for (size_t r = 0; r < adapter.rank; ++r) {
            delta_features[i] += intermediate[r] * adapter.matrix_B[i][r];
        }
        
        // Add theta_delta if present
        if (!adapter.theta_delta.empty()) {
            delta_features[i] += adapter.theta_delta[i];
        }
        
        // Scale by alpha and adapter scaling
        delta_features[i] *= adapter.alpha * adapter.scaling;
    }
    
    // Apply the LoRA-modified rotation
    for (size_t pair_idx = 0; pair_idx < getConfig().num_rotation_pairs; ++pair_idx) {
        size_t idx_0 = pair_idx * 2;
        size_t idx_1 = pair_idx * 2 + 1;
        
        if (idx_1 >= rotated.size()) break;
        
        // Compute additional rotation from LoRA delta
        double delta_theta = delta_features[pair_idx] * static_cast<double>(position);
        double cos_delta = std::cos(delta_theta);
        double sin_delta = std::sin(delta_theta);
        
        // Apply additional rotation: rotate by delta_theta
        float x = rotated[idx_0];
        float y = rotated[idx_1];
        rotated[idx_0] = static_cast<float>(x * cos_delta - y * sin_delta);
        rotated[idx_1] = static_cast<float>(x * sin_delta + y * cos_delta);
    }
    
    return rotated;
}

std::vector<std::vector<float>> LoRARotaryEmbedding::rotateBatchWithAdapter(
    const std::vector<std::vector<float>>& embeddings,
    const std::vector<size_t>& positions,
    const std::string& adapter_name
) const {
    if (embeddings.size() != positions.size()) {
        throw std::invalid_argument("Embeddings and positions size mismatch");
    }
    
    std::vector<std::vector<float>> results;
    results.reserve(embeddings.size());
    
    for (size_t i = 0; i < embeddings.size(); ++i) {
        results.push_back(rotateWithAdapter(embeddings[i], positions[i], adapter_name));
    }
    
    return results;
}

bool LoRARotaryEmbedding::registerAdapter(
    const std::string& /*name*/,
    const LoRARopeAdapter& adapter
) {
    // Validate adapter dimensions against current config
    if (!adapter.isValid(getConfig().num_rotation_pairs)) {
        return false;
    }
    
    return adapter_registry_->registerAdapter(adapter);
}

bool LoRARotaryEmbedding::unregisterAdapter(const std::string& name) {
    return adapter_registry_->unregisterAdapter(name);
}

std::vector<std::string> LoRARotaryEmbedding::listAdapters() const {
    return adapter_registry_->listAdapters();
}

bool LoRARotaryEmbedding::hasAdapter(const std::string& adapter_name) const {
    return adapter_registry_->hasAdapter(adapter_name);
}

bool LoRARotaryEmbedding::setAdapterEnabled(const std::string& name, bool enabled) {
    return adapter_registry_->setAdapterEnabled(name, enabled);
}

std::vector<float> LoRARotaryEmbedding::rotateWithAdapterBlend(
    const std::vector<float>& embedding,
    size_t position,
    const std::vector<std::string>& adapter_names,
    const std::vector<float>& weights
) const {
    if (adapter_names.empty()) {
        return rotate(embedding, position);  // No adapters, return base rotation
    }
    
    if (adapter_names.size() != weights.size()) {
        throw std::invalid_argument("Adapter names and weights size mismatch");
    }
    
    // Normalize weights
    float weight_sum = std::accumulate(weights.begin(), weights.end(), 0.0f);
    if (weight_sum <= 0.0f) {
        throw std::invalid_argument("Weight sum must be positive");
    }
    
    std::vector<float> normalized_weights = weights;
    for (auto& w : normalized_weights) {
        w /= weight_sum;
    }
    
    // Get embedding size from first adapter rotation
    // (We compute this once to determine result vector size)
    auto first_rotation = rotateWithAdapter(embedding, position, adapter_names[0]);
    std::vector<float> result(first_rotation.size(), 0.0f);
    
    // Weighted average of all adapter rotations
    // Note: adapter rotations already include base rotation, so we blend them directly
    for (size_t i = 0; i < adapter_names.size(); ++i) {
        if (normalized_weights[i] <= 0.0f) continue;
        
        auto adapter_rotation = (i == 0) ? first_rotation : rotateWithAdapter(embedding, position, adapter_names[i]);
        for (size_t j = 0; j < result.size(); ++j) {
            result[j] += normalized_weights[i] * adapter_rotation[j];
        }
    }
    
    return result;
}

// ============================================================================
// Private Helper Methods
// ============================================================================

std::vector<double> LoRARotaryEmbedding::extractRotationFeatures([[maybe_unused]] size_t position) const {
    // Create a feature vector based on position and base theta values
    // This serves as input to the LoRA matrices
    std::vector<double> features(getConfig().num_rotation_pairs);
    
    for (size_t i = 0; i < getConfig().num_rotation_pairs; ++i) {
        // Use position-scaled theta as features
        // This captures the rotation information that LoRA can modify
        double base_theta = getConfig().theta_cache[i];
        features[i] = std::sin(static_cast<double>(position) * base_theta);
    }
    
    return features;
}

} // namespace themis
