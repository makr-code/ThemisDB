> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

# Prompt Engineering Module — Architecture Guide
<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: src/prompt_engineering/README.md · src/prompt_engineering/ROADMAP.md · src/prompt_engineering/FUTURE_ENHANCEMENTS.md · docs/de/prompt_engineering/README.md -->

**Version:** 1.0
**Last Updated:** 2026-04-06
**Module Path:** `src/prompt_engineering/`

---

## 1. Overview

The Prompt Engineering module provides complete lifecycle management for LLM prompt templates:
creation and storage, Git-like version control, iterative optimization via meta-prompts,
feedback collection, performance tracking, A/B testing with statistical significance, and a
self-improvement orchestrator that automatically detects underperforming prompts and triggers
optimization cycles.

---

## 2. Design Principles

- **Template as Code** – prompt templates are versioned artifacts with commit history,
  branching, diffing, and rollback, analogous to source code.
- **Data-Driven Optimization** – prompt improvements are driven by empirical feedback and
  measurable quality metrics, not intuition.
- **Safety by Default** – `PromptInjectionDetector` scans all user-provided prompt inputs
  and model responses before use.
- **Observable** – Prometheus-compatible metrics for every prompt: success rate, latency,
  satisfaction score, A/B test results.
- **Self-Improving** – the orchestrator runs a background thread that automatically
  identifies underperforming prompts and kicks off optimization cycles.

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `prompt_manager.cpp` | Template CRUD with RocksDB persistence and context rendering |
| `prompt_version_control.cpp` | Git-like versioning: commits, branches, diffs, rollback |
| `prompt_optimizer.cpp` | Iterative prompt improvement (evaluation + rewrite loop) |
| `meta_prompt_generator.cpp` | Generates meta-prompts for LLM-assisted prompt rewriting |
| `feedback_collector.cpp` | User/system feedback: positive/negative, hallucinations, etc. |
| `prompt_performance_tracker.cpp` | Per-prompt metrics: success rate, latency, satisfaction |
| `prompt_evaluator.cpp` | Quality scoring: semantic similarity, relevance, exact/partial match |
| `prompt_injection_detector.cpp` | Injection attack detection and sanitization |
| `self_improvement_orchestrator.cpp` | Background auto-optimization: detect → optimize → deploy |
| `prompt_engineering_integration.cpp` | High-level integration façade for all subsystems |
| `prompt_engineering_metrics.cpp` | Prometheus metrics export |

### 3.2 Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│        Caller (aql, rag, server, training modules)              │
│   integration.getPrompt("qa_template", {question: "..."})       │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│             PromptEngineeringIntegration (façade)                │
│                                                                  │
│  ┌──────────────────┐  ┌──────────────────┐  ┌───────────────┐ │
│  │  PromptManager   │  │  VersionControl  │  │  Optimizer    │ │
│  │  (CRUD + render) │  │  (commits, diff) │  │  (eval+rewrite│ │
│  └──────────────────┘  └──────────────────┘  └───────────────┘ │
│                                                                  │
│  ┌──────────────────┐  ┌──────────────────┐  ┌───────────────┐ │
│  │FeedbackCollector │  │ PerformanceTracker│  │  A/B Testing  │ │
│  └──────────────────┘  └──────────────────┘  └───────────────┘ │
│                                                                  │
│  PromptInjectionDetector: scan all user inputs and responses    │
│  SelfImprovementOrchestrator: background auto-optimization      │
└──────────────────────────────────────────────────────────────────┘
```

---

## 4. Data Flow

### 4.1 Template Retrieval with Injection Check

```
integration.getPrompt("qa_template", {question: user_question})
    │
    ├─ PromptInjectionDetector: scan user_question
    │       → injection patterns? → sanitize / reject
    │
    ├─ PromptManager: load template "qa_template"
    │       → substitute {question} placeholder
    │
    └─ return rendered prompt string
```

### 4.2 A/B Testing

```
A/B test: "qa_template_v1" vs "qa_template_v2"
    │
    ├─ route 50% of traffic to each variant
    │
    ├─ FeedbackCollector: record outcomes per variant
    │
    ├─ PromptEvaluator: score outputs
    │
    └─ Welch t-test: p-value < 0.05? → winner declared → deploy winner
```

### 4.3 Auto-Optimization

```
SelfImprovementOrchestrator (background thread, interval: 1h):
    │
    ├─ PerformanceTracker: identify prompts with success_rate < threshold
    │
    ├─ MetaPromptGenerator: generate rewrite meta-prompt
    │
    ├─ LLM inference: generate improved prompt candidates
    │
    ├─ PromptEvaluator: score candidates vs. current
    │
    └─ best candidate better? → VersionControl.commit() → deploy
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Used by** | `src/aql/` | AQL query explanation and generation prompts |
| **Used by** | `src/rag/` | RAG context injection templates |
| **Used by** | `src/llm/` | Prompt management and optimization |
| **Uses** | `src/llm/` | LLM inference for meta-prompt optimization |
| **Uses** | `src/storage/` | RocksDB persistence for templates and feedback |
| **Provides to** | `src/observability/` | Prometheus metrics |

---

## 6. Threading & Concurrency Model

- `PromptManager` uses TBB `concurrent_hash_map` for thread-safe template access.
- `FeedbackCollector` uses append-only writes with a per-prompt mutex.
- `SelfImprovementOrchestrator` runs on a dedicated background thread; it does not block
  the request path.
- A/B test traffic splitting uses atomic counter per test group.

---

## 7. Performance Architecture

| Technique | Detail |
|---|---|
| Template caching | Frequently accessed templates cached in-memory |
| Background optimization | Auto-optimization runs off the request path |
| Batch evaluation | `evaluateBatch()` amortizes scoring overhead |

---

## 8. Security Considerations

- **Prompt injection detection**: `PromptInjectionDetector` scans all user inputs for
  instruction overrides, role-play hijacking, and system-block patterns.
- **Feedback checksums**: every `FeedbackEntry` has a FNV-1a audit checksum.
- **Version history**: all template changes are logged with author and timestamp.
- **No raw LLM output stored** without sanitization check.

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `prompt_engineering.optimization.threshold` | 0.7 | Success rate threshold for auto-optimization |
| `prompt_engineering.orchestrator.interval_s` | 3600 | Auto-optimization check interval |
| `prompt_engineering.ab_test.min_samples` | 100 | Min samples before A/B significance test |
| `prompt_engineering.ab_test.p_value` | 0.05 | Significance threshold |
| `prompt_engineering.injection.enabled` | true | Enable injection detection |

---

## 10. Error Handling

| Error Type | Strategy |
|---|---|
| Template not found | Return structured error; do not auto-create |
| Injection detected | Sanitize and log; return warning to caller |
| Optimization failure | Keep existing template; log error; alert |
| LLM unavailable during optimization | Skip cycle; retry on next interval |

---

## 11. Known Limitations & Future Work

- Multi-modal prompts (images, audio) are out of scope.
- Token counting and context-window management are not provided.
- Version branching is implemented; merge/rebase is not yet supported.
- A/B test scheduling (time-bounded tests) is planned.

---

## 12. References

- `src/prompt_engineering/README.md` — module overview
- `src/prompt_engineering/FUTURE_ENHANCEMENTS.md` — roadmap
- `docs/prompt_engineering/` — prompt engineering guides
- `ARCHITECTURE.md` (root) — full system architecture
