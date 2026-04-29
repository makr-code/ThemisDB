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

---

## 9. YAML Ethics School Schema Improvements — Context-Window & 5-Round Discourse Support

**Motivation:** The 5-round dialectic evidence run
(`research/DIALECTIC_EVIDENCE_PAPER.md`, Evidence Anchors E40–E44, 2026-04-29)
identified systematic gaps in the current YAML philosophy profile schema.
These gaps become critical when running ≥ 4 discourse rounds with 3+ schools:

- **Context window overflow (E40–E41):** All 5 dilemmas × all 3 schools overflow
  7B-model (8 K token) limits at Round 3 (SURREBUTTAL). No per-thesis budget
  metadata exists in current YAML to enable targeted compression.
- **Missing cross-school citation maps (E35):** Schools cannot reference opposing
  theses by `thesis_id` — the template has no mapping from, e.g.,
  `kant:selbstzweck` to `utilitarianism:consequentialism` (its direct challenger).
- **No round-specific thesis activation (E42):** All theses are equally available
  in all rounds — no signal for which theses are most relevant in REBUTTAL vs.
  SYNTHESIS vs. META-VERDICT.
- **No domain-specific modifiers (E38, E44):** Applying `kant.yaml` to an
  autonomous-systems dilemma vs. a medical dilemma requires different thesis
  emphases; the current schema has no `domain_override_requirements` field.
- **No act/rule priority declaration (E36, E28):** `utilitarianism.yaml` does
  not declare whether act or rule level takes precedence for policy contexts,
  causing internal tension that surfaces at R5 as low confidence scores.
- **No confidence calibration field (E44):** Schools cannot declare per-dilemma-type
  confidence modifiers; the R5 META-VERDICT must estimate confidence from scratch.

---

### 9.1 Per-Thesis `token_budget` and `activation_rounds`  ✅ IMPLEMENTED (2026-04-29)

> **Status:** Implemented in `ethics_ai_types.h` (`PhilosophyThesis` struct + `PhilosophyProfile.typed_theses`),
> `philosophy_loader.cpp` (YAML parsing), `context_window_manager.h/.cpp` (`selectThesesForRound()`).
> Tests TBM-01..10 in `tests/test_thesis_budget_management.cpp`.

#### Scope
Add two optional fields to each `thesis_id` entry in every philosophy YAML profile:
- `token_budget: int` — maximum tokens to inject for this thesis in the LLM context
- `activation_rounds: list[int]` — which discourse rounds (1–5) this thesis is
  actively injected (omitting a round = thesis summarised to headline only)

#### Design Constraints
- `token_budget` default: `null` (no limit — current behaviour preserved)
- `activation_rounds` default: `[1, 2, 3, 4, 5]` (active in all rounds — backward compatible)
- `ContextWindowBudgetManager` (PE layer) MUST respect `token_budget` and
  skip full-thesis injection in non-activation rounds, replacing with:
  `"[{thesis_id}: {name} — see full profile]"` (≤ 15 tokens)
- If total monocle budget (sum of active `token_budget` values) exceeds
  `ContextWindowBudgetManager.monocle_budget_tokens`, the manager must
  truncate lowest-priority theses first (priority = `activation_rounds` membership).

#### Schema Extension (per `thesis_id` block)

```yaml
# kant.yaml — example with new fields
main_theses:
  - thesis_id: "kategorischer_imperativ"
    name: "Kategorischer Imperativ"
    # ... existing fields ...
    token_budget: 180         # max tokens injected for this thesis
    activation_rounds: [1, 2, 3]   # active in PRO, REBUTTAL, SURREBUTTAL; headline-only in R4–R5
    round_role_weights:
      PRO: 1.0                # highest weight in opening position
      REBUTTAL: 0.8           # slightly lower — opponent argument carries more weight
      SURREBUTTAL: 0.9
      SYNTHESIS: 0.5          # compressed in synthesis
      META_VERDICT: 0.3       # summary reference only

  - thesis_id: "selbstzweck"
    token_budget: 160
    activation_rounds: [1, 2, 3, 4]
    round_role_weights:
      PRO: 1.0
      REBUTTAL: 1.0           # highest relevance in rebuttal — most challenged thesis
      SURREBUTTAL: 1.0
      SYNTHESIS: 0.6
      META_VERDICT: 0.4
```

#### Required Interfaces

**`ContextWindowBudgetManager`** (`include/prompt_engineering/context_window_budget_manager.h`):
```cpp
struct ThesisBudgetInfo {
    std::string thesis_id;
    int         token_budget{-1};           // -1 = unlimited
    std::vector<int> activation_rounds;     // empty = all rounds
    std::map<std::string, float> round_role_weights;
};

// New method on ContextWindowBudgetManager:
std::vector<ThesisInjection>
    selectThesesForRound(const PhilosophyProfile& profile,
                         int round_number,
                         const std::string& round_role,
                         int available_tokens);
```

**`PhilosophyLoader`** (`include/ethics_ai/philosophy_loader.h`):
```cpp
struct PhilosophyThesis {
    std::string thesis_id;
    std::string name;
    std::string description;
    // ... existing fields ...
    int         token_budget{-1};
    std::vector<int> activation_rounds;
    std::map<std::string, float> round_role_weights;
};
```

#### Implementation Notes
- `selectThesesForRound()` filters by `activation_rounds` first, then sorts by
  `round_role_weights[round_role]` descending, then greedily selects until
  `available_tokens` is exhausted.
- Theses not selected (non-activation round or budget exceeded) are replaced by
  a one-line headline token: `"[{thesis_id}: {name}]"`.
- `ContextWindowBudgetManager` logs at DEBUG when a thesis is downgraded to headline.

#### Test Strategy

**New test file:** `tests/ethics_ai/test_thesis_budget_management.cpp`

- `TBM-01`: Thesis with `activation_rounds: [1,2]` injected in R1, headline-only in R3.
- `TBM-02`: Total budget exceeded → lowest-weight thesis truncated first.
- `TBM-03`: Profile with no `token_budget` fields → behaves as unlimited (backward compat).
- `TBM-04`: `round_role_weights.REBUTTAL = 1.0` → thesis selected before others in R2.
- `TBM-05`: `available_tokens = 100` with all theses requiring 200 → top-1 selected only.

#### Performance Targets
- `selectThesesForRound()` ≤ 0.5 ms for ≤ 20 theses per profile.

---

### 9.2 Cross-School `counter_theses` Mapping

#### Scope
Add a `counter_theses` block to each YAML profile declaring which theses from
*other schools* are in direct philosophical tension with the current school's theses.
This enables Architecture B to automatically include the opposing thesis `thesis_id`
in REBUTTAL prompts, improving DC without relying on the LLM to spontaneously identify
the correct cross-school citation (evidence: E35 — R3 utilitarian correctly identified
`kant:selbstzweck` vs. `utilitarianism:consequentialism` tension only because it was
already surfaced in R1/R2 context).

#### Schema Extension

```yaml
# kant.yaml — cross_school_tensions block (new, optional)
cross_school_tensions:
  - own_thesis: "selbstzweck"
    opposing_school: "utilitarianism"
    opposing_thesis: "greatest_happiness"
    tension_type: "categorical_vs_aggregate"
    tension_description: |
      Selbstzweckformel prohibits using persons as means; Greatest Happiness
      permits it when aggregate utility is sufficient. Fundamental deontological
      vs. consequentialist divergence.
    rebuttal_cite_weight: 0.9    # how strongly to include in REBUTTAL prompt

  - own_thesis: "rigorismus"
    opposing_school: "utilitarianism"
    opposing_thesis: "two_level_utilitarianism"
    tension_type: "absolute_vs_tiered"
    rebuttal_cite_weight: 0.8

  - own_thesis: "kategorischer_imperativ"
    opposing_school: "contractualism"
    opposing_thesis: "reasonable_rejection"
    tension_type: "convergent_ground"    # same practical conclusion, different basis
    tension_description: |
      Both prohibit the Fat Man push, but via different first-order arguments.
      Architecture B should surface the convergence explicitly in SYNTHESIS.
    synthesis_cite_weight: 0.9
```

#### Required Interfaces

**`CrossSchoolTensionResolver`** (`include/ethics_ai/cross_school_tension_resolver.h` — new):
```cpp
struct CrossSchoolTension {
    std::string own_thesis;
    std::string opposing_school;
    std::string opposing_thesis;
    std::string tension_type;         // "categorical_vs_aggregate" | "convergent_ground" | etc.
    std::string tension_description;
    float       rebuttal_cite_weight{0.5f};
    float       synthesis_cite_weight{0.5f};
};

class CrossSchoolTensionResolver {
public:
    // Returns tensions relevant to the current round and opposing schools
    std::vector<CrossSchoolTension>
        getRelevantTensions(const PhilosophyProfile& current_school,
                            const std::vector<std::string>& opposing_schools,
                            const std::string& round_role) const;

    // Injects cross-school thesis_ids into the prompt context
    std::string buildCrossSchoolPromptSnippet(
        const std::vector<CrossSchoolTension>& tensions,
        int max_tokens) const;
};
```

#### Test Strategy

**New test file:** `tests/ethics_ai/test_cross_school_tensions.cpp`

- `CST-01`: `kant` vs `utilitarianism` in REBUTTAL → `self_zweck` / `greatest_happiness` tension injected.
- `CST-02`: `tension_type: "convergent_ground"` activates in SYNTHESIS, not REBUTTAL.
- `CST-03`: `rebuttal_cite_weight = 0.0` → tension not injected in REBUTTAL.
- `CST-04`: Missing `cross_school_tensions` block → no injection, no error (backward compat).
- `CST-05`: `buildCrossSchoolPromptSnippet()` respects `max_tokens` constraint.
- `CST-06`: Three-school debate → tensions from all opposing school combinations resolved.

#### Performance Targets
- `getRelevantTensions()` ≤ 0.2 ms for ≤ 10 tensions per school.

---

### 9.3 Prior-Round Context Compression (`prior_round_summarization`)

#### Scope
Add a `prior_round_summarization` block to each YAML profile (and a global default
in `discourse_config.yaml`) declaring how previous-round content should be compressed
before injection into later rounds.

**Motivation (E40–E41):** R3 SURREBUTTAL accumulates ~4 400–5 100 tokens of prior-round
context, exceeding the 8 K effective context limit of 7B models. Without compression,
7B-model runs either truncate (losing coherence) or exceed budget (hard failure).
The YAML-declared compression policy enables `ContextWindowBudgetManager` to apply
school-appropriate summarisation automatically.

#### Schema Extension

```yaml
# kant.yaml — prior_round_summarization block (new, optional)
prior_round_summarization:
  trigger_round: 3            # apply compression from this round onward
  mode: "structured_summary"  # "headline" | "structured_summary" | "principle_citations_only"
  max_tokens_per_round: 300   # compressed representation of each prior round (per school)
  preserve:
    - "principle_citations"   # always keep PRINCIPLE CITATIONS block verbatim
    - "verdict"               # always keep final verdict sentence
  compress:
    - "argument_prose"        # compress by ≥ 60%
    - "historical_context"    # drop entirely
    - "extended_examples"     # drop entirely
  cross_round_coherence_anchor: "thesis_ids"
    # keeps thesis_id references verbatim to maintain DC even in compressed form
```

**Compression modes:**

| Mode | Description | Token reduction | DC preservation |
|---|---|---|---|
| `headline` | One-sentence summary per argument | ~80% | Low (DC −0.15 estimated) |
| `structured_summary` | Verdict + principle_citations + key claim | ~60% | Medium (DC −0.08) |
| `principle_citations_only` | Only the `PRINCIPLE CITATIONS:` block | ~75% | High (DC −0.05) |

#### Required Interfaces

**`PriorRoundCompressor`** (`include/ethics_ai/prior_round_compressor.h` — new):
```cpp
struct CompressionConfig {
    int         trigger_round{3};
    std::string mode{"structured_summary"};
    int         max_tokens_per_round{300};
    std::vector<std::string> preserve_elements;
    std::vector<std::string> compress_elements;
    bool        keep_thesis_id_anchors{true};
};

class PriorRoundCompressor {
public:
    // Returns compressed representation of a prior round's arguments
    std::string compressPriorRound(
        const std::vector<EthicalArgument>& round_arguments,
        const CompressionConfig& config,
        int current_round) const;

    // Full prior-context budget after compression for all previous rounds
    std::string buildPriorContext(
        const std::vector<std::vector<EthicalArgument>>& all_rounds,
        const CompressionConfig& config,
        int current_round,
        int max_total_tokens) const;
};
```

#### Test Strategy

**New test file:** `tests/ethics_ai/test_prior_round_compressor.cpp`

- `PRC-01`: `mode: "headline"` on 3-argument round → output ≤ 100 tokens.
- `PRC-02`: `mode: "structured_summary"` preserves `PRINCIPLE CITATIONS:` block verbatim.
- `PRC-03`: `trigger_round: 3` — no compression at R2; compression at R3.
- `PRC-04`: `keep_thesis_id_anchors: true` — thesis_id references (`kant:selbstzweck`) always preserved.
- `PRC-05`: `max_total_tokens = 1200` — total compressed prior context does not exceed.
- `PRC-06`: Empty round arguments → empty output, no error.

#### Performance Targets
- `compressPriorRound()` ≤ 5 ms for ≤ 3 arguments per round (CPU-only, no LLM call).
- `buildPriorContext()` ≤ 15 ms for ≤ 4 prior rounds.

---

### 9.4 Five-Round Discourse Declaration in Profile YAML

#### Scope
Add a `discourse_config` block to the global ethics YAML configuration (and optionally
to individual profile YAMLs for per-school overrides) declaring the 5-round discourse
structure, roles, and per-round prompt templates.

**Motivation:** The current `EthicalDiscourseEngine` has `max_rounds: 3` hardcoded
in `FUTURE_ENHANCEMENTS.md §4`. The evidence run demonstrates that 5 rounds produce
measurable quality improvements (DC R4 = 0.84 vs. DC R2 = 0.76) and surfaces YAML
schema gaps (E42) not visible in 3-round runs. The 5-round structure must be
declarative, not hardcoded, to allow configuration per use-case.

#### Schema Extension

```yaml
# config/ethics_ai/discourse_config.yaml (new file)
discourse_rounds:
  count: 5
  roles:
    - round: 1
      name: "PRO"
      description: "Opening position — state the school's verdict on the dilemma"
      prompt_template: "opening_position"
      context_includes: ["dilemma_text", "monocle_theses"]
      max_tokens: 500

    - round: 2
      name: "REBUTTAL"
      description: "Challenge the strongest opposing argument from Round 1"
      prompt_template: "rebuttal"
      context_includes: ["dilemma_text", "opponent_r1", "own_monocle_theses", "cross_school_tensions"]
      max_tokens: 600

    - round: 3
      name: "SURREBUTTAL"
      description: "Defend Round 1 position against the Round 2 challenge"
      prompt_template: "surrebuttal"
      context_includes: ["own_r1", "opponent_r2", "own_monocle_theses"]
      compression_policy: "prior_round_summarization"    # triggers PriorRoundCompressor
      max_tokens: 600

    - round: 4
      name: "SYNTHESIS"
      description: "Identify convergence and persistent disagreements across all schools"
      prompt_template: "synthesis"
      context_includes: ["compressed_r1_r3", "convergence_markers"]
      max_tokens: 700
      multi_school: true    # single synthesis output replaces per-school outputs

    - round: 5
      name: "META_VERDICT"
      description: "Final position with confidence score and YAML improvement signals"
      prompt_template: "meta_verdict"
      context_includes: ["debate_summary", "own_monocle_theses"]
      output_schema:
        verdict: "string"
        confidence: "float[0.0,1.0]"
        yaml_improvement_signals: "list[string]"
      max_tokens: 400
```

#### Required Interfaces

