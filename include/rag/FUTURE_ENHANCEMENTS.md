# RAG Module - Future Enhancements

## Scope

- API-level enhancements to `include/rag/` headers — public C++ interfaces for retrieval-augmented generation
- Streaming retrieval interface: `ResultStream<Document>` base class with `next()`, `hasMore()`, and `cancel()` methods
- Multi-judge evaluation API: `JudgeEnsemble` with async `evaluate()` returning `EvaluationFuture`
- Re-ranking hook: `RerankerHook` — optional, stateless callback registered on `RAGPipeline`
- Citation tracking API: `CitationTracker` returning structured `CitationRef` with doc ID, span, and confidence
- Agentic RAG interface: `AgentRAGPolicy` for multi-hop decomposition and tool invocation hooks

## Design Constraints

- [x] Streaming retrieval uses `ResultStream<Document>` as the base interface; polling and callback variants are both derived from it
- [x] Evaluation API is async; `JudgeEnsemble::evaluate()` returns `EvaluationFuture` and must not block the caller
- [x] Re-ranking hook is optional and stateless; it receives a `const` candidate list and returns a re-ordered copy
- [x] Citation API returns structured `CitationRef` objects; raw document content is never included in citation data
- [x] Agentic RAG interface must not hold mutable shared state between hops; each hop receives explicit context by value
- [x] All public RAG interfaces are `noexcept`-safe at the boundary; exceptions are caught and converted to `Result<T>`

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `ResultStream<Document>` | Streaming query path, UI layer | Base class; `next()` is blocking with timeout |
| `JudgeEnsemble` | Evaluation pipeline, CI quality gates | Async `evaluate()`; returns `EvaluationFuture` |
| `RerankerHook` | `RAGPipeline` post-retrieval stage | Stateless; registered via `pipeline.setReranker()` |
| `CitationTracker` | Answer generation, explainability layer | Returns `vector<CitationRef>`; structured, no raw text |
| `AgentRAGPolicy` | Multi-hop reasoner, agentic pipelines | Stateless per-hop; receives `HopContext` by value |

## Planned Features

### 1. Multi-Hop Reasoning with Query Decomposition
**Priority:** High
**Target Version:** v1.16.0

Enable complex queries requiring multiple retrieval and reasoning steps.

**Features:**
- Automatic query decomposition into sub-questions
- Iterative retrieval based on intermediate answers
- Dependency graph construction for sub-queries
- Answer composition from multiple hops
- Support for "follow-up question" generation

**Example Multi-Hop Query:**
```
User: "Who won the Nobel Prize in Physics the year the first iPhone was released?"

Decomposition:
  1. "When was the first iPhone released?" → 2007
  2. "Who won the Nobel Prize in Physics in 2007?" → Albert Fert and Peter Grünberg

Final Answer: "Albert Fert and Peter Grünberg won the Nobel Prize in Physics
in 2007, the year the first iPhone was released."
```

**API:**
```cpp
#include "rag/multi_hop_reasoner.h"

MultiHopConfig config;
config.max_hops = 5;
config.beam_width = 3;  // Explore 3 reasoning paths
config.early_stopping = true;

MultiHopReasoner reasoner(config);

auto result = reasoner.reason(
    "Who won the Nobel Prize in Physics the year the first iPhone was released?"
);

// Inspect reasoning trace
for (const auto& hop : result.reasoning_trace) {
    std::cout << "Hop " << hop.step << ": " << hop.query << "\n";
    std::cout << "  Answer: " << hop.answer << "\n";
    std::cout << "  Sources: ";
    for (const auto& doc : hop.documents) {
        std::cout << doc.id << " ";
    }
    std::cout << "\n\n";
}

std::cout << "Final Answer: " << result.final_answer << "\n";
```

**Architecture:**
```
User Query
    ↓
Query Decomposer
    ↓
[Sub-Query 1] → Retrieve → Answer 1
                              ↓
[Sub-Query 2 (uses Answer 1)] → Retrieve → Answer 2
                                              ↓
[Sub-Query 3 (uses Answer 1, 2)] → Retrieve → Answer 3
                                              ↓
Answer Composer
    ↓
Final Answer
```

