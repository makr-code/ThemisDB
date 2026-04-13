<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · AUDIT.md · SECURITY.md -->

# Roadmap — AQL Module Public Headers

**Module Path:** `include/aql/`  
**Implementation Roadmap:** `../../src/aql/ROADMAP.md`

---

## Current Status

Public headers at v1.8.0. LLM backend abstraction, query builder, validator, schema provider,
autocomplete, confidence scoring, conversation context, few-shot examples, LoRA fine-tuning,
and multimodal inference headers are all stable.

---

## Completed Features

- [x] `IAsyncLLMBackend` with streaming support
- [x] `ILLMAQLHandler` for NL → AQL translation
- [x] `IAQLAgent` for autonomous query agents
- [x] `IAQLAutocomplete` and `IAQLConfidenceScorer`
- [x] `IAQLConversationContext` for multi-turn dialogue
- [x] `IAQLQueryBuilder` and `IAQLQueryTemplateLibrary`
- [x] `IAQLQueryValidator` and `IAQLSchemaProvider`
- [x] `IAQLSyntaxHighlighter` and `AQLTokenStream`
- [x] `IAQLOptimizerAdvisor` and `IAQLFewshotExampleLibrary`
- [x] `IAQLLoRAFinetuner` and `IAQLMigrationAssistant`
- [x] `ILLMMetricsCollector`, `ILLMTimeoutManager`, `ILLMTokenEstimator`
- [x] `MultimodalInferRequest` for image/audio/video inputs

---

## Planned Features

- [x] `IAQLQueryDiffExplainer` for explaining differences between two queries (Target: Q3 2026)
- [x] `IAQLRollbackSuggester` for automatic rollback query generation (Target: Q4 2026)
- [x] `IModelRouter` for multi-model routing and fallback (Target: Q3 2026)

---

## Implementation Phases

### Phase 1: Core LLM & Query Interfaces
- [x] `IAsyncLLMBackend`, `ILLMAQLHandler`, `IAQLAgent`

### Phase 2: Query Tooling Headers
- [x] Builder, validator, schema provider, autocomplete, confidence scorer

### Phase 3: Observability & Error Headers
- [x] `ILLMMetricsCollector`, `ILLMTimeoutManager`, `llm_error_codes.h`

### Phase 4: Advanced AI Features
- [x] Few-shot examples, LoRA fine-tuner, migration assistant
- [x] Multimodal inference, token estimator

### Phase 5: Future Query Intelligence
- [x] `IAQLQueryDiffExplainer` (Q3 2026)
- [x] `IModelRouter` (Q3 2026)

### Phase 6: Documentation & Acceptance
- [x] Architecture and audit docs
- [ ] Doxygen fully annotated
- [ ] LoRA compile flag documented

---

## Production Readiness Checklist

- [x] LLM backend fully abstracted
- [x] Validation and schema headers present
- [x] Observability headers complete
- [ ] `THEMIS_ENABLE_LORA` compile guard verified and documented
- [ ] `IModelRouter` published
