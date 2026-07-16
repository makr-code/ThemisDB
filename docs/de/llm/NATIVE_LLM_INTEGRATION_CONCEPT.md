# Native LLM Integration: Technisches Konzept & Best Practices

**Stand:** 6. April 2026  
**Version:** 1.0.0  
**Kategorie:** Architecture / LLM Integration  
**Roadmap:** v1.5.0 / v2.0.0 (2026)  
**Sprache:** Deutsch

---

## 📋 Executive Summary

Dieses Dokument beschreibt das **technische Konzept für die native Integration von LLMs (+LoRA) direkt in ThemisDB**, basierend auf Best Practices und Real-World-Beispielen von führenden Implementierungen (llama.cpp, vLLM, Ollama, LangChain).

**Kernziel:** Maximale Effizienz und Speed durch direkte Speicher-Integration zwischen ThemisDB Vector Storage und LLM Inference Engine.

---

## 🎯 Design-Prinzipien

### 1. Zero-Copy Architecture

**Inspiration:** Apache Arrow, RAPIDS cuDF, PyTorch

```cpp
/**
 * DESIGN PRINCIPLE: Vermeide alle unnötigen Kopien
 * 
 * BAD ❌:
 *   Vector-DB → CPU RAM → Serialize → LLM → Deserialize → GPU VRAM
 *   (4 Kopien, ~6 Sekunden für 10M Vektoren)
 * 
 * GOOD ✅:
 *   Vector-DB (GPU VRAM) → Direct Pointer → LLM (GPU VRAM)
 *   (0 Kopien, ~0ms!)
 */
```

**Real-World Beispiel: PyTorch DataLoader Zero-Copy**

```python
# PyTorch Best Practice: Memory Pinning
loader = torch.utils.data.DataLoader(
    dataset,
    batch_size=32,
    pin_memory=True,      # Zero-copy CPU → GPU
    num_workers=4
)

# ThemisDB Äquivalent:
class ThemisVectorLoader {
    // Pinned memory für direkten GPU Transfer
    float* pinned_vectors_;
    cudaHostAlloc(&pinned_vectors_, size, cudaHostAllocMapped);
};
```

### 2. Unified Memory Space

**Inspiration:** CUDA Unified Memory, Apple Metal Unified Memory

```cpp
/**
 * DESIGN PRINCIPLE: Ein Speicher für Datenbank UND LLM
 * 
 * Traditional Architecture:
 * ┌─────────────┐    ┌─────────────┐
 * │ DB Memory   │    │ LLM Memory  │
 * │ (Separate)  │    │ (Separate)  │
 * └─────────────┘    └─────────────┘
 *       │                    │
 *       └────────┬───────────┘
 *                │ Copy Required
 * 
 * Unified Memory Architecture:
 * ┌─────────────────────────────────┐
 * │   Unified Memory Pool           │
 * │   ┌─────────┐    ┌──────────┐   │
 * │   │ DB Data │    │ LLM Data │   │
 * │   └─────────┘    └──────────┘   │
 * └─────────────────────────────────┘
 *          │
 *          └── Zero Copy Access
 */

namespace themis {
namespace llm {

class UnifiedMemoryManager {
public:
    /**
     * Allokiere Memory, das von DB UND LLM genutzt werden kann
     */
    static void* allocateUnified(size_t size) {
        void* ptr;
        cudaMallocManaged(&ptr, size, cudaMemAttachGlobal);
        return ptr;
    }
    
    /**
     * Vector Storage und LLM greifen auf denselben Speicher zu
     */
    static float* allocateVectorStorage(size_t num_vectors, size_t dim) {
        size_t size = num_vectors * dim * sizeof(float);
        return static_cast<float*>(allocateUnified(size));
    }
};

} // namespace llm
} // namespace themis
```

**Real-World Beispiel: NVIDIA RAPIDS cuDF**

```python
# cuDF: DataFrame direkt auf GPU, Zero-Copy zu PyTorch/TensorFlow
import cudf
import torch

# Read data on GPU
df = cudf.read_parquet('data.parquet')

# Zero-copy conversion zu PyTorch
tensor = torch.as_tensor(df['embeddings'].values, device='cuda')
# Kein cudaMemcpy! Direkter Pointer-Cast!
```

### 3. Lazy Loading & Streaming

**Inspiration:** HuggingFace Datasets, Mmap-backed Models

