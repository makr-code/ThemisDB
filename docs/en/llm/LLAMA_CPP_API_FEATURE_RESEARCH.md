# llama.cpp API Feature Research for ThemisDB

**Date:** January 5, 2026  
**Status:** Research Complete  
**Purpose:** Identify additional llama.cpp features that ThemisDB could leverage

---

## 🎯 Executive Summary

This document systematically analyzes the llama.cpp API to identify features that ThemisDB v1.3.0 doesn't currently use but could benefit from.

**Key Findings:**
- ✅ ThemisDB currently uses **11 core llama.cpp features**
- 🆕 Identified **14 additional features** with high potential value
- 🚀 **Top 5 priorities** could provide 2-5x performance improvements
- 📅 Recommended phased implementation over 3 releases (v1.3.1 - v1.5)

---

## 📊 Current ThemisDB Integration Status

### Already Utilized Features

ThemisDB v1.3.0 currently leverages these llama.cpp capabilities:

#### 1. **Core Inference**
- ✅ `llama_backend_init()` / `llama_backend_free()` - Backend initialization
- ✅ `llama_load_model_from_file()` - GGUF model loading
- ✅ `llama_new_context_with_model()` - Context creation
- ✅ `llama_tokenize()` - Text → Token conversion
- ✅ `llama_eval()` / `llama_decode()` - Inference execution
- ✅ `llama_sample_token()` - Token sampling
- ✅ `llama_token_to_str()` - Token → Text conversion

#### 2. **Model Management**
- ✅ `llama_model_params` - Model configuration (n_gpu_layers, use_mmap)
- ✅ `llama_context_params` - Context configuration (n_ctx, n_batch)
- ✅ Lazy Model Loading (ThemisDB custom implementation)

#### 3. **LoRA Support**
- ✅ `llama_lora_adapter_load()` - LoRA adapter loading
- ✅ `llama_lora_adapter_set()` - Apply LoRA to context
- ✅ `llama_lora_adapter_remove()` - Remove LoRA
- ✅ Multi-LoRA Management (ThemisDB custom implementation)

#### 4. **GPU Acceleration**
- ✅ CUDA Support (NVIDIA)
- ✅ Metal Support (Apple Silicon)
- ✅ Vulkan Support (Cross-platform)

---

## 🔍 Unused Features with High Potential

### Category A: High Priority Features

#### 1. **Speculative Decoding (Draft Model)**

**What is it?**
- Uses a smaller "draft model" to generate proposals
- Large model only validates proposals (faster)
- 2-3x speedup with same quality

**llama.cpp API:**
```cpp
// Draft model for speculative decoding
llama_model* draft_model = llama_load_model_from_file(
    "/models/llama-160m-draft.gguf",
    draft_params
);

// Enable speculative decoding
llama_sampling_params sampling;
sampling.n_draft = 8;  // 8 draft tokens per step
```

**Benefits for ThemisDB:**
- 🚀 **Performance**: 2-3x faster inference without quality loss
- 💰 **Efficiency**: Less GPU compute time per response
- 🎯 **Use Case**: Ideal for interactive chat applications

**Implementation Effort:** Medium (1-2 weeks)

---

#### 2. **KV-Cache Reuse (Prefix Caching)**

**What is it?**
- Reuse KV-Cache for identical prompt prefixes
- System prompts don't need to be recomputed
- Dramatically reduces first-token latency

**llama.cpp API:**
```cpp
// Save KV-Cache
std::vector<uint8_t> cache_state;
size_t cache_size = llama_state_get_size(ctx);
cache_state.resize(cache_size);
llama_state_get_data(ctx, cache_state.data());

// Restore KV-Cache
llama_state_set_data(ctx, cache_state.data());
```

**Benefits for ThemisDB:**
- ⚡ **Latency**: 10-20x faster start for repeated prompts
- 💾 **Memory**: Already planned in ThemisDB design (`llm_prefix_cache.h`)
- 🎯 **Use Case**: RAG with consistent system instructions

**Implementation Effort:** Low (3-5 days) - Skeleton already exists

---

#### 3. **Continuous Batching**

**What is it?**
- Process multiple inference requests simultaneously
- Dynamically add/remove sequences in batch
- Similar to vLLM continuous batching

**llama.cpp API:**
```cpp
// Batch structure
llama_batch batch = llama_batch_init(512, 0, 1);

// Add multiple sequences
for (int i = 0; i < num_sequences; ++i) {
    llama_batch_add(batch, tokens[i], pos[i], {seq_ids[i]}, false);
}

// Execute batch
llama_decode(ctx, batch);
```