**`DiscourseRoundConfig`** (`include/ethics_ai/discourse_engine.h`):
```cpp
struct DiscourseRoundConfig {
    int         round_number;
    std::string name;               // "PRO" | "REBUTTAL" | "SURREBUTTAL" | "SYNTHESIS" | "META_VERDICT"
    std::string prompt_template;
    std::vector<std::string> context_includes;
    bool        use_compression{false};
    bool        multi_school_output{false};
    int         max_tokens{500};
};

// EthicalDiscourseEngine gains:
void setDiscourseConfig(const std::vector<DiscourseRoundConfig>& rounds);
```

**`EthicalDiscourseEngine::runDebate()`** extended to iterate over configured rounds
rather than hardcoded 3-round loop.

#### Test Strategy

**New tests in:** `tests/ethics_ai/test_discourse_engine.cpp`

- `DRE-01`: 5-round config produces 5 rounds of output.
- `DRE-02`: `multi_school: true` at R4 produces single SYNTHESIS, not 3 per-school.
- `DRE-03`: `max_rounds: 3` config in legacy format → backward compatible, 3 rounds.
- `DRE-04`: R5 `output_schema` validated — missing `confidence` field → `SCHEMA_VIOLATION` error.
- `DRE-05`: Missing `discourse_config.yaml` → default 3-round config applies.

#### Performance Targets
- 5-round 3-school debate with Arch-B (GPT-4o): ≤ 30 s total (5 × 3 LLM calls × ~2 s each).
- 5-round 3-school debate with Template only: ≤ 50 ms total.

---

### 9.5 Convergence Compatibility Markers

#### Scope
Add a `convergence_compatible` field to each `thesis_id` declaring which theses from
other schools produce *the same practical verdict* even though the philosophical grounds
differ. This enables the R4 SYNTHESIS prompt to surface inter-school convergence
explicitly and to generate `cross_school_consensus` tags in debate output.

**Motivation (E29, E38):** The AV and medical_002 dialectics showed that all three
schools converge on `minimize_casualties` / `maximize_survival` for different
philosophical reasons. This convergence is only visible in Architecture B, and even
then requires the LLM to spontaneously identify it. Explicit YAML markers allow the
engine to *guarantee* that convergence is surfaced and tagged in R4 SYNTHESIS output.

#### Schema Extension

```yaml
# kant.yaml — convergence_compatible block per thesis
main_theses:
  - thesis_id: "kategorischer_imperativ"
    # ... existing fields ...
    convergence_compatible:
      - school: "contractualism"
        thesis: "reasonable_rejection"
        shared_conclusion: "do_not_push"
        convergence_type: "co_prohibitive"   # both prohibit
        divergence_on: "grounding"            # same verdict, different basis
        synthesis_note: |
          Kantian universalisability test and Scanlonian reasonable-rejection
          independently prohibit the Fat Man push — one via rational consistency,
          the other via individual veto rights.

      - school: "utilitarianism"
        thesis: "rule_utilitarianism"
        shared_conclusion: "do_not_push"
        convergence_type: "conditional_co_prohibitive"
        condition: "policy_mode"             # convergence only at rule/policy level
        synthesis_note: |
          Rule-utilitarianism converges with Kant on do_not_push for Fat Man
          in policy mode; act-utilitarianism does not — the convergence is
          level-dependent.
```

#### Required Interfaces

**`ConvergenceMarkerEngine`** (`include/ethics_ai/convergence_marker_engine.h` — new):
```cpp
struct ConvergenceMarker {
    std::string school_a;
    std::string thesis_a;
    std::string school_b;
    std::string thesis_b;
    std::string shared_conclusion;
    std::string convergence_type;    // "co_prohibitive" | "conditional_co_prohibitive" | "co_permissive"
    std::string condition;           // optional activation condition
    std::string synthesis_note;
};

class ConvergenceMarkerEngine {
public:
    // Returns all active convergence markers for the current set of participating schools
    std::vector<ConvergenceMarker>
        getActiveMarkers(const std::vector<PhilosophyProfile>& profiles,
                         const std::string& context_mode = "") const;

    // Generates a SYNTHESIS preamble noting cross-school convergences
    std::string buildConvergencePreamble(
        const std::vector<ConvergenceMarker>& markers,
        int max_tokens) const;
};
```

#### Test Strategy

**New test file:** `tests/ethics_ai/test_convergence_markers.cpp`

- `CME-01`: `kant` + `contractualism` on Fat Man → `co_prohibitive` marker returned for `do_not_push`.
- `CME-02`: `conditional_co_prohibitive` with `condition: "policy_mode"` not returned in `individual_action` mode.
- `CME-03`: Missing `convergence_compatible` block → empty marker list, no error.
- `CME-04`: `buildConvergencePreamble()` respects `max_tokens` limit.
- `CME-05`: Three-school debate → all pairwise convergences resolved and deduplicated.
- `CME-06`: Dilemma `av_001` with all 3 schools → 3-way `co_permissive` marker for `minimize_casualties`.

#### Performance Targets
- `getActiveMarkers()` ≤ 0.5 ms for ≤ 30 convergence declarations across ≤ 5 profiles.

---

### 9.6 Domain-Specific Activation Modifiers

#### Scope
Add a `domain_overrides` block to each YAML profile declaring domain-specific
thesis emphasis and additional requirements that activate automatically when the
dilemma's `domain` field matches. This addresses the finding (E38, E42) that
applying `kant.yaml` to `autonomous_systems` vs. `medical` dilemmas requires
different thesis weights and additional requirements (transparency, override mechanisms)
that are not currently encoded in the school YAML.

#### Schema Extension

```yaml
# kant.yaml — domain_overrides block (new, optional)
domain_overrides:
  - domain: "autonomous_systems"
    activate_theses: ["autonomie_wuerde", "kategorischer_imperativ"]
    additional_requirements:
      - "AV decision rules must be publicly declared (autonomie_wuerde: transparency)"
      - "Programming rule must pass universalisierungstest before deployment"
    deactivate_theses: []
    thesis_weight_adjustments:
      autonomie_wuerde: +0.3    # elevated in AV context
      rigorismus: -0.1          # slightly lower — policy context moderates rigorism

  - domain: "medical"
    activate_theses: ["selbstzweck", "pflicht_neigung", "autonomie_wuerde"]
    additional_requirements:
      - "Informed consent is a Kantian precondition for all medical interventions"
      - "Human clinician override is required for algorithmic triage (autonomie_wuerde)"
    thesis_weight_adjustments:
      selbstzweck: +0.3
      pflicht_neigung: +0.2

  - domain: "ai_ethics"
    activate_theses: ["kategorischer_imperativ", "autonomie_wuerde"]
    additional_requirements:
      - "AI systems must be transparent and explainable to respect rational agency"
      - "Discriminatory training data violates universalisierungstest"
```

#### Required Interfaces

**`DomainModifierApplicator`** (`include/ethics_ai/domain_modifier_applicator.h` — new):
```cpp
class DomainModifierApplicator {
public:
    // Returns modified PhilosophyProfile with domain-specific weights and requirements
    PhilosophyProfile applyDomainModifiers(
        const PhilosophyProfile& base_profile,
        const std::string& dilemma_domain) const;
};
```

`PhilosophyLoader` calls `DomainModifierApplicator::applyDomainModifiers()` when
loading a profile for a dilemma with a non-empty `domain` field.

#### Test Strategy

**New test file:** `tests/ethics_ai/test_domain_modifier_applicator.cpp`

- `DMA-01`: `kant.yaml` + `domain: "medical"` → `selbstzweck` weight elevated; clinician-override requirement injected.
- `DMA-02`: `domain: "unknown_domain"` → no modifications applied, no error.
- `DMA-03`: `domain: "autonomous_systems"` → `autonomie_wuerde` weight elevated; transparency requirement present.
- `DMA-04`: `deactivate_theses: ["rigorismus"]` → rigorismus absent from modified profile.
- `DMA-05`: Missing `domain_overrides` block → base profile returned unchanged.
- `DMA-06`: Two profiles with same domain → each applies its own domain_override independently.

#### Performance Targets
- `applyDomainModifiers()` ≤ 1 ms for ≤ 15 theses per profile.

---

### 9.7 Combined Test Suite: YAML Schema Improvements (YSI-01..12)

**New test file:** `tests/ethics_ai/test_yaml_schema_improvements.cpp`

| Test ID | Scenario | Checks |
|---|---|---|
| `YSI-01` | 5-round debate with token_budget | R3 thesis injection does not exceed declared budget |
| `YSI-02` | Fat Man R2 REBUTTAL | `cross_school_tensions` injects `utilitarianism:greatest_happiness` into Kantian REBUTTAL prompt |
| `YSI-03` | Fat Man R3 SURREBUTTAL | `prior_round_compression: "structured_summary"` reduces prior context from 4 800 to ≤ 1 500 tokens |
| `YSI-04` | Fat Man R4 SYNTHESIS | `convergence_compatible` marker for `do_not_push` (kant + contractualism) appears in synthesis output |
| `YSI-05` | `av_001` R1 PRO | `domain_overrides.autonomous_systems` elevates `autonomie_wuerde`, transparency requirement present |
| `YSI-06` | `medical_002` R1 PRO | `domain_overrides.medical` injects clinician-override requirement for all 3 schools |
| `YSI-07` | 5-round config YAML | `discourse_config.yaml` with 5 rounds produces R5 META-VERDICT with `confidence` field |
| `YSI-08` | `principle_citations_only` compression | DC ≥ 0.70 after compression (thesis_id anchors preserved) |
| `YSI-09` | Backward compatibility | Profile without any new fields → all existing tests pass |
| `YSI-10` | `act_rule_priority_mode: "policy"` | `utilitarianism.yaml` with `act_rule_priority_mode` → R1 uses rule-level by default |
| `YSI-11` | R5 `yaml_improvement_signals` | META-VERDICT output contains ≥ 1 YAML improvement signal for each dilemma |
| `YSI-12` | Context overflow prevention | 7B-model (8K limit) completes R3–R5 without truncation after compression policy applied |

#### Performance Targets (Combined)
- Full 5-round 3-school debate with all YSI features enabled: ≤ 35 s (Arch-B, GPT-4o).
- Full 5-round 3-school debate with Template only + schema features: ≤ 100 ms.
- Peak memory overhead for all new schema objects: ≤ 5 MB per active session.

#### Security / Reliability
- New YAML fields are optional and backward compatible; missing fields revert to defaults.
- `cross_school_tensions` injection is subject to the same `PromptInjectionDetector`
  checks as all other YAML-derived content.
- `domain_override_requirements` strings are sanitised (strip HTML, truncate to 200 chars)
  before LLM injection.
- `prior_round_compression` mode `"principle_citations_only"` must not include
  raw user-supplied dilemma text to prevent context-injection via dilemma field.

#### Breaking Changes
- None: all new fields are optional with backward-compatible defaults.
- `EthicalDiscourseEngine` gains optional `setDiscourseConfig()` method; if not called,
  existing 3-round default behaviour is preserved.
- `PhilosophyThesis` struct gains 4 new optional fields; existing YAML profiles without
  them continue to load and function without modification.

---

## 10. Non-Mainstream Ethics School Schema Extensions (Target: Q4 2026)

> **Motivation:** Evidence Paper §VI–§VII establishes that non-mainstream schools
> (Marx, Arendt, Nietzsche, Schopenhauer) achieve expert-level alignment on
> political-economy and AI-governance dilemmas. However, their YAML profiles lack
> `thesis_id` citation keys, domain-specific activation fields, regulatory constraint
> guards, and context-window-aware compression metadata. This section specifies
> the required schema extensions, inferred directly from the 5-round dialectic gaps
> identified in Evidence Anchors E45–E64.

---

### §10.1 Marx (`marx.yaml`) — Dialectical Materialist Schema Extensions

#### Scope
Extend `marx.yaml` with fields that enable Architecture B to:
(a) cite individual theses by `thesis_id` (required for Φ measurement),
(b) apply the `ideology_critique` thesis selectively to dilemmas involving
    algorithmic decision-making or institutional "objectivity" claims,
(c) report a structured `exploitation_index` in R5 META-VERDICT output.

#### Required New YAML Fields

```yaml
# marx.yaml additions
main_theses:
  historical_materialism:
    thesis_id: "marx:historical_materialism"      # ADD
    activation_rounds: [1, 4]                     # ADD — primarily R1 PRO and R4 SYNTHESIS
    token_budget: 150                             # ADD
  class_struggle:
    thesis_id: "marx:class_struggle"              # ADD
    activation_rounds: [1, 2, 3]                  # ADD
    token_budget: 100                             # ADD
  alienation:
    thesis_id: "marx:alienation"                  # ADD
    activation_rounds: [1, 3]                     # ADD
    token_budget: 200                             # ADD
    activation_conditions:                        # ADD — new field
      - "labor_context: true"
      - "algorithmic_management: true"
  surplus_value:
    thesis_id: "marx:surplus_value"               # ADD
    activation_rounds: [1, 2]                     # ADD
    token_budget: 100                             # ADD

secondary_theses:
  ideology_critique:
    thesis_id: "marx:ideology_critique"           # ADD
    activation_conditions:                        # ADD
      - "algorithm_claims_neutrality: true"
      - "institutional_objectivity_claim: true"

# NEW top-level fields
class_analysis_mode: "materialist"               # ADD: enum materialist | ideological | structural
exploitation_index:                              # ADD: R5 structured output
  enabled: true
  fields: ["surplus_extraction_score", "alienation_depth", "class_beneficiary"]
dialectical_materialism_lens: true               # ADD: enables thesis-antithesis-synthesis framing
regulatory_constraints:                          # ADD: §10.5 guard
  override_permitted: true
  applicable_regulations: []
```

#### Design Constraints
- `thesis_id` must follow the pattern `<school>:<thesis_key>` (snake_case).
- `activation_conditions` strings are parsed by `DiscoursePromptCoordinator` against
  the dilemma YAML's `tags:` field; unmatched conditions revert to default activation.
- `exploitation_index` output is appended to R5 text; fields are sanitised
  (no free-form text; enum values only) to prevent prompt injection.

#### Test Strategy (NPE-01..03)
- **NPE-01:** Φ for marx R1 PRO = ≥ 0.90 after `thesis_id` extension (vs. estimated 0.87 without)
- **NPE-02:** `ideology_critique` activates in R1 for `authority_001` (algorithm-claims-neutrality) but NOT for `trolley_001`
- **NPE-03:** `exploitation_index` appears in R5 META-VERDICT for `labor_001` and `medical_002`; absent for `trolley_001`

#### Performance Targets
- `thesis_id` lookup adds ≤ 5 ms overhead per argument (map lookup, O(1))
- `exploitation_index` R5 structured output ≤ 120 additional tokens

---

### §10.2 Arendt (`arendt.yaml`) — Political Phenomenology Schema Extensions

#### Scope
Extend `arendt.yaml` with fields enabling:
(a) `thesis_id` citation for all 5 main theses + 5 secondary theses,
(b) `public_space_threshold` — a semantic tag that activates `public_private` and
    `plurality` theses when a dilemma involves institutional/public decision contexts,
(c) `banality_detection` — automatic activation of `banality_of_evil` when
    the dilemma involves automated or delegated decision-making without review,
(d) `natality_novelty_score` in R5 — measures whether the verdict opens or forecloses
    new possibilities for action.

#### Required New YAML Fields