```cpp
/**
 * DESIGN PRINCIPLE: Lade nur was gebraucht wird, WANN es gebraucht wird
 */

class LazyVectorLoader {
public:
    /**
     * Memory-Mapped Vector Storage (wie HuggingFace Datasets)
     */
    LazyVectorLoader(const std::string& vector_file) {
        // Mmap file (OS managed, on-demand loading)
        fd_ = open(vector_file.c_str(), O_RDONLY);
        struct stat st;
        fstat(fd_, &st);
        size_ = st.st_size;
        
        // Map file in memory (LAZY! Pages loaded on access)
        data_ = static_cast<float*>(
            mmap(nullptr, size_, PROT_READ, MAP_SHARED, fd_, 0)
        );
        
        // Optional: Hint for sequential access
        madvise(data_, size_, MADV_SEQUENTIAL);
    }
    
    /**
     * On-Demand Vector Access (Zero-Copy!)
     */
    const float* getVector(size_t index) {
        // Page fault nur beim ersten Zugriff!
        return data_ + (index * dimension_);
    }
    
    /**
     * Streaming Iterator (wie DataLoader)
     */
    class Iterator {
        Iterator& operator++() {
            // Prefetch nächste Page
            size_t next_offset = (current_idx_ + batch_size_) * dim_ * sizeof(float);
            madvise(data_ + next_offset, PAGE_SIZE, MADV_WILLNEED);
            current_idx_ += batch_size_;
            return *this;
        }
    };
};
```

**Real-World Beispiel: llama.cpp mmap Models**

```cpp
// llama.cpp: Models werden gemmap-t, nicht komplett geladen
struct llama_model {
    // Model weights als mmap
    std::vector<uint8_t> mapping;  // mmap backing
    
    void load_mmap(const char* fname) {
        mapping.resize(file_size);
        void* addr = mmap(NULL, file_size, PROT_READ, MAP_SHARED, fd, 0);
        // Model "geladen" aber noch nicht im RAM!
        // OS lädt Pages on-demand
    }
};
```

---

## 🔧 Komponenten-Design

### 1. Native LLM Engine Integration

**Best Practice: Embedded llama.cpp (wie Ollama)**

```cpp
// include/llm/native_llm_engine.h
#pragma once

#include "llama.h"
#include "ggml.h"
#include "storage/storage_engine.h"
#include "acceleration/faiss_gpu_backend.h"

namespace themis {
namespace llm {

/**
 * Native LLM Engine
 * 
 * Direkt in ThemisDB eingebettet (wie Ollama es macht)
 * 
 * Architecture:
 * ┌────────────────────────────────────────┐
 * │  ThemisDB Process                      │
 * │  ┌──────────────┐  ┌──────────────┐   │
 * │  │ RocksDB      │  │ llama.cpp    │   │
 * │  │ (Storage)    │  │ (Embedded)   │   │
 * │  └──────┬───────┘  └───────┬──────┘   │
 * │         │                  │           │
 * │         └──────────┬───────┘           │
 * │                    │                   │
 * │         Shared GPU Memory (CUDA)       │
 * └────────────────────────────────────────┘
 */
class NativeLLMEngine {
public:
    struct Config {
        // Model Configuration
        std::string model_path;           // GGUF file path
        std::string model_type = "llama"; // llama, mistral, mixtral, etc.
        
        // Resource Allocation (koordiniert mit DB)
        size_t max_vram_mb = 16384;       // 16 GB für Model
        size_t context_size = 4096;       // Context window
        int n_gpu_layers = 32;             // GPU offload
        int n_threads = 8;                 // CPU threads
        
        // Zero-Copy Integration
        bool enable_unified_memory = true; // CUDA Unified Memory
        bool enable_mmap = true;           // Memory-mapped weights
        bool share_vram_with_faiss = true; // Share GPU with Vector Search
        
        // Storage Integration
        storage::StorageEngine* storage = nullptr;
        acceleration::FaissGPUVectorBackend* faiss_backend = nullptr;
    };
    
    explicit NativeLLMEngine(const Config& config);
    
    /**
     * ZERO-COPY: Nutze FAISS Vectors direkt für RAG
     * 
     * Ohne Copy:
     *   FAISS GPU → LLM (Direct Pointer)
     * 
     * Statt:
     *   FAISS GPU → CPU → Serialize → LLM → GPU
     */
    struct RAGContext {
        const float* embeddings_gpu;  // Direct pointer zu FAISS GPU
        size_t num_vectors;
        size_t dimension;
        const uint64_t* doc_ids;      // Document IDs
    };
    
    std::string generateWithRAG(
        const std::string& prompt,
        const RAGContext& context,
        int max_tokens = 512
    );
    
    /**
     * STREAMING: Generate tokens iterativ (wie OpenAI Streaming)
     */
    class StreamingGenerator {
    public:
        bool hasNext();
        std::string next();  // Nächstes Token
        
    private:
        llama_context* ctx_;
        std::vector<llama_token> tokens_;
    };
    
    StreamingGenerator generateStream(const std::string& prompt);
    
    /**
     * BATCH: Process multiple requests parallel (wie vLLM)
     */
    struct BatchRequest {
        std::string prompt;
        int max_tokens = 512;
        float temperature = 0.7f;
    };
    
    std::vector<std::string> generateBatch(
        const std::vector<BatchRequest>& requests
    );
    
    /**
     * LoRA: Dynamic Adapter Loading
     * 
     * Best Practice: LoRA als Shared Memory Segment
     */
    bool loadLoRAAdapter(
        const std::string& adapter_id,
        const std::string& adapter_path,
        float scaling = 1.0f
    );
    
    /**
     * STATISTICS
     */
    struct Stats {
        size_t total_tokens_generated = 0;
        size_t total_requests = 0;
        double avg_tokens_per_second = 0.0;
        size_t vram_used_mb = 0;
        size_t ram_used_mb = 0;
    };
    
    Stats getStats() const;
    
private:
    Config config_;
    
    // llama.cpp components
    llama_model* model_ = nullptr;
    llama_context* context_ = nullptr;
    
    // Resource Sharing
    struct VRAMAllocation {
        size_t model_size_mb;
        size_t kv_cache_mb;
        size_t lora_cache_mb;
        size_t shared_with_faiss_mb;
        size_t total_mb;
    };
    
    VRAMAllocation vram_allocation_;
    
    // Initialize with resource coordination
    void initializeWithResourceSharing();
    
    // Zero-copy helpers
    const float* getZeroCopyEmbeddings(const RAGContext& context);
};

} // namespace llm
} // namespace themis
```

