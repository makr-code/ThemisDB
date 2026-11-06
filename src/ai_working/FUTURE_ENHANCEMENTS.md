# Future Enhancements — ai_working Module

<!-- Status: current | validated: 2026-11-06 -->
<!-- Links: README.md · ROADMAP.md · PERFORMANCE_EXPECTATIONS.md -->
<!-- AI_Used: true | AI_Models: unknown | AI_Contributor: GitHub Copilot | Human_Review: false | Human_Author: unknown -->
<!-- Exact model backend not verifiable from final document text alone. -->

## Scope

- Keep `ai_working` a documentation and working-artifact governance area; do not add C++ runtime code without a demonstrated product requirement and approved API design.
- Improve safety, provenance, discovery, and validation of artifacts managed by `scripts/ai-working-compact.py` and `scripts/archive_ai_working.py`.
- Keep root `ai_working/` execution artifacts distinct from module governance documents under `src/ai_working/`.

## Design Constraints

- `ROADMAP.md` and the current repository governance documents remain authoritative; compacted pages, manifests, and archived snapshots are derivative or historical.
- Do not equate file age, document length, or successful structural checks with semantic freshness.
- Preserve raw artifacts during compaction and archival; no silent overwrite, deletion, or secret redaction claims.
- Use existing Python and repository tooling; do not add dependencies without necessity and review.
- Treat artifact content as untrusted input. Any future LLM-backed operation requires a separate threat model, data-boundary approval, source citations, and human review.

## Required Interfaces

| Interface | Requirement |
|---|---|
| Existing archive helper | Preserve `scripts/archive_ai_working.py --archive-stale` and `--check-freshness`; document that archival selects unprotected top-level Markdown and freshness checks only required-document presence, a placeholder pattern, and minimum size. |
| Existing compactor | Preserve root `ai_working/` compaction, age-based archive/cleanup, and dry-run behavior; keep it separate from the `src/ai_working/` archive helper. |
| Proposed artifact manifest (v2.5.0, approval required) | Versioned records with normalized relative path, topic, content hash, byte size, source modification time, generation time, and lifecycle state; generated content must not become a source of truth. |
| Proposed discovery evaluation (v3.0.0) | Human-reviewed queries with gold source paths; measure source retrieval separately from generated-answer quality. |

## Implementation Notes

- v2.5.0 is limited to focused tests for current file operations, a reproducible performance baseline, and an approved minimal manifest plus path/schema validation.
- v3.0.0 may add discovery-quality evaluation, lifecycle evidence, and scale hardening after the manifest and human-reviewed evaluation corpus exist.
- Benchmark 1,000- and 10,000-artifact corpora with recorded artifact sizes and runner; report repeated-run p50/p95 latency and peak memory. The roadmap's 15% p95 regression limit is conditional on first capturing a same-runner baseline.
- Keep any semantic freshness validator advisory until its false-positive/false-negative behavior has been evaluated and maintainers approve a blocking gate.
- Research grounding: the RAG evaluation survey and RAGAS separate retrieval from generation quality, but neither directly benchmarks the current file compactor ([Yu et al.](https://arxiv.org/abs/2405.07437); [Es et al.](https://arxiv.org/abs/2309.15217)). NIST AI 600-1, OWASP LLM Top 10 2025, and NIST SP 800-92 inform conditional AI-risk, sensitive-data, and lifecycle/audit controls; they do not establish module compliance.

## Test Strategy

- Add focused standard-library Python tests for protected files, dry-run non-mutation, duplicate-name collisions, empty and oversized inputs, deterministic ordering, malformed manifest data, path containment, and safe failure.
- Verify repeated manifest generation over unchanged inputs yields equivalent manifest data and that every indexed path resolves inside the repository.
- For v3.0.0, use at least 50 maintainer-reviewed discovery queries with gold source paths; target at least 90% overall top-5 source recall and at least 80% per topic. Keep any answer-generation evaluation separate and require citations that resolve to authoritative sources.
- Run the repository's configured secret-scanning check on changed active and archived artifacts; review rather than silently suppress findings.

## Performance Targets

- No runtime latency or throughput target applies to the present documentation-only module.
- Capture a baseline before setting absolute limits. For a stable 1,000- and 10,000-artifact corpus, document runner, corpus size/bytes, 20 repeated runs, p50/p95 elapsed time, and peak memory.
- Proposed v3.0.0 regression gate: p95 elapsed time no more than 15% above the same-runner baseline. This is a target, not a measured result.

## Security / Reliability

- Preserve original artifacts and fail safely on invalid metadata, out-of-repository paths, archive collisions, and interrupted operations.
- Apply secret scanning and human review to changed working and archived artifacts; archive movement does not redact sensitive content.
- Treat model-generated or compacted content as non-normative. Require source lineage and human review for any future LLM-assisted artifact discovery or summarization.
- Current risk controls do not establish semantic freshness, complete secret-scanner coverage, or compliance certification; verify coverage and results before release claims.
