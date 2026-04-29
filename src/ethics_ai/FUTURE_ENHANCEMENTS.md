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

### 9.1 Per-Thesis `token_budget` and `activation_rounds`

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