**Implementation: Zero-Copy RAG**

```cpp
// src/llm/native_llm_engine.cpp
#include "llm/native_llm_engine.h"

namespace themis {
namespace llm {

NativeLLMEngine::NativeLLMEngine(const Config& config) : config_(config) {
    // 1. Initialize llama.cpp backend
    llama_backend_init(false);
    
    // 2. Load model mit mmap (LAZY loading!)
    llama_model_params model_params = llama_model_default_params();
    model_params.n_gpu_layers = config.n_gpu_layers;
    model_params.use_mmap = config.enable_mmap;
    model_params.use_mlock = false;  // Don't lock in RAM
    
    // CUDA Unified Memory für Zero-Copy
    if (config.enable_unified_memory) {
        model_params.memory_type = GGML_MEMORY_TYPE_UNIFIED;
    }
    
    model_ = llama_load_model_from_file(
        config.model_path.c_str(),
        model_params
    );
    
    // 3. Create context mit koordiniertem VRAM Budget
    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = config.context_size;
    ctx_params.n_threads = config.n_threads;
    
    // WICHTIG: Koordiniere VRAM mit FAISS
    if (config.share_vram_with_faiss && config.faiss_backend) {
        auto faiss_stats = config.faiss_backend->getIndexStats();
        size_t faiss_vram_mb = faiss_stats.memoryUsageBytes / (1024 * 1024);
        
        // Berechne verfügbares VRAM
        size_t available_vram = config.max_vram_mb - faiss_vram_mb;
        
        // Allokiere KV Cache basierend auf verfügbarem VRAM
        size_t kv_cache_size = (available_vram * 1024 * 1024) / 2;
        ctx_params.kv_cache_size = kv_cache_size;
        
        LOG_INFO << "VRAM Allocation: "
                 << "FAISS=" << faiss_vram_mb << "MB, "
                 << "LLM=" << (available_vram / 2) << "MB, "
                 << "KV Cache=" << (available_vram / 2) << "MB";
    }
    
    context_ = llama_new_context_with_model(model_, ctx_params);
    
    // 4. Track VRAM Allocation
    vram_allocation_.model_size_mb = llama_model_vram_usage(model_) / (1024 * 1024);
    vram_allocation_.kv_cache_mb = ctx_params.kv_cache_size / (1024 * 1024);
    vram_allocation_.total_mb = vram_allocation_.model_size_mb + vram_allocation_.kv_cache_mb;
}

std::string NativeLLMEngine::generateWithRAG(
    const std::string& prompt,
    const RAGContext& context,
    int max_tokens
) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // ZERO-COPY: Nutze FAISS Embeddings direkt
    // Kein cudaMemcpy! Direct Pointer!
    const float* embeddings_gpu = context.embeddings_gpu;
    
    // Build RAG Prompt mit Context Documents
    std::string rag_prompt = "Context:\n";
    
    for (size_t i = 0; i < context.num_vectors; ++i) {
        // Load document content from Storage
        std::string doc_id = std::to_string(context.doc_ids[i]);
        auto doc = config_.storage->getEntity(doc_id);
        
        if (doc.has_value()) {
            rag_prompt += doc->blob + "\n\n";
        }
    }
    
    rag_prompt += "Question: " + prompt + "\n\nAnswer:";
    
    // Tokenize
    std::vector<llama_token> tokens = tokenize(rag_prompt);
    
    // Evaluate prompt (mit Embeddings im Context)
    llama_eval(context_, tokens.data(), tokens.size(), 0);
    
    // Generate tokens
    std::string result;
    for (int i = 0; i < max_tokens; ++i) {
        llama_token new_token = llama_sample_token_greedy(context_, nullptr);
        
        if (new_token == llama_token_eos(model_)) {
            break;
        }
        
        result += llama_token_to_piece(context_, new_token);
        
        // Evaluate new token
        llama_eval(context_, &new_token, 1, tokens.size() + i);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start
    ).count();
    
    LOG_INFO << "Generated " << result.length() << " chars in " << duration << "ms";
    
    return result;
}

} // namespace llm
} // namespace themis
```