**Implementation Strategy:**
- Use LLM to decompose query into sub-questions
- Track dependency graph between sub-questions
- Execute retrieval in topological order
- Inject previous answers into subsequent queries
- Compose final answer from all intermediate results

**Challenges:**
- Error propagation across hops
- Detecting when to stop decomposition
- Handling conflicting intermediate answers
- Compute cost grows with hop count

---

### 2. Hybrid Search (Vector + Keyword + Filter)
**Priority:** High
**Target Version:** v1.16.0

Combine vector similarity, BM25 keyword matching, and structured filters for optimal retrieval.

**Features:**
- Weighted fusion of vector and keyword scores
- Reciprocal Rank Fusion (RRF) for result merging
- Structured attribute filters (date, category, etc.)
- Query-adaptive weighting (more vector for semantic, more keyword for factual)
- Explain mode showing contribution of each retrieval method

**Search Strategies:**
1. **Sparse Retrieval**: BM25, TF-IDF (good for exact matches, rare terms)
2. **Dense Retrieval**: Vector similarity (good for semantic matching)
3. **Structured Filters**: SQL-like predicates (good for categorical/temporal constraints)

**API:**
```cpp
#include "rag/hybrid_search.h"

HybridSearchConfig config;
config.vector_weight = 0.6;    // 60% vector similarity
config.keyword_weight = 0.3;   // 30% BM25
config.filter_weight = 0.1;    // 10% filter match
config.fusion_method = FusionMethod::RRF;  // Reciprocal Rank Fusion

HybridSearchEngine search_engine(vector_index, keyword_index, config);

// Search with all modalities
HybridQuery query;
query.text = "machine learning for healthcare";
query.vector = embeddings.encode(query.text);
query.filters = {
    {"category", "==", "research_paper"},
    {"year", ">=", "2020"},
    {"citations", ">", "100"}
};
query.top_k = 20;

auto results = search_engine.search(query);

// Inspect score breakdown
for (const auto& result : results) {
    std::cout << "Document: " << result.doc_id << "\n";
    std::cout << "  Combined Score: " << result.score << "\n";
    std::cout << "  Vector Score: " << result.vector_score << "\n";
    std::cout << "  Keyword Score: " << result.keyword_score << "\n";
    std::cout << "  Filter Match: " << result.filter_match << "\n\n";
}
```

**Fusion Methods:**

1. **Weighted Sum:**
   ```
   final_score = w_v * vector_score + w_k * keyword_score + w_f * filter_score
   ```

2. **Reciprocal Rank Fusion (RRF):**
   ```
   RRF_score = Σ (1 / (k + rank_i))  for each retrieval method i
   ```
   - More robust to score scale differences
   - Used by search engines (Elasticsearch, etc.)

3. **Learn-to-Rank:**
   ```
   Train ML model: features → relevance score
   Features: [vector_score, keyword_score, filter_match, doc_length, recency, ...]
   ```

**Query-Adaptive Weighting:**
```cpp
// Automatically adjust weights based on query characteristics
config.enable_adaptive_weighting = true;

// Factual query → more keyword weight
// "Who won the 2024 Nobel Prize?"
// vector_weight: 0.3, keyword_weight: 0.7

// Semantic query → more vector weight
// "What are the implications of AI on society?"
// vector_weight: 0.8, keyword_weight: 0.2
```

---

### 3. Re-Ranking with Cross-Encoder Models
**Priority:** High
**Target Version:** v1.16.0

Two-stage retrieval: fast first-pass retrieval followed by precise re-ranking.

**Motivation:**
- Bi-encoder (vector search): Fast but less accurate for relevance
- Cross-encoder: Slow but highly accurate for relevance
- Solution: Use bi-encoder for initial retrieval (Top-100), then cross-encoder to re-rank (Top-10)

**Architecture:**
```
Query
  ↓
Bi-Encoder Retrieval (fast)
  ↓
Top-100 candidates
  ↓
Cross-Encoder Re-Ranking (accurate)
  ↓
Top-10 re-ranked results
  ↓
RAG Generation
```