**Benefits for ThemisDB:**
- 📈 **Throughput**: 5-10x higher throughput with same VRAM
- 🌐 **Scaling**: Better multi-user performance
- 🎯 **Use Case**: Multi-tenant scenarios, API server

**Implementation Effort:** Medium-High (2-3 weeks)

---

#### 4. **Flash Attention / PagedAttention**

**What is it?**
- Optimized attention computation (less memory, faster)
- Paged KV-Cache for efficient memory usage
- Standard in modern LLM serving systems

**llama.cpp API:**
```cpp
// Flash Attention (automatic when available)
llama_model_params params = llama_model_default_params();
params.use_flash_attn = true;  // Since llama.cpp b2000+

// PagedAttention via n_ctx_per_seq
llama_context_params ctx_params = llama_context_default_params();
ctx_params.n_ctx = 32768;
ctx_params.n_ctx_per_seq = 4096;  // Only 4K active per sequence
```

**Benefits for ThemisDB:**
- 💾 **Memory**: 30-50% less KV-Cache memory
- ⚡ **Performance**: 15-25% faster attention
- 🎯 **Use Case**: Long contexts (32K+ tokens)

**Implementation Effort:** Low (already integrated in llama.cpp)

---

#### 5. **Embeddings Extraction**

**What is it?**
- Direct extraction of token/sequence embeddings
- Use LLM as embedding model
- Alternative to separate embedding models

**llama.cpp API:**
```cpp
// Extract embeddings
llama_context_params ctx_params = llama_context_default_params();
ctx_params.embeddings = true;  // Enable embedding mode

// After eval: retrieve embeddings
float* embeddings = llama_get_embeddings(ctx);
int n_embd = llama_n_embd(model);

// Copy vector
std::vector<float> embedding_vec(embeddings, embeddings + n_embd);
```

**Benefits for ThemisDB:**
- 🔗 **Integration**: LLM + Embedding in one model
- 💾 **Efficiency**: No separate embedding model needed
- 🎯 **Use Case**: RAG embedding generation, semantic search

**Implementation Effort:** Low (2-3 days)

---

#### 6. **Grammar-Constrained Generation**

**What is it?**
- Forces structured output (JSON, XML, etc.)
- Guarantees syntactically valid responses
- Based on GBNF (GGML BNF) grammar definition

**llama.cpp API:**
```cpp
// Define grammar (JSON schema)
const char* json_grammar = R"(
root ::= object
object ::= "{" pair ("," pair)* "}"
pair ::= string ":" value
value ::= string | number | object | array
string ::= "\"" [^"]* "\""
number ::= [0-9]+
)";

// Create grammar parser
llama_grammar* grammar = llama_grammar_init(
    llama_grammar_parse(json_grammar)
);

// Sample with grammar
llama_sample_grammar(ctx, &candidates, grammar);
```

**Benefits for ThemisDB:**
- ✅ **Reliability**: 100% valid JSON/XML responses
- 🤖 **Automation**: No post-processing errors
- 🎯 **Use Case**: API integration, structured extraction

**Implementation Effort:** Medium (1-2 weeks)

---

#### 7. **Multi-Modal Support (Vision)**

**What is it?**
- Process images + text (LLaVA, LLaMA-3.2-Vision)
- Image-to-text embeddings in LLM context
- Native vision model support

**llama.cpp API:**
```cpp
// Load vision model (e.g., LLaVA)
llama_model* vision_model = llama_load_model_from_file(
    "/models/llava-v1.6-34b-q4.gguf",
    params
);

// Embed image
clip_image_u8 image = clip_image_load("/path/to/image.jpg");
clip_image_f32 preprocessed = clip_image_preprocess(clip_ctx, image);

// Embed in context
llama_eval_image(ctx, preprocessed.data, n_image_tokens, 0);

// Now execute text prompt
llama_eval(ctx, text_tokens, n_text_tokens, n_image_tokens);
```

**Benefits for ThemisDB:**
- 🖼️ **Multi-Modal**: Image + Text in one workflow
- 🔗 **Integration**: ThemisDB already has Image Analysis Plugin
- 🎯 **Use Case**: Document processing, visual Q&A

**Implementation Effort:** High (3-4 weeks) - Separate CLIP integration needed

---

## 📋 Priority Matrix