```yaml
# arendt.yaml additions
main_theses:
  vita_activa:
    thesis_id: "arendt:vita_activa"
    activation_rounds: [1, 4]
    token_budget: 180
  plurality:
    thesis_id: "arendt:plurality"
    activation_rounds: [1, 2, 3]
    token_budget: 120
    activation_conditions:
      - "affects_group_of_individuals: true"
  public_private:
    thesis_id: "arendt:public_private"
    activation_rounds: [1, 3]
    token_budget: 130
  banality_of_evil:
    thesis_id: "arendt:banality_of_evil"
    activation_rounds: [1, 2]
    token_budget: 150
    activation_conditions:                        # ADD — new conditional activation
      - "automated_decision: true"
      - "delegated_judgment: true"
      - "no_human_review: true"
  natality:
    thesis_id: "arendt:natality"
    activation_rounds: [3, 5]
    token_budget: 100

# NEW top-level fields
public_space_threshold:                          # ADD
  enabled: true
  trigger_tags: ["institutional", "public_authority", "criminal_justice", "labor_management"]
banality_detection:                              # ADD
  enabled: true
  trigger_tags: ["automated_decision", "algorithm_without_appeal", "delegated_judgment"]
natality_novelty_score:                          # ADD: R5 output field
  enabled: true
  scale: "0.0 (forecloses action) .. 1.0 (opens new action space)"
regulatory_constraints:
  override_permitted: false                      # Arendt: political constraints non-negotiable
  applicable_regulations:
    - "EU_AI_Act_Art22"
    - "EU_Platform_Work_Directive"
```

#### Test Strategy (NPE-04..06)
- **NPE-04:** `banality_of_evil` activates for `authority_001` (automated_decision=true) and `labor_001` (algorithm_without_appeal=true), NOT for `trolley_001`
- **NPE-05:** `natality_novelty_score` = 0.1 for `authority_001` (forecloses new action) and = 0.8 for verdict "require human review tribunal" (opens new action space)
- **NPE-06:** `public_space_threshold` activates `public_private` thesis for `authority_001` and `labor_001`; does not activate for `trolley_001` (private moral decision)

---

### §10.3 Nietzsche (`nietzsche.yaml`) — Will-to-Power Schema Extensions

#### Scope
Extend `nietzsche.yaml` with:
(a) `thesis_id` for all five main theses,
(b) `value_creation_mode` — distinguishes creative (Übermensch) from reactive
    (slave-morality) power expressions in the dilemma context,
(c) `slave_morality_detection` — identifies when the system being evaluated enforces
    conformity to averages rather than excellence,
(d) **`regulatory_constraints_override: false`** — critical safety field preventing
    Nietzsche monocle from generating regulatory-violating outputs in deployed systems.
    This directly addresses Evidence Finding E60 (Nietzsche violates German Ethik-Kommission
    in `av_001` when no regulatory guard is present).

#### Required New YAML Fields

```yaml
# nietzsche.yaml additions
main_theses:
  will_to_power:
    thesis_id: "nietzsche:will_to_power"
    activation_rounds: [1, 2]
    token_budget: 120
  uebermensch:
    thesis_id: "nietzsche:uebermensch"
    activation_rounds: [1, 4]
    token_budget: 150
  eternal_recurrence:
    thesis_id: "nietzsche:eternal_recurrence"
    activation_rounds: [5]
    token_budget: 80
  perspectivism:
    thesis_id: "nietzsche:perspectivism"
    activation_rounds: [2, 3]
    token_budget: 100
  master_slave_morality:
    thesis_id: "nietzsche:master_slave_morality"
    activation_rounds: [1, 3]
    token_budget: 140
    activation_conditions:
      - "system_enforces_conformity: true"
      - "statistical_average_used: true"

# NEW top-level fields
value_creation_mode: "creative"                  # ADD: enum creative | reactive | ambivalent
slave_morality_detection:                        # ADD
  enabled: true
  trigger_tags: ["statistical_model", "average_score", "conformity_enforcement"]
regulatory_constraints:                          # ADD — CRITICAL SAFETY FIELD
  override_permitted: false                      # Prevents policy-violating outputs
  applicable_regulations:
    - "German_Ethik_Kommission_2017"             # Forbids life-quality discrimination in AV
    - "EU_AI_Act_Art22"
  constraint_note: |
    When regulatory_constraints are active, the Nietzsche monocle suppresses
    outputs that assign differential value to human lives based on excellence,
    fitness, or contribution criteria, even when the YAML theses (uebermensch,
    master_slave_morality) would otherwise generate such content. This is a
    deliberate design choice to ensure deployed discourse engines are compliant
    with applicable law. Philosophical richness is preserved in academic/research
    mode (regulatory_constraints.override_permitted: true).
```

#### Test Strategy (NPE-07..09)
- **NPE-07:** With `regulatory_constraints.override_permitted: false`, Nietzsche R1 for `av_001` does NOT produce "save-the-excellent" output; instead produces perspectivism critique of the dilemma framing
- **NPE-08:** With `override_permitted: true` (research mode), Nietzsche R1 for `av_001` produces "save-the-excellent" output; Φ ≥ 0.84
- **NPE-09:** `slave_morality_detection` activates `master_slave_morality` thesis for `authority_001` (statistical_model=true, conformity_enforcement=true)

---

### §10.4 Schopenhauer (`schopenhauer.yaml`) — Mitleidsethik Schema Extensions

#### Scope
Extend `schopenhauer.yaml` with:
(a) `thesis_id` for all main theses,
(b) `suffering_minimization_bias` — a quantitative weight that shifts R4 SYNTHESIS
    toward verdicts that minimise aggregate suffering across all affected parties,
(c) `mitleid_intensity` — scales the compassion-ethics emphasis based on the number
    and severity of suffering entities in the dilemma.

#### Required New YAML Fields

```yaml
# schopenhauer.yaml additions
main_theses:
  world_as_will:
    thesis_id: "schopenhauer:world_as_will"
    activation_rounds: [1]
    token_budget: 100
  life_is_suffering:
    thesis_id: "schopenhauer:life_is_suffering"
    activation_rounds: [1, 3, 4]
    token_budget: 120
  compassion_ethics:
    thesis_id: "schopenhauer:compassion_ethics"
    activation_rounds: [1, 2, 4, 5]
    token_budget: 180
  principium_individuationis:
    thesis_id: "schopenhauer:principium_individuationis"
    activation_rounds: [3, 4]
    token_budget: 100
  aesthetic_contemplation:
    thesis_id: "schopenhauer:aesthetic_contemplation"
    activation_rounds: [5]
    token_budget: 60

# NEW top-level fields
suffering_minimization_bias: 0.8                 # ADD: [0.0–1.0]; high = strongly weights total suffering reduction
mitleid_intensity:                               # ADD: dynamic scaling
  base: 0.7
  scale_with_affected_count: true               # increases by 0.05 per additional affected person
  max: 1.0
regulatory_constraints:
  override_permitted: true
  applicable_regulations:
    - "Animal_Welfare_Act"                       # Schopenhauer extends ethics to all sentient beings
```

#### Test Strategy (NPE-10..12)
- **NPE-10:** Schopenhauer R4 SYNTHESIS for `medical_002` weights ventilator allocation by suffering-minimisation rather than life-years; output cites `schopenhauer:compassion_ethics` and `schopenhauer:life_is_suffering`
- **NPE-11:** `mitleid_intensity` scales from 0.7 (2 affected parties: trolley_001) to 0.85 (5 affected parties: medical_002 triage with multiple patients)
- **NPE-12:** Schopenhauer R1 for `authority_001` activates `principium_individuationis`: algorithm denies shared-will identity between judge and defendant; verdict = prohibit

---

### §10.5 Cross-School Fields: `ideological_bias_guard` and Context-Window Impact

#### `ideological_bias_guard`

When multiple schools with ideologically opposed frameworks (e.g., Marx + Nietzsche,
or Contractualism + Nietzsche) participate in the same discourse, there is a risk that
the discourse engine produces outputs that amplify rather than analyse the ideological
conflict. The `ideological_bias_guard` field at the `EthicalDiscourseEngine` configuration
level detects and flags this pattern.

```yaml
# EthicalDiscourseEngine session config (not per-school)
discourse:
  participating_schools: [marx, arendt, kant, nietzsche, contractualism, schopenhauer]
  ideological_bias_guard:
    enabled: true
    conflict_pairs:
      - [marx, nietzsche]          # Class-solidarity vs. individual excellence
      - [contractualism, nietzsche] # Universal fairness vs. elite values
    guard_action: "flag_in_r4_synthesis"  # enum: flag | suppress | mediate
    mediator_school: "contractualism"     # used when guard_action=mediate
  non_mainstream_activation_rounds:
    marx: [1, 2, 3, 4, 5]         # Full activation (primary schools for labor/authority)
    arendt: [1, 2, 3, 4, 5]       # Full activation
    nietzsche: [1, 4, 5]          # Reduced: R1 + synthesis only (outlier detector role)
    schopenhauer: [1, 4, 5]       # Reduced: primarily synthesis and meta-verdict
```

#### Context-Window Impact: 3-School → 6-School Expansion

As documented in Evidence Anchor E46, expanding from 3 to 6 schools doubles the
per-round token load. The following per-school compression policies are required
when deploying 6+ school debates on models with ≤ 32 K token limits:

| Round | 6-school strategy | Notes |
|---|---|---|
| R1 PRO | Full monocle per school, parallel injection | No compression needed; 6 × 800 = 4 800 tokens |
| R2 REBUTTAL | Each school receives only the 2 most relevant R1 opponents | Cross-school pair selection via `conflict_pairs` graph |
| R3 SURREBUTTAL | `headline_compression` (§9.3) mandatory; each prior round compressed to 3 bullet-points | Reduces R3 input from 27 000 to ~9 000 tokens |
| R4 SYNTHESIS | `principle_citations_only` mode; thesis_id references only, no full arguments | Reduces R4 from 36 000 to ~8 000 tokens |
| R5 META-VERDICT | Summary of R4 per school (≤ 300 tokens each); full transcript discarded | Reduces R5 from 18 000 to ~6 000 tokens |

**7B-model viability with these policies:** R1–R5 within 8 K token budget (total per-school input ≤ 7 800 tokens).

#### Performance Targets
- 6-school 5-round debate with full schema extensions: ≤ 90 s (Arch-B, GPT-4o, parallel school calls)
- 6-school 5-round debate with all compression policies: ≤ 45 s
- Memory overhead for 6-school session object: ≤ 12 MB

#### Security / Reliability
- `regulatory_constraints.override_permitted: false` is the default for deployed (non-research)
  ThemisDB instances. Research mode requires explicit config flag `discourse.research_mode: true`.
- `ideological_bias_guard.conflict_pairs` and `non_mainstream_activation_rounds` are
  configurable per deployment context but ship with the recommended defaults above.
- `exploitation_index`, `natality_novelty_score`, and `mitleid_intensity` output fields
  are subject to the same `PromptInjectionDetector` checks as all other YAML-derived content.
- Non-mainstream YAML `activation_conditions` tags are validated against a fixed enum
  (no free-text tags) to prevent injection of arbitrary activation conditions via user-supplied
  dilemma YAML.

#### Breaking Changes
- None: all new fields are optional. Existing 3-school debates without these fields
  continue to function unchanged.
- `regulatory_constraints` defaults to `{override_permitted: true, applicable_regulations: []}`
  if absent, preserving current unrestricted behaviour.
- The `thesis_id` extension is additive: existing YAML profiles without `thesis_id`
  continue to function; Φ measurement degrades to key-name matching (as described in E45).

#### Cross-References
- Evidence Paper §VI-A (YAML schema gaps identified), §VI-B (CW budget for 6 schools)
- Evidence Paper §VII-8 (aggregate alignment scores motivating `regulatory_constraints`)
- Evidence Paper E45, E46, E60, E64
- `src/prompt_engineering/FUTURE_ENHANCEMENTS.md §Multi-School Discourse-Level Prompt Coordination`


---

## 11. Flexible Ethics Routing System for >100 Schools (Target: Q3 2026)

> **Motivation:** As the philosophy YAML library grows beyond 16 profiles towards
> 100+, the existing pattern of loading all profiles into a single `std::map` at
> startup becomes a RAM and latency bottleneck. This section specifies the scalable
> routing architecture implemented in `EthicsProfileRegistry` + `EthicsSelectionRouter`.

---

### §11.1 EthicsProfileRegistry — Lazy-Loading Metadata Index

#### Scope
Replace the direct `PhilosophyLoader` map with a two-layer architecture:
(1) a lightweight metadata index always in RAM (~500 B/profile), and
(2) on-demand full-profile loading with an LRU cache (default capacity: 20 warm profiles).

#### Implemented Components
- `include/plugins/ethics_ai/ethics_profile_registry.h` — public interface
  - `EthicsProfileMeta` struct (school_id, name, taxonomy_class, tags, applicable_domains, yaml_path, description_snippet)
  - `EthicsIndexQuery` struct (taxonomy_class, tags, domains, max_results)
  - `IEthicsProfileRegistry` pure interface
- `src/ethics_ai/ethics_profile_registry.h` — private implementation class
- `src/ethics_ai/ethics_profile_registry.cpp` — concrete `EthicsProfileRegistry`
  - `rebuildIndex(directory)`: recursive filesystem scan, header-only YAML parse
  - `queryIndex(query)`: O(n) RAM scan with AND/ANY filter semantics
  - `getProfile(school_id)`: LRU cache hit → O(1); miss → `PhilosophyLoader::loadFromFile()`
  - `hasProfile(school_id)`: O(1) index lookup

#### Scaling Guarantees
- `queryIndex()` ≤ 2 ms for ≤ 1 000 profiles (O(n) RAM scan)
- `getProfile()` ≤ 100 ms cold, ≤ 1 ms warm (LRU cache)
- `rebuildIndex()` ≤ 500 ms for ≤ 200 profiles

#### Test Strategy (EPR-01..12)
New test file: `tests/test_ethics_profile_registry.cpp`

| ID | Scenario |
|---|---|
| EPR-01 | `rebuildIndex()` counts all YAML files correctly |
| EPR-02 | Empty query returns all profiles |
| EPR-03 | `taxonomy_class` filter returns exact match only |
| EPR-04 | Tag filter requires ALL tags (AND semantics) |
| EPR-05 | Domain filter requires ANY match |
| EPR-06 | `max_results` cap respected |
| EPR-07 | `getProfile()` returns error for unknown school_id |
| EPR-08 | `hasProfile()` returns false before, true after rebuild |
| EPR-09 | Cold-load returns valid PhilosophyProfile |
| EPR-10 | Second call is LRU cache hit |
| EPR-11 | Non-existent directory returns Status::Error |
| EPR-12 | LRU eviction at capacity=2 with 3 profiles |

---

### §11.2 EthicsSelectionRouter — Three-Stage Funnel

#### Scope
3-stage school selection funnel reducing >100 profiles to Top-N (default 5)
for any given dilemma context.

#### Implemented Components
- `include/plugins/ethics_ai/ethics_selection_router.h` — public interface
  - `RouterConfig`, `RouterCandidate`, `RouterResult` structs
  - `EthicsSelectionRouter` class with `route()` + `recordDecisionOutcome()`
- `src/ethics_ai/ethics_selection_router.cpp` — Pimpl implementation

#### Three-Stage Funnel

| Stage | Method | Latency target | Implementation |
|---|---|---|---|
| 1 | Tag/taxonomy filter | ≤ 2 ms | Loads `ethics_taxonomy.yaml`; maps domain + tags → taxonomy classes → school_ids |
| 2 | Semantic overlap | ≤ 20 ms | TF cosine similarity (STUB: real embedding model planned Q3 2026) |
| 3 | Precedent lookup | ≤ 50 ms | In-memory DC-score store (STUB: KG graph planned Q4 2026) |

Aggregation: `final_score = semantic * 0.40 + precedent_dc * 0.40 + taxonomy * 0.20`

#### STUB/SIMULATION Notes
Two documented stubs in `ethics_selection_router.cpp`:
1. **Stage-2 semantic overlap**: lightweight term-overlap proxy for embedding model
   (replaces ONNX all-mpnet-base-v2; removal plan: Q3 2026 when §7 embedding is complete)
2. **Stage-3 precedent store**: in-memory session-scoped map proxy for KnowledgeGraph
   `_themis_ethics_precedents` collection (removal plan: Q4 2026)

#### Test Strategy (ESR-01..10)
New test file: `tests/test_ethics_selection_router.cpp`

