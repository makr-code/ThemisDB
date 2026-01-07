# llama.cpp Feature Implementation Guide für ThemisDB

**Datum:** 5. Januar 2026  
**Status:** Implementation Guide  
**Zielgruppe:** ThemisDB Core Developers

---

## 🎯 Übersicht

Dieser Guide zeigt **konkrete Implementierungsschritte** für die identifizierten llama.cpp Features, die ThemisDB noch nicht nutzt.

Siehe auch: [LLAMA_CPP_API_FEATURE_RESEARCH.md](./LLAMA_CPP_API_FEATURE_RESEARCH.md) für die vollständige Feature-Analyse.

---

## 📌 Quick Start: Flash Attention (Sofort implementierbar)

### Was wird aktiviert?
- Flash Attention 2 für schnellere Attention-Berechnung
- Automatische GPU-Optimierung
- Keine Code-Änderungen außer Config

### Implementierung

**Schritt 1:** Config erweitern (`include/llm/llama_wrapper.h`)

```cpp
struct Config {
    // ... existing fields ...
    
    // NEW: Flash Attention Support
    bool use_flash_attn = true;      // Flash Attention aktivieren
    bool use_flash_attn_f16 = true;  // FP16 für Flash Attention
};
```

**Schritt 2:** Model Params setzen (`src/llm/llama_wrapper.cpp`)

```cpp
bool LlamaWrapper::loadModel(
    const std::string& model_path,
    const json& config
) {
    // ... existing code ...
    
    llama_model_params model_params = llama_model_default_params();
    model_params.n_gpu_layers = config_.n_gpu_layers;
    model_params.use_mmap = config_.use_mmap;
    
    // NEW: Flash Attention aktivieren
    #ifdef LLAMA_FLASH_ATTN
    model_params.flash_attn = config_.use_flash_attn;
    #endif
    
    // Model laden
    llama_model* model = llama_load_model_from_file(
        model_path.c_str(),
        model_params
    );
    
    // ...
}
```

**Schritt 3:** Config-File updaten (`config/llm_config.example.yaml`)

```yaml
llm:
  llama_wrapper:
    # Flash Attention (llama.cpp b2000+)
    use_flash_attn: true
    use_flash_attn_f16: true  # Für CUDA
    
    # Existing config
    n_gpu_layers: 32
    n_ctx: 4096
```

**Testing:**

```bash
# Build mit Flash Attention
cmake -B build -S . -DTHEMIS_ENABLE_LLM=ON

# Test
./build/test_embedded_llm --gtest_filter="*FlashAttention*"
```

**Erwarteter Speedup:** 15-25% schnellere Inferenz, 30% weniger Memory

---

## 🔥 Feature 1: KV-Cache Reuse (Prefix Caching)

### Problem
Gleiche System-Prompts werden jedes Mal neu berechnet:
```cpp
// Jede Inferenz berechnet "You are a helpful assistant..." neu
InferenceRequest req1;
req1.system_prompt = "You are a helpful assistant...";  // 50 tokens → 200ms
req1.user_prompt = "What is 2+2?";                      // 10 tokens → 40ms

InferenceRequest req2;
req2.system_prompt = "You are a helpful assistant...";  // 50 tokens → 200ms wieder!
req2.user_prompt = "What is 3+3?";                      // 10 tokens → 40ms
```

### Lösung: KV-Cache wiederverwenden

**Schritt 1:** Neue Klasse erstellen (`include/llm/kv_cache_manager.h`)