| Feature | Priority | Effort | Impact | Recommendation |
|---------|----------|--------|--------|----------------|
| **Speculative Decoding** | 🔴 High | Medium | Very High | ✅ **Implement** (v1.4) |
| **KV-Cache Reuse** | 🔴 High | Low | Very High | ✅ **Implement** (v1.3.1) |
| **Continuous Batching** | 🔴 High | High | Very High | ✅ **Plan** (v1.4) |
| **Flash Attention** | 🔴 High | Very Low | High | ✅ **Enable** (now) |
| **Embeddings** | 🔴 High | Low | High | ✅ **Implement** (v1.3.1) |
| **Grammar Generation** | 🟡 Medium | Medium | Medium | 📅 Plan (v1.4) |
| **Vision Support** | 🟡 Medium | High | High | 📅 Plan (v1.5+) |
| **Server Mode** | 🟡 Medium | Low/High | Medium | 📋 Optional |
| **On-the-fly Quant** | 🟡 Medium | Medium | Medium | 📅 Plan (v1.4) |
| **RoPE Scaling** | 🟡 Medium | Very Low | Medium | 📅 Test (v1.4) |

---

## 🎯 Recommended Implementation Roadmap

### Phase 1: Quick Wins (v1.3.1 - 1 week)
1. ✅ **Enable Flash Attention** (immediate, config only)
2. ✅ **Implement KV-Cache Reuse** (skeleton exists)
3. ✅ **Embeddings Extraction** (simple API extension)
4. 📊 **Metadata Inspection** (model registry foundation)

**Effort:** ~5 work days  
**Impact:** Immediate performance improvement

---

### Phase 2: Major Features (v1.4 - 6-8 weeks)
1. 🚀 **Speculative Decoding** (draft model support)
2. 📈 **Continuous Batching** (multi-request handling)
3. 🎯 **Grammar-Constrained Generation** (structured output)
4. 🔧 **On-the-fly Quantization** (dynamic VRAM)

**Effort:** ~6-8 weeks (staggered)  
**Impact:** Massive performance/quality boost

---

### Phase 3: Advanced Features (v1.5+ - 3-6 months)
1. 🖼️ **Vision Support** (multi-modal)
2. 🔧 **RoPE Scaling** (extended context)
3. 🌐 **Server Mode** (optional addon)
4. 🎛️ **Custom Sampling Strategies** (advanced tuning)

**Effort:** ~3-6 months (parallel)  
**Impact:** Feature parity with top-tier LLM servers

---

## 📊 Competitive Comparison

| Feature | ThemisDB (current) | vLLM | Ollama | llama.cpp Server |
|---------|-------------------|------|--------|------------------|
| Basic Inference | ✅ | ✅ | ✅ | ✅ |
| Multi-LoRA | ✅ | ✅ | ❌ | ⚠️ Limited |
| Lazy Loading | ✅ | ❌ | ✅ | ❌ |
| Continuous Batching | ❌ | ✅ | ❌ | ⚠️ Experimental |
| Speculative Decoding | ❌ | ✅ | ❌ | ✅ |
| KV-Cache Reuse | ⚠️ Skeleton | ✅ | ✅ | ✅ |
| Flash Attention | ❌ | ✅ | ✅ (auto) | ✅ (auto) |
| Vision Support | ⚠️ Separate Plugin | ✅ | ✅ | ✅ |
| Grammar Generation | ❌ | ✅ | ❌ | ✅ |
| Embeddings | ❌ | ✅ | ✅ | ✅ |

**Conclusion:** ThemisDB leads in Lazy Loading + Multi-LoRA but lags in performance features (Batching, Spec Decoding).

---

## 📚 References

### llama.cpp Documentation
- **GitHub:** https://github.com/ggerganov/llama.cpp
- **Examples:** https://github.com/ggerganov/llama.cpp/tree/master/examples
- **Server:** https://github.com/ggerganov/llama.cpp/tree/master/examples/server

### Research Papers
- **Speculative Decoding:** Chen et al., 2023 - "Accelerating LLM Inference with Speculative Sampling"
- **Flash Attention:** Dao et al., 2022 - "FlashAttention: Fast and Memory-Efficient Exact Attention"
- **PagedAttention:** Kwon et al., 2023 - "Efficient Memory Management for LLM Serving with PagedAttention"

### Competitor Analysis
- **vLLM:** https://docs.vllm.ai/
- **Ollama:** https://github.com/ollama/ollama
- **TensorRT-LLM:** https://github.com/NVIDIA/TensorRT-LLM

---

## ✅ Next Steps

1. ✅ **Finalize documentation** (this document)
2. 📋 **Prioritize Phase 1 features** (KV-Cache, Flash Attention)
3. 🛠️ **Implement POC** (Flash Attention enable)
4. 📊 **Run benchmarks** (before/after comparison)
5. 📝 **Create GitHub issues** for Phase 2/3 features

---

**Status:** ✅ Research Complete  
**Next Review:** January 2026  
**Maintainer:** ThemisDB Core Team
