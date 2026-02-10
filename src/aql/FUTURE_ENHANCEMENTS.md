# AQL Module - Future Enhancements

## Planned Features

### Multi-Modal LLM Support
**Priority:** High  
**Target Version:** v1.7.0

Extend LLM commands to support images, audio, and video inputs.

**Features:**
- Image understanding (vision models like LLaVA, CogVLM)
- Audio transcription and understanding (Whisper integration)
- Video frame analysis
- Multi-modal embeddings (CLIP, ImageBind)

**Syntax:**
```aql
-- Image understanding
LLM INFER 'Describe this image'
  IMAGE FROM 'images/photo.jpg'
  MODEL 'llava-v1.6-34b'

-- Audio transcription + summarization
LLM INFER 'Summarize this audio recording'
  AUDIO FROM 'recordings/meeting.mp3'
  MODEL 'whisper-large-v3'
  THEN MODEL 'llama-3-70b'

-- Multi-modal RAG
LLM RAG 'Find similar products to this image'
  SEARCH IN products
  IMAGE @queryImage
  TOP 10
  MODEL 'clip-vit-large'
```

---

### Advanced RAG Techniques
**Priority:** High  
**Target Version:** v1.6.0

Implement state-of-the-art RAG enhancements.

**Techniques:**
- **HyDE (Hypothetical Document Embeddings)**: Generate hypothetical answers, search with their embeddings
- **Multi-Query RAG**: Generate multiple search queries for comprehensive coverage
- **Re-ranking**: Two-stage retrieval with cross-encoder re-ranking
- **Parent Document Retrieval**: Retrieve chunks, return full parent documents
- **RAG Fusion**: Combine results from multiple retrieval strategies

**Syntax:**
```aql
-- HyDE RAG
LLM RAG 'What are quantum computing applications?'
  SEARCH IN knowledge_base
  STRATEGY 'hyde'  -- Generate hypothetical answer first
  TOP 10
  RERANK true      -- Apply cross-encoder re-ranking

-- Multi-query RAG
LLM RAG 'Explain neural networks'
  SEARCH IN documentation
  MULTI_QUERY 3    -- Generate 3 different search queries
  TOP 5 PER_QUERY
  FUSION 'rrf'     -- Reciprocal Rank Fusion
```

---

### Fine-Tuning Pipeline Integration
**Priority:** Medium  
**Target Version:** v1.7.0

In-database model fine-tuning with LoRA.

**Features:**
- Supervised fine-tuning (SFT) on custom datasets
- Direct Preference Optimization (DPO)
- Reward modeling
- Automatic LoRA adapter generation

**Syntax:**
```aql
-- Create training dataset
FOR doc IN training_data
  FILTER doc.category == 'medical'
  RETURN {
    instruction: doc.question,
    response: doc.answer
  }
  INTO medical_training_set

-- Fine-tune model
LLM FINETUNE
  BASE_MODEL 'llama-3-8b'
  DATASET medical_training_set
  EPOCHS 3
  LEARNING_RATE 1e-4
  LORA_RANK 16
  OUTPUT 'medical-llama-3-8b'

-- Use fine-tuned model
LLM INFER 'Explain hypertension treatment'
  MODEL 'llama-3-8b'
  LORA 'medical-llama-3-8b'
```

---

### Agent Framework Integration
**Priority:** Medium  
**Target Version:** v1.8.0

Multi-step reasoning with tool calling and planning.

**Features:**
- ReAct (Reasoning + Acting) pattern
- Tool/function calling
- Multi-agent collaboration
- Planning and reflection

**Syntax:**
```aql
-- Define agent with tools
LLM AGENT CREATE data_analyst
  MODEL 'llama-3-70b-instruct'
  TOOLS [
    {name: 'query_database', aql: 'FOR doc IN @collection FILTER @condition RETURN doc'},
    {name: 'calculate', fn: 'MATH.eval'},
    {name: 'visualize', fn: 'CHART.create'}
  ]
  MAX_ITERATIONS 10

-- Execute agent task
LLM AGENT EXECUTE data_analyst
  TASK 'Analyze sales trends for Q4 2024 and create a visualization'
  CONTEXT {quarter: 'Q4', year: 2024}
```

---

### Natural Language Schema Generation
**Priority:** Low  
**Target Version:** v1.8.0

Generate database schemas from natural language descriptions.

**Features:**
- Table/collection schema generation
- Index recommendations
- Relationship extraction
- Sample data generation

**Syntax:**
```aql
LLM GENERATE SCHEMA
  DESCRIPTION 'An e-commerce system with users, products, orders, and reviews'
  INCLUDE_INDEXES true
  INCLUDE_SAMPLE_DATA 100
  RETURN AS JSON
```

---

### Query Optimization AI
**Priority:** Medium  
**Target Version:** v1.7.0

LLM-powered query optimization suggestions.

**Features:**
- Analyze slow queries
- Suggest index creation
- Recommend query rewriting
- Predict query performance

**Syntax:**
```aql
LLM OPTIMIZE QUERY
  QUERY @slowQuery
  ANALYZE_PLAN true
  SUGGEST_INDEXES true
  RETURN ALTERNATIVES 3
```

