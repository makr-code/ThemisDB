# Rewrite Engine Architecture

## Status

Draft architecture proposal for a deterministic rewrite engine integrated into ThemisDB prompt engineering, NL→AQL preprocessing, and post-generation validation pipelines.

## Purpose

The Rewrite Engine introduces a formal, testable transformation layer for prompt normalization, policy enforcement, NL→AQL preprocessing, and post-generation canonicalization. It is designed to complement existing ThemisDB subsystems such as `PromptManager`, typed prompt templates, grammar-constrained generation, and AQL validation.

Primary goals:
- Improve determinism and auditability of prompt handling.
- Reduce invalid or unsafe generated AQL before execution.
- Provide a reusable rule engine for prompt engineering, ReAct agent output normalization, and query rewriting.
- Preserve compatibility with existing PromptManager and LLM pipelines.

## Scope

In scope:
- Deterministic rewrite passes over natural language prompts and generated text.
- Rule-based normalization, annotation, and canonicalization.
- Explainable transformation traces.
- YAML-configurable rules for low-risk patterns.
- C++ rule interfaces for complex semantic rewrites.

Out of scope for the first implementation:
- General nondeterministic search over rewrite graphs.
- Full symbolic execution or theorem-proving over prompts.
- Replacing grammar-constrained generation.
- Replacing the LLM or PromptManager.

## Architectural Position

The Rewrite Engine sits at formal transformation boundaries in the request pipeline.

```text
User Input
  ↓
[RewriteEngine: Input Phase]
  ↓
[PromptManager / Typed Prompt DSL]
  ↓
[LLM / llama.cpp / grammar constraints]
  ↓
[RewriteEngine: Output Phase]
  ↓
[AQL Parser / Validator / Planner]
  ↓
Execution
```

Recommended use points:
1. Before prompt template selection and context injection.
2. Before NL→AQL generation.
3. After LLM output but before parser/executor handoff.
4. Before tool execution in structured ReAct-style agent flows.

## Design Principles

- Deterministic by default.
- Markov-style ordered rule application rather than open-ended rewrite exploration.
- Fully traceable transformations with before/after snapshots.
- Small, composable rules with explicit phase assignment.
- Hard limits to prevent loops and pathological expansions.
- Backward compatible integration with existing prompt engineering and LLM modules.

## Execution Model

### Recommended Default: Ordered Deterministic Rewriting

Rules are evaluated in priority order within a phase. The first matching rule is applied. The engine continues until:
- no more rules match,
- a terminal rule fires,
- or `max_steps` is reached.

This model is inspired by Markov-style ordered substitution systems and is preferred because it is:
- deterministic,
- explainable,
- testable,
- suitable for policy enforcement,
- suitable for canonicalization and normalization.

### Optional Future Extension: Candidate Rewrite Expansion

For query optimization and plan search, a later secondary engine may support multiple rewrite candidates with ranking/cost selection. This is explicitly deferred and should not be part of the MVP.

## Core Data Structures

### `RewriteDocument`

Represents the current transformation target and its accumulated annotations.

```cpp
struct RewriteDocument {
    std::string raw_text;
    std::string normalized_text;
    nlohmann::json attributes;
    nlohmann::json annotations;
    std::vector<std::string> tags;
};
```

Responsibilities:
- preserve original input,
- hold normalized working text,
- attach domain and policy annotations,
- carry structured attributes between phases.

### `RewriteContext`

Represents runtime metadata and constraints.

```cpp
struct RewriteContext {
    std::string tenant_id;
    std::string user_id;
    std::string language;
    std::string pipeline_stage;
    std::string domain;
    bool safety_strict = false;
    bool explain = false;
    size_t max_steps = 32;
    nlohmann::json environment;
};
```

Responsibilities:
- constrain execution,
- provide tenant/user/domain context,
- communicate safety mode,
- expose phase/stage information.

### `RewriteResult`

```cpp
struct RewriteResult {
    RewriteDocument document;
    bool changed = false;
    bool terminal = false;
    std::string rule_id;
    std::string explanation;
    nlohmann::json metrics;
};
```