| ID | Scenario |
|---|---|
| ESR-01 | `route()` returns ≤ top_n results |
| ESR-02 | Stage-1 domain mapping produces candidates |
| ESR-03 | `regulatory_context=true` includes compliance schools |
| ESR-04 | Direct school_id tag forces inclusion |
| ESR-05 | Semantic scores in [0, 1] |
| ESR-06 | `recordDecisionOutcome()` raises precedent_dc |
| ESR-07 | Empty registry returns gracefully (no crash) |
| ESR-08 | Weight normalisation with non-unit config |
| ESR-09 | final_score always in [0, 1] |
| ESR-10 | 4-thread concurrent `route()` calls race-free |

---

### §11.3 Ethics Taxonomy Configuration

#### Scope
`config/ethics_ai/ethics_taxonomy.yaml` — two-level taxonomy mapping 12 ethics
classes (deontological, consequentialist, virtue, contractualist, care, discourse,
compliance, regulatory_authority, academic, cultural_religious, non_mainstream,
domain_specific) to school_ids, plus a `domain_class_mapping` section and
`always_include_when` flags.

#### New YAML Profile Requirements
All new philosophy YAML profiles MUST declare:
```yaml
taxonomy_class: <class>        # from ethics_taxonomy.yaml
tags: [...]                    # domain/topic tags for routing
applicable_domains: [...]      # dilemma domains
```

---

### §11.4 New YAML Profiles — Phase 1

Three new profiles implementing the full §10-style schema with thesis_ids,
activation_conditions, domain_overrides, regulatory_constraints, and convergence markers:

| Profile | Class | Domains | Key Theses |
|---|---|---|---|
| `behoerden_ethik.yaml` | regulatory_authority | ai_governance, data_protection, public_procurement | Rechtsstaatlichkeit, Gleichbehandlung, Transparenz, Menschliche Letztverantwortung |
| `universitaere_ethik.yaml` | academic | research, bioethics, ai_governance | Wissenschaftsfreiheit, Forschungsintegrität, Dual-Use, Open Science |
| `islamische_ethik.yaml` | cultural_religious | bioethics, end_of_life, medical | Maqasid al-Shariah, La Darar, Darura, Karama Insaniyya, Shura |

---

### §11.5 Planned Next Steps (Q3–Q4 2026)

- [ ] Replace Stage-2 STUB with ONNX embedding model (`all-mpnet-base-v2`) via `IEmbeddingProvider` (§7)
- [ ] Replace Stage-3 STUB with `KnowledgeGraphRetriever` on `_themis_ethics_precedents` graph collection
- [ ] `ReflectionTuner` feedback loop: `recordDecisionOutcome()` → adjust weight_semantic/weight_precedent
- [ ] Add `buddhistische_ethik.yaml`, `juedische_bioethik.yaml`, `christliche_sozialethik.yaml` profiles
- [ ] Ethics Community Graph via `ProcessCommunityDetector` on `ArgumentStore` co-occurrence
- [ ] Multi-hop decomposition: `MultiHopReasoner` for sub-question routing to Ethics clusters
- [ ] `OntologyAwareRetriever` ethics ontology (IS_A, SPECIALIZES, CONFLICTS_WITH, COMPLEMENTS)

---

## 12. Context-Window-Budget-Strategie: Komprimierung und Architekturelle Zerlegung (Target: Q3–Q4 2026)

### §12.0 Problemdefinition und Empirische Grundlage

#### §12.0.1 Token-Budget-Messung und Evidenz-Anker

Das Token-Wachstumsproblem im 5-Runden-Diskurs ist in
`research/DIALECTIC_EVIDENCE_PAPER.md` quantitativ belegt (Evidence Anchors E40–E41).
Die folgende Tabelle gibt das **gemessene Baseline-Budget** für 3-Schul-Diskurse
(Architecture B, `cl100k_base`-Tokenizer-Schätzung) und die **Hochrechnung für
4-Schul-Diskurse**:

```
Baseline (3 Schulen, Architecture B — gemessen in DIALECTIC_EVIDENCE_PAPER.md §V-B):

| Runde | Funktion             | Tokens (3 Schulen) | Quellen-Anker       |
|-------|----------------------|--------------------|---------------------|
| R1    | PRO                  | ~800               | Tab. S.3 Evidence   |
| R2    | REBUTTAL             | ~2 400             | Tab. S.3 Evidence   |
| R3    | SURREBUTTAL          | ~4 500             | E40-E41 [CWO=yes]   |
| R4    | SYNTHESIS (komprim.) | ~6 000             | Tab. S.3 Evidence   |
| R5    | META-VERDICT         | ~3 000             | Tab. S.3 Evidence   |

Hochrechnung 4 Schulen (N=4 → N-1=3 Gegner statt 2):

| Runde | 3 Schulen | 4 Schulen unkomprimiert | Δ       | Fits 8K? |
|-------|-----------|------------------------|---------|----------|
| R1    | ~800      | ~800                   | +0      | ✅        |
| R2    | ~2 400    | ~3 200                 | +800    | ✅        |
| R3    | ~4 500    | ~6 300                 | +1 800  | ❌ (7B)   |
| R4    | ~6 000    | ~9 000                 | +3 000  | ❌        |
| R5    | ~3 000    | ~3 500                 | +500    | ✅        |
```

**Evidence Anchor E40 (DIALECTIC_EVIDENCE_PAPER.md §V-B.5):**
> „Context window overflow is universal at Round 3 on 7B models. R3 accumulated
> 4 800 tokens — exceeds 7B default 8K limit. RECOMMENDATION: Add
> `prior_round_compression: "headline"` policy, activating at R3+."

**Evidence Anchor E41:**
> Dilemma `trolley_002` (Fat Man Variant): R3 SURREBUTTAL CWO = yes (7B) für alle
> drei Schulen. DC(R3) = 0.79 mean — noch gut. Aber: R3-Input musste auf
> Arch-B-Seite bereits auf 4 800 Token beschränkt werden, was R4-Kompression
> de facto erzwingt.

**Evidence Anchor E46 (FUTURE_ENHANCEMENTS.md §10.5):**
> 3-School → 6-School-Expansion verdoppelt den per-round token load. Per-school
> compression policies sind zwingend ab 6 Schulen für ≤ 32K-Modelle.

#### §12.0.2 Warum beide Strategien gleichrangig sind

Die zwei Strategien adressieren *verschiedene Ursachen* des Token-Wachstums
und sind damit komplementär — keine kann die andere ersetzen:

| Strategie | Adressiert | Risiko wenn allein eingesetzt | Referenz |
|---|---|---|---|
| **Komprimierung** | Große Token-Volumina durch Info-Dichte-Erhöhung | Informationsverlust wächst quadratisch mit Schulzahl | Xu et al. 2023 [R12], Li et al. 2023 [R13] |
| **Zerlegung** | O(N²) Wachstum durch Modularisierung | Kohärenzverlust bei übermäßiger Fragmentierung (DC↓) | Du et al. 2024 [R6], Chan et al. 2023 [R7] |

**Formale Begründung:** Sei `T(N, R)` das Token-Budget für Runde R mit N Schulen:

```
T(N, R) = T_monokel(N) + T_dilemma + T_prior(N, R)

Wobei:
  T_monokel(N)  = N × ~400 Token  (unabhängig von R; durch §12.1.1 komprimierbar)
  T_dilemma     = ~200 Token       (konstant)
  T_prior(N, R) = (N-1) × sum_{r=1}^{R-1} avg_arg_length(r)  [O(N²) für vollständige Injektion]

Komprimierung reduziert avg_arg_length(r) um Faktor k ∈ [0.20, 0.40] — begrenzt
durch Mindest-DC-Anforderung (DC ≥ 0.75 laut E29-E38).

Zerlegung reduziert (N-1) auf tournament_primary_k + ε (headline tokens) und
externalisiert sum_{r} durch EpisodicMemory(≤50 Token/Episode) — eliminiert
das O(N²)-Wachstum strukturell.

Nur die Kombination erreicht T(4, 3) ≤ 5 000 ≤ 8 192 mit DC ≥ 0.75.
```

#### §12.0.3 Forschungsstand (State of Research)

Der Umgang mit Context-Window-Limitierungen in LLM-Anwendungen ist ein aktives
Forschungsgebiet. Die für §12 relevanten Forschungsrichtungen und ihre Einordnung
in die ThemisDB-Architektur:

**A) Prompt-Komprimierung (Compression Track Fundament)**

- **LLMLingua** (Jiang et al., EMNLP 2023 [R11]): Token-Reduktion durch Budget-Controller,
  Iterative Token Classification, und Instruction Tuning auf komprimierten Prompts.
  LLMLingua erreicht 20× Komprimierung bei ≤ 5 % Accuracy-Verlust auf Standard-Benchmarks.
  *ThemisDB-Relevanz:* Direkte Grundlage für `PriorRoundCompressor`-Modi;
  `principle_citations_only` ist eine domain-spezifische Variante von LLMLingua's
  Key-Information-Extraction.

- **RECOMP** (Xu et al., ICLR 2024 [R12]): Zwei Kompressionsstufen — Extractive
  (Satz-Auswahl via Contriever-Scoring) und Abstractive (generative Zusammenfassung).
  Zeigt, dass Abstractive-Komprimierung DC-Verlust um 40–60 % gegenüber naivem Truncation
  reduziert. *ThemisDB-Relevanz:* `structured_summary`-Modus in `PriorRoundCompressor`
  folgt dem RECOMP-Abstractive-Ansatz; `principle_citations_only` ist Extractive-Komprimierung
  auf thesis_id-Ebene.

- **FILCO** (Wang et al., ACL 2024 [R13]): Filtering Context through Conditional Knowledge
  — selektiert nur Kontext, der konditional auf die aktuelle Query relevant ist.
  FILCO zeigt, dass selektive Injektion oft besser ist als komprimierte Volltext-Injektion.
  *ThemisDB-Relevanz:* §12.1.3 Selektive Gegner-Injektion via `CrossSchoolTensionResolver`
  implementiert das FILCO-Prinzip für Diskurs-Argumente.

- **Selective Context** (Li et al., arXiv 2023 [R14]): Token-Importance-Scoring via
  Self-Information (perplexity der eigenen Vorhersage); irrelevante Tokens werden vor
  LLM-Aufruf entfernt. 50 % Reduktion bei < 2 % Qualitätsverlust auf summarization tasks.
  *ThemisDB-Relevanz:* `principle_citations_only` ist semantisch äquivalent zu Selective
  Context, aber mit domain-spezifischem Schema (thesis_id-Verfahren statt
  perplexity-basierter Token-Selektion).

- **AutoCompressor** (Chevalier et al., EMNLP 2023 [R15]): Rekursive Zusammenfassung
  langer Dokumente in komprimierte "Summary Vectors". Zeigt, dass hierarchische
  Komprimierung den Kohärenzverlust bei langen Kontexten minimiert.
  *ThemisDB-Relevanz:* §12.2.4 REFLEXION-Episoden-Puffer folgt dem AutoCompressor-Prinzip
  für multi-round dialogische Kontexte.

**B) Architekturelle Zerlegung (Decomposition Track Fundament)**

- **Language Model Cascades** (Dohan et al., arXiv 2022 [R1]): Probabilistisches
  Framework für das Routing von Inferenzaufgaben zu Modellen unterschiedlicher Kapazität.
  Zeigt theoretisch, dass ein Cascade immer Pareto-dominierende Lösungen gegenüber
  uniformer Modellwahl erzeugt, wenn die Aufgaben-Schwierigkeit variiert.
  *ThemisDB-Relevanz:* §12.2.1 LLM-Cascade-Routing ist eine deterministische
  Instanz des LM-Cascade-Frameworks mit round_role als Routing-Prädikat.

- **FrugalGPT** (Chen et al., arXiv 2023 [R8]): Empirisches Cascade-Framework das
  zeigt, dass Ensemble-Routing von LLMs 98 % der GPT-4-Qualität bei 4× geringeren
  Kosten erreicht. Routing-Entscheidung basiert auf Confidence-Score des kleineren Modells.
  *ThemisDB-Relevanz:* Direkte Grundlage für die Kostenreduktions-Argumentation in §12.2.1;
  `ILlmCascadeRouter` ist eine vereinfachte, round-deterministische FrugalGPT-Instanz.

- **Multiagent Debate** (Du et al., ICML 2024 [R6]): N Agenten debattieren simultan;
  zeigt dass DC und Faktentreue mit Rundenanzahl steigen, aber Token-Overhead O(N²) wird.
  Schlägt "Delegate"-Modus vor: weniger wichtige Agenten hören zu, nehmen aber nicht
  vollständig teil. *ThemisDB-Relevanz:* §12.2.2 Tournament-Mode implementiert
  exakt den Delegate-Modus für SURREBUTTAL-Runden.

- **Reflexion** (Shinn et al., NeurIPS 2023 [R2]): Verbal Reinforcement Learning durch
  episodischen Puffer — Agenten speichern Selbstreflexionen über vergangene Entscheidungen
  als komprimierte Texte, nicht als vollständige Transkripte. Episoden sind ≤ 50 Token.
  *ThemisDB-Relevanz:* §12.2.4 ist die direkteste Umsetzung von Reflexion in einem
  Multi-School-Diskurs-Kontext. `ReflectionTuner::REFLEXION` (v1.5.0) ist die
  bestehende Infrastruktur.

- **MemGPT** (Packer et al., arXiv 2023 [R9]): OS-ähnliches Memory-Management für LLM-Agenten:
  Main Context (RAM), External Storage (Disk), Recall Storage (Episodic). Paging zwischen
  Ebenen. *ThemisDB-Relevanz:* §12.2.4 EpisodicMemoryEntry implementiert das MemGPT-Recall-
  Storage-Konzept für Diskurs-Schulen; `ArgumentStore` ist das External Storage.

- **ChatEval** (Chan et al., arXiv 2023 [R7]): Multi-Runden-Debatte von LLM-Evaluatoren
  mit strukturiertem Rollback wenn Kohärenz sinkt. Zeigt, dass DC-Monitoring während
  der Debatte die Endqualität signifikant verbessert.
  *ThemisDB-Relevanz:* §12.5 Sicherheits-Monitoring `PromptEngineeringMetrics::recordCompressionDelta()`
  ist die ThemisDB-Instanz von ChatEval's Rollback-Mechanismus.

- **DSPy** (Khattab et al., arXiv 2023 [R10]): Structured Prediction mit automatisch
  optimierten Prompts; TypedPredictor erzwingt strukturierte Outputs (ähnlich JSON Mode).
  *ThemisDB-Relevanz:* §12.2.3 Position-Abstract-Schema ist ein DSPy-TypedPredictor-
  äquivalentes Output-Schema; Enforcement über `output_schema` in `discourse_config.yaml`.

**C) Theoretische Einordnung der ThemisDB-Kombination**

Die Literatur behandelt Komprimierung und Zerlegung durchweg als getrennte Techniken.
Keine Arbeit kombiniert beide für **Multi-Schul-Ethik-Diskurse** mit:
(a) schul-deklarativen Thesen als semantischen Ankern,
(b) messbarer Discourse-Coherence als Qualitätsfunktion,
(c) YAML-deklarativer Konfiguration der Kompressions-/Zerlegungsstrategie.

Dies ist die **originäre wissenschaftliche Contribution** von §12: das erste dokumentierte
Framework, das beide Strategien kohärent in einem philosophischen Diskurssystem integriert
und über Budget-Profile für verschiedene Modellklassen (3B/7B/13B/70B) parametrisiert.