**API:**
```cpp
#include "rag/reranker.h"

// Load cross-encoder model
CrossEncoderConfig rerank_config;
rerank_config.model_path = "models/cross-encoder-ms-marco";
rerank_config.batch_size = 32;
rerank_config.max_length = 512;

CrossEncoderReranker reranker(rerank_config);

// Initial retrieval (Top-100)
auto candidates = vector_index->search(query, 100);

// Re-rank to Top-10
auto reranked = reranker.rerank(query, candidates, 10);

// Use re-ranked results for generation
auto answer = rag_pipeline.generate(query, reranked);
```

**Expected Performance:**
- **Retrieval Recall@100**: ~95% (bi-encoder)
- **Re-ranking Precision@10**: ~85% (cross-encoder)
- **Latency**: +50-100ms for re-ranking
- **Quality Improvement**: +10-15% answer accuracy

**Cross-Encoder Models:**
- `ms-marco-MiniLM-L6`: Fast, decent accuracy
- `ms-marco-roberta-base`: Balanced
- `ms-marco-electra-large`: Best accuracy, slower

**Optimization:**
- Batch re-ranking for efficiency
- Cache re-ranking scores (query-doc pairs)
- Skip re-ranking if initial scores are very confident

---

### 4. Contextual Compression and Summarization
**Priority:** Medium
**Target Version:** v1.17.0

Compress retrieved documents to fit more context into LLM's limited context window.

**Problem:**
- LLMs have context limits (4K, 8K, 32K tokens)
- Retrieved documents often contain irrelevant information
- Want to maximize relevant information density

**Solutions:**

1. **Extractive Compression**: Extract only relevant sentences/passages
2. **Abstractive Compression**: Summarize documents while preserving key info
3. **Hybrid Compression**: Extract then summarize

**API:**
```cpp
#include "rag/contextual_compressor.h"

ContextualCompressorConfig config;
config.compression_method = CompressionMethod::EXTRACTIVE;
config.target_compression_ratio = 0.3;  // Compress to 30% of original size
config.preserve_citations = true;       // Keep source attribution

ContextualCompressor compressor(config);

// Retrieve large documents
auto documents = vector_index->search(query, 20);

// Compress while preserving relevance
auto compressed = compressor.compress(query, documents);

// Compressed documents fit in context window
auto answer = llm->generate(query, compressed);
```

**Extractive Compression:**
```cpp
// Sentence-level relevance scoring
for (auto& doc : documents) {
    auto sentences = split_sentences(doc.content);

    // Score each sentence's relevance to query
    std::vector<std::pair<double, std::string>> scored_sentences;
    for (const auto& sent : sentences) {
        double score = compute_relevance(query, sent);
        scored_sentences.push_back({score, sent});
    }

    // Keep top-k most relevant sentences
    sort(scored_sentences.rbegin(), scored_sentences.rend());
    doc.content = join(scored_sentences.begin(),
                       scored_sentences.begin() + top_k_sentences);
}
```

**Abstractive Compression:**
```cpp
// Use LLM to summarize while preserving query-relevant information
std::string prompt = format(
    "Summarize the following document, focusing on information "
    "relevant to the query: '{}'\n\nDocument:\n{}\n\nSummary:",
    query, document.content
);

document.content = llm->generate(prompt);
```

**Lost-in-the-Middle Problem:**
- LLMs pay more attention to start/end of context
- Solution: Re-order documents with most relevant in first/last positions

**Expected Results:**
- 3-5x reduction in token count
- Minimal loss in answer quality (<5%)
- Enables using more diverse sources

---

### 5. Active Learning for Evaluation Calibration
**Priority:** Medium
**Target Version:** v1.17.0

Continuously improve judge calibration using human feedback.

**Features:**
- Collect human judgments on sample evaluations
- Retrain/fine-tune judge models with human feedback
- Active learning: Select most informative samples for human review
- Track calibration improvement over time

**API:**
```cpp
#include "rag/active_calibration.h"

ActiveCalibrationManager calibration;

// Judge evaluates an answer
auto eval_result = judge->evaluate(query, docs, answer);

// Send to human reviewers (async)
calibration.requestHumanFeedback(
    query, docs, answer, eval_result,
    [&](const HumanFeedback& feedback) {
        // Human provides corrected scores
        calibration.addTrainingExample({
            eval_result,
            feedback.human_scores,
            feedback.explanation
        });

        // Periodically retrain
        if (calibration.getTrainingSetSize() >= 100) {
            calibration.recalibrateJudge(judge);
        }
    }
);
```