Responsibilities:
- return transformed document,
- indicate whether a rule changed state,
- optionally signal termination,
- attach metrics and human-readable explanation.

### `RewriteTrace`

```cpp
struct RewriteTraceEntry {
    std::string rule_id;
    std::string before;
    std::string after;
    std::string explanation;
    std::string phase;
    int priority = 0;
};

struct RewriteTrace {
    std::vector<RewriteTraceEntry> entries;
};
```

Responsibilities:
- capture every applied rule,
- support auditability,
- provide debugging and operator explainability,
- enable benchmark correlation with rewrite decisions.

## Rule Model

### `IRewriteRule`

```cpp
class IRewriteRule {
public:
    virtual ~IRewriteRule() = default;

    virtual std::string id() const = 0;
    virtual std::string phase() const = 0;
    virtual int priority() const = 0;

    virtual bool matches(const RewriteContext& ctx,
                         const RewriteDocument& doc) const = 0;

    virtual RewriteResult apply(const RewriteContext& ctx,
                                const RewriteDocument& doc) const = 0;
};
```

Rule requirements:
- stable identifier,
- explicit phase,
- explicit priority,
- pure matching semantics,
- side-effect-free transformation result.

Rules must not mutate external state directly. Logging, metrics, and audit publication should occur through the engine or higher-level integration layers.

## Rewrite Phases

The engine should support explicit phases to prevent accidental cross-application of rules.

### Phase 1: Input Normalization
Examples:
- synonym normalization,
- locale-specific phrasing normalization,
- alias mapping,
- canonical sort/filter markers.

### Phase 2: Safety and Policy Rewriting
Examples:
- block or neutralize unsafe instructions,
- flag exfiltration-like requests,
- restrict to allow-listed query forms,
- annotate risky content for the ethics/security stack.

### Phase 3: NL→AQL Preparation
Examples:
- identify intent markers,
- enrich request attributes,
- normalize collection/table aliases,
- add structural hints for AQL generation.

### Phase 4: Post-Generation Canonicalization
Examples:
- normalize generated AQL text,
- enforce canonical formatting,
- apply safe corrections before parsing,
- annotate suspicious constructs for validator escalation.

### Phase 5: Structured Agent Output Rewrite
Examples:
- validate or normalize ReAct tool names,
- normalize JSON arguments,
- reject invalid tool invocation patterns,
- convert malformed structured outputs into a deterministic error path.

## Rule Categories

### 1. Regex Rewrite Rules
For low-risk lexical normalization.

Examples:
- German phrasing normalization.
- Canonical markers such as `ORDER_HINT:<field>`.
- Simple alias conversions.

### 2. Dictionary Rewrite Rules
For controlled synonym/alias mapping.

Examples:
- `kunden` → `customers`
- `umsatz` → `revenue`

### 3. Policy Rewrite Rules
For security and governance constraints.

Examples:
- block unsafe schema exfiltration prompts in restricted contexts,
- downgrade unsafe instructions to safe explanatory behavior,
- force explicit review flags.

### 4. Intent Annotation Rules
For tagging user requests.

Examples:
- `intent:aggregation`
- `intent:schema.describe`
- `domain:administrative_law`

### 5. Canonicalization Rules
For output cleanup and standardization.

Examples:
- normalize casing/spacing in AQL,
- standardize generated structured fragments,
- enforce canonical key order for emitted JSON-like payloads where required.

### 6. Planner/Optimization Rules
Deferred to later phases.

Examples:
- predicate pushdown hints,
- graph traversal pruning hints,
- query decomposition candidates.

## Engine Interface

```cpp
class RewriteEngine {
public:
    RewriteEngine();

    void registerRule(std::shared_ptr<IRewriteRule> rule);

    RewriteResult run(const RewriteContext& ctx,
                      const RewriteDocument& input,
                      RewriteTrace* trace = nullptr) const;

private:
    std::vector<std::shared_ptr<IRewriteRule>> rules_;

    std::vector<std::shared_ptr<IRewriteRule>>
    selectRulesForPhase(const std::string& phase) const;
};
```