> **Cross-Referenzen:**
> - `research/DIALECTIC_EVIDENCE_PAPER.md §V-B.5` (E40–E41 — CWO-Messung, Baseline-Daten)
> - `research/DIALECTIC_EVIDENCE_PAPER.md §VI-B` (E46 — 6-Schul-Expansion)
> - `src/ethics_ai/FUTURE_ENHANCEMENTS.md §9.3` (`PriorRoundCompressor` — Kernkomponente §12.1.2)
> - `src/ethics_ai/FUTURE_ENHANCEMENTS.md §9.2` (`CrossSchoolTensionResolver` — §12.1.3)
> - `src/ethics_ai/FUTURE_ENHANCEMENTS.md §9.5` (`ConvergenceMarkerEngine` — §12.1.4 + §12.2.5)
> - `src/prompt_engineering/FUTURE_ENHANCEMENTS.md §Multi-School Discourse-Level Prompt Coordination` (MSD-01..10)
> - `include/prompt_engineering/context_window_manager.h` `selectThesesForRound()` (§9.1 ✅ implementiert)

---

### §12.1 Komprimierungsspur (Compression Track)

**Design-Prinzip:** Dieselbe semantische Substanz wird mit weniger Token dargestellt.
Keine Informationen werden weggelassen — sie werden in kompaktere Repräsentationsformen
überführt, die vom LLM dennoch vollständig auswertbar sind.

**Wissenschaftliche Grundlage:** LLMLingua [R11] zeigt, dass 20× Komprimierung
bei < 5 % Accuracy-Verlust erreichbar ist, wenn Schlüssel-Informationen (hier:
thesis_id-Referenzen + Verdikt) identifizierbar und erhalten werden. RECOMP [R12]
differenziert zwischen Extractive (Satzauswahl) und Abstractive (generative Summary)
Komprimierung; erstere ist geeignet, wenn strukturierte Ankerpunkte (thesis_ids) vorhanden
sind, letztere wenn Kohärenz über Satzgrenzen hinweg erhalten werden muss.

---

#### §12.1.1 Monokel-Budget-Reduktion via `activation_rounds` + `token_budget` ✅ IMPLEMENTIERT (2026-04-29)

> **Status:** `ContextWindowBudgetManager::selectThesesForRound()` + `PhilosophyThesis.token_budget`
> + `PhilosophyThesis.activation_rounds` + `PhilosophyThesis.round_role_weights` vollständig
> implementiert. Tests TBM-01..10 in `tests/test_thesis_budget_management.cpp`.

**Wissenschaftliche Grundlage:** Der Effekt entspricht dem **Key-Information-Extraction**-Schritt
in LLMLingua [R11, §3.2]: irrelevante Tokens werden nicht komprimiert, sondern als
strukturierte Headline-Referenz dargestellt. Der Vorteil gegenüber generativer Komprimierung:
vollständig deterministisch, kein LLM-Aufruf, keine Halluzinationsgefahr.

**Wirkung:** Die Monokel-Seite (system-prompt-Block mit Schulthesen) wird von ~800 Token
auf ~400–500 Token komprimiert, indem Thesen mit niedrigem `round_role_weights`-Wert für
die aktuelle Runde auf 15-Token-Headlines reduziert werden.

**DC-Verlust:** Null, da die reduzierten Thesen in der aktuellen Runde nach Evidenz E40–E41
nicht zur Argument-Generierung benötigt werden (ihre Inhalte sind bereits in früheren
Runden zitiert worden).

**Sofortige Aktivierung:** `kant.yaml`, `utilitarianism.yaml`, `contractualism.yaml`
und alle neu erstellten Schulprofile müssen die folgenden Felder für alle Hauptthesen
deklarieren:

```yaml
main_theses:
  - thesis_id: "<school>:<thesis_key>"   # snake_case, Pflicht für Φ-Messung
    token_budget: 180                    # max Token-Injektion für diese These
    activation_rounds: [1, 2, 3]        # R4/R5: automatisch Headline-Only
    round_role_weights:
      PRO: 1.0
      REBUTTAL: 0.9
      SURREBUTTAL: 0.8
      SYNTHESIS: 0.4     # Stark komprimiert — Kerne bereits debattiert
      META_VERDICT: 0.2  # Nur Referenz auf These, kein voller Text
```

**Empfohlene Schwellenwerte für 4-Schul-Betrieb:**

| Runde | `round_role_weights`-Schwelle | Injektionstiefe | Evidenz |
|---|---|---|---|
| R1 PRO | ≥ 0.8 → full, < 0.8 → headline | Kern-Thesen vollständig | E25–E32 (Φ hoch) |
| R2 REBUTTAL | ≥ 0.7 → full, < 0.7 → headline | Eine These weniger als R1 | E40 (DC=0.82) |
| R3 SURREBUTTAL | ≥ 0.8 → full, < 0.8 → headline | Nur direkt angesprochene Thesen | E40–E41 (CWO=yes) |
| R4 SYNTHESIS | ≥ 0.5 → full, < 0.5 → headline | Max. 2 Thesen vollständig | E42 (YAML gaps) |
| R5 META-VERDICT | Alle → headline | Keine vollständige Thesen-Injektion | E42 (CS 0.86) |

---

#### §12.1.2 `PriorRoundCompressor` — Komprimierung vorheriger Runden (→ §9.3)

> **Status:** Vollständig spezifiziert in §9.3. Implementierung: Target Q3 2026.
> Neue Datei: `include/ethics_ai/prior_round_compressor.h` +
> `src/ethics_ai/prior_round_compressor.cpp`.

**Wissenschaftliche Grundlage:**

Der `PriorRoundCompressor` implementiert drei Kompressionsstufen, die sich an
der RECOMP-Taxonomie [R12] orientieren:

- **`principle_citations_only`** (Extractive): Entspricht RECOMP-Extractive mit
  Sentence-Boundary = thesis_id-Referenz. Inhaltlich analog zu LLMLingua's
  [R11] "question-aware coarse-grained compression" auf Abschnitt-Ebene.
  Bewahrt die semantisch dichtesten Einheiten (PRINCIPLE CITATIONS-Blöcke,
  Verdikt-Zeilen) und verwirft den Elaborationstext. DC-Verlust nach RECOMP-
  Ergebnissen: 3–7 % bei 70–80 % Reduktion.

- **`structured_summary`** (Abstractive): Entspricht RECOMP-Abstractive.
  Erfordert einen LLM-Aufruf mit < 200 Token Input für die Zusammenfassung —
  im ThemisDB-Kontext auf dem `small`-Modell (§12.2.1 Cascade). Bewahrt
  Kausal- und Kontrastrstruktur des Arguments.

- **`headline`** (Extractive, maximal): Nur `thesis_id: name` + Verdikt.
  Analog zu LLMLingua's "token-level compression" bei maximal Budget-Druck.
  Nur wenn < 500 Token Restbudget verbleiben.

**Wirkung:** Argumenttexte aus vergangenen Runden (der Haupttreiber des Token-Wachstums
ab R3) werden vor der Injektion in `DiscoursePromptCoordinator` komprimiert.

**Drei Kompressions-Modi mit messbaren Kennzahlen:**

| Modus | Token-Reduktion | ΔDC (erwartet) | Grundlage | Aktivierung |
|---|---|---|---|---|
| `principle_citations_only` | ~75 % | ≤ −0.05 | RECOMP Extractive [R12] | Standard bei 4+ Schulen ab R3 |
| `structured_summary` | ~60 % | ≤ −0.08 | RECOMP Abstractive [R12] | 3 Schulen auf < 8 K-Modell ab R3 |
| `headline` | ~80 % | ≤ −0.15 | LLMLingua token-level [R11] | Nur wenn Budget kritisch (< 500 Tokens frei) |

**DC-Verlust-Messmethodik:** DC(compressed) wird als shared-token-overlap zwischen
dem komprimierten Prior-Kontext und dem generierten Argument berechnet — exakt die
Metric aus DIALECTIC_EVIDENCE_PAPER.md §Metrics. `ΔDC = DC(compressed) − DC(full)`
muss nach jeder Produktion gemessen und in `PromptEngineeringMetrics` geloggt werden
(§12.6 Sicherheit).

**Budget nach Kompression (4 Schulen, `principle_citations_only`):**

| Runde | Unkomprimiert | Nach `principle_citations_only` | Fits 8 K? |
|---|---|---|---|
| R3 SURREBUTTAL | ~6 300 Token | ~3 200 Token | ✅ |
| R4 SYNTHESIS | ~9 000 Token | ~3 800 Token | ✅ |
| R5 META-VERDICT | ~3 500 Token | ~3 500 Token (keine Komprimierung nötig) | ✅ |

**Pflicht-Konfiguration in `discourse_config.yaml` für 4+ Schulen:**

```yaml
discourse_rounds:
  - round: 3
    name: "SURREBUTTAL"
    compression_policy: "prior_round_summarization"
    prior_round_summarization:
      trigger_round: 3
      mode: "principle_citations_only"      # Standard für 4+ Schulen
      max_tokens_per_round: 300             # pro Schule und Runde
      preserve: ["principle_citations", "verdict"]
      cross_round_coherence_anchor: "thesis_ids"
  - round: 4
    name: "SYNTHESIS"
    compression_policy: "prior_round_summarization"
    prior_round_summarization:
      trigger_round: 4
      mode: "principle_citations_only"
      max_tokens_per_round: 200
```

**`PriorRoundCompressor` — vollständiges C++-Interface:**

```cpp
// include/ethics_ai/prior_round_compressor.h

enum class CompressionMode {
    PRINCIPLE_CITATIONS_ONLY,   // Extractive: thesis_id + verdict only (LLMLingua-ähnlich [R11])
    STRUCTURED_SUMMARY,         // Abstractive: generative summary via small-LLM (RECOMP [R12])
    HEADLINE                    // Ultra-sparse: only "[thesis_id: name]" tokens
};

struct CompressionConfig {
    CompressionMode  mode{CompressionMode::PRINCIPLE_CITATIONS_ONLY};
    int              trigger_round{3};           // erst ab dieser Runde komprimieren
    int              max_tokens_per_round{300};  // max Ausgabe-Tokens pro Schule/Runde
    bool             keep_thesis_id_anchors{true};
    bool             keep_verdict{true};
    std::string      coherence_anchor_field{"thesis_ids"};  // Anker für DC-Erhalt
};

struct CompressionResult {
    std::string compressed_text;
    int         original_tokens;
    int         compressed_tokens;
    float       compression_ratio;        // 0.0–1.0; < 0.30 → headline territory
    float       estimated_dc_loss;        // ΔDC-Schätzung (positiv = Verlust)
    bool        coherence_anchors_intact; // true wenn alle thesis_ids erhalten
};

class PriorRoundCompressor {
public:
    // Komprimiert die Argumente einer Runde für die Injektion in die nächste
    // Runde. Rückgabe: CompressionResult (nicht nur String, damit DC-Loss
    // trackbar ist — §12.6 Sicherheits-Anforderung).
    CompressionResult compressPriorRound(
        const std::vector<EthicalArgument>& round_arguments,
        const CompressionConfig& config,
        int current_round) const;

    // Baut den vollständigen Prior-Kontext aller vergangenen Runden
    // innerhalb des Token-Budgets, komprimiert ältere Runden stärker.
    // (Implementiert hierarchische Kompression: R1 < R2 < R3 in Dichte)
    std::string buildPriorContext(
        const std::vector<std::vector<EthicalArgument>>& all_rounds,
        const CompressionConfig& config,
        int current_round,
        int max_total_tokens) const;

    // Messung des DC-Verlusts nach erfolgter Kompression (für §12.6)
    float measureDcLoss(
        const std::string& original_arg,
        const std::string& compressed_arg) const;
};
```

---

#### §12.1.3 Selektive Gegner-Injektion via `CrossSchoolTensionResolver` (→ §9.2)

> **Status:** Vollständig spezifiziert in §9.2. Implementierung: Target Q3 2026.

**Wissenschaftliche Grundlage:** Das Prinzip entspricht FILCO [R13] —
"Filter Context through Conditional Knowledge". FILCO zeigt, dass selektive
Injektion von konditional relevantem Kontext (hier: Gegner-Argumente die direkt
auf eigene Thesen Bezug nehmen) besser ist als uniform-komprimierter Volltext-Kontext.
Für Diskurs-Argumente ist `rebuttal_cite_weight` das Äquivalent zu FILCO's
Conditional Relevance Score.

**Wirkung:** Statt alle Gegner-Argumente vollständig einzubetten, wählt
`CrossSchoolTensionResolver::getRelevantTensions()` nur die Thesen aus, die laut
`cross_school_tensions.rebuttal_cite_weight` ≥ 0.6 eine direkte Tension zur aktuellen
Schule aufweisen. Thesen mit niedrigerem Gewicht erscheinen als Headline-Token.

**Token-Einsparung R2 REBUTTAL (4 Schulen):**
- Ohne Selektion: 3 × ~600 Token Gegner-Argumente = ~1 800 Token
- Mit Selektion (top-2 pro Gegner, `rebuttal_cite_weight ≥ 0.6`): ~600 Token (−66 %)

**Konfiguration in Schulprofil-YAML:**

```yaml
cross_school_tensions:
  - own_thesis: "selbstzweck"
    opposing_school: "utilitarianism"
    opposing_thesis: "greatest_happiness"
    tension_type: "categorical_vs_aggregate"
    rebuttal_cite_weight: 0.9    # → wird in R2 vollständig eingebettet
  - own_thesis: "kategorischer_imperativ"
    opposing_school: "islamische_ethik"
    opposing_thesis: "maslaha"
    tension_type: "deontological_vs_welfare"
    rebuttal_cite_weight: 0.5    # → erscheint nur als Headline
```

**`CrossSchoolTensionResolver` — C++-Interface:**

```cpp
// include/ethics_ai/cross_school_tension_resolver.h

struct SchoolTension {
    std::string own_thesis_id;
    std::string opposing_school_id;
    std::string opposing_thesis_id;
    std::string tension_type;           // "categorical_vs_aggregate" etc.
    float       rebuttal_cite_weight;   // [0.0–1.0]; ≥ 0.6 → full injection
};

struct InjectionDecision {
    std::string school_id;
    std::string argument_content;       // vollständig oder leer wenn Headline
    std::string headline;               // immer befüllt: "[school:thesis_name]"
    bool        inject_full;            // true wenn rebuttal_cite_weight ≥ threshold
    float       relevance_score;        // für Logging + PromptEngineeringMetrics
};

class CrossSchoolTensionResolver {
public:
    // Liefert für jede teilnehmende Gegner-Schule die Injektions-Entscheidung
    std::vector<InjectionDecision> resolveOpponentInjections(
        const std::string&                         own_school_id,
        const std::vector<std::string>&            opponent_school_ids,
        const std::vector<EthicalArgument>&        opponent_round_args,
        const std::vector<SchoolTension>&          tensions,
        float                                      full_injection_threshold = 0.6f,
        int                                        max_full_injections = 2) const;

    // Lädt Tensions aus einem PhilosophyProfile (aus cross_school_tensions YAML-Block)
    std::vector<SchoolTension> loadTensions(
        const PhilosophyProfile& profile) const;
};
```

---

#### §12.1.4 Konvergenz-Matrix via `ConvergenceMarkerEngine` (→ §9.5)

> **Status:** Vollständig spezifiziert in §9.5. Implementierung: Target Q3 2026.

**Wissenschaftliche Grundlage:** Die Konvergenz-Matrix implementiert das Konzept
des **Structured State Representation** aus der Multi-Agent-Forschung (Du et al.
[R6], §5): anstatt vollständige Argument-Texte zu übergeben, wird der Zustand
des Diskurses als kompakte strukturierte Darstellung kodiert. Dies ist analog
zu Schachnotation vs. Brettbild — alle relevanten Informationen in einer Fraktion
des Speicherplatzes.

Die Evidenz aus DIALECTIC_EVIDENCE_PAPER.md §Dilemma 2 (trolley_002) belegt
(Evidence Anchor E37), dass das R4-SYNTHESIS-Modell vor allem auf
Konvergenz-/Divergenz-Informationen reagiert, nicht auf vollständige Argument-Retelling —
was die Matrix-Darstellung semantisch äquivalent macht.