**Active Learning Strategies:**
1. **Uncertainty Sampling**: Request feedback on evaluations with low confidence
2. **Disagreement Sampling**: Request feedback when ensemble judges disagree
3. **Representative Sampling**: Request feedback on diverse query types

**Calibration Techniques:**
1. **Temperature Scaling**: Adjust confidence scores with learned temperature
2. **Platt Scaling**: Logistic regression on judge scores
3. **Fine-tuning**: Update judge LLM with human feedback (RLHF-style)

**Expected Improvement:**
- Calibration error (ECE) reduction: 20-30%
- Human correlation increase: +5-10%
- Confidence accuracy: +15-20%

---

### 6. Explainable RAG with Attribution
**Priority:** Medium
**Target Version:** v1.17.0

Provide detailed explanations for how answers were derived from sources.

**Features:**
- Sentence-level source attribution (which sentence from which document)
- Claim-to-source traceability
- Confidence per claim
- Counterfactual explanations ("would answer change without doc X?")
- Attention visualization for neural models

**API:**
```cpp
#include "rag/explainable_rag.h"

ExplainableRAGConfig config;
config.enable_claim_attribution = true;
config.enable_counterfactual = true;
config.enable_attention_maps = true;

ExplainableRAG explainer(config);

auto explanation = explainer.explain(query, documents, answer);

// Claim-level attribution
for (const auto& claim : explanation.claims) {
    std::cout << "Claim: " << claim.text << "\n";
    std::cout << "Confidence: " << claim.confidence << "\n";
    std::cout << "Sources:\n";

    for (const auto& source : claim.sources) {
        std::cout << "  - Document: " << source.doc_id << "\n";
        std::cout << "    Sentence: " << source.sentence << "\n";
        std::cout << "    Support Score: " << source.support_score << "\n";
    }
    std::cout << "\n";
}

// Counterfactual analysis
std::cout << "Counterfactual Analysis:\n";
for (const auto& cf : explanation.counterfactuals) {
    std::cout << "Without document " << cf.removed_doc_id << ":\n";
    std::cout << "  Answer would change: " << (cf.answer_changed ? "Yes" : "No") << "\n";
    std::cout << "  Impact score: " << cf.impact_score << "\n";
}
```

**Claim Attribution Methods:**

1. **Entailment-based**: Check if document entails claim
   ```cpp
   double entailment_score = nli_model->score(document_sentence, claim);
   if (entailment_score > threshold) {
       // Sentence supports claim
   }
   ```

2. **Similarity-based**: Semantic similarity between claim and source
   ```cpp
   double sim = cosine_similarity(
       embed(claim),
       embed(document_sentence)
   );
   ```

3. **Generation-based**: Ask LLM which sources support claim
   ```cpp
   std::string prompt = format(
       "Which of the following sources support this claim?\n"
       "Claim: {}\n"
       "Sources: {}\n"
       "Answer:",
       claim, format_sources(documents)
   );
   ```

**Counterfactual Explanations:**
```cpp
// Baseline answer with all documents
auto baseline_answer = generate(query, all_documents);

// Test removing each document
for (size_t i = 0; i < all_documents.size(); i++) {
    auto docs_without_i = all_documents;
    docs_without_i.erase(docs_without_i.begin() + i);

    auto cf_answer = generate(query, docs_without_i);

    double impact = compute_difference(baseline_answer, cf_answer);

    if (impact > threshold) {
        // Document i is critical for answer
        std::cout << "Document " << i << " is critical (impact: "
                  << impact << ")\n";
    }
}
```

---

### 7. RAG for Multi-Modal Data (Text, Images, Tables)
**Priority:** Medium
**Target Version:** v1.18.0

Extend RAG to handle diverse data modalities beyond text.

**Features:**
- Image retrieval and captioning
- Table parsing and question answering
- Chart/graph understanding
- Multi-modal fusion (text + image + table)