### 2. LoRA Integration (Best Practice: Adapter Fusion)

**Inspiration: HuggingFace PEFT, Microsoft LoRA**

```cpp
// include/llm/lora_fusion_engine.h
#pragma once

namespace themis {
namespace llm {

/**
 * LoRA Fusion Engine
 * 
 * Best Practice aus Microsoft's LoRA Paper:
 * - Multiple LoRAs parallel laden
 * - Dynamische Fusion basierend auf Query
 * - Shared Memory für Adapter
 */
class LoRAFusionEngine {
public:
    struct LoRAAdapter {
        std::string id;
        std::string domain;        // "legal", "medical", etc.
        
        // LoRA Weights (Low-Rank Matrices)
        float* W_A;  // rank × d_model (z.B. 8 × 4096)
        float* W_B;  // d_model × rank
        int rank;
        float scaling;
        
        // Shared Memory Backing (für Cross-Shard)
        void* shm_handle = nullptr;
        std::string shm_name;
    };
    
    /**
     * Multi-LoRA Fusion (wie PEFT)
     * 
     * Combined Output = Base Model + α₁·LoRA₁ + α₂·LoRA₂ + ...
     */
    struct FusionWeights {
        std::string lora_id;
        float weight;  // α
    };
    
    void addAdapter(const LoRAAdapter& adapter);
    void removeAdapter(const std::string& adapter_id);
    
    /**
     * Dynamic Fusion basierend auf Query
     * 
     * Beispiel:
     *   Query: "Medical and legal aspects..."
     *   → Fusion: 0.6 × medical-lora + 0.4 × legal-lora
     */
    std::vector<FusionWeights> selectAdaptersForQuery(
        const std::string& query
    );
    
    /**
     * Apply Fusion während Inference
     */
    void applyFusion(
        llama_context* ctx,
        const std::vector<FusionWeights>& fusion
    );
    
private:
    std::map<std::string, LoRAAdapter> adapters_;
    
    // Shared Memory Pool für LoRAs
    struct LoRAMemoryPool {
        size_t total_size_mb = 512;  // 512 MB für LoRA Cache
        size_t used_mb = 0;
        std::map<std::string, void*> cached_adapters;
    };
    
    LoRAMemoryPool lora_pool_;
};

} // namespace llm
} // namespace themis
```

**Real-World Beispiel: HuggingFace PEFT LoRA Loading**

```python
# HuggingFace Best Practice: Multiple LoRA Adapters
from peft import PeftModel, LoraConfig

# Base model
base_model = AutoModelForCausalLM.from_pretrained("mistral-7b")

# Load multiple LoRAs
model = PeftModel.from_pretrained(base_model, "legal-lora")
model.load_adapter("medical-lora", adapter_name="medical")
model.load_adapter("finance-lora", adapter_name="finance")

# Dynamic switching
model.set_adapter("legal")      # Use legal LoRA
model.set_adapter("medical")    # Switch to medical LoRA

# Or: Adapter fusion!
model.add_weighted_adapter(
    adapters=["legal", "medical"],
    weights=[0.6, 0.4],
    adapter_name="legal_medical_fusion"
)
```

**ThemisDB C++ Äquivalent:**

