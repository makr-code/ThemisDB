> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ROADMAP.md · ARCHITECTURE.md -->

# Future Enhancements — Ethics AI Module

---

## 1. LLM-Based Argument Content Generation

### Scope
Replace the current template-based `generateArgument()` with a full LLM call that
produces semantically rich, philosophy-grounded argument text.

### Design Constraints
- Maximum 500 tokens per argument to keep RAG context windows manageable.
- Latency budget: ≤ 3 s per argument at p95; LLM timeout triggers template fallback.
- Must pass dilemma text as `user` role, never `system`, to prevent prompt injection.
- Philosophy `main_theses` injected as few-shot examples in the `system` prompt.

### Required Interfaces
- New `IArgumentGenerator` interface injected into `EthicalDiscourseEngine`.
- `LlmArgumentGenerator` implementation wrapping the ThemisDB LLM backend.
- `TemplateArgumentGenerator` (current behaviour) as fallback.

### Implementation Notes
- `EthicalDiscourseEngine::generateArgument()` becomes a virtual dispatch.
- Argument cache keyed on `(philosophy_school, dilemma_hash)` to avoid duplicate LLM calls.

### Test Strategy
- Unit tests with mock `IArgumentGenerator` verifying dispatch logic.
- Integration tests with a real LLM (ollama local model) verifying argument coherence.
- Golden-output comparison for deterministic test LLMs (temperature=0).

### Performance Targets
- Single argument: ≤ 3 s at p95
- Batch of 5 arguments (5 schools): ≤ 8 s via parallel LLM calls

### Security / Reliability
- Dilemma text sanitised before LLM injection (strip control characters, truncate to 1000 chars).
- Template fallback guarantees availability if LLM is unreachable.

---

## 2. Dynamic Confidence and Consensus Scoring

### Scope
Replace the hardcoded `confidence = 0.75` and `consensus_level = 0.70` placeholders
with scores computed from the actual argument distribution.

### Design Constraints
- `confidence` = weighted average of `ArgumentStrength` values across all generated arguments.
- `consensus_level` = fraction of philosophy schools with a non-contradicting primary argument.
- Both scores must remain in `[0.0, 1.0]`.

### Required Interfaces
- `EthicsEvaluator::computeConfidence(const std::vector<EthicalArgument>&) -> double`
- `EthicsEvaluator::computeConsensus(const std::vector<EthicalArgument>&) -> double`

### Test Strategy
- Unit tests: single school → consensus = 1.0; opposing PRO/CON → consensus < 0.5.
- Property test: confidence monotonically increases as `ArgumentStrength` increases.

### Performance Targets
- O(n) in number of arguments; ≤ 1 ms for n ≤ 100.

---

## 3. Real Embedding Generation for Semantic Search

### Scope
Replace the zero-vector stub in `RAGContextEngine::generateEmbedding()` with a real
sentence embedding model so that `vectorSemanticSearch()` returns semantically
meaningful results.

### Design Constraints
- Target model: `all-MiniLM-L6-v2` (384-dimensional) via ONNX Runtime.
- Embedding dimension must match the ThemisDB vector index configuration.
- Thread-safe: multiple `RAGContextEngine` instances may share a single inference session.

### Required Interfaces
- New `IEmbeddingProvider` interface injected into `RAGContextEngine`.
- `OnnxEmbeddingProvider` implementation using ONNX Runtime.

### Test Strategy
- Unit test: cosine similarity of identical strings = 1.0 ± 1e-6.
- Integration test: `vectorSemanticSearch` returns results with similarity > 0.7 for
  semantically related queries.

### Performance Targets
- Embedding latency ≤ 20 ms per 512-token input on CPU.
- Batch of 10 queries: ≤ 150 ms.

### Security / Reliability
- Model loaded from a signed, version-pinned ONNX file.
- Fallback to keyword-overlap scoring if ONNX Runtime is unavailable.

---

## 4. Multi-Round Debates with Counter-Arguments

### Scope
Extend `EthicalDiscourseEngine` to support iterative debate rounds where philosophy
schools can respond to each other's arguments with counter-arguments.

### Design Constraints
- Maximum 3 rounds (6 turns per school) to bound computation cost.
- Each counter-argument is stored as a `CON` or `NEUTRAL` `EthicalArgument` with a
  `replyTo` field pointing to the target argument ID.