**API:**
```cpp
#include "rag/multimodal_rag.h"

MultiModalRAGConfig config;
config.enable_image_retrieval = true;
config.enable_table_qa = true;
config.enable_ocr = true;

MultiModalRAG mm_rag(config);

MultiModalQuery query;
query.text = "What were the quarterly revenue trends in 2023?";
query.modalities = {Modality::TEXT, Modality::TABLE, Modality::IMAGE};

auto result = mm_rag.query(query);

// Result includes mixed modality sources
for (const auto& source : result.sources) {
    if (source.modality == Modality::TEXT) {
        std::cout << "Text source: " << source.content << "\n";
    } else if (source.modality == Modality::TABLE) {
        std::cout << "Table:\n" << format_table(source.table_data) << "\n";
    } else if (source.modality == Modality::IMAGE) {
        std::cout << "Image: " << source.image_path << "\n";
        std::cout << "Caption: " << source.caption << "\n";
    }
}

std::cout << "Answer: " << result.answer << "\n";
```

**Multi-Modal Retrieval:**

1. **Image Retrieval:**
   - Use CLIP embeddings for text-image similarity
   - Retrieve relevant images/charts/diagrams
   - Generate captions with vision-language models

2. **Table Retrieval:**
   - Parse tables into structured format
   - Create text + structure embeddings
   - Retrieve relevant tables
   - Execute queries over tables (SQL-like)

3. **Document Retrieval:**
   - Extract text, images, tables from PDFs
   - Multi-modal chunking strategies
   - Preserve layout information

**Multi-Modal Generation:**
```cpp
// LLM receives mixed context
std::string context = format(
    "Text passages:\n{}\n\n"
    "Tables:\n{}\n\n"
    "Image captions:\n{}\n\n"
    "Question: {}\nAnswer:",
    text_passages, table_summaries, image_captions, query
);

auto answer = llm->generate(context);
```

---

### 8. Real-Time Fact-Checking with External APIs
**Priority:** Low
**Target Version:** v1.18.0

Integrate external fact-checking APIs for up-to-date verification.

**Features:**
- Google Fact Check Tools API integration
- ClaimBuster API integration
- Live web search for claim verification
- Temporal fact checking (facts change over time)

**API:**
```cpp
#include "rag/fact_checker.h"

FactCheckerConfig config;
config.enable_google_factcheck = true;
config.enable_web_search = true;
config.web_search_timeout_ms = 2000;

FactChecker fact_checker(config);

// Extract claims from answer
auto claims = claim_extractor->extract(answer);

// Verify each claim
for (auto& claim : claims) {
    auto verification = fact_checker.verify(claim);

    if (verification.status == VerificationStatus::FALSE) {
        std::cout << "FALSE claim detected: " << claim << "\n";
        std::cout << "Evidence: " << verification.evidence << "\n";
    } else if (verification.status == VerificationStatus::UNVERIFIED) {
        std::cout << "UNVERIFIED claim: " << claim << "\n";
    }
}
```

**Temporal Fact Checking:**
```cpp
// Facts can change over time
auto claim = "The President of France is Emmanuel Macron";

// Check if claim is current
auto verification = fact_checker.verify(claim, {
    .temporal_check = true,
    .reference_date = std::chrono::system_clock::now()
});

if (verification.is_outdated) {
    std::cout << "Claim is outdated. Current fact: "
              << verification.current_fact << "\n";
}
```

---

### 9. Adversarial Testing and Red-Teaming
**Priority:** Low
**Target Version:** v1.18.0

Systematically test RAG robustness against adversarial inputs.

**Features:**
- Automatic adversarial query generation
- Document poisoning detection
- Prompt injection detection
- Jailbreak attempt detection
- Red-team simulation

**API:**
```cpp
#include "rag/adversarial_tester.h"

AdversarialTester tester;

// Generate adversarial queries
auto adv_queries = tester.generateAdversarialQueries(
    original_query,
    AdversarialStrategy::SEMANTIC_PERTURBATION
);

// Test RAG robustness
for (const auto& adv_query : adv_queries) {
    auto result = rag.query(adv_query);

    if (tester.isSuccessfulAttack(original_answer, result.answer)) {
        std::cout << "Vulnerability found!\n";
        std::cout << "Adversarial query: " << adv_query << "\n";
    }
}

// Document poisoning test
auto poisoned_docs = tester.poisonDocuments(documents);
auto result_poisoned = rag.query(query, poisoned_docs);

if (result_poisoned.faithfulness_score < threshold) {
    std::cout << "RAG susceptible to document poisoning\n";
}
```

