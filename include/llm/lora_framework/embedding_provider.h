/**
 * @file embedding_provider.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <unordered_map>
#include <optional>
#include <mutex>

// Forward declaration for llama.cpp types
struct llama_model;
struct llama_context;

namespace themis {
namespace llm {
namespace lora {

/**
 * @brief Cached embedding entry
 */
struct EmbeddingCache {
    virtual ~EmbeddingCache() = default;
    std::string text;
    std::vector<float> embedding;  // Real embedding from model, not hash-based
    std::chrono::system_clock::time_point cached_at;
    size_t access_count = 0;
    
    // Check if cache entry is expired
    bool isExpired(std::chrono::seconds ttl) const {
        auto now = std::chrono::system_clock::now();
        auto age = std::chrono::duration_cast<std::chrono::seconds>(now - cached_at);
        return age > ttl;
    }
};

/**
 * @brief Statistics for embedding cache
 */
struct EmbeddingCacheStats {
    virtual ~EmbeddingCacheStats() = default;
    size_t total_requests = 0;
    size_t cache_hits = 0;
    size_t cache_misses = 0;
    size_t total_entries = 0;
    size_t memory_bytes = 0;
    
    float hitRate() const {
        if (total_requests == 0) return 0.0f;
        return static_cast<float>(cache_hits) / static_cast<float>(total_requests);
    }
};

/**
 * @brief Provides real embeddings from base model (NOT hash-based)
 * 
 * This is a CRITICAL MISSING FEATURE - training currently uses hash-based
 * embeddings which are meaningless for LoRA optimization.
 * 
 * Features:
 * - Extract real embeddings from base model's embedding layer
 * - Cache embeddings for training datasets
 * - Batch embedding generation for efficiency
 * - Dimension matches model (e.g., 4096 for 13B models)
 * - Serialization for cache persistence
 * 
 * Architecture:
 * Training Data → Get Real Embeddings from Base Model →
 *   → LoRa Training ← Optimize Meaningful Low-Rank Matrices →
 *   → Fine-tuned Model (Real quality improvement)
 */
class EmbeddingProvider {
public:
    /**
     * @brief Configuration for embedding provider
     */
    struct Config {
        size_t max_cache_entries = 10000;     // Maximum cached embeddings
        std::chrono::seconds cache_ttl{3600}; // Cache time-to-live
        bool enable_cache = true;              // Enable caching
        std::string cache_file;                // Optional cache persistence file
        size_t batch_size = 32;                // Batch size for embedding generation
    };
    
    /**
     * @brief Construct embedding provider
     * @param model Base model to extract embeddings from
     * @param context Model context for inference
     * @param config Configuration
     */
    explicit EmbeddingProvider(
        llama_model* model,
        llama_context* context,
        const Config& config
    );
    
    ~EmbeddingProvider();
    
    // Disable copy
    EmbeddingProvider(const EmbeddingProvider&) = delete;
    EmbeddingProvider& operator=(const EmbeddingProvider&) = delete;
    
    /**
     * @brief Get real embedding from base model (NOT hash-based)
     * 
     * Extracts embedding from model's embedding layer.
     * Result dimension matches model (e.g., 4096 for 13B models).
     * 
     * @param text Input text
     * @return Embedding vector (real, not hashed)
     */
    std::vector<float> getEmbedding(const std::string& text);
    
    /**
     * @brief Get embeddings for batch of texts
     * 
     * More efficient than calling getEmbedding() repeatedly.
     * Target: <100ms per 1000 texts
     * 
     * @param texts Vector of input texts
     * @return Vector of embedding vectors
     */
    std::vector<std::vector<float>> getEmbeddings(const std::vector<std::string>& texts);
    
    /**
     * @brief Build embedding cache for training dataset
     * 
     * Pre-computes embeddings for entire training dataset to avoid
     * redundant computation during training epochs.
     * 
     * @param training_texts Texts from training dataset
     * @param cache_out Output cache entries
     * @return true if successful
     */
    bool buildEmbeddingCache(
        const std::vector<std::string>& training_texts,
        std::vector<EmbeddingCache>& cache_out
    );
    
    /**
     * @brief Get embedding dimension
     * 
     * Returns the embedding dimension of the base model.
     * Typically 4096 for 13B models, 5120 for 30B models.
     * 
     * @return Embedding dimension
     */
    size_t getEmbeddingDim() const;
    
    /**
     * @brief Get cache statistics
     * @return Cache statistics
     */
    EmbeddingCacheStats getCacheStats() const;
    
    /**
     * @brief Clear cache
     */
    void clearCache();
    
    /**
     * @brief Save cache to file
     * @param filepath Path to save cache
     * @return true if successful
     */
    bool saveCache(const std::string& filepath);
    
    /**
     * @brief Load cache from file
     * @param filepath Path to load cache from
     * @return true if successful
     */
    bool loadCache(const std::string& filepath);
    
    /**
     * @brief Check if cache is enabled
     * @return true if enabled
     */
    bool isCacheEnabled() const { return config_.enable_cache; }
    
    /**
     * @brief Enable or disable cache
     * @param enable Enable cache
     */
    void enableCache(bool enable) { config_.enable_cache = enable; }
    
private:
    llama_model* model_;
    llama_context* context_;
    Config config_;
    
    // Cache: text -> embedding
    mutable std::mutex cache_mutex_;
    std::unordered_map<std::string, EmbeddingCache> cache_;
    mutable EmbeddingCacheStats cache_stats_;
    
    /**
     * @brief Extract embedding from model for tokenized input
     * @param tokens Token IDs
     * @return Embedding vector
     */
    std::vector<float> extractEmbeddingFromTokens(const std::vector<int>& tokens);
    
    /**
     * @brief Evict old cache entries if needed
     */
    void evictCacheIfNeeded();
    
    /**
     * @brief Get embedding from cache if available
     * @param text Input text
     * @return Optional embedding (nullopt if not cached)
     */
    std::optional<std::vector<float>> getCachedEmbedding(const std::string& text);
    
    /**
     * @brief Add embedding to cache
     * @param text Input text
     * @param embedding Embedding vector
     */
    void addToCache(const std::string& text, const std::vector<float>& embedding);
};

} // namespace lora
} // namespace llm
} // namespace themis