---

## Performance Optimizations

### Continuous Batching (Iteration-Level Batching)
**Priority:** High  
**Target Version:** v1.6.0

Process multiple inference requests in the same batch for higher throughput.

**Current:** Sequential processing of requests  
**Target:** Continuous batching with dynamic sequence insertion/removal

**Expected Improvement:** 2-5x throughput increase

---

### Speculative Decoding
**Priority:** High  
**Target Version:** v1.6.0

Use small draft model + large target model for faster inference.

**Approach:**
- Small model (1-3B) generates tokens quickly
- Large model (70B+) validates/corrects in parallel
- Accept speculative tokens when correct

**Expected Improvement:** 2-3x latency reduction

---

### KV Cache Optimization
**Priority:** Medium  
**Target Version:** v1.7.0

Optimize key-value cache memory usage and reuse.

**Techniques:**
- PagedAttention for efficient memory allocation
- KV cache compression (quantization)
- Prefix caching for common prompts
- Multi-request KV cache sharing

**Expected Improvement:** 50% memory reduction, 30% faster repeated queries

---

### Embedding Cache
**Priority:** High  
**Target Version:** v1.6.0

Cache embeddings to avoid recomputation.

**Features:**
- Document content hash → embedding mapping
- LRU eviction with configurable size
- Automatic cache warming for frequently accessed documents
- Distributed cache for multi-node deployments

**Expected Improvement:** 10-100x faster for cached embeddings

---

### Model Sharding Across GPUs
**Priority:** Medium  
**Target Version:** v1.7.0

Distribute large models across multiple GPUs.

**Techniques:**
- Tensor parallelism for within-layer distribution
- Pipeline parallelism for across-layer distribution
- Automatic sharding based on available GPUs

**Expected Improvement:** Support for 70B+ models on consumer GPUs

---

## Refactoring Opportunities

### Unified LLM Backend Interface
**Priority:** High  
**Target Version:** v1.6.0

Abstract LLM backend for multiple inference engines.

**Current:**
```cpp
// Tightly coupled to llama.cpp
LlamaCppBackend backend;
```

**Proposed:**
```cpp
// Abstract interface
class ILLMBackend {
public:
    virtual Result<std::string> infer(const InferenceRequest& req) = 0;
    virtual Result<std::vector<float>> embed(const std::string& text) = 0;
};

// Multiple implementations
class LlamaCppBackend : public ILLMBackend { /* ... */ };
class VLLMBackend : public ILLMBackend { /* ... */ };
class OllamaBackend : public ILLMBackend { /* ... */ };
class OpenAIBackend : public ILLMBackend { /* ... */ };
```

**Benefits:**
- Support for VLLM, Ollama, OpenAI API, etc.
- Easier testing with mock backends
- Runtime backend selection

---

### Streaming Response API
**Priority:** Medium  
**Target Version:** v1.7.0

Stream tokens as they're generated instead of waiting for completion.

**Current API:**
```cpp
Result<std::string> infer(const std::string& prompt);
// Returns only after full completion
```

**Proposed API:**
```cpp
Result<TokenStream> inferStreaming(const std::string& prompt);
// Returns immediately with iterator

for (auto token : stream) {
    std::cout << token;  // Print as generated
}
```

**Benefits:**
- Better user experience (progressive output)
- Lower time-to-first-token
- Cancel long-running inferences

---

### Prompt Template Engine
**Priority:** Medium  
**Target Version:** v1.7.0

Structured prompt templates with variable substitution.

**Proposed:**
```cpp
// Define template
PromptTemplate rag_template(R"(
Context:
{{#each documents}}
- {{this.title}}: {{this.content}}
{{/each}}

Question: {{question}}

Answer based on the context above:
)");

// Use template
auto prompt = rag_template.render({
    {"documents", retrieved_docs},
    {"question", user_question}
});
```

---

### Function Calling Framework
**Priority:** High  
**Target Version:** v1.7.0

Structured function calling for tool use.

**Proposed:**
```cpp
// Define functions
FunctionRegistry registry;
registry.register("query_database", [](const json& args) {
    return executeAQL(args["query"].get<std::string>());
});

// LLM with function calling
LLMConfig config;
config.functions = registry.getFunctions();
config.function_calling_enabled = true;

auto result = llm.infer(prompt, config);
if (result.function_call) {
    auto output = registry.call(result.function_call);
    // Continue conversation with function output
}
```

---

## Known Issues

### Issue #1: Long Context Handling
**Severity:** Medium  
**Reported:** v1.5.0

Models with limited context windows (2K-8K tokens) struggle with long documents.

**Workaround:** Chunk documents and retrieve top-k chunks

**Fix:** Implement:
- Sliding window attention
- Sparse attention mechanisms
- Long-context models (Llama-3.1 with 128K context)

**Planned Fix:** v1.6.0

---

### Issue #2: RAG Accuracy Issues
**Severity:** Medium  
**Reported:** v1.5.1

Retrieved documents sometimes don't contain relevant information.