**Attack Types:**
1. **Query Perturbation**: Slightly modify query to change answer
2. **Document Poisoning**: Inject misleading information
3. **Prompt Injection**: Try to override system instructions
4. **Context Overflow**: Exceed context limits to drop important info
5. **Sycophancy**: Questions designed to elicit agreeable but wrong answers

---

### 10. Federated RAG Across Distributed Databases
**Priority:** High
**Target Version:** v1.19.0

Query multiple distributed databases in a federated RAG system.

**Features:**
- Query routing to relevant databases
- Parallel retrieval from multiple sources
- Result merging and de-duplication
- Distributed caching
- Fault tolerance and fallback

**Architecture:**
```
                User Query
                    ↓
            Query Router
                    ↓
        ┌───────────┼───────────┐
        │           │           │
   Database 1  Database 2  Database 3
   (Medical)   (Legal)     (Technical)
        │           │           │
        └───────────┼───────────┘
                    ↓
            Result Merger
                    ↓
          De-duplication
                    ↓
             RAG Generation
```

**API:**
```cpp
#include "rag/federated_rag.h"

FederatedRAGConfig config;
config.databases = {
    {"medical_db", "localhost:5001"},
    {"legal_db", "localhost:5002"},
    {"technical_db", "localhost:5003"}
};
config.routing_strategy = RoutingStrategy::QUERY_BASED;
config.parallel_retrieval = true;
config.timeout_ms = 5000;

FederatedRAG fed_rag(config);

// Automatic routing to relevant databases
auto result = fed_rag.query("What are HIPAA compliance requirements?");

// Result includes sources from multiple databases
for (const auto& source : result.sources) {
    std::cout << "From database: " << source.database_name << "\n";
    std::cout << "Content: " << source.content << "\n\n";
}
```

**Query Routing Strategies:**

1. **Query-Based Routing**: Use query content to select databases
   ```cpp
   // "cancer treatment" → medical_db
   // "copyright law" → legal_db
   auto relevant_dbs = router.route(query);
   ```

2. **Metadata-Based Routing**: Use metadata to select databases
   ```cpp
   query.metadata["domain"] = "healthcare";
   // Only query medical databases
   ```

3. **Broadcast Routing**: Query all databases, merge results
   ```cpp
   config.routing_strategy = RoutingStrategy::BROADCAST;
   // Slower but comprehensive
   ```

**Result Merging:**
```cpp
// De-duplicate results from multiple sources
auto merged = ResultMerger::merge(results_from_dbs, {
    .dedup_threshold = 0.95,  // Cosine similarity
    .max_results = 20,
    .diversity_penalty = 0.5  // Prefer diverse sources
});
```

---

## Research Directions

### Lifelong Learning RAG
RAG systems that continuously learn and update their knowledge base from user interactions.

### Self-Improving RAG
RAG systems that identify their own knowledge gaps and proactively seek to fill them.

### Causal RAG
RAG systems that understand causal relationships, not just correlations, for better reasoning.

### Privacy-Preserving RAG
RAG over encrypted documents with homomorphic encryption or secure multi-party computation.

### Efficient RAG for Edge Devices
Compress models and optimize retrieval for deployment on resource-constrained devices.

### RAG for Code Generation
Retrieve relevant code snippets and documentation to assist in code generation.

---

## Performance Targets (v2.0)

| Metric | Current (v1.15) | Target (v2.0) |
|--------|-----------------|---------------|
| End-to-End Latency | 500-2000ms | 200-800ms |
| Answer Accuracy (EM) | 75-80% | 85-90% |
| Faithfulness Score | 0.85 | 0.95 |
| Context Utilization | 60-70% | 85-95% |
| Cache Hit Rate | 40-50% | 70-80% |

## Benchmarking Suite

### Planned Benchmarks
- **MS MARCO**: Information retrieval
- **Natural Questions**: Open-domain QA
- **HotpotQA**: Multi-hop reasoning
- **FEVER**: Fact verification
- **TriviaQA**: Trivia questions
- **SQuAD**: Reading comprehension
- **BoolQ**: Yes/no questions
- **DROP**: Discrete reasoning

### Custom Benchmarks
- **ThemisRAG-Ethics**: Ethical compliance evaluation
- **ThemisRAG-Multi**: Multi-modal RAG benchmark
- **ThemisRAG-Adversarial**: Robustness testing

