> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Security Module — Examples

Examples for the `security` module demonstrating Paper 2 Layer 7 (IntentClassifier) implementation patterns.

## Contents

| File | Paper | Issue | Status |
|------|-------|-------|--------|
| `intent_classifier_example.cpp` | Paper 2 §Layer 7 | IMPL-B7 | Specification / planned API |

## intent_classifier_example.cpp

Demonstrates:

1. **SQL injection detection** — query classified as `SQL_INJECTION_ATTEMPT` with confidence ≥ 0.92
2. **Benign SELECT** — classified as `NORMAL` with confidence ≥ 0.95; no action
3. **MASS_EXPORT** below confidence threshold — advisory log only, no block
4. **ZeroTrustPolicyEnforcer integration** — high-confidence alert updates `session_risk_score`
5. **GDPR guard** — `evidence_snippet` limited to ≤ 128 characters; no PII in payload
6. **DecisionRecord** written to `AIDecisionAuditor`

Calls to planned IMPL-B7 API are marked with `/* PLANNED */` comments. The confidence thresholds and integration patterns serve as acceptance criteria.

## Related Documentation

- Issue spec: `docs/issues/optimization_layers/IMPL-B7-intent-classifier.md`
- Research paper: `docs/en/research/LLM_OPTIMIZATION_LAYERS_MATRIX.md` §Layer 7
- Module ROADMAP: `include/security/ROADMAP.md` §Phase 7

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

## Usage

Include the relevant headers from this module:

```cpp
#include "security/module_header.h"
```

See [`ARCHITECTURE.md`](ARCHITECTURE.md) and [`ROADMAP.md`](ROADMAP.md) for details.