- Round transcripts must be retrievable via `ArgumentStore::getDebateTranscript(debate_id)`.

### Required Interfaces
- `EthicalDiscourseEngine::continueDebate(debate_id, round) -> std::variant<DebateRound, Status>`
- `ArgumentStore::getDebateTranscript(debate_id) -> std::variant<std::vector<EthicalArgument>, Status>`

### Test Strategy
- Unit: 2-school debate over 2 rounds produces 4 arguments.
- Integration: counter-argument references are correctly stored and retrievable.

### Performance Targets
- Each debate round ≤ 5 s including LLM generation.

---

## 5. Built-in Philosophy Profile Library

### Scope
Ship a curated set of YAML philosophy profiles as part of the module so that operators
do not need to author profiles from scratch.

### Planned Profiles

| Profile ID | Framework |
|------------|-----------|
| `utilitarianism` | Bentham / Mill utility maximisation |
| `kantian_ethics` | Categorical imperative, deontological duties |
| `virtue_ethics` | Aristotelian virtues and practical wisdom |
| `care_ethics` | Relationships, context, responsibility of care |
| `contractualism` | Rawlsian veil of ignorance, fairness |
| `gdpr_compliance` | GDPR Articles 5/25 — data minimisation, purpose limitation |
| `iso_42001` | AI management system ethical requirements |
| `ieee_7000` | IEEE standard for ethical AI design |

### Implementation Notes
- Profiles installed to `<install_prefix>/share/themisdb/ethics/` by CMake.
- `EthicsAiPlugin` loads from installed path if no override is provided in config.

### Test Strategy
- Unit test: all 8 profiles load without error.
- Integration test: `makeDecision()` with each profile produces a non-empty decision.

---

## 6. Prometheus Metrics

### Scope
Expose operational metrics for the Ethics AI module via the ThemisDB Prometheus endpoint.

### Planned Metrics

| Metric | Type | Description |
|--------|------|-------------|
| `ethics_decisions_total` | Counter | Total decisions synthesised |
| `ethics_decision_latency_seconds` | Histogram | End-to-end `makeDecision()` latency |
| `ethics_rag_context_hits_total` | Counter | RAG queries that returned ≥ 1 result |
| `ethics_argument_confidence_avg` | Gauge | Rolling average confidence score |
| `ethics_argument_store_size` | Gauge | Total arguments in store |

### Performance Targets
- Metrics collection overhead ≤ 0.1 ms per decision.

---

## 7. Vector Search Integration for ArgumentStore (Target: v1.6.0)

**Stub:** `src/ethics_ai/argument_store.cpp` — `storeArgument()` vector path

### Scope
- Add `IVectorWriter` injection to `ArgumentStore::initialize()`.
- On `storeArgument(arg, store_vector=true)`: generate embedding via
  `IEmbeddingBackend::embed(arg.content)` and write to the vector index via
  `IVectorWriter::upsert(arg.id, embedding, metadata)`.
- Add `ArgumentStore::searchSimilarArguments(query_text, k)` for semantic retrieval.
- Affected files:
  - `src/ethics_ai/argument_store.cpp` — remove STUB NOTE, implement vector path
  - `include/ethics_ai/argument_store.h` — add `setVectorWriter(IVectorWriter*)`,
    `setEmbeddingBackend(IEmbeddingBackend*)`, `searchSimilarArguments()` API

### Design Constraints
- `store_vector` parameter (default `true`) must be honoured; if `IVectorWriter`
  is not set, log a one-time WARN and skip silently.
- Vector dimension: 768 (all-mpnet-base-v2 compatible)
- Embedding must be idempotent (same content → same vector)

### Required Interfaces
- `IVectorWriter::upsert(id, embedding, metadata)` — already in `include/rag/rag_ingestion_bridge.h`
- `IEmbeddingBackend::embed(text) → std::vector<float>` — defined in `include/content/embedding_backend.h`

### Implementation Notes
- Use the same embedding backend already wired in `RAGIngestionBridge`; inject via
  `ArgumentStore::setEmbeddingBackend(backend)`.
- Metadata stored alongside embedding: `{philosophy_school, argument_type, confidence}`

### Test Strategy
- Unit: store 3 arguments; call `searchSimilarArguments("utilitarian harm reduction", k=2)`;
  assert results are ordered by cosine similarity.
- Regression: all existing `test_argument_store.cpp` tests pass with `store_vector=false`.