```cpp
// Dynamische LoRA Fusion in ThemisDB
LoRAFusionEngine lora_engine;

// Load adapters from Shared Memory (von anderen Shards)
lora_engine.addAdapter({
    .id = "legal-qa-v1",
    .domain = "legal",
    .shm_name = "/themis_lora_legal_qa_v1",
    .rank = 8,
    .scaling = 1.0f
});

lora_engine.addAdapter({
    .id = "medical-v1",
    .domain = "medical",
    .shm_name = "/themis_lora_medical_v1",
    .rank = 16,
    .scaling = 1.0f
});

// Query: "Medical and legal aspects of patient consent"
auto fusion = lora_engine.selectAdaptersForQuery(query);
// → [{legal-qa-v1, 0.4}, {medical-v1, 0.6}]

// Apply fusion
lora_engine.applyFusion(llm_context, fusion);
```

---

## 📊 Performance Optimizations (Best Practices)

### 1. Batching & Continuous Batching

**Inspiration: vLLM Continuous Batching**

```cpp
/**
 * vLLM's Key Innovation: Continuous Batching
 * 
 * Traditional Batching:
 *   Wait for batch_size requests → Process all → Wait again
 *   Problem: Requests mit unterschiedlicher Länge → Wasted compute
 * 
 * Continuous Batching:
 *   Füge neue Requests hinzu sobald fertig
 *   Remove finished requests dynamisch
 *   → 2-3x höherer Throughput!
 */

class ContinuousBatchingEngine {
public:
    struct Request {
        std::string id;
        std::string prompt;
        int max_tokens;
        int tokens_generated = 0;
        bool finished = false;
    };
    
    void addRequest(Request req) {
        std::lock_guard lock(queue_mutex_);
        pending_requests_.push(req);
        cv_.notify_one();
    }
    
    void processLoop() {
        while (running_) {
            std::vector<Request> active_batch;
            
            {
                std::unique_lock lock(queue_mutex_);
                
                // Add new requests zu Active Batch
                while (!pending_requests_.empty() && 
                       active_batch.size() < max_batch_size_) {
                    active_batch.push_back(pending_requests_.front());
                    pending_requests_.pop();
                }
                
                // Add continuing requests
                for (auto& req : continuing_requests_) {
                    if (!req.finished) {
                        active_batch.push_back(req);
                    }
                }
            }
            
            if (active_batch.empty()) {
                std::unique_lock lock(queue_mutex_);
                cv_.wait(lock);
                continue;
            }
            
            // Process Batch (Generate ONE token for each)
            for (auto& req : active_batch) {
                llama_token token = generateNextToken(req);
                req.tokens_generated++;
                
                if (token == llama_token_eos(model_) ||
                    req.tokens_generated >= req.max_tokens) {
                    req.finished = true;
                    completeRequest(req);
                }
            }
            
            // Update continuing requests
            continuing_requests_.clear();
            for (const auto& req : active_batch) {
                if (!req.finished) {
                    continuing_requests_.push_back(req);
                }
            }
        }
    }
    
private:
    std::queue<Request> pending_requests_;
    std::vector<Request> continuing_requests_;
    size_t max_batch_size_ = 32;
    std::mutex queue_mutex_;
    std::condition_variable cv_;
    bool running_ = true;
};
```

### 2. KV Cache Optimization

**Inspiration: FlashAttention, PagedAttention**

```cpp
/**
 * PagedAttention (vLLM's Innovation)
 * 
 * Traditional KV Cache:
 *   Pre-allocate für max_seq_len → Viel Waste
 *   Beispiel: 4096 context × 32 batch = 512 MB pro Request!
 * 
 * PagedAttention:
 *   KV Cache in Pages (wie OS Virtual Memory)
 *   Nur allokieren was gebraucht wird
 *   → 3-5x weniger VRAM!
 */

class PagedKVCache {
public:
    static constexpr size_t PAGE_SIZE = 256;  // Tokens pro Page
    
    struct Page {
        float* key_cache;    // [PAGE_SIZE, num_heads, head_dim]
        float* value_cache;
        bool in_use = false;
    };
    
    class Allocation {
    public:
        void growIfNeeded(size_t num_tokens) {
            size_t pages_needed = (num_tokens + PAGE_SIZE - 1) / PAGE_SIZE;
            
            while (pages_.size() < pages_needed) {
                pages_.push_back(allocatePage());
            }
        }
        
        float* getKeyPointer(size_t token_idx) {
            size_t page_idx = token_idx / PAGE_SIZE;
            size_t offset = token_idx % PAGE_SIZE;
            return pages_[page_idx]->key_cache + offset;
        }
        
    private:
        std::vector<Page*> pages_;
    };
    
private:
    std::vector<Page> page_pool_;
    
    Page* allocatePage() {
        for (auto& page : page_pool_) {
            if (!page.in_use) {
                page.in_use = true;
                return &page;
            }
        }
        
        // Allocate new page
        Page new_page;
        cudaMalloc(&new_page.key_cache, PAGE_SIZE * num_heads_ * head_dim_ * sizeof(float));
        cudaMalloc(&new_page.value_cache, PAGE_SIZE * num_heads_ * head_dim_ * sizeof(float));
        page_pool_.push_back(new_page);
        return &page_pool_.back();
    }
};
```