```cpp
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <llama.h>

namespace themis {
namespace llm {

/**
 * @brief KV-Cache Manager für Prefix-Caching
 * 
 * Speichert KV-Cache-States für häufig verwendete Prompts.
 * Reduziert First-Token-Latency um 10-20x bei wiederkehrenden Prefixen.
 */
class KVCacheManager {
public:
    struct Config {
        size_t max_cache_entries = 100;    // Max. gecachte Prefixe
        size_t max_cache_size_mb = 1024;   // Max. Cache-Größe (1 GB)
        bool enable_lru_eviction = true;   // LRU-basierte Eviction
    };
    
    explicit KVCacheManager(const Config& config);
    ~KVCacheManager();
    
    /**
     * @brief Speichert KV-Cache für ein Prompt-Prefix
     * 
     * @param ctx llama_context mit evaluiertem Prefix
     * @param prefix_hash Hash des Prefix-Texts
     * @param n_past Anzahl Tokens im Cache
     * @return true bei Erfolg
     */
    bool saveCache(
        llama_context* ctx,
        const std::string& prefix_hash,
        int n_past
    );
    
    /**
     * @brief Lädt KV-Cache für ein Prompt-Prefix
     * 
     * @param ctx llama_context zum Wiederherstellen
     * @param prefix_hash Hash des Prefix-Texts
     * @param out_n_past Output: Anzahl Tokens im Cache
     * @return true bei Cache-Hit
     */
    bool loadCache(
        llama_context* ctx,
        const std::string& prefix_hash,
        int& out_n_past
    );
    
    /**
     * @brief Berechnet Hash für Text
     */
    std::string computeHash(const std::string& text) const;
    
    /**
     * @brief Cache-Statistiken
     */
    struct Stats {
        size_t cache_hits = 0;
        size_t cache_misses = 0;
        size_t total_entries = 0;
        size_t total_size_bytes = 0;
        double hit_rate() const {
            size_t total = cache_hits + cache_misses;
            return total > 0 ? (double)cache_hits / total : 0.0;
        }
    };
    
    Stats getStats() const;
    void clearCache();
    
private:
    struct CacheEntry {
        std::vector<uint8_t> state_data;
        int n_past;
        size_t size_bytes;
        int64_t last_access_time_ms;
    };
    
    Config config_;
    std::unordered_map<std::string, CacheEntry> cache_;
    mutable std::mutex mutex_;
    Stats stats_;
    
    void evictOldestIfNeeded();
    size_t getCurrentCacheSizeBytes() const;
};

} // namespace llm
} // namespace themis
```

**Schritt 2:** Implementation (`src/llm/kv_cache_manager.cpp`)