### Performance Targets
- `storeArgument()` overhead with embedding: ≤ 50 ms per argument (CPU inference)
- `searchSimilarArguments()` P99 ≤ 20 ms for ≤ 100k stored arguments

### Security / Reliability
- Embeddings stored in the same security domain as the RocksDB entities; no separate auth boundary.
- If embedding fails (backend error), storage still completes (no rollback); missing embedding
  is logged at WARN.

---

## 8. Domain LoRA Adapter Stack (lora_stack: YAML Field) (Target: Q3–Q4 2026)

### Scope
Extend the YAML philosophy profile schema with an optional `lora_stack:` field that
declares one or more domain-specific LoRA adapters to load into the argument-generating
LLM at inference time. Implement the ThemisDB LoRA Registry, the `DomainLoRATrainer`,
and multi-LoRA merging strategies. Enable the continuous LoRA training pipeline that
keeps domain adapters current from data residing in ThemisDB corpus collections.

See paper: `research/ETHICS_AI_YAML_DISCOURSE_ENGINE_PAPER_DRAFT.md` §3-F (YAML-Declared
LoRA Composition) and §III-E (Orthogonal Specialization Model) for full design rationale.

### Design Constraints
- `lora_stack:` field is optional; absence is equivalent to an empty stack (backward compatible).
- Adapter weights must be in `[0.0, 1.0]`; composition strategy: `weighted_merge` | `sequential` | `ties` | `task_vector`.
- `version: "latest"` resolves at load time against `_themis_lora_registry` collection.
- If a required adapter is unavailable (`required: true`), `loadProfile()` returns `ADAPTER_NOT_FOUND`.
- If an optional adapter is unavailable, it is skipped with a WARN log; remaining stack is applied.
- LoRA merge overhead ≤ 200 ms for ≤ 5 adapters at rank=16 on 7B model.
- Merged adapter is cached per-session; unloaded on session teardown.
- Each `EthicalArgument` entity stores: `lora_adapters_loaded[]`, `lora_versions[]`,
  `lora_composition`, `lora_training_sources[]` for full training provenance.

### Required Interfaces

**Schema extension** (`include/ethics_ai/philosophy_loader.h`):
```cpp
struct LoraAdapterDecl {
    std::string adapter_key;       // registry key, e.g. "legal/bgh_civil_liability_v4"
    float       weight{1.0f};
    std::string domain;
    std::string training_source;
    std::string version{"latest"};
    bool        required{false};
    std::vector<std::string> tags;
};

struct PhilosophyProfile {
    // ... existing fields ...
    std::vector<LoraAdapterDecl> lora_stack;
    std::string lora_composition{"weighted_merge"};
    std::string lora_load_on{"session_start"};
};
```

**Registry interface** (`include/ethics_ai/lora_registry.h` — new file):
```cpp
class ILoraRegistry {
public:
    virtual ~ILoraRegistry() = default;
    virtual std::variant<ResolvedAdapter, LoraRegistryError>
        resolveAdapter(const LoraAdapterDecl& decl) = 0;
    virtual std::vector<ResolvedAdapter>
        resolveStack(const std::vector<LoraAdapterDecl>& stack) = 0;
    virtual bool registerVersion(const AdapterVersionMetadata& meta) = 0;
    virtual bool promoteCanary(const std::string& adapter_key) = 0;
    virtual bool demoteToDeprecated(const std::string& adapter_key) = 0;
};
```

**Argument generator extension** (`include/ethics_ai/llm_argument_generator.h`):
```cpp
struct GeneratedArgument {
    std::string content;
    std::vector<std::string> principle_citations;
    std::vector<std::string> legal_citations;    // new: from domain LoRA knowledge
    float       fidelity_score{0.0f};
    float       legal_accuracy_score{0.0f};      // new: estimated by LoRA Judge
    bool        escape_detected{false};
    std::vector<std::string> lora_adapters_loaded;
    std::vector<std::string> lora_versions;
};
```

**Domain LoRA trainer** (`include/ethics_ai/domain_lora_trainer.h` — new):
```cpp
class DomainLoRATrainer {
public:
    struct TrainingConfig {
        std::string adapter_key;
        std::string corpus_uri;   // "argumentation_store://legal/bgh_decisions"
        int         max_new_docs{500};
        int         rank{16};
        int         alpha{32};
        int         epochs{3};
        float       learning_rate{2e-4f};
        float       accuracy_threshold{0.0f};  // must beat predecessor
    };

    TrainingResult trainIncremental(const TrainingConfig& config);
    bool evaluateOnHeldOut(const std::string& adapter_key,
                           const std::string& eval_set_uri,
                           float& accuracy_out);
};
```