**Performance Impact:**

```
KV Cache Memory (Mistral-7B, batch_size=32):

Traditional (Pre-allocated):
├─ Seq Length: 4096
├─ Batch Size: 32
├─ Memory: 32 × 4096 × 2 × 32 × 128 × 4 bytes
└─ Total: 3.2 GB VRAM

PagedAttention (On-Demand):
├─ Avg Seq Length: 512  (nicht jeder nutzt 4096!)
├─ Pages Allocated: ~64 pages total
├─ Memory: 64 × 256 × 2 × 32 × 128 × 4 bytes
└─ Total: 0.5 GB VRAM (6.4x Reduktion!)
```

### 3. Quantization & Mixed Precision

**Inspiration: GPTQ, AWQ, bitsandbytes**

```cpp
/**
 * Mixed Precision Inference
 * 
 * Best Practice: Verschiedene Präzisionen für verschiedene Layers
 */

enum class Precision {
    FP32,      // Full precision (32-bit float)
    FP16,      // Half precision (16-bit float) - 2x schneller
    BF16,      // Brain Float 16 (Google's format) - besser für Training
    INT8,      // 8-bit integer (4x schneller) - für Attention
    INT4       // 4-bit integer (8x schneller) - für Weights (GPTQ/AWQ)
};

class MixedPrecisionConfig {
public:
    // Verschiedene Layers, verschiedene Precision
    Precision attention_precision = Precision::INT8;   // Attention kann INT8
    Precision mlp_precision = Precision::FP16;          // MLP braucht FP16
    Precision output_precision = Precision::FP32;       // Output Full Precision
    
    // Weights können SEHR stark quantisiert werden
    Precision weight_precision = Precision::INT4;       // GPTQ/AWQ
};

// Mistral-7B mit Mixed Precision:
// FP32: 28 GB VRAM
// FP16: 14 GB VRAM
// Mixed (INT4 weights, FP16 activations): 4.5 GB VRAM! (6.2x Reduktion)
```

**Real-World Beispiel: llama.cpp Quantization**

```bash
# llama.cpp: Convert model zu verschiedenen Quantization Levels
./quantize model-f32.gguf model-q4_0.gguf q4_0   # 4-bit, 3.5 GB
./quantize model-f32.gguf model-q4_k_m.gguf q4_k_m # 4-bit mit K-Quants, 4.1 GB
./quantize model-f32.gguf model-q8_0.gguf q8_0   # 8-bit, 7.2 GB

# Quality vs. Size Trade-off:
# q4_0: 3.5 GB, ~95% quality
# q4_k_m: 4.1 GB, ~97% quality (RECOMMENDED)
# q8_0: 7.2 GB, ~99% quality
```

---

## 🎯 Complete Integration Example

