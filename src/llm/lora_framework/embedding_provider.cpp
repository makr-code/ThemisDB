/**
 * @file embedding_provider.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/embedding_provider.h"
#include "llm/lora_framework/llama_tokenizer.h"
#include <llama.h>
#include <spdlog/spdlog.h>
#include <fstream>
#include <algorithm>
#include <cstring>

namespace themis {
namespace llm {
namespace lora {

EmbeddingProvider::EmbeddingProvider(
    llama_model* model,
    llama_context* context,
    const Config& config
) : model_(model), context_(context), config_(config) {
    if (!model_) {
        throw std::invalid_argument("EmbeddingProvider requires valid model");
    }
    if (!context_) {
        throw std::invalid_argument("EmbeddingProvider requires valid context");
    }
    
    spdlog::info("EmbeddingProvider initialized:");
    spdlog::info("  Embedding dim: {}", getEmbeddingDim());
    spdlog::info("  Cache enabled: {}", config_.enable_cache);
    spdlog::info("  Max cache entries: {}", config_.max_cache_entries);
    spdlog::info("  Cache TTL: {} seconds", config_.cache_ttl.count());
    
    // Load cache from file if specified
    if (!config_.cache_file.empty()) {
        if (loadCache(config_.cache_file)) {
            spdlog::info("  Loaded cache from: {}", config_.cache_file);
        }
    }
}

EmbeddingProvider::~EmbeddingProvider() {
    // Save cache to file if specified
    if (!config_.cache_file.empty() && config_.enable_cache) {
        if (saveCache(config_.cache_file)) {
            spdlog::info("Saved embedding cache to: {}", config_.cache_file);
        }
    }
}

std::vector<float> EmbeddingProvider::getEmbedding(const std::string& text) {
    // Check cache first
    if (config_.enable_cache) {
        auto cached = getCachedEmbedding(text);
        if (cached.has_value()) {
            return cached.value();
        }
    }
    
    // Tokenize text
    // NOTE: We need a tokenizer instance. For now, we'll use llama.cpp's tokenization directly.
    const llama_vocab* vocab = llama_model_get_vocab(model_);
    if (!vocab) {
        spdlog::error("llama_model_get_vocab returned null while generating embedding");
        return std::vector<float>();
    }
    
    std::vector<llama_token> tokens_buffer(text.size() + 16);
    if (tokens_buffer.size() > static_cast<size_t>(std::numeric_limits<int32_t>::max())) {
        spdlog::error("Token buffer too large for llama_tokenize");
        return std::vector<float>();
    }

    const int32_t token_capacity = static_cast<int32_t>(tokens_buffer.size());
    int32_t n_tokens = llama_tokenize(
        vocab,
        text.c_str(),
        static_cast<int32_t>(text.length()),
        tokens_buffer.data(),
        token_capacity,
        true,   // add_bos
        false   // special
    );
    
    if (n_tokens < 0) {
        tokens_buffer.resize(-n_tokens);
        if (tokens_buffer.size() > static_cast<size_t>(std::numeric_limits<int32_t>::max())) {
            spdlog::error("Retried token buffer too large for llama_tokenize");
            return std::vector<float>();
        }
        const int32_t retry_capacity = static_cast<int32_t>(tokens_buffer.size());
        n_tokens = llama_tokenize(
            vocab,
            text.c_str(),
            static_cast<int32_t>(text.length()),
            tokens_buffer.data(),
            retry_capacity,
            true,
            false
        );
        
        if (n_tokens < 0) {
            spdlog::error("Failed to tokenize text for embedding");
            return std::vector<float>();
        }
    }
    
    tokens_buffer.resize(n_tokens);
    
    // Convert to int vector
    std::vector<int> tokens(tokens_buffer.begin(), tokens_buffer.end());
    
    // Extract embedding
    auto embedding = extractEmbeddingFromTokens(tokens);
    
    // Cache if enabled
    if (config_.enable_cache && !embedding.empty()) {
        addToCache(text, embedding);
    }
    
    return embedding;
}

std::vector<std::vector<float>> EmbeddingProvider::getEmbeddings(
    const std::vector<std::string>& texts
) {
    std::vector<std::vector<float>> embeddings;
    embeddings.reserve(texts.size());
    
    // Process in batches for efficiency
    for (size_t i = 0; i < texts.size(); i += config_.batch_size) {
        size_t batch_end = std::min(i + config_.batch_size, texts.size());
        
        for (size_t j = i; j < batch_end; ++j) {
            embeddings.push_back(getEmbedding(texts[j]));
        }
    }
    
    return embeddings;
}

bool EmbeddingProvider::buildEmbeddingCache(
    const std::vector<std::string>& training_texts,
    std::vector<EmbeddingCache>& cache_out
) {
    spdlog::info("Building embedding cache for {} texts", training_texts.size());
    
    cache_out.clear();
    cache_out.reserve(training_texts.size());
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Generate embeddings in batches
    auto embeddings = getEmbeddings(training_texts);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Build cache entries
    for (size_t i = 0; i < training_texts.size(); ++i) {
        if (i < embeddings.size() && !embeddings[i].empty()) {
            EmbeddingCache entry;
            entry.text = training_texts[i];
            entry.embedding = embeddings[i];
            entry.cached_at = std::chrono::system_clock::now();
            cache_out.push_back(entry);
        }
    }
    
    spdlog::info("✓ Built embedding cache: {} entries in {} ms", 
                 cache_out.size(), duration.count());
    spdlog::info("  Average: {:.2f} ms per embedding", 
                 static_cast<float>(duration.count()) / training_texts.size());
    
    // Target: <100ms per 1000 texts = 0.1ms per text
    float ms_per_text = static_cast<float>(duration.count()) / training_texts.size();
    if (ms_per_text > 0.1f) {
        spdlog::warn("Embedding generation slower than target (0.1ms per text)");
    }
    
    return !cache_out.empty();
}

size_t EmbeddingProvider::getEmbeddingDim() const {
    if (!model_) {
      return 0;
    }
    
    // Get embedding dimension from model
    // For llama models, this is typically:
    // - 4096 for 7B/13B models
    // - 5120 for 30B models
    // - 8192 for 65B models
    
    // NOTE: llama.cpp API for getting embedding dimension
    // This may need to be adjusted based on llama.cpp version
    int32_t n_embd = llama_n_embd(model_);
    
    return static_cast<size_t>(n_embd);
}

EmbeddingCacheStats EmbeddingProvider::getCacheStats() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // Update memory usage
    cache_stats_.total_entries = cache_.size();
    cache_stats_.memory_bytes = 0;
    
    for (const auto& [text, entry] : cache_) {
        cache_stats_.memory_bytes += sizeof(EmbeddingCache);
        cache_stats_.memory_bytes += text.size();
        cache_stats_.memory_bytes += entry.embedding.size() * sizeof(float);
    }
    
    return cache_stats_;
}

void EmbeddingProvider::clearCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    spdlog::info("Clearing embedding cache ({} entries)", cache_.size());
    cache_.clear();
    cache_stats_.total_entries = 0;
    cache_stats_.memory_bytes = 0;
}

bool EmbeddingProvider::saveCache(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    try {
        std::ofstream file(filepath, std::ios::binary);
        if (!file) {
            spdlog::error("Failed to open cache file for writing: {}", filepath);
            return false;
        }
        
        // Write header
        uint32_t version = 1;
        uint32_t num_entries = static_cast<uint32_t>(cache_.size());
        uint32_t embedding_dim = static_cast<uint32_t>(getEmbeddingDim());
        
        file.write(reinterpret_cast<const char*>(&version), sizeof(version));
        file.write(reinterpret_cast<const char*>(&num_entries), sizeof(num_entries));
        file.write(reinterpret_cast<const char*>(&embedding_dim), sizeof(embedding_dim));
        
        // Write entries
        for (const auto& [text, entry] : cache_) {
            // Write text length and text
            uint32_t text_len = static_cast<uint32_t>(text.size());
            file.write(reinterpret_cast<const char*>(&text_len), sizeof(text_len));
            file.write(text.c_str(), text_len);
            
            // Write embedding
            uint32_t emb_size = static_cast<uint32_t>(entry.embedding.size());
            file.write(reinterpret_cast<const char*>(&emb_size), sizeof(emb_size));
            file.write(reinterpret_cast<const char*>(entry.embedding.data()), 
                      emb_size * sizeof(float));
            
            // Write metadata
            auto timestamp = entry.cached_at.time_since_epoch().count();
            file.write(reinterpret_cast<const char*>(&timestamp), sizeof(timestamp));
            file.write(reinterpret_cast<const char*>(&entry.access_count), sizeof(entry.access_count));
        }
        
        spdlog::info("Saved {} cache entries to {}", cache_.size(), filepath);
        return true;
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to save cache: {}", e.what());
        return false;
    }
}

bool EmbeddingProvider::loadCache(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    try {
        std::ifstream file(filepath, std::ios::binary);
        if (!file) {
            spdlog::debug("Cache file not found: {}", filepath);
            return false;
        }
        
        // Read header
        uint32_t version, num_entries, embedding_dim;
        file.read(reinterpret_cast<char*>(&version), sizeof(version));
        file.read(reinterpret_cast<char*>(&num_entries), sizeof(num_entries));
        file.read(reinterpret_cast<char*>(&embedding_dim), sizeof(embedding_dim));
        
        if (version != 1) {
            spdlog::error("Unsupported cache file version: {}", version);
            return false;
        }
        
        if (embedding_dim != getEmbeddingDim()) {
            spdlog::error("Cache embedding dim ({}) doesn't match model ({})", 
                         embedding_dim, getEmbeddingDim());
            return false;
        }
        
        // Read entries
        cache_.clear();
        for (uint32_t i = 0; i < num_entries; ++i) {
            // Read text
            uint32_t text_len = {};
            file.read(reinterpret_cast<char*>(&text_len), sizeof(text_len));
            
            std::string text(text_len, '\0');
            file.read(&text[0], text_len);
            
            // Read embedding
            uint32_t emb_size = {};
            file.read(reinterpret_cast<char*>(&emb_size), sizeof(emb_size));
            
            std::vector<float> embedding(emb_size);
            file.read(reinterpret_cast<char*>(embedding.data()), emb_size * sizeof(float));
            
            // Read metadata
            int64_t timestamp;
            size_t access_count = {};
            file.read(reinterpret_cast<char*>(&timestamp), sizeof(timestamp));
            file.read(reinterpret_cast<char*>(&access_count), sizeof(access_count));
            
            // Create cache entry
            EmbeddingCache entry;
            entry.text = text;
            entry.embedding = embedding;
            entry.cached_at = std::chrono::system_clock::time_point(
                std::chrono::system_clock::duration(timestamp));
            entry.access_count = access_count;
            
            cache_[text] = entry;
        }
        
        spdlog::info("Loaded {} cache entries from {}", cache_.size(), filepath);
        return true;
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to load cache: {}", e.what());
        return false;
    }
}

std::vector<float> EmbeddingProvider::extractEmbeddingFromTokens(
    const std::vector<int>& tokens
) {
    if (tokens.empty()) {
        return std::vector<float>();
    }
    
    // Convert to llama_token
    std::vector<llama_token> llama_tokens(tokens.begin(), tokens.end());

    if (llama_tokens.size() > static_cast<size_t>(std::numeric_limits<int32_t>::max())) {
        spdlog::error("Token sequence too large for llama_batch_init");
        return std::vector<float>();
    }

    const int32_t llama_token_count = static_cast<int32_t>(llama_tokens.size());

    // Create batch for inference
    llama_batch batch = llama_batch_init(llama_token_count, 0, 1);

    // Fill batch
    for (int32_t i = 0; i < llama_token_count; ++i) {
        batch.token[i] = llama_tokens[static_cast<size_t>(i)];
        batch.pos[i] = static_cast<llama_pos>(i);
        batch.n_seq_id[i] = 1;
        batch.seq_id[i][0] = 0;
        batch.logits[i] = (i == llama_token_count - 1) ? 1 : 0;  // Only last token needs logits
    }
    batch.n_tokens = llama_token_count;
    
    // Decode to get embeddings
    int result = llama_decode(context_, batch);
    
    if (result != 0) {
        spdlog::error("Failed to decode tokens for embedding (error: {})", result);
        llama_batch_free(batch);
        return std::vector<float>();
    }
    
    // Get embeddings from last token
    // For embedding extraction, we typically use the last token's hidden state
    float* embeddings_raw = llama_get_embeddings(context_);
    
    if (!embeddings_raw) {
        spdlog::error("Failed to get embeddings from context");
        llama_batch_free(batch);
        return std::vector<float>();
    }
    
    // Copy embeddings
    size_t emb_dim = getEmbeddingDim();
    std::vector<float> embeddings(embeddings_raw, embeddings_raw + emb_dim);
    
    llama_batch_free(batch);
    
    return embeddings;
}

void EmbeddingProvider::evictCacheIfNeeded() {
    if (cache_.size() <= config_.max_cache_entries) {
        return;
    }
    
    // Find least recently used entries
    std::vector<std::pair<std::string, std::chrono::system_clock::time_point>> entries;
    for (const auto& [text, entry] : cache_) {
        entries.push_back({text, entry.cached_at});
    }
    
    // Sort by age (oldest first)
    std::sort(entries.begin(), entries.end(),
        [](const auto& a, const auto& b) {
            return a.second < b.second;
        });
    
    // Remove oldest 20%
    size_t to_remove = config_.max_cache_entries / 5;
    for (size_t i = 0; i < to_remove && i < entries.size(); ++i) {
        cache_.erase(entries[i].first);
    }
    
    spdlog::debug("Evicted {} old cache entries", to_remove);
}

std::optional<std::vector<float>> EmbeddingProvider::getCachedEmbedding(
    const std::string& text
) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    cache_stats_.total_requests++;
    
    auto it = cache_.find(text);
    if (it == cache_.end()) {
        cache_stats_.cache_misses++;
        return std::nullopt;
    }
    
    // Check if expired
    if (it->second.isExpired(config_.cache_ttl)) {
        cache_.erase(it);
        cache_stats_.cache_misses++;
        return std::nullopt;
    }
    
    // Update access stats
    it->second.access_count++;
    cache_stats_.cache_hits++;
    
    return it->second.embedding;
}

void EmbeddingProvider::addToCache(
    const std::string& text,
    const std::vector<float>& embedding
) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    evictCacheIfNeeded();
    
    EmbeddingCache entry;
    entry.text = text;
    entry.embedding = embedding;
    entry.cached_at = std::chrono::system_clock::now();
    entry.access_count = 0;
    
    cache_[text] = entry;
}

} // namespace lora
} // namespace llm
} // namespace themis