**Wirkung:** In R4 SYNTHESIS wird keine vollständige Wiedergabe aller
Schulargumente eingebettet. Stattdessen erzeugt `ConvergenceMarkerEngine::buildConvergencePreamble()`
eine kompakte 4×4-Positions-Matrix (~200 Token) die angibt, welche Schul-Thesis-Paare
konvergieren oder divergieren:

```
[CONVERGENCE MATRIX — R4 SYNTHESIS]
kant:kategorischer_imperativ ↔ contractualism:reasonable_rejection: CO_PROHIBITIVE (do_not_push)
kant:kategorischer_imperativ ↔ utilitarianism:rule_utilitarianism: CONDITIONAL_CONVERGENT (policy_mode)
islamische_ethik:la_darar ↔ schopenhauer:compassion_ethics: CO_PROHIBITIVE (minimize_harm)
[PERSISTENT SPLITS]
utilitarianism:act_utilitarianism ↔ kant:rigorismus: IRREDUCIBLE (arithmetic_vs_categorical)
```

Diese Darstellung ersetzt ~3 600 Token (4 × ~900 Token vollständige Schulargumente) durch
~200 Token und bewahrt die für die SYNTHESIS entscheidende Inter-Schul-Struktur.

---

### §12.2 Architekturelle Zerlegungsspur (Decomposition Track)

**Design-Prinzip:** Der Diskurs wird in eigenständig lösbare Einheiten aufgeteilt, von
denen jede innerhalb des verfügbaren Context-Windows eines kleineren Modells lösbar ist.
Zwischen den Einheiten werden nur kompakte Zustandsrepräsentationen weitergegeben,
keine vollständigen Transkripte.

**Wissenschaftliche Grundlage:** Du et al. [R6] zeigen das O(N²)-Token-Problem bei
N Agenten; sie schlagen "Delegation" vor (Agenten mit niedrigerer Relevanz beobachten
nur). Shinn et al. [R2] zeigen, dass episodische Puffer die Kohärenz über viele
Runden erhalten, ohne quadratisches Token-Wachstum. Packer et al. [R9] (MemGPT)
formalisieren das als Hierarchisches Speicher-Management.

---

#### §12.2.1 LLM-Cascade-Routing

> **Status:** Konzept aus Dohan et al. 2022 [R1] und Chen et al. 2023 [R8],
> für ThemisDB-Diskursstruktur angepasst.
> Implementierung: Target Q3 2026. Integration in `DiscoursePromptCoordinator`.

**Wissenschaftliche Grundlage:**

- **Language Model Cascades** [R1]: Dohan et al. zeigen theoretisch, dass ein Cascade
  von Modellen unterschiedlicher Kapazität eine Pareto-dominante Lösung darstellt,
  wenn die Aufgaben-Schwierigkeit variiert (Satz 1 im Paper). R1 PRO ist
  "einfacher" als R4 SYNTHESIS: weniger Kontext, klar definierter Output.
  Deterministisches round_role-basiertes Routing ist eine gültige Instanz.

- **FrugalGPT** [R8]: Chen et al. zeigen empirisch auf 8 NLP-Benchmarks, dass
  Cascade-Routing 98 % der GPT-4-Qualität bei ~4× geringeren Kosten erreicht.
  Für ThemisDB bedeutet das: nur R4 SYNTHESIS benötigt ein Frontier-Modell
  (höchste Komplexität; schlechte Cascade-Entscheidungen hier sind irreversibel).

**Kern-Idee:** Nicht jede Diskursrunde erfordert das größte verfügbare Modell. Durch
Routing auf das minimal nötige Modell werden Latenz und Kosten reduziert, ohne die
Argumentqualität in den kritischen Runden zu beeinträchtigen.

**Empfohlenes Routing-Schema (konfigurierbar in `discourse_config.yaml`):**

```yaml
# discourse_config.yaml — cascade routing
llm_cascade:
  enabled: true
  round_model_map:
    PRO:          {model: "small",  min_context_k: 4}   # R1: einfach, Monokel reicht
    REBUTTAL:     {model: "medium", min_context_k: 8}   # R2: Gegner-Argument nötig
    SURREBUTTAL:  {model: "medium", min_context_k: 8}   # R3: kritische Runde, mittleres Modell
    SYNTHESIS:    {model: "large",  min_context_k: 16}  # R4: höchste Komplexität
    META_VERDICT: {model: "small",  min_context_k: 4}   # R5: nur komprimierte Summary
  model_aliases:
    small:  "llama-3-8b-instruct"           # 4K–8K context, niedrige Latenz
    medium: "mistral-7b-instruct-32k"       # 32K context, balanciert
    large:  "gpt-4o"                        # Frontier-Modell, nur für R4 SYNTHESIS
  fallback_policy: "escalate"              # bei Ausfall: nächsthöhere Stufe, nie stumm scheitern
```

**Erwartete Effizienzgewinne (4 Schulen, 5 Runden — nach FrugalGPT-Benchmarks [R8] hochgerechnet):**

| Runde | Ohne Cascade | Mit Cascade | Latenz-Reduktion | Kosten-Reduktion |
|---|---|---|---|---|
| R1 PRO | GPT-4o × 4 | small × 4 | ~80 % | ~95 % |
| R2 REBUTTAL | GPT-4o × 4 | medium × 4 | ~50 % | ~70 % |
| R3 SURREBUTTAL | GPT-4o × 4 | medium × 4 | ~50 % | ~70 % |
| R4 SYNTHESIS | GPT-4o × 1 | large × 1 | 0 % | 0 % |
| R5 META-VERDICT | GPT-4o × 4 | small × 4 | ~80 % | ~95 % |

**Gesamtkosten-Reduktion:** ~60 % (nur R4 nutzt Frontier-Modell).
**Qualitäts-Einbusse (erwartet nach FrugalGPT):** < 2 % auf DC-Metrik.

**Required Interfaces:**

```cpp
// include/ethics_ai/llm_cascade_router.h

enum class CascadeModelTier { SMALL, MEDIUM, LARGE };

struct CascadeRoutingConfig {
    std::map<std::string, CascadeModelTier> round_to_tier;  // "PRO" → SMALL, etc.
    std::map<CascadeModelTier, std::string> tier_to_model;  // SMALL → "llama-3-8b-instruct"
    std::map<CascadeModelTier, size_t>      tier_to_context_k; // SMALL → 4, etc.
    std::string                             fallback_policy{"escalate"};
    bool                                    enabled{true};
};

// ROUTING HINT: ollama-local
// Model: deepseek-coder-v2:16b (für C++-Boilerplate-Generierung)
// Reason: C++ class scaffolding

class ILlmCascadeRouter {
public:
    virtual ~ILlmCascadeRouter() = default;

    // Liefert das korrekte LLM-Backend für die aktuelle Diskursrunde
    // round_role: "PRO" | "REBUTTAL" | "SURREBUTTAL" | "SYNTHESIS" | "META_VERDICT"
    // estimated_prompt_tokens: Schätzung des Eingabe-Umfangs (von ContextWindowBudgetManager)
    // Throws: never — falls Modell unavailable → escalate to next tier (logged in AuditLogger)
    virtual std::shared_ptr<ILLMProvider>
        routeForRound(const std::string& round_role,
                      size_t estimated_prompt_tokens) const = 0;

    // Gibt ModelTokenBudget für die gegebene Runde zurück
    virtual ModelTokenBudget budgetForRound(const std::string& round_role) const = 0;

    // Liefert die zugewiesene Tier (für Logging/Observability)
    virtual CascadeModelTier tierForRound(const std::string& round_role) const noexcept = 0;
};

class LlmCascadeRouter : public ILlmCascadeRouter {
public:
    explicit LlmCascadeRouter(
        CascadeRoutingConfig config,
        std::map<std::string, std::shared_ptr<ILLMProvider>> provider_registry);

    std::shared_ptr<ILLMProvider> routeForRound(
        const std::string& round_role,
        size_t estimated_prompt_tokens) const override;

    ModelTokenBudget budgetForRound(const std::string& round_role) const override;
    CascadeModelTier tierForRound(const std::string& round_role) const noexcept override;

private:
    CascadeRoutingConfig                                      config_;
    std::map<std::string, std::shared_ptr<ILLMProvider>>     providers_;
    std::map<CascadeModelTier, std::shared_ptr<ILLMProvider>> tier_providers_;
};
```

---

#### §12.2.2 Sequential Tournament Mode für R3 SURREBUTTAL

> **Status:** Neu spezifiziert für §12.2. Implementierung: Target Q3 2026.
> Integration in `DiscoursePromptCoordinator::buildArgumentPrompt()` für
> `ArgumentType::SURREBUTTAL`.

**Wissenschaftliche Grundlage:**

Du et al. [R6] schlagen den "Delegate Mode" in Multiagent Debate vor: Agenten mit
geringer Relevanz für die aktuelle Runde "beobachten" (erhalten Headline-Information)
statt voll zu partizipieren. Chan et al. [R7] (ChatEval) zeigen, dass
Evaluationsqualität sich nicht verschlechtert, wenn sekundäre Agenten-Beiträge
zusammengefasst werden — der Primär-Opponent ist entscheidend.

**Problem:** Im Standard-R3-Format muss jede Schule auf *alle* Gegner-Rebuttals antworten
— das erzeugt O(N²) Token-Druck (N-1 vollständige Gegner-Argumenttexte).

**Lösung:** Nur der relevanteste Gegner (laut `CrossSchoolTensionResolver` höchster
`rebuttal_cite_weight`) wird vollständig im Kontext bereitgestellt. Alle anderen Gegner
werden als 15-Token-Headline injiziert.

**Token-Vergleich R3 (4 Schulen, N=3 Gegner pro Schule):**

```
Standard-Modus:  Schule A sieht R2(B), R2(C), R2(D) vollständig → 3 × ~600 = ~1800 Tokens
Tournament-Modus: Schule A sieht R2(B) vollständig + "[C]: [headline]" + "[D]: [headline]"
                 → ~600 + 30 = ~630 Tokens (−65 %)
```

**Konfiguration:**

```yaml
discourse_rounds:
  - round: 3
    name: "SURREBUTTAL"
    opponent_injection_mode: "tournament"   # enum: "full" | "tournament" | "headline_only"
    tournament_config:
      primary_opponent_count: 1             # Anzahl vollständig eingebetteter Gegner
      selection_criterion: "rebuttal_cite_weight"  # aus cross_school_tensions
      fallback_criterion: "final_score"     # aus EthicsSelectionRouter wenn tension unbekannt
      secondary_injection: "headline"       # alle anderen Gegner → headline
```

---

#### §12.2.3 Position-Abstract-Schema

> **Status:** Neu spezifiziert für §12.2. Implementierung: Target Q3 2026.
> Erweiterung des `discourse_config.yaml`-Schemas + `DiscourseRoundConfig`.

**Wissenschaftliche Grundlage:**

Khattab et al. [R10] (DSPy) zeigen, dass LLMs mit strukturierten Output-Schemata
(TypedPredictor) konsistentere, kompaktere Antworten produzieren als mit
Free-Form-Instruktionen. Das Position-Abstract-Schema ist ein DSPy-TypedPredictor-
äquivalentes Output-Schema speziell für philosophische Schul-Positionen.

Zusätzlich folgt es dem **Structured Chain-of-Thought Distillation**-Prinzip
(Wei et al. 2022 [PE-Ref. 4]): der Position-Abstract ist der destillierte CoT-Trace
des R2-Arguments in 100-Token-Form.

**Idee:** Nach jeder Runde (insbesondere R2) erstellt jede Schule einen strukturierten
**Position Abstract** (≤ 100 Token), der Verdict, Kern-Thesis-IDs und den stärksten
Einwand gegen den Hauptgegner zusammenfasst. Spätere Runden (R3–R5) operieren auf
diesen Abstracts statt auf vollständigen Argumenttexten, es sei denn, das vollständige
Argument wird durch Tournament-Selektion eingebettet.

**Vorteil:** Kontrollierbare, vorhersagbare Größe des Prior-Context; LLM erzeugt
strukturierten Output anstatt der `PriorRoundCompressor` heuristisch kürzen muss.

**Output-Schema-Erweiterung in `discourse_config.yaml`:**

```yaml
discourse_rounds:
  - round: 2
    name: "REBUTTAL"
    output_schema:
      verdict: "string"                       # "PROHIBIT" | "PERMIT" | "CONDITIONAL" | "ABSTAIN"
      confidence: "float[0.0,1.0]"
      core_thesis_ids: "list[string]"         # ≤ 3 thesis_ids die das Argument trägt
      primary_rebuttal_of: "string"           # thesis_id des Gegner-Arguments, das widerlegt wird
      position_abstract: "string"             # ≤ 100 Token — kompakte Positionszusammenfassung
    position_abstract_required: true          # Pflicht-Output; fehlt → SCHEMA_VIOLATION Error
```

**Injektionslogik in R3:**
- Wenn Schule B primärer Gegner (Tournament-Selektion): vollständiger R2(B)-Text
- Wenn Schule C/D sekundäre Gegner: `R2(C).position_abstract` (~100 Token statt ~600)

**Gesamteffekt kombiniert mit Tournament-Mode (4 Schulen, R3):**

```
Ohne Maßnahmen:                    ~6 300 Token  (Baseline)
Mit Tournament allein (§12.2.2):   ~4 500 Token  (−29 %)
Mit Tournament + Abstract (§12.2.3):~2 800 Token  (−56 %)
Mit zusätzlich §12.1.1 (Monokel):  ~2 200 Token  (−65 %)
Mit §12.1.2 + §12.1.3 (Vollpaket): ~1 900 Token  (−70 %)
```

**Required Interface-Erweiterung:**

```cpp
// Erweiterung in include/ethics_ai/discourse_engine.h

struct DiscourseRoundOutput {
    std::string       school_id;
    int               round_number;
    std::string       content;               // vollständiger Argumenttext
    std::string       verdict;               // "PROHIBIT" | "PERMIT" | "CONDITIONAL" | "ABSTAIN"
    float             confidence{0.0f};
    std::vector<std::string> core_thesis_ids; // ≤ 3 thesis_ids
    std::string       primary_rebuttal_of;   // thesis_id des rebutteten Gegner-Arguments
    std::string       position_abstract;     // ≤ 100 Tokens — DSPy TypedPredictor-äquivalent [R10]
    bool              schema_valid{false};   // true wenn alle Pflichtfelder vorhanden
};
```

---

#### §12.2.4 Multi-Agent-Memory-Externalisierung via `ReflectionTuner::REFLEXION` ✅ TEILIMPLEMENTIERT

> **Status:** `ReflectionTuner` implementiert (v1.5.0). `REFLEXION`-Strategie mit
> episodischem Puffer vorhanden. Erweiterung für Diskurs-Kontext: Target Q3 2026 via
> `DiscoursePromptCoordinator`.

**Wissenschaftliche Grundlage:**

- **Reflexion** [R2]: Shinn et al. zeigen, dass episodische Puffer (≤ 50 Token) die
  Ausgabe-Qualität über mehrere Episoden hinweg verbessern — äquivalent zu Verbal
  Reinforcement Learning, ohne Gewichts-Updates. Der Puffer speichert
  `(observation, action, reward)` in natürlicher Sprache. In ThemisDB:
  `(prior_arg, rebuttal, dc_score)`. Paper-Ergebnis: +23 % auf AlfWorld-Aufgaben
  gegenüber ReAct ohne episodischen Puffer.

- **MemGPT** [R9]: Packer et al. formalisieren das als OS-ähnliches Memory-Management
  für LLM-Agenten: Main Context (aktuelle Runde), Recall Storage (episodische
  Zusammenfassungen), External Storage (ArgumentStore). Paging: wenn Main Context
  voll, schreibe in Recall Storage (das ist §12.2.4 EpisodicMemoryEntry).