Expected behavior:
- select rules by phase,
- sort by priority and stable registration order,
- apply until fixpoint/termination/max-step bound,
- emit optional trace,
- publish metrics externally.

## Integration with Existing ThemisDB Components

### PromptManager Integration

Before `PromptManager::getPromptWithContext()`:
- rewrite raw user query,
- add annotations/tags,
- inject normalized query into prompt context,
- optionally include serialized trace for diagnostics.

Illustrative flow:

```cpp
RewriteDocument doc{.raw_text = user_query, .normalized_text = user_query};
RewriteContext ctx;
ctx.pipeline_stage = "input";
ctx.domain = detected_domain;
ctx.explain = true;

RewriteTrace trace;
auto rewritten = rewrite_engine_->run(ctx, doc, &trace);

auto prompt = prompt_manager_->getPromptWithContext(
    selected_prompt_id,
    {
        {"query", rewritten.document.normalized_text},
        {"rewrite_trace", serializeTrace(trace)}
    }
);
```

### NL→AQL Integration

Recommended flow:
1. Rewrite NL request.
2. Generate AQL via LLM/template pipeline.
3. Canonicalize generated output.
4. Parse with `AQLParser`.
5. Retry or fail safely on invalid syntax.

This complements the documented roadmap need for post-generation AQL validation.

### ReAct / Structured Agent Integration

For grammar-constrained agent output:
- rewrite/validate action names,
- normalize argument structures,
- enforce allow-listed action usage,
- reject invalid sequences before execution.

### Self-Awareness / Capability Questions

The repo research material identifies missing self-awareness and capability discovery. The Rewrite Engine can normalize questions such as:
- "Was kannst du?"
- "Welche Daten hast du?"
- "Wie ist das Schema?"

into canonical intents such as:
- `capabilities.describe`
- `schema.describe`
- `storage.locate`

This enables consistent downstream prompt selection or direct capability/schema endpoints.

## Configuration Model

A mixed model is recommended.

### YAML Rules for Low-Risk Patterns

Example:

```yaml
rules:
  - id: normalize_german_sorting
    phase: input
    priority: 10
    type: regex
    pattern: "sortiere nach ([a-zA-Z_]+)"
    replacement: "ORDER_HINT:$1"

  - id: detect_schema_question
    phase: input
    priority: 20
    type: regex
    pattern: "was (kannst du|ist dein schema|welche daten)"
    replacement: "INTENT:capabilities_or_schema"
```

### Compiled C++ Rules for Semantic/Safety Logic

Use C++ rule implementations when:
- multiple attributes must be updated,
- context-sensitive checks are required,
- policy interactions are nontrivial,
- explainability and metrics are richer than static replacement.

## Safety and Reliability Controls

Mandatory controls for MVP:
- `max_steps` cap.
- phase isolation.
- deterministic priority ordering.
- maximum output growth guard.
- terminal/block rules.
- audit trace support.
- explicit error path for malformed rule configuration.

Recommended additional controls:
- idempotence checks for critical normalization rules,
- cycle detection for repeated before/after states,
- allow-list-only mode for production-sensitive deployments,
- per-rule metrics for match/apply counts and latency.

## Observability

The engine should expose metrics compatible with existing observability conventions.

Suggested metrics:
- `rewrite_rules_evaluated_total`
- `rewrite_rules_applied_total{rule_id,phase}`
- `rewrite_terminal_rules_total{rule_id}`
- `rewrite_failures_total{phase,reason}`
- `rewrite_latency_ms{phase}`
- `rewrite_max_steps_reached_total`

Suggested structured log fields:
- request_id,
- tenant_id,
- phase,
- rule_id,
- changed,
- terminal,
- latency_us,
- truncated_trace.

## Security Considerations

- Rules must not silently bypass downstream validators.
- Policy rules must be explicitly versioned and reviewable.
- Sensitive inputs should not be copied verbatim into logs/traces without sanitization.
- Output rewrites must never turn invalid or blocked content into executable content without validation.
- Rule loading from YAML must validate schema and reject malformed definitions deterministically.

## Performance Considerations

The engine should be lightweight enough for pre-LLM usage on every request.