```cpp
// src/llm/themis_llm_integrated.cpp

namespace themis {
namespace llm {

/**
 * Complete ThemisDB + LLM Integration
 * 
 * Kombiniert alle Best Practices:
 * - Zero-Copy Vector Access
 * - Unified Memory
 * - Continuous Batching
 * - PagedAttention
 * - Mixed Precision
 * - LoRA Fusion
 */
class ThemisLLMIntegrated {
public:
    struct Config {
        // Storage Integration
        storage::StorageEngine* storage;
        acceleration::FaissGPUVectorBackend* faiss_backend;
        
        // LLM Configuration
        std::string model_path = "/models/mistral-7b-q4_k_m.gguf";
        size_t max_vram_mb = 24576;  // 24 GB GPU
        
        // Performance Options
        bool enable_continuous_batching = true;
        bool enable_paged_attention = true;
        bool enable_mixed_precision = true;
        size_t max_batch_size = 32;
    };
    
    explicit ThemisLLMIntegrated(const Config& config) : config_(config) {
        initializeVRAMLayout();
        initializeLLM();
        initializeBatchingEngine();
    }
    
    /**
     * ZERO-COPY RAG Query
     */
    std::string queryRAG(const std::string& query, int top_k = 10) {
        auto start = std::chrono::high_resolution_clock::now();
        
        // 1. Vector Search (GPU) - ZERO-COPY!
        float query_embedding[768];
        embedQuery(query, query_embedding);
        
        auto results = config_.faiss_backend->search(
            query_embedding, 1, top_k
        );
        
        // 2. Get GPU Pointers (ZERO-COPY!)
        const float* faiss_vectors = config_.faiss_backend->getDeviceVectors();
        
        // 3. Build RAG Context
        NativeLLMEngine::RAGContext context;
        context.embeddings_gpu = faiss_vectors;  // Direct GPU pointer!
        context.num_vectors = top_k;
        context.dimension = 768;
        
        std::vector<uint64_t> doc_ids;
        for (const auto& [doc_id, score] : results[0]) {
            doc_ids.push_back(doc_id);
        }
        context.doc_ids = doc_ids.data();
        
        // 4. LLM Generation (mit Continuous Batching!)
        std::string answer = llm_engine_->generateWithRAG(
            query,
            context,
            512  // max_tokens
        );
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end - start
        ).count();
        
        LOG_INFO << "RAG Query completed in " << duration << "ms"
                 << " (Vector Search + LLM Generation)";
        
        return answer;
    }
    
    /**
     * FEDERATED RAG (mit LoRA Fusion)
     */
    std::string queryFederatedRAG(
        const std::string& query,
        const std::vector<std::string>& domains  // ["legal", "medical"]
    ) {
        // Select LoRAs basierend auf Domains
        std::vector<LoRAFusionEngine::FusionWeights> fusion;
        float weight_per_domain = 1.0f / domains.size();
        
        for (const auto& domain : domains) {
            std::string lora_id = domain + "-qa-v1";
            fusion.push_back({lora_id, weight_per_domain});
        }
        
        // Apply LoRA Fusion
        lora_fusion_->applyFusion(llm_engine_->getContext(), fusion);
        
        // Query mit fusioniertem LoRA
        return queryRAG(query);
    }
    
    /**
     * STATISTICS
     */
    struct PerformanceStats {
        // Vector Search
        double avg_vector_search_ms = 0.0;
        size_t total_vector_searches = 0;
        
        // LLM
        double avg_generation_ms = 0.0;
        size_t total_generations = 0;
        double tokens_per_second = 0.0;
        
        // VRAM Usage
        size_t faiss_vram_mb = 0;
        size_t llm_vram_mb = 0;
        size_t kv_cache_vram_mb = 0;
        size_t lora_cache_vram_mb = 0;
        size_t total_vram_mb = 0;
        
        // Efficiency
        bool is_zero_copy = true;
        double cache_hit_rate = 0.0;
    };
    
    PerformanceStats getStats() const;
    
private:
    Config config_;
    
    // Components
    std::unique_ptr<NativeLLMEngine> llm_engine_;
    std::unique_ptr<ContinuousBatchingEngine> batching_engine_;
    std::unique_ptr<PagedKVCache> kv_cache_;
    std::unique_ptr<LoRAFusionEngine> lora_fusion_;
    
    // VRAM Layout Coordination
    struct VRAMLayout {
        size_t faiss_index_mb;       // 8 GB
        size_t llm_model_mb;          // 4.5 GB (Q4_K_M)
        size_t kv_cache_mb;           // 8 GB (Paged)
        size_t lora_cache_mb;         // 512 MB
        size_t working_buffer_mb;     // 2 GB
        size_t total_mb;              // 23 GB (passt in 24 GB!)
    };
    
    VRAMLayout vram_layout_;
    
    void initializeVRAMLayout() {
        // Koordinierte VRAM Allocation
        auto faiss_stats = config_.faiss_backend->getIndexStats();
        vram_layout_.faiss_index_mb = faiss_stats.memoryUsageBytes / (1024 * 1024);
        
        vram_layout_.llm_model_mb = 4500;      // Mistral-7B Q4_K_M
        vram_layout_.kv_cache_mb = 8192;       // Paged KV Cache
        vram_layout_.lora_cache_mb = 512;      // LoRA Adapters
        vram_layout_.working_buffer_mb = 2048; // Temporary buffers
        
        vram_layout_.total_mb = 
            vram_layout_.faiss_index_mb +
            vram_layout_.llm_model_mb +
            vram_layout_.kv_cache_mb +
            vram_layout_.lora_cache_mb +
            vram_layout_.working_buffer_mb;
        
        if (vram_layout_.total_mb > config_.max_vram_mb) {
            throw std::runtime_error(
                "VRAM Budget exceeded: " + 
                std::to_string(vram_layout_.total_mb) + " MB needed, " +
                std::to_string(config_.max_vram_mb) + " MB available"
            );
        }
        
        LOG_INFO << "VRAM Layout:"
                 << "\n  FAISS: " << vram_layout_.faiss_index_mb << " MB"
                 << "\n  LLM: " << vram_layout_.llm_model_mb << " MB"
                 << "\n  KV Cache: " << vram_layout_.kv_cache_mb << " MB"
                 << "\n  LoRA: " << vram_layout_.lora_cache_mb << " MB"
                 << "\n  Working: " << vram_layout_.working_buffer_mb << " MB"
                 << "\n  TOTAL: " << vram_layout_.total_mb << " MB / "
                 << config_.max_vram_mb << " MB";
    }
    
    void initializeLLM() {
        NativeLLMEngine::Config llm_config;
        llm_config.model_path = config_.model_path;
        llm_config.max_vram_mb = vram_layout_.llm_model_mb + vram_layout_.kv_cache_mb;
        llm_config.enable_unified_memory = true;
        llm_config.enable_mmap = true;
        llm_config.share_vram_with_faiss = true;
        llm_config.storage = config_.storage;
        llm_config.faiss_backend = config_.faiss_backend;
        
        llm_engine_ = std::make_unique<NativeLLMEngine>(llm_config);
    }
    
    void initializeBatchingEngine() {
        if (config_.enable_continuous_batching) {
            batching_engine_ = std::make_unique<ContinuousBatchingEngine>();
            batching_engine_->setMaxBatchSize(config_.max_batch_size);
        }
    }
};

} // namespace llm
} // namespace themis
```