**Workaround:** Increase retrieval count, improve chunking strategy

**Fix:**
- Hybrid search (vector + BM25)
- Re-ranking with cross-encoder
- Query expansion/rewriting

**Planned Fix:** v1.6.0

---

### Issue #3: Model Loading Performance
**Severity:** Low  
**Reported:** v1.5.0

Large models take 10-30 seconds to load.

**Workaround:** Pre-load models at server startup

**Fix:**
- Memory-mapped model loading (mmap)
- Model caching in shared memory
- Lazy weight loading

**Planned Fix:** v1.6.0

---

### Issue #4: Embedding Dimension Mismatch
**Severity:** Medium  
**Reported:** v1.5.2

Switching embedding models breaks existing indexes due to dimension changes.

**Workaround:** Re-index all documents with new model

**Fix:**
- Dimension adapters (PCA, autoencoder)
- Multi-model index support
- Migration tools

**Planned Fix:** v1.7.0

---

## Research Areas

### Mixture of Experts (MoE) Optimization
**Focus:** Efficient sparse MoE inference

Optimize inference for MoE models (Mixtral, Grok):
- Expert caching
- Expert load balancing
- GPU-efficient expert routing

**Research Questions:**
- How to optimize expert selection for latency?
- Can we predict which experts will be needed?
- What's the optimal expert count for different tasks?

---

### Quantization-Aware RAG
**Focus:** Balancing quality vs performance

Explore quantization impact on RAG:
- 4-bit vs 8-bit quantization for embeddings
- Mixed precision: FP16 retrieval, INT8 generation
- Adaptive quantization based on query complexity

**Research Questions:**
- What's the quality/speed trade-off?
- Can we use higher precision for critical queries?
- How to detect when quantization degrades results?

---

### Personalized RAG
**Focus:** User-specific context and preferences

Adapt RAG to individual users:
- User embedding profiles
- Personalized retrieval ranking
- Context-aware response generation

**Research Questions:**
- How to balance personalization with privacy?
- Can we learn user preferences from interactions?
- What's the cold-start problem solution?

---

### Cross-Lingual RAG
**Focus:** Multi-language query and retrieval

Support queries and documents in multiple languages:
- Multilingual embeddings (mBERT, XLM-R)
- Translation-based RAG
- Language-aware re-ranking

**Research Questions:**
- Which multilingual model performs best?
- Should we translate queries or documents?
- How to handle language-specific nuances?

---

## Migration Paths

### v1.5.x → v1.6.x: Unified LLM Backend
**Breaking Changes:** Backend interface changes

**Old API:**
```cpp
LlamaCppBackend backend(config);
auto result = backend.infer(prompt);
```

**New API:**
```cpp
auto backend = LLMBackendFactory::create("llamacpp", config);
auto result = backend->infer(prompt);
```

**Migration Steps:**
1. Update to v1.6.0
2. Replace direct LlamaCppBackend with factory
3. Test with existing models

**Timeline:** 6 months deprecation period

---

### v1.6.x → v1.7.x: Streaming API
**Breaking Changes:** Inference API signature change

**Old API:**
```cpp
Result<std::string> infer(const std::string& prompt);
```

**New API (backward compatible):**
```cpp
Result<std::string> infer(const std::string& prompt);  // Still works
Result<TokenStream> inferStreaming(const std::string& prompt);  // New
```

**Migration Steps:**
1. Update to v1.7.0
2. Optionally adopt streaming API for better UX
3. No breaking changes (additive only)

**Timeline:** N/A (backward compatible)

---

### v1.7.x → v1.8.x: Agent Framework
**Breaking Changes:** None (new features)

**New Features:**
```aql
LLM AGENT CREATE ...
LLM AGENT EXECUTE ...
```

**Migration Steps:**
1. Update to v1.8.0
2. Explore agent capabilities
3. No code changes required

**Timeline:** N/A (additive)

---

## Community Contributions Welcome

We welcome contributions in the following areas:

### High-Impact, Beginner-Friendly
- [ ] Additional prompt templates for common use cases
- [ ] Documentation improvements and examples
- [ ] Embedding model benchmarks
- [ ] RAG quality evaluation metrics

### Medium Complexity
- [ ] Additional LLM backend implementations (VLLM, Ollama, OpenAI)
- [ ] Streaming response API
- [ ] HyDE RAG implementation
- [ ] Cross-encoder re-ranking

### Advanced Topics
- [ ] Fine-tuning pipeline integration
- [ ] Agent framework with tool calling
- [ ] Speculative decoding
- [ ] Multi-modal LLM support
- [ ] Distributed model sharding

**Contribution Guide:** See [CONTRIBUTING.md](../../CONTRIBUTING.md)

---

## Feedback and Discussion

Have ideas for AQL and LLM improvements? We'd love to hear from you:

- 💡 Feature requests: [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- 💬 Design discussions: [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
- 🐛 Bug reports: [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- 📊 Benchmark results: Share in discussions

---

*Last Updated: February 2026*  
*Module Version: v1.5.x*  
*Next Review: v1.6.0 Release*