```cpp
#include "llm/kv_cache_manager.h"
#include <spdlog/spdlog.h>
#include <openssl/sha.h>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace themis {
namespace llm {

KVCacheManager::KVCacheManager(const Config& config)
    : config_(config) {
    spdlog::info("KVCacheManager initialized:");
    spdlog::info("  Max entries: {}", config_.max_cache_entries);
    spdlog::info("  Max size: {} MB", config_.max_cache_size_mb);
}

KVCacheManager::~KVCacheManager() {
    clearCache();
}

bool KVCacheManager::saveCache(
    llama_context* ctx,
    const std::string& prefix_hash,
    int n_past
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // KV-Cache-Größe ermitteln
    size_t state_size = llama_state_get_size(ctx);
    
    if (state_size == 0) {
        spdlog::warn("Cannot save cache: state size is 0");
        return false;
    }
    
    // Cache-Daten extrahieren
    std::vector<uint8_t> state_data(state_size);
    size_t written = llama_state_get_data(ctx, state_data.data());
    
    if (written != state_size) {
        spdlog::error("Failed to extract cache state: {} != {}", 
                     written, state_size);
        return false;
    }
    
    // Eviction durchführen wenn nötig
    evictOldestIfNeeded();
    
    // Cache-Entry erstellen
    CacheEntry entry;
    entry.state_data = std::move(state_data);
    entry.n_past = n_past;
    entry.size_bytes = state_size;
    entry.last_access_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    cache_[prefix_hash] = std::move(entry);
    
    spdlog::debug("KV-Cache saved: hash={}, n_past={}, size={}KB",
                 prefix_hash.substr(0, 8), n_past, state_size / 1024);
    
    return true;
}

bool KVCacheManager::loadCache(
    llama_context* ctx,
    const std::string& prefix_hash,
    int& out_n_past
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = cache_.find(prefix_hash);
    if (it == cache_.end()) {
        stats_.cache_misses++;
        return false;
    }
    
    // Cache-Hit
    CacheEntry& entry = it->second;
    
    // KV-Cache wiederherstellen
    size_t read = llama_state_set_data(ctx, entry.state_data.data());
    
    if (read != entry.state_data.size()) {
        spdlog::error("Failed to restore cache state: {} != {}",
                     read, entry.state_data.size());
        stats_.cache_misses++;
        return false;
    }
    
    // Update statistics
    stats_.cache_hits++;
    entry.last_access_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    out_n_past = entry.n_past;
    
    spdlog::debug("KV-Cache loaded: hash={}, n_past={}, hit_rate={:.2f}%",
                 prefix_hash.substr(0, 8), out_n_past, getStats().hit_rate() * 100);
    
    return true;
}

std::string KVCacheManager::computeHash(const std::string& text) const {
    // SHA-256 Hash
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, text.c_str(), text.size());
    SHA256_Final(hash, &sha256);
    
    // Hex-String
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') 
           << static_cast<int>(hash[i]);
    }
    return ss.str();
}

KVCacheManager::Stats KVCacheManager::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    Stats result = stats_;
    result.total_entries = cache_.size();
    result.total_size_bytes = getCurrentCacheSizeBytes();
    
    return result;
}

void KVCacheManager::clearCache() {
    std::lock_guard<std::mutex> lock(mutex_);
    cache_.clear();
    spdlog::info("KV-Cache cleared");
}

void KVCacheManager::evictOldestIfNeeded() {
    // Check entry count
    if (cache_.size() >= config_.max_cache_entries) {
        // Find oldest entry
        auto oldest = cache_.begin();
        for (auto it = cache_.begin(); it != cache_.end(); ++it) {
            if (it->second.last_access_time_ms < oldest->second.last_access_time_ms) {
                oldest = it;
            }
        }
        
        spdlog::debug("Evicting oldest cache entry: hash={}", 
                     oldest->first.substr(0, 8));
        cache_.erase(oldest);
    }
    
    // Check total size
    size_t total_size = getCurrentCacheSizeBytes();
    size_t max_size = config_.max_cache_size_mb * 1024 * 1024;
    
    while (total_size > max_size && !cache_.empty()) {
        // Evict oldest
        auto oldest = cache_.begin();
        for (auto it = cache_.begin(); it != cache_.end(); ++it) {
            if (it->second.last_access_time_ms < oldest->second.last_access_time_ms) {
                oldest = it;
            }
        }
        
        total_size -= oldest->second.size_bytes;
        cache_.erase(oldest);
    }
}

size_t KVCacheManager::getCurrentCacheSizeBytes() const {
    size_t total = 0;
    for (const auto& [hash, entry] : cache_) {
        total += entry.size_bytes;
    }
    return total;
}

} // namespace llm
} // namespace themis
```

**Schritt 3:** Integration in LlamaWrapper (`src/llm/llama_wrapper.cpp`)

```cpp
// In LlamaWrapper Constructor
LlamaWrapper::LlamaWrapper(const Config& config)
    : config_(config) {
    
    // ... existing code ...
    
    // NEW: KV-Cache Manager initialisieren
    KVCacheManager::Config cache_config;
    cache_config.max_cache_entries = 100;
    cache_config.max_cache_size_mb = 1024;  // 1 GB
    kv_cache_manager_ = std::make_unique<KVCacheManager>(cache_config);
}

// In generate() method
InferenceResponse LlamaWrapper::generate(const InferenceRequest& request) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // ... model loading ...
    
    // NEW: KV-Cache Reuse
    int n_past = 0;
    bool cache_hit = false;
    
    if (!request.system_prompt.empty()) {
        // Hash berechnen
        std::string prefix_hash = kv_cache_manager_->computeHash(
            request.system_prompt
        );
        
        // Cache laden versuchen
        if (kv_cache_manager_->loadCache(context_, prefix_hash, n_past)) {
            cache_hit = true;
            spdlog::info("KV-Cache hit: Skipping {} tokens", n_past);
        } else {
            // Cache miss: System-Prompt evaluieren
            auto system_tokens = tokenizeInternal(
                model, request.system_prompt, true
            );
            
            llama_eval(context_, system_tokens.data(), 
                      system_tokens.size(), 0);
            
            n_past = system_tokens.size();
            
            // Cache speichern
            kv_cache_manager_->saveCache(context_, prefix_hash, n_past);
        }
    }
    
    // User-Prompt evaluieren (ab Position n_past)
    auto user_tokens = tokenizeInternal(model, request.prompt, false);
    llama_eval(context_, user_tokens.data(), user_tokens.size(), n_past);
    
    // ... rest of generation ...
}
```