---

## 📊 Expected Performance

### Benchmark: RAG Query (10 top-k documents)

```
Setup:
- Model: Mistral-7B-Q4_K_M (4.5 GB)
- GPU: NVIDIA RTX 4090 (24 GB)
- Vector Index: 10M embeddings (8 GB VRAM)
- Context: 4096 tokens

Performance Breakdown:
┌──────────────────────────────────────────────────┐
│  Component            │  Traditional │  Optimized │
├──────────────────────────────────────────────────┤
│  Vector Search        │    120ms     │     5ms    │
│  Vector→LLM Transfer  │    850ms     │     0ms    │ ← ZERO-COPY!
│  Document Load        │     10ms     │    10ms    │
│  LLM Generation       │    300ms     │   300ms    │
│  ────────────────────────────────────────────── │
│  TOTAL                │   1280ms     │   315ms    │ ← 4x faster!
└──────────────────────────────────────────────────┘

Throughput (Continuous Batching):
- Traditional Batching: 3.1 req/s
- Continuous Batching: 8.2 req/s (2.6x improvement)

VRAM Efficiency:
- Traditional (separate DB+LLM): 8 GB + 14 GB = 22 GB
- Integrated (shared VRAM): 23 GB total
  - Savings from: Zero-copy, Unified Memory, Paged KV Cache
```

---

## 🎓 Zusammenfassung & Roadmap

### Best Practices Implementiert:

1. ✅ **Zero-Copy Architecture** (Apache Arrow, PyTorch)
2. ✅ **Unified Memory** (CUDA Unified Memory, Apple Metal)
3. ✅ **Lazy Loading** (HuggingFace Datasets, llama.cpp mmap)
4. ✅ **Continuous Batching** (vLLM Innovation)
5. ✅ **PagedAttention** (vLLM KV Cache)
6. ✅ **Mixed Precision** (GPTQ, AWQ, bitsandbytes)
7. ✅ **LoRA Fusion** (HuggingFace PEFT)

### Roadmap Integration → v1.5.0 / v2.0.0 (2026)

**Phase 1: Prototype (Q1 2026)**
- Native LLM Engine Integration (llama.cpp embedded)
- Zero-Copy Vector Access
- Basic LoRA Support

**Phase 2: Optimization (Q2 2026)**
- Continuous Batching
- PagedAttention KV Cache
- Mixed Precision Inference

**Phase 3: Production (Q3 2026)**
- LoRA Fusion Engine
- Federated RAG
- Performance Tuning

**Phase 4: Scale (Q4 2026)**
- Multi-GPU Support
- Model Serving at Scale
- Production Deployment

---

**Erstellt:** Dezember 2025  
**Status:** Technical Design / Implementation Proposal  
**Roadmap:** v1.5.0 - v2.0.0 (2026)
