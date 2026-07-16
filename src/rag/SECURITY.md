# Security - RAG Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

Report vulnerabilities via SECURITY.md.

## Threat Model

| Threat | Mitigation surface |
|---|---|
| prompt-injection payloads in retrieved content | prompt injection detector and sanitizer paths |
| unsafe context propagation to generation or judge stages | bounded context assembly and guardrail checks |
| unauthorized retrieval scope expansion | upstream authorization and scoped retrieval integration |
| low-quality or unverifiable responses leaking to clients | judge and quality-control pipeline gates |
| cache/result cross-contamination risks | deterministic keying and scoped runtime integration |

## Security Controls

- prompt-injection detection and sanitization is enforced on dedicated RAG paths
- evaluation and quality-gate flows detect unsupported or low-trust outputs
- retrieval and ingestion bridge surfaces expose explicit failure signaling
- audit and reporting surfaces provide evidence for operational triage

## Defense-in-Depth: Input Validation Boundaries

RAG module implements a layered security model for all user-supplied input (query, documents, generated_answer):

### Boundary 1: Size Validation (Lines 205-234 in rag_judge.cpp)

**Location**: `RAGJudge::evaluateWithConfig()` entry point
**Protection**: Reject oversized inputs (DoS prevention)
**Enforced on**:
- `input.query` (max 100KB)
- `input.generated_answer` (max 100KB)
- `input.documents` (max 1000 documents)

**Rationale**: Prevents memory exhaustion and uncontrolled resource consumption before downstream processing.

### Boundary 2: Injection Detection (Lines 256-316 in rag_judge.cpp)

**Location**: `RAGJudge::evaluateWithConfig()` after size validation
**Protection**: Active scanning for prompt injection patterns in retrieved documents
**Behavior**:
- `PromptInjectionDetector::scanDocuments()` identifies injection patterns (HIGH/CRITICAL severity)
- Optionally blocks evaluation if high-severity injection detected (configurable via `block_on_high_severity_injection`)
- Logs all findings for audit/observability

**Rationale**: Early detection prevents injection patterns from reaching downstream evaluation and LLM integration.

### Boundary 3: Injection Sanitization (Lines 320-360 in rag_judge.cpp)

**Location**: After injection detection, before dimension evaluations
**Protection**: Explicit sanitization of user input via `PromptInjectionSanitizer`
**Enforced on**:
- `safe_input.query = injection_sanitizer->sanitize(input.query)`
- `safe_input.generated_answer = injection_sanitizer->sanitize(input.generated_answer)`
- `safe_input = injection_sanitizer->sanitizeInput(input)` (document set sanitization)

**Rationale**: Escapes special characters and removes known injection payloads before input reaches LLM calls.

### Boundary 4: Safe Evaluation (Lines 370+ in rag_judge.cpp)

**Location**: All dimension evaluation calls (faithfulness, relevance, coherence, etc.)
**Protection**: Dimension evaluators receive only `safe_input` (sanitized copy)
**Guarantee**: No unsanitized user data reaches any LLM prompt or security-sensitive operation

**Rationale**: Complete isolation between raw input and evaluated/judged output ensures no injection patterns leak through.

### Batch Evaluator & Adversarial Tester

**batch_evaluator.cpp**: Implements identical size validation + dual sanitization (PromptInjectionSanitizer + shared LLM safety policy)
**adversarial_tester.cpp**: Uses shared `sanitizeEvaluationInput()` helper for consistent protection across test case variants

---

## Thread Safety Boundaries

All shared data structures are protected by dedicated mutexes with RAII lock guards:

| Structure | Mutex | Protected Operations |
|-----------|-------|---|
| `impl_->cache` | `cache_mutex` | Read (lines 244-249), Write (lines 574-576) |
| `impl_->eval_history` | `bias_history_mutex` | Append (lines 581-585) |
| `eval_queue_` | `queue_mutex_` | Enqueue (lines 607-614) |

All locks use `std::lock_guard<std::mutex>` (RAII) to ensure release even on exception paths.

---

- guard effectiveness depends on deployment configuration and active backend mix
- heuristic detection paths may require periodic calibration under new attack patterns
- distributed deployment combinations require continuous regression evidence

## Sourcecode Verification (Module: rag/security)

- Verified files:
  - src/rag/prompt_injection_detector.cpp
  - src/rag/rag_judge.cpp
  - src/rag/quality_control_pipeline.cpp
  - src/rag/rag_context_assembler.cpp
  - src/rag/rag_ingestion_bridge.cpp
  - src/rag/bias_detector.cpp
  - src/rag/adversarial_tester.cpp
  - src/rag/evaluation_report_exporter.cpp
- Verified controls:
  - prompt and context safety gates
  - quality/evaluation guard behavior
  - operational evidence and reliability-related signals

## Issue Scope Traceability

- Wave B tracking issue: `https://github.com/makr-code/ThemisDB/issues/5039`
- dependent Wave A issue: `https://github.com/makr-code/ThemisDB/issues/5038`
- follow-on Wave C issue: `https://github.com/makr-code/ThemisDB/issues/5040`