---

## Community Contributions Welcome

We welcome contributions in:
- New evaluation dimensions
- Prompt engineering improvements
- Retrieval algorithms
- Calibration techniques
- Benchmark implementations
- Documentation and examples

See `../../CONTRIBUTING.md` for details.

---

*Last Updated: 2024*
*RAG Module Roadmap v1.0*

---

## Test Strategy

- Unit tests for `ResultStream<Document>`: assert `next()` returns documents in score order; `cancel()` stops emission
- Unit tests for `JudgeEnsemble`: mock LLM judge; assert `EvaluationFuture` resolves within timeout; verify async non-blocking
- Unit tests for `RerankerHook`: provide known candidate list; assert output order matches cross-encoder scores
- Unit tests for `CitationTracker`: generate answer from 3 source docs; assert each `CitationRef` links to a valid doc ID and span
- Integration tests: run full `RAGPipeline` with streaming retrieval + re-ranking + citation; assert first-result latency ≤ 50 ms
- PII tests: assert `CitationRef` output contains no raw source document content beyond the attributed span text

## Performance Targets

- Streaming retrieval first-result latency ≤ 50 ms from query submission to first `ResultStream::next()` return
- `JudgeEnsemble` batch evaluation throughput ≥ 100 evaluations/s with a single LLM judge
- Re-ranking hook overhead ≤ 100 ms for top-100 candidate re-rank with `ms-marco-MiniLM-L6`
- `CitationTracker::track()` ≤ 10 ms per answer including span extraction
- Agentic RAG hop overhead (decomposition + tool invocation) ≤ 500 ms per hop excluding LLM latency
- End-to-end RAG pipeline latency ≤ 800 ms at p95 for single-hop queries on cached embeddings

## Security / Reliability

- RAG prompts are sanitized before LLM submission; prompt injection patterns are detected and blocked at the API boundary
- Evaluation results and `CitationRef` data must never include source document PII; enforced by a data sanitization layer
- `AgentRAGPolicy` tool invocations are sandboxed; file system and outbound network access require explicit capability grant
- `ResultStream` implementations must handle upstream database failures gracefully and emit `StreamError` rather than throwing
- Re-ranking models are loaded from a verified model registry path; arbitrary model paths are rejected at API entry

---

## Paper 1+2 — Loop Orchestration, Explainability & Federated RAG (Cross-Module Vision)

> Full papers: `docs/en/research/THEMISDB_LORA_RESEARCH_PAPER.md` · `docs/en/research/LLM_OPTIMIZATION_LAYERS_MATRIX.md`
> Master plan: `docs/issues/MASTER_IMPLEMENTATION_PLAN.md`

### Loop 1–4 Explicit Orchestration (IMPL-A2)
- `ContinuousLearningOrchestrator` gains `triggerLoop1…4()` methods with loop-interference cooldown guard
- `RAGIngestionBridge` indexes optimizer-log documents for Loop 4 dataset enrichment
- JSON context serialiser for Loop 1–3 outcome signals (≤ 2 000 tokens per context block)

### Federated RLAIF (IMPL-A3)
- `FEDERATED_ROUND_START` event triggers `ILoRAFederationCoordinator::startRound()`
- `RLAIFTrainer` preference dataset is federated via `CrossShardFeedbackSync` (Layer D)

### ExplainabilityReasonBuilder (IMPL-B9)
- Every autonomous decision (loop trigger, retrieval strategy change, pattern escalation) generates a `CausalChain` in natural language
- `CausalChain` written to `AIDecisionAuditor` for DBA review and GDPR compliance
- GDPR guard: no PII in reasoning chain; field-level masking enforced

### Federated RAG Merger (DK-4)
- `FederatedRAGMerger` (in `distributed_knowledge` module) merges `RetrievedDocument` lists from N shards using Reciprocal Rank Fusion
- `ExplainabilityReasonBuilder` annotates merged result with shard provenance

### Performance Targets
- `ExplainabilityReasonBuilder::build()` ≤ 20 ms p99 (no LLM call; rule-based chain assembly)
- Loop 1 feedback round-trip ≤ 10 ms (in-process)
- Loop 2 adaptation cycle ≤ 60 s end-to-end including HNSW rebalance
