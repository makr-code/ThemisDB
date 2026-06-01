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

## Known Limitations

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