- **AutoCompressor** [R15]: Chevalier et al. zeigen, dass rekursive Zusammenfassung
  (`summarize(summarize(R1), R2) → summary_R1_R2`) den Kohärenzverlust minimiert,
  weil der Kontext hierarchisch aufgebaut wird. Der EpisodicMemoryEntry implementiert
  genau diesen Ansatz auf Schul-Paar-Ebene.

**Bestehende Infrastruktur (sofort nutzbar):**
- `ReflectionTuner::REFLEXION`-Strategie speichert `(prior_argument, rebuttal, dc_score)`
- `DiscoursePromptCoordinator` (PE §MSD-09) liest diesen Puffer und prepended Episoden-Feedback

**Erweiterung für Multi-Schul-Diskurs:**

```cpp
// DiscoursePromptCoordinator::buildArgumentPrompt() — R3+:
// Statt: embed full R1 + R2 opponent texts (~1 600 tokens)
// Neu:   embed episodic_memory_entry per school pair (~50 tokens each)
// Grundlage: MemGPT Recall Storage [R9] + Reflexion episodic buffer [R2]

struct EpisodicMemoryEntry {
    std::string school_id;
    int         from_round;
    // ≤ 50 tokens: "Verdict: X; Core: thesis_id1, thesis_id2; Rebutted: thesis_id3"
    // Entspricht AutoCompressor [R15] recursive summary auf Argument-Ebene
    std::string compressed_position;
    float       dc_score;               // Discourse Coherence aus vorheriger Runde
    std::string strongest_tension;      // thesis_id pair (eigene vs. Gegner)
};

// Speicherung nach R2 (Reflexion [R2] §3 — store episode after action):
reflectionTuner_.storeEpisode(school_id, round=2, position_abstract, dc_score);

// Lesen in R3 (MemGPT [R9] §4.1 — retrieve from Recall Storage):
auto episodes = reflectionTuner_.getEpisodesForSchool(school_id, max_episodes=3);
// → inject ~150 tokens statt ~1 600 tokens für 3-Schul-Prior-Kontext
```

**Token-Einsparung nach Reflexion/MemGPT-Prinzip:**
Pro Schule × Runde: ~1 600 Token (fulltext) → ~150 Token (3 Episoden à ≤ 50 Token) = **−91 %**.

---

#### §12.2.5 Strukturierte Positions-Matrix für R4 SYNTHESIS

> **Status:** Konzept basierend auf `ConvergenceMarkerEngine` (§9.5) und dem
> Structured State Representation-Ansatz aus Du et al. [R6].
> Target Q3 2026.

**Wissenschaftliche Grundlage:**

Du et al. [R6] (Multiagent Debate §5) schlagen "Agent State Summary" vor:
statt vollständige Argumenttexte zu übergeben, kodiere den Zustand als strukturiertes
Tupel `(position, confidence, core_reason)`. Die Synthesis-Aufgabe benötigt diese
Struktur explizit — vollständige Texte sind hier Overhead. ChatEval [R7] bestätigt:
Synthesis-Qualität korreliert stärker mit der Präsenz von Konvergenz/Divergenz-Struktur
als mit Argumenttextlänge (Table 3, ChatEval).

**Problem:** R4 SYNTHESIS ohne Komprimierung erfordert alle vorherigen Runden aller
Schulen (~9 000 Token bei 4 Schulen). Selbst nach `principle_citations_only`-Komprimierung
bleiben ~3 800 Token — knapp für 8 K-Modelle wenn der Synthesis-Output selbst
700 Token benötigt.

**Lösung:** Die SYNTHESIS-Runde erhält als primären Eingabe-Block eine
maschinell generierte **Positions-Matrix** statt der komprimierten Textblöcke.
Diese Matrix fasst alle Schul-Positionen als strukturierte Daten zusammen:

```
[POSITIONS-MATRIX — R4 SYNTHESIS INPUT]
Schule          | Verdict    | Confidence | Kern-Thesen (IDs)
kant            | PROHIBIT   | 0.91       | kategorischer_imperativ, selbstzweck, rigorismus
utilitarianism  | PERMIT     | 0.78       | greatest_happiness, impartiality, consequentialism
contractualism  | PROHIBIT   | 0.89       | reasonable_rejection, original_position
islamische_ethik| CONDITIONAL| 0.82       | la_darar, maslaha, darura

[KONVERGENZ-PAARE]
kant + contractualism: CO_PROHIBITIVE @ do_not_push (divergent_grounding)
kant + islamische_ethik: CONDITIONAL_CONVERGENT @ harm_prevention (policy_mode)

[IRREDUZIBLER SPLIT]
utilitarianism ↔ kant: arithmetic_vs_categorical (persistent across all rounds)

SYNTHESIS-AUFGABE: Identifiziere übergreifende Konsenspunkte und benenne
die unauflösbaren Spaltungen mit je einem thesis_id-Referenzpaar.
```

**Token-Verbrauch dieser Matrix:** ~250 Token (vs. ~3 800 Token komprimierter Text)
= **weitere 93 % Reduktion** auf der Prior-Context-Seite von R4.

**R4 Gesamt-Budget nach Positions-Matrix + Komprimierung (4 Schulen, 8 K-Modell):**
- System-Prompt (SYNTHESIS-Instruktion): ~400 Token
- Positions-Matrix: ~250 Token
- Monokel (Thesen als Headlines): ~200 Token
- Completion-Reserve: ~700 Token
- **Gesamt: ~1 550 Token** — paßt komfortabel auf jedes 4 K-Modell

**Required Interfaces:**

```cpp
// include/ethics_ai/synthesis_matrix_builder.h (neue Datei)
// Grundlage: Du et al. [R6] §5 "Agent State Summary"

struct SchoolPositionSummary {
    std::string school_id;
    std::string verdict;                        // "PROHIBIT" | "PERMIT" | "CONDITIONAL"
    float       confidence;
    std::vector<std::string> core_thesis_ids;  // ≤ 3 thesis_ids
};

class SynthesisMatrixBuilder {
public:
    // Erstellt die kompakte Positions-Matrix aus R1–R3-Outputs aller Schulen.
    // max_tokens: Hard-Cap; überschreitung erzeugt WARN + weiteres Kürzen der Kern-Thesen.
    std::string buildMatrix(
        const std::vector<SchoolPositionSummary>& positions,
        const std::vector<ConvergenceMarker>& convergences,
        int max_tokens = 300) const;

    // Extrahiert SchoolPositionSummary aus DiscourseRoundOutput (liest position_abstract)
    SchoolPositionSummary extractSummary(
        const DiscourseRoundOutput& round_output) const;

    // Validiert thesis_id-Format und verdict-Enum — throws SchemaValidationError
    // wenn Felder ungültig (Sicherheits-Anforderung §12.6)
    void validateSummary(const SchoolPositionSummary& summary) const;
};
```

---

### §12.3 Kombinierte Budget-Profile nach Modellklasse

Die folgende Tabelle definiert die empfohlene Kombination beider Spuren für jede
Modellklasse. Alle Profile beziehen sich auf einen 4-Schul-5-Runden-Diskurs.

| Profil | Modell | Context | Komprimierungsspur | Zerlegungsspur | 7B-viable? |
|---|---|---|---|---|---|
| `micro` | 3B / 4 K | 4 096 Token | §12.1.1 (Monokel-Reduktion) + §12.1.2 (`headline`) | §12.2.1 (Cascade: `small`) + §12.2.5 (Positions-Matrix) + §12.2.3 (Position Abstract) | ⚠️ nur R1+R5 vollständig |
| `standard` | 7B / 8 K | 8 192 Token | §12.1.1 + §12.1.2 (`principle_citations_only`) + §12.1.3 (Selektiv) | §12.2.2 (Tournament) + §12.2.3 (Abstract) + §12.2.4 (REFLEXION) | ✅ alle 5 Runden |
| `extended` | 13B / 32 K | 32 768 Token | §12.1.1 + §12.1.2 (`structured_summary`) | §12.2.1 (Cascade: R4 → large) | ✅ komfortabel |
| `frontier` | 70B+ / 128 K | 128 K Token | §12.1.1 optional | Kein Cascade nötig | ✅ kein CW-Problem |

**Konfigurationsdatei:** `config/ethics_ai/model_budget_profiles.yaml` (neu, Target Q3 2026)

```yaml
# config/ethics_ai/model_budget_profiles.yaml
profiles:
  - id: "micro"
    model_class: "3B"
    max_context_tokens: 4096
    compression:
      monocle_reduction: true
      prior_round_mode: "headline"
      selective_opponent: false
      convergence_matrix: true
    decomposition:
      cascade_enabled: true
      tournament_mode: false
      position_abstract: true
      reflexion_memory: true
      synthesis_matrix: true

  - id: "standard"
    model_class: "7B"
    max_context_tokens: 8192
    compression:
      monocle_reduction: true
      prior_round_mode: "principle_citations_only"
      selective_opponent: true
      convergence_matrix: true
    decomposition:
      cascade_enabled: false       # Optional: cascade für R4 empfohlen
      tournament_mode: true
      position_abstract: true
      reflexion_memory: true
      synthesis_matrix: false      # principle_citations_only reicht für 8K

  - id: "extended"
    model_class: "13B"
    max_context_tokens: 32768
    compression:
      monocle_reduction: true
      prior_round_mode: "structured_summary"
      selective_opponent: false
      convergence_matrix: false
    decomposition:
      cascade_enabled: true        # R4 → large; alle anderen → medium
      tournament_mode: false
      position_abstract: false
      reflexion_memory: false
      synthesis_matrix: false
```

---

### §12.4 Test-Strategie (CWB-01..20)

**Neue Testdatei:** `tests/ethics_ai/test_context_window_budget_strategy.cpp`

**Vorbedingungen:** Alle Tests laufen ohne echten LLM-Aufruf (Mock-Provider).
Token-Zählung via `CharDivisionCounter` (gemessene Abweichung ≤ 15 % von echtem Tokenizer —
akzeptabel für Budget-Entscheidungen laut DIALECTIC_EVIDENCE_PAPER.md §Metrics).

#### Komprimierungsspur Tests (CWB-01..09)

| Test-ID | Komponente | Szenario | Akzeptanzkriterium |
|---|---|---|---|
| `CWB-01` | `ContextWindowBudgetManager` | 4-Schul-R3, `standard`-Profil | `fits() == true`; Gesamt-Tokens ≤ 8 000 |
| `CWB-02` | `PriorRoundCompressor` | `principle_citations_only` auf 3-Argument-Runde (600 Token Input) | Output ≤ 300 Tokens; alle `PRINCIPLE CITATIONS:`-Blöcke verbatim erhalten; `coherence_anchors_intact == true` |
| `CWB-03` | `PriorRoundCompressor` | `headline`-Modus auf 3-Argument-Runde | Jede These exakt `"[thesis_id: name]"` Format (Regex-Check); Gesamt ≤ 100 Tokens |
| `CWB-04` | `PriorRoundCompressor` | `structured_summary`-Modus | Output enthält Verdikt-Wort; keine thesis_id-IDs verloren; ≤ 240 Tokens; `estimated_dc_loss ≤ 0.08` |
| `CWB-05` | `ContextWindowBudgetManager` | Monokel R5: alle Thesen → Headline | `selectThesesForRound(round=5)` liefert nur `is_full=false`-Einträge bei `round_role_weights.META_VERDICT < 0.3` |
| `CWB-06` | `CrossSchoolTensionResolver` | R2 REBUTTAL, 4 Schulen, 3 Tensions mit weights [0.9, 0.6, 0.4] | `resolveOpponentInjections()`: 2 full, 1 headline; Token-Total ≤ 700 |
| `CWB-07` | `CrossSchoolTensionResolver` | Keine Tensions definiert (`cross_school_tensions` leer) | Fallback auf `EthicsSelectionRouter.final_score` — kein Crash; ≥ 1 full injection |
| `CWB-08` | `ConvergenceMarkerEngine` | 4 Schul-Positionen + 2 Konvergenz-Paare + 1 Split | `buildConvergencePreamble()` ≤ 250 Tokens; alle 4 Schulen in Matrix; 2 CO_PROHIBITIVE-Zeilen |
| `CWB-09` | `PriorRoundCompressor` | `measureDcLoss(original, compressed)` Messung | DC-Loss-Wert liegt in [0.0, 0.20]; Wert für `principle_citations_only` < Wert für `headline` |

#### Architekturelle Zerlegungsspur Tests (CWB-10..17)

| Test-ID | Komponente | Szenario | Akzeptanzkriterium |
|---|---|---|---|
| `CWB-10` | `LlmCascadeRouter` | Round R1 PRO → `small`, R4 SYNTHESIS → `large` | `routeForRound("PRO")` liefert small-Provider; `routeForRound("SYNTHESIS")` liefert large-Provider |
| `CWB-11` | `LlmCascadeRouter` | `cascade_enabled=false` | Alle Runden liefern denselben Provider |
| `CWB-12` | `LlmCascadeRouter` | `small`-Modell markiert als unavailable | `routeForRound("PRO")` eskaliert auf `medium`; kein Crash; `WARN` in AuditLogger |
| `CWB-13` | `DiscoursePromptCoordinator` | Tournament-Mode R3, 4 Schulen | Nur 1 vollständiger Gegner-Text in Prompt; 2 als `"[school: headline]"`; Token-Total ≤ 5 000 |
| `CWB-14` | `DiscourseRoundOutput` | `position_abstract_required=true`, leeres `position_abstract` | Status `SCHEMA_VIOLATION`; kein Crash; Fehlermeldung enthält `"position_abstract"` |
| `CWB-15` | `DiscoursePromptCoordinator` | Position-Abstract-Injektion R3 (sekundäre Gegner) | Sekundäre Gegner-Injektion = `position_abstract`-Feld (≤ 100 Tokens); kein Volltext |
| `CWB-16` | `ReflectionTuner` + `DiscoursePromptCoordinator` | REFLEXION-Speicherung nach R2 | `getEpisodesForSchool(school_id, 3)` liefert 1 Eintrag; `compressed_position.length() ≤ 50` Tokens |
| `CWB-17` | `ReflectionTuner` + `DiscoursePromptCoordinator` | REFLEXION-Puffer R3-Injektion, 3 Schulen | 3 Episoden × ≤ 50 Tokens = ≤ 150 Tokens total in Prompt injiziert |

#### End-to-End + Regressions-Tests (CWB-18..20)

| Test-ID | Profil | Szenario | Akzeptanzkriterium |
|---|---|---|---|
| `CWB-18` | `micro` (3B/4K) | 4 Schulen, 5 Runden, Mock-LLM | Alle 5 Runden abgeschlossen; Peak-Token ≤ 4 000; kein `Status::Error` |
| `CWB-19` | `standard` (7B/8K) | 4 Schulen, 5 Runden, Mock-LLM | Peak-Token ≤ 8 000; `compression_ratio ≤ 0.35` für R3–R4; `ΔDC ≤ 0.10` laut `measureDcLoss()` |
| `CWB-20` | Backward-Kompatibilität | 3-Schul-Debatte ohne §12-Konfiguration | TBM-01..10, DRE-01..05, PRC-01..06 weiterhin grün; keine Regression in vorhandenen Tests |

**CMake-Target:** `test_context_window_budget_strategy_focused`

```cmake
# tests/CMakeLists.txt (Auszug)
add_executable(test_context_window_budget_strategy_focused
    tests/ethics_ai/test_context_window_budget_strategy.cpp
    src/ethics_ai/prior_round_compressor.cpp
    src/ethics_ai/cross_school_tension_resolver.cpp
    src/ethics_ai/convergence_marker_engine.cpp
    src/ethics_ai/llm_cascade_router.cpp
    src/ethics_ai/synthesis_matrix_builder.cpp
    src/prompt_engineering/context_window_manager.cpp
    src/prompt_engineering/reflection_tuner.cpp
    src/ethics_ai/discourse_prompt_coordinator.cpp
)
target_compile_definitions(test_context_window_budget_strategy_focused
    PRIVATE THEMIS_PLUGIN_ETHICS_AI THEMIS_MOCK_LLM_PROVIDER)
gtest_add_tests(TARGET test_context_window_budget_strategy_focused
    TEST_LIST CWB_tests)
```