**Schritt 4:** Tests schreiben (`tests/test_kv_cache_manager.cpp`)

```cpp
#include <gtest/gtest.h>
#include "llm/kv_cache_manager.h"
#include "llm/llama_wrapper.h"

using namespace themis::llm;

class KVCacheTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize llama backend
        llama_backend_init();
        
        // Load model
        wrapper_config_.n_gpu_layers = 0;  // CPU-only for tests
        wrapper_config_.n_ctx = 2048;
        wrapper_ = std::make_unique<LlamaWrapper>(wrapper_config_);
        
        wrapper_->loadModel("/models/test-model.gguf");
    }
    
    void TearDown() override {
        wrapper_.reset();
        llama_backend_free();
    }
    
    LlamaWrapper::Config wrapper_config_;
    std::unique_ptr<LlamaWrapper> wrapper_;
};

TEST_F(KVCacheTest, CacheHitReducesLatency) {
    InferenceRequest req;
    req.system_prompt = "You are a helpful assistant.";
    req.prompt = "What is 2+2?";
    req.max_tokens = 10;
    
    // First request: Cache miss (slow)
    auto start1 = std::chrono::steady_clock::now();
    auto response1 = wrapper_->generate(req);
    auto duration1 = std::chrono::steady_clock::now() - start1;
    
    // Second request: Cache hit (fast)
    req.prompt = "What is 3+3?";  // Different user prompt
    auto start2 = std::chrono::steady_clock::now();
    auto response2 = wrapper_->generate(req);
    auto duration2 = std::chrono::steady_clock::now() - start2;
    
    // Cache hit should be significantly faster
    EXPECT_LT(duration2, duration1 * 0.5)  // At least 2x faster
        << "Cache hit should be faster than cache miss";
}

TEST_F(KVCacheTest, CacheStatsTracking) {
    auto stats_before = wrapper_->getKVCacheStats();
    EXPECT_EQ(stats_before.cache_hits, 0);
    
    // Generate with same system prompt 3 times
    InferenceRequest req;
    req.system_prompt = "You are a helpful assistant.";
    
    for (int i = 0; i < 3; i++) {
        req.prompt = "Query " + std::to_string(i);
        wrapper_->generate(req);
    }
    
    auto stats_after = wrapper_->getKVCacheStats();
    
    // First request: miss, next 2: hits
    EXPECT_EQ(stats_after.cache_misses, 1);
    EXPECT_EQ(stats_after.cache_hits, 2);
    EXPECT_NEAR(stats_after.hit_rate(), 0.666, 0.01);
}
```

**Erwarteter Speedup:**
- First-Token-Latency: **10-20x schneller** bei Cache-Hit
- Beispiel: 200ms → 10ms für wiederholte System-Prompts

---

## 🚀 Feature 2: Speculative Decoding

### Problem
Token-für-Token-Generierung ist langsam:
```
Request → [eval] → token1 → [eval] → token2 → [eval] → token3 ...
          50ms     5ms      50ms     5ms      50ms     5ms
          
Total: 10 tokens × 50ms = 500ms
```

### Lösung: Draft-Model generiert, Target-Model validiert

```
Request → [draft×8] → [verify] → 8 tokens!
          20ms       30ms
          
Total: 10 tokens in ~100ms (5x schneller)
```

**Schritt 1:** Draft-Model-Support (`include/llm/speculative_decoder.h`)