MVP targets:
- sub-millisecond p50 for small rule sets,
- bounded behavior under adversarial inputs,
- no unbounded regex backtracking,
- no quadratic text blowups from chained replacements.

Performance safeguards:
- precompile regexes,
- cache parsed rule config,
- limit trace payload sizes,
- benchmark representative prompt and AQL workloads.

## Testing Strategy

### Unit Tests
For each rule:
- matching cases,
- non-matching cases,
- expected transformation,
- idempotence where applicable,
- edge-case coverage.

### Engine Tests
- priority ordering,
- phase isolation,
- terminal rule handling,
- max step enforcement,
- trace correctness,
- deterministic repeated execution.

### Integration Tests
- PromptManager preprocessing path,
- NL→AQL normalization path,
- post-generation AQL canonicalization path,
- structured agent output normalization.

### Property/Robustness Tests
- no infinite loops,
- no unsafe expansion beyond configured limits,
- malformed YAML rejected cleanly,
- repeated execution converges or stops at configured bound.

### Benchmarks
- rewrite latency by rule count,
- latency by input size,
- invalid-AQL reduction impact,
- trace overhead.

## Proposed File Layout

```text
include/prompt_engineering/
  rewrite_engine.h
  rewrite_rule.h
  rewrite_context.h
  rewrite_document.h
  rewrite_result.h
  rewrite_trace.h
  regex_rewrite_rule.h
  dictionary_rewrite_rule.h
  policy_rewrite_rule.h

src/prompt_engineering/
  rewrite_engine.cpp
  regex_rewrite_rule.cpp
  dictionary_rewrite_rule.cpp
  policy_rewrite_rule.cpp
  rewrite_rule_loader.cpp

config/rewrite_rules/
  input_normalization.yaml
  safety_rewrites.yaml
  nl_to_aql.yaml
  aql_canonicalization.yaml

tests/prompt_engineering/
  test_rewrite_engine.cpp
  test_regex_rewrite_rule.cpp
  test_policy_rewrite_rule.cpp
  test_nl_to_aql_rewrites.cpp
```

## MVP Delivery Plan

### MVP 1: Deterministic Input Normalization
- Engine skeleton
- `IRewriteRule`
- regex/dictionary rules
- YAML loading
- trace support

### MVP 2: Safety and Policy Layer
- blocking/terminal rules
- audit integration
- allow-list modes
- edge-case hardening

### MVP 3: NL→AQL Assistance
- intent annotation rules
- collection/field alias normalization
- post-generation canonicalization before parser handoff

### MVP 4: Advanced Planner and Graph Rewrite Support
- optimization-oriented rules,
- candidate generation extension,
- planner cost integration.

## Rationale for Choosing a Markov-Style Rewrite Engine

A deterministic ordered rewrite engine is the best initial fit for ThemisDB because the repository already contains:
- structured prompt templates,
- grammar-constrained generation,
- ReAct-style structured grammars,
- prompt engineering diagnostics and optimization hooks,
- documented need for AQL validation and query rewriting.

A Markov-style engine complements those components by handling the transformation layer before and after constrained generation. It does not compete with grammars; it strengthens the system around them.

## Compatibility Statement

This proposal is additive and does not require breaking changes to existing prompt engineering, LLM, or AQL APIs. Existing pipelines can integrate the Rewrite Engine incrementally and phase-by-phase.

## Open Questions

- Should rewrite traces be exposed to end users, operators only, or both?
- Which rule types are safe to load from YAML versus requiring compiled C++ implementations?
- Should output canonicalization operate on raw text only or move quickly to AST-aware rewrites for AQL?
- Should ethics/security modules be able to inject terminal rewrite rules dynamically?
- Which existing prompt engineering metrics should be correlated directly with rewrite traces?

## Recommendation

Proceed with a minimal deterministic rewrite engine in `src/prompt_engineering/` and integrate it first into:
1. prompt input normalization,
2. safety/policy preprocessing,
3. NL→AQL preprocessing,
4. post-generation AQL canonicalization.

This provides immediate value with low architectural risk and aligns closely with ThemisDB's current roadmap around prompt hardening, validation, and structured AI workflows.