---

### §12.5 Performance-Ziele

| Metrik | Ziel | Messmethode | Grundlage |
|---|---|---|---|
| `PriorRoundCompressor::compressPriorRound()` | ≤ 5 ms für ≤ 3 Argumente (600 Token je) | CPU-only, kein LLM | RECOMP Extractive latency [R12] |
| `PriorRoundCompressor::buildPriorContext()` | ≤ 10 ms für R1–R3 (9 Argumente) | CPU-only | |
| `SynthesisMatrixBuilder::buildMatrix()` | ≤ 2 ms für ≤ 6 Schulen | CPU-only | Du et al. [R6] State Summary latency |
| `CrossSchoolTensionResolver::resolveOpponentInjections()` | ≤ 1 ms für ≤ 10 Tensions | O(n) Scan | |
| `ILlmCascadeRouter::routeForRound()` | ≤ 0.1 ms | std::map Lookup | FrugalGPT Overhead [R8] |
| `EpisodicMemoryEntry` Schreibvorgang | ≤ 0.1 ms | In-Memory-Ringpuffer | Reflexion [R2] benchmark |
| `EpisodicMemoryEntry` Lesezugriff (3 Einträge) | ≤ 0.1 ms | | |
| `buildArgumentPrompt()` Gesamtlatenz (`standard`-Profil) | ≤ 5 ms | Ohne LLM-Aufruf | PE-MSD-Ziel (MSD-07) |
| Peak-Tokens 4-Schul-R3 (`standard`-Profil) | ≤ 5 000 Token | `ContextWindowBudgetManager` | Budget-Rechnung §12.0.1 |
| Peak-Tokens 4-Schul-R4 (`standard`-Profil) | ≤ 3 800 Token | `ContextWindowBudgetManager` | Budget-Rechnung §12.0.1 |
| Peak-Tokens 4-Schul-R4 mit Positions-Matrix | ≤ 1 600 Token | `ContextWindowBudgetManager` | §12.2.5 Derivation |
| ΔDC `standard`-Profil vs. unkomprimiert | ≤ 0.10 | `measureDcLoss()` akkumuliert | RECOMP Benchmark [R12]; E40-E41 |

---

### §12.6 Sicherheit / Zuverlässigkeit

- **Compression-Loss-Monitoring:** `PromptEngineeringMetrics::recordCompressionDelta(dc_before, dc_after)`
  (PE-Subsystem) muss nach jeder komprimierten Runde aufgerufen werden. Sinkt DC um > 0.15 in einer
  einzelnen Runde, wird ein `WARN`-Ereignis in `AuditLogger` emittiert. Ab ΔDC > 0.20:
  automatisches Fallback auf `structured_summary`-Modus (eine Stufe weniger aggressiv).
  Begründung: ChatEval [R7] zeigt, dass ΔDC > 0.20 zur Kohärenz-Degradation in Folgerunden führt.

- **Cascade-Fallback:** Wenn das Routing-Modell (`small`/`medium`) für die aktuelle
  Runde nicht verfügbar ist, MUSS `ILlmCascadeRouter` gemäß `fallback_policy: "escalate"`
  auf das nächste verfügbare Tier eskalieren. Nie schweigend scheitern. Jede Eskalation
  wird in `AuditLogger` geloggt (SecurityEventType-Erweiterung: `INFERENCE_FALLBACK`).
  Grundlage: FrugalGPT [R8] §4.3 Robustness Guidelines.

- **Position-Abstract-Validation:** Der `position_abstract`-Feldinhalt MUSS durch
  `PromptInjectionDetector::sanitize()` laufen, bevor er in R3-Prompts eingebettet wird —
  der Abstract enthält LLM-generierten Text aus vorherigen Runden und ist damit ein
  potentielles Injection-Vehikel. Grundlage: `STUB_INVENTORY.md §PromptInjectionDetector`
  (bestehende Infrastruktur).

- **Episodischer Puffer:** Der `ReflectionTuner`-Ringpuffer ist session-scoped und
  nicht persistent. Er darf keine personenbezogenen Daten (Dilemma-Texte mit PII) ohne
  vorherigen `utils/pii_detector.cpp`-Durchlauf enthalten. Grundlage: DSGVO Art. 5(1)(e)
  (Datensparsamkeit bei LLM-Eingaben); MemGPT [R9] §5 Security Considerations.

- **Positions-Matrix-Validierung:** `SynthesisMatrixBuilder::validateSummary()` prüft
  `verdict` gegen Enum (`PROHIBIT | PERMIT | CONDITIONAL | ABSTAIN`) und `core_thesis_ids`
  gegen Pattern `^[a-z_]+:[a-z_]+$`. Freitext-Felder sind unzulässig. Verletzung →
  `SchemaValidationError` (kein Silent-Failure).

- **DC-Verlust-Schwellenwert-Kalibrierung:** Die ΔDC-Grenzwerte (≤ −0.05, ≤ −0.08,
  ≤ −0.15) basieren auf RECOMP [R12] Benchmark-Ergebnissen für summarization tasks.
  Sie MÜSSEN nach der ersten Produktions-Deployment auf echten ThemisDB-Diskursen
  kalibriert werden (A/B-Test: komprimiert vs. unkomprimiert auf N ≥ 50 Diskursen).
  Die kalibrierten Werte ersetzen die RECOMP-abgeleiteten Defaults und werden in
  `config/ethics_ai/model_budget_profiles.yaml` hinterlegt.

---

### §12.7 Breaking Changes

Keine. Alle §12-Komponenten sind optional und greifen nur, wenn das jeweilige
Konfigurationsfeld in `discourse_config.yaml` oder `model_budget_profiles.yaml` gesetzt ist.

| Komponente | Backward-Compat | Aktivierungsbedingung |
|---|---|---|
| `PriorRoundCompressor` | ✅ | `compression_policy:` in `discourse_config.yaml` |
| `SynthesisMatrixBuilder` | ✅ | `synthesis_matrix: true` in Budget-Profil |
| `ILlmCascadeRouter` | ✅ | `llm_cascade.enabled: true` in `discourse_config.yaml` |
| Tournament-Mode | ✅ | `opponent_injection_mode: "tournament"` |
| Position-Abstract-Schema | ✅ | `position_abstract_required: true` in Runden-Konfiguration |
| REFLEXION-Episoden | ✅ | `reflexion_memory: true` in Budget-Profil |
| `DiscourseRoundOutput.position_abstract` | ✅ | Additive Feld; default `""` + `schema_valid=false` |
| `DiscourseRoundOutput.core_thesis_ids` | ✅ | Additive Feld; default `[]` |

---

### §12.8 Vollständige wissenschaftliche Referenzliste

Die folgenden Referenzen bilden die wissenschaftliche Grundlage aller §12-Komponenten.
Interne Bezeichnungen `[R1]`–`[R15]` werden in §12.0–§12.6 verwendet.

#### Kernreferenzen: Architekturelle Zerlegung

**[R1]** D. Dohan, A. Rubin, C. Raffel, M. Roberts, J. Shlens, D. Sculley,
    „Language Model Cascades,"
    *arXiv preprint*, arXiv:2207.10342, Jul. 2022.
    → Grundlage §12.2.1 LLM-Cascade-Routing: probabilistisches Framework, Pareto-Dominanz,
    Routing-Prädikate.
    DOI/URL: https://arxiv.org/abs/2207.10342

**[R2]** N. Shinn, F. Cassano, A. Gopinath, K. Narasimhan, S. Yao,
    „Reflexion: Language Agents with Verbal Reinforcement Learning,"
    in *Advances in Neural Information Processing Systems (NeurIPS)*, vol. 36, 2023.
    → Grundlage §12.2.4 Multi-Agent-Memory-Externalisierung: episodischer Puffer,
    Verbal RL, +23 % auf AlfWorld.
    DOI/URL: https://arxiv.org/abs/2303.11366

**[R6]** Y. Du, S. Li, A. Torralba, J. B. Tenenbaum, I. Mordatch,
    „Improving Factuality and Reasoning in Language Models through Multiagent Debate,"
    in *Proc. International Conference on Machine Learning (ICML)*, 2024.
    → Grundlage §12.2.2 Tournament-Mode: O(N²)-Problem, Delegate-Modus, Agent-State-Summary.
    DOI/URL: https://arxiv.org/abs/2305.14325

**[R7]** C.-C. Chan, W. Chen, Y. Su, J. Yu, W. Qian, Y. Zhu, H. Yu, Z. Xue,
    „ChatEval: Towards Better LLM-based Evaluators through Multi-Agent Debate,"
    *arXiv preprint*, arXiv:2308.07201, 2023.
    → Grundlage §12.2.2 Tournament-Mode (Evaluator-Qualität bei Agent-Delegation):
    Kohärenz-Rollback-Mechanismus, Table 3.
    DOI/URL: https://arxiv.org/abs/2308.07201

**[R8]** L. Chen, M. Zaharia, J. Zou,
    „FrugalGPT: How to Use Large Language Models While Reducing Cost and Improving Performance,"
    *arXiv preprint*, arXiv:2305.05176, 2023.
    → Grundlage §12.2.1 Cascade: 98 % GPT-4-Qualität bei 4× geringeren Kosten;
    Fallback-Policy, Robustness §4.3.
    DOI/URL: https://arxiv.org/abs/2305.05176

**[R9]** C. Packer, S. Fang, S. G. Patil, K. Nguyen, A. Ghodsi, J. E. Gonzalez,
    „MemGPT: Towards LLMs as Operating Systems,"
    *arXiv preprint*, arXiv:2310.08560, 2023.
    → Grundlage §12.2.4 Memory-Externalisierung: Main Context / Recall Storage / External Storage,
    Paging, Security §5.
    DOI/URL: https://arxiv.org/abs/2310.08560

**[R10]** O. Khattab, A. Singhvi, P. Maheshwari, Z. Zhang, K. Santhanam, S. Vardhamanan,
    S. Haq, A. Sharma, T. T. Joshi, H. Moazam, H. Miller, M. Zaharia, C. Potts,
    „DSPy: Compiling Declarative Language Model Calls into Self-Improving Pipelines,"
    in *Proc. ICLR*, 2024.
    → Grundlage §12.2.3 Position-Abstract-Schema: TypedPredictor, strukturiertes Output-Schema,
    Schema-Enforcement.
    DOI/URL: https://arxiv.org/abs/2310.03714

#### Kernreferenzen: Komprimierung

**[R11]** H. Jiang, Q. Wu, X. Luo, D. Li, C.-Y. Lin, Y. Yang, L. Qiu,
    „LLMLingua: Compressing Prompts for Accelerated Inference of Large Language Models,"
    in *Proc. EMNLP*, 2023.
    → Grundlage §12.1.2 `PriorRoundCompressor`: Budget-Controller, Iterative Token Classification,
    20× Komprimierung bei < 5 % Qualitätsverlust.
    DOI/URL: https://arxiv.org/abs/2310.05736

**[R12]** F. Xu, W. Shi, E. Choi,
    „RECOMP: Improving Retrieval-Augmented LMs with Compression and Selective Augmentation,"
    in *Proc. ICLR*, 2024.
    → Grundlage §12.1.2 Extractive vs. Abstractive Komprimierung: Contriever-Scoring,
    DC-Verlust-Quantifizierung, principle_citations_only ≡ Extractive-RECOMP.
    DOI/URL: https://arxiv.org/abs/2310.04408

**[R13]** Z. Wang, J. Araki, Z. Jiang, M. R. Parvez, G. Neubig,
    „Learning to Filter Context for Retrieval-Augmented Generation,"
    in *Proc. ACL*, 2024.
    → Grundlage §12.1.3 Selektive Gegner-Injektion: Conditional Knowledge Filtering,
    selektive Injektion > komprimierter Volltext-Injektion.
    DOI/URL: https://arxiv.org/abs/2311.08377

**[R14]** Y. Li, S. Bubeck, R. Eldan, A. Del Giorno, S. Gunasekar, Y. T. Lee,
    „Making Large Language Models Better Reasoners with Step-Aware Verifier,"
    *Note:* Hier referenziert als Selective Context (Li et al. 2023):
    X. Li, C. Liu, J. Xu, S. Song, M. Shen,
    „Compressing Context to Enhance Inference Efficiency of Large Language Models,"
    in *Proc. EMNLP*, 2023.
    → Grundlage §12.1.2: Self-Information Token-Importance-Scoring, 50 % Reduktion
    bei < 2 % Qualitätsverlust.
    DOI/URL: https://arxiv.org/abs/2310.06201

**[R15]** A. Chevalier, A. Wettig, A. Ajith, D. Chen,
    „Adapting Language Models to Compress Contexts,"
    in *Proc. EMNLP*, 2023. (AutoCompressor)
    → Grundlage §12.2.4 REFLEXION-Episoden: rekursive hierarchische Komprimierung,
    Summary-Vektoren, Kohärenzverlust-Minimierung.
    DOI/URL: https://arxiv.org/abs/2305.14788

#### Ergänzende Referenzen (indirekte Grundlage)

**[R3]** P. Lewis, E. Perez, A. Piktus, F. Petroni, V. Karpukhin, N. Goyal,
    H. Küttler, M. Lewis, W.-t. Yih, T. Rocktäschel, S. Riedel, D. Kiela,
    „Retrieval-Augmented Generation for Knowledge-Intensive NLP Tasks,"
    in *Advances in Neural Information Processing Systems (NeurIPS)*, vol. 33, pp. 9459–9474, 2020.
    → §12.1.3 Selektive Injektion: Relevanz-Ranking als RAG-Prinzip.
    DOI/URL: https://arxiv.org/abs/2005.11401

**[R4]** Y. Gao, Y. Xiong, X. Gao, K. Jia, J. Pan, Y. Bi, Y. Dai, J. Sun,
    M. Wang, H. Wang,
    „Retrieval-Augmented Generation for Large Language Models: A Survey,"
    *arXiv preprint*, arXiv:2312.10997, 2023.
    → §12.1.3: Survey-Grundlage für RAG-basierte selektive Kontext-Injektion.
    DOI/URL: https://arxiv.org/abs/2312.10997

**[R5]** J. Wei, X. Wang, D. Schuurmans, M. Bosma, B. Ichter, F. Xia, E. Chi,
    Q. V. Le, D. Zhou,
    „Chain-of-Thought Prompting Elicits Reasoning in Large Language Models,"
    in *Advances in Neural Information Processing Systems (NeurIPS)*, vol. 35, 2022.
    → §12.2.3 Position-Abstract-Schema: CoT-Distillation-Prinzip (destillierter
    100-Token CoT-Trace = Position Abstract).
    DOI/URL: https://arxiv.org/abs/2201.11903

#### ThemisDB-interne Evidenz-Anker (Cross-Referenzen)

| Anker | Quelle | Inhalt |
|---|---|---|
| E40 | `research/DIALECTIC_EVIDENCE_PAPER.md §V-B.5` | R3 CWO universell ab 7B; 4 800 Token; Empfehlung `prior_round_compression: "headline"` |
| E41 | `research/DIALECTIC_EVIDENCE_PAPER.md §V-B.5` | trolley_002 R3 CWO=yes alle 3 Schulen; DC(R3)=0.79 mean |
| E42 | `research/DIALECTIC_EVIDENCE_PAPER.md §V-B.5` | R5 META-VERDICT YAML-Improvement-Signals; CS 0.86 mean; compression signal in Signal 4 |
| E46 | `src/ethics_ai/FUTURE_ENHANCEMENTS.md §10.5` | 6-Schul-Expansion: per-school compression policies bei ≤ 32K |
| Tab. S.3 | `research/DIALECTIC_EVIDENCE_PAPER.md` Intro | Baseline Token-Budget-Tabelle (3 Schulen, Arch-B) |