```cpp
#pragma once

#include <vector>
#include <memory>
#include <llama.h>

namespace themis {
namespace llm {

/**
 * @brief Speculative Decoding mit Draft-Model
 * 
 * Verwendet kleines Draft-Model für Vorschläge,
 * großes Target-Model zur Validierung.
 * 2-3x Speedup ohne Qualitätsverlust.
 */
class SpeculativeDecoder {
public:
    struct Config {
        std::string draft_model_path;
        int n_draft = 8;              // Draft-Tokens pro Schritt
        int n_gpu_layers_draft = 0;   // Draft meist auf CPU
        float acceptance_threshold = 0.8f;
    };
    
    explicit SpeculativeDecoder(const Config& config);
    ~SpeculativeDecoder();
    
    /**
     * @brief Initialisiert Draft-Model
     */
    bool initialize();
    
    /**
     * @brief Speculative Decoding durchführen
     * 
     * @param target_ctx Target-Model Context
     * @param prompt Prompt-Tokens
     * @param max_tokens Max. zu generierende Tokens
     * @return Generierte Tokens
     */
    std::vector<llama_token> decode(
        llama_context* target_ctx,
        const std::vector<llama_token>& prompt,
        int max_tokens
    );
    
    struct Stats {
        size_t total_draft_tokens = 0;
        size_t accepted_tokens = 0;
        double acceptance_rate() const {
            return total_draft_tokens > 0 
                ? (double)accepted_tokens / total_draft_tokens 
                : 0.0;
        }
    };
    
    Stats getStats() const;
    
private:
    Config config_;
    llama_model* draft_model_ = nullptr;
    llama_context* draft_ctx_ = nullptr;
    Stats stats_;
    
    std::vector<llama_token> generateDraftTokens(
        const std::vector<llama_token>& context,
        int n_tokens
    );
    
    std::vector<bool> verifyDraftTokens(
        llama_context* target_ctx,
        const std::vector<llama_token>& context,
        const std::vector<llama_token>& draft_tokens
    );
};

} // namespace llm
} // namespace themis
```

**Implementierungs-Details:** Siehe Code in Repository

**Erwarteter Speedup:** 2-3x bei gleicher Output-Qualität

---

## 📊 Benchmarking Guide

### Benchmark-Setup

```cpp
// benchmarks/bench_llama_features.cpp

#include <benchmark/benchmark.h>
#include "llm/llama_wrapper.h"

static void BM_InferenceWithoutCache(benchmark::State& state) {
    LlamaWrapper wrapper(config);
    wrapper.loadModel("/models/llama-3-8b-q4.gguf");
    
    for (auto _ : state) {
        InferenceRequest req;
        req.system_prompt = "You are a helpful assistant.";
        req.prompt = "Hello!";
        req.max_tokens = 50;
        
        auto response = wrapper.generate(req);
        benchmark::DoNotOptimize(response);
    }
}
BENCHMARK(BM_InferenceWithoutCache);

static void BM_InferenceWithCache(benchmark::State& state) {
    // ... with KV-Cache enabled ...
}
BENCHMARK(BM_InferenceWithCache);

BENCHMARK_MAIN();
```

**Ausführen:**

```bash
cmake -B build -DTHEMIS_ENABLE_LLM=ON -DTHEMIS_ENABLE_BENCHMARKS=ON
cmake --build build --target bench_llama_features
./build/benchmarks/bench_llama_features --benchmark_out=results.json
```

---

## 📚 Weiterführende Ressourcen

- [LLAMA_CPP_API_FEATURE_RESEARCH.md](./LLAMA_CPP_API_FEATURE_RESEARCH.md) - Vollständige Feature-Liste
- [llama.cpp GitHub](https://github.com/ggerganov/llama.cpp)
- [llama.cpp Examples](https://github.com/ggerganov/llama.cpp/tree/master/examples)

---

**Status:** ✅ Implementation Guide Complete  
**Nächste Schritte:** Phase 1 Features implementieren  
**Maintainer:** ThemisDB Core Team