### Implementation Notes
- Reuse `LlamaLoraAdapter::loadLoraModel()` / `isLoraActive()` infrastructure [E18]
  from the AI Safety Layer (ASL-13); extend to support multi-adapter loading.
- TIES-Merging implementation: apply `trimTopP(delta_W, p=0.2)` then elect sign by
  supermajority across all adapter deltas; sum signed trimmed deltas.
- `_themis_lora_registry` ArangoDocumentStore collection with AQL index on
  `(namespace, status, version.created_at)` for `"latest"` resolution.
- `DomainLoRATrainer` is Loop 5 of `ContinuousLearningOrchestrator`; triggered by
  corpus doc-count threshold (default: +100 new documents since last training run).
- Canary deployment: new versions start at `status: "canary"` (10% traffic);
  promote after 1,000 canary arguments with mean fidelity ≥ predecessor version.

### Test Strategy

**Unit tests** (new file: `tests/ethics_ai/test_lora_registry.cpp`):
- `LOR-01`: Resolve adapter with `version: "latest"` — returns highest `created_at` ready adapter.
- `LOR-02`: Resolve adapter with pinned version — exact tag match.
- `LOR-03`: Missing required adapter → `ADAPTER_NOT_FOUND` error.
- `LOR-04`: Missing optional adapter → WARN logged, remaining stack resolved.
- `LOR-05`: `weighted_merge` with 3 adapters → merged weight matrices sum to 1.0.
- `LOR-06`: `ties` merge with conflicting signs → only supermajority parameters retained.
- `LOR-07`: Canary promotion after 1,000 fidelity-passing arguments.
- `LOR-08`: Deprecation on first CONTRADICTION-level escape.

**Integration tests** (new file: `tests/ethics_ai/test_domain_lora_composition.cpp`):
- `DLC-01`: `kant.yaml` with 1-adapter stack loads and generates argument with `legal_citations`.
- `DLC-02`: 5-adapter TIES stack produces `lora_adapters_loaded` with all 5 keys.
- `DLC-03`: Backward compatibility — profile without `lora_stack:` behaves as v0.4.
- `DLC-04`: Session teardown unloads adapters within 100 ms.
- `DLC-05`: `DomainLoRATrainer.trainIncremental()` on mock corpus of 50 docs completes without error.
- `DLC-06`: `EthicalArgument` entity stores full `lora_training_sources[]` provenance.
- `DLC-07`: Two concurrent sessions with different `lora_stack:` do not interfere (session isolation).

### Performance Targets
- `lora_stack:` resolution from registry: ≤ 10 ms for ≤ 5 adapters (AQL lookup).
- Weighted merge (3 adapters, rank=16, 7B model): ≤ 50 ms.
- TIES-Merging (5 adapters, rank=16, 7B model): ≤ 200 ms.
- LoRA unload on session teardown: ≤ 100 ms.
- `DomainLoRATrainer` training time: 2–8 hours per adapter on single A100-class GPU,
  500 new documents, 3 epochs. Not a real-time path; runs asynchronously.
- Continuous training trigger latency (doc ingestion to training job start): ≤ 60 s.

### Security / Reliability
- LoRA adapter files stored with content-hash verification in the registry;
  tampered adapters are rejected at load time.
- Training corpus access requires `LORA_TRAINER` privilege role; read-only ethics
  users cannot trigger training runs.
- `training_source` URI validation: only `argumentation_store://` and
  `file:///var/themis/lora_corpora/` schemes allowed; external URLs rejected.
- Adapter bias audit: `lora_bias_audit` MCP tool (Q4 2026) queries registry for
  demographic distribution metadata in legal corpus entries.
- Canary deployment limits blast radius of new adapter versions.

### Breaking Changes
- `PhilosophyProfile` struct gains `lora_stack` and `lora_composition` fields
  (backward compatible: default empty / "weighted_merge").
- `EthicalArgument` entity gains `lora_adapters_loaded`, `lora_versions`,
  `lora_training_sources` fields in the RocksDB schema
  (migration: old arguments get empty arrays; query backward compatible).
