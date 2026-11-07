# ROADMAP — ai_working Module

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-11-06 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->
<!-- AI_Used: true | AI_Models: unknown | AI_Contributor: GitHub Copilot | Human_Review: false | Human_Author: unknown -->
<!-- Exact model backend not verifiable from final document text alone. -->

## Current Status

`ai_working` is a documentation and working-artifact governance area, not a runtime module. Source review found no implementation files under `src/ai_working/`, no public headers under `include/ai_working/`, no test cases or registered target under `tests/ai_working/`, and no benchmark code or target under `benchmarks/ai_working/`. Do not infer product runtime maturity, API readiness, or performance from this module.

| Area | Verified state | Planning maturity* |
|---|---|---:|
| Runtime/API | No C++ runtime, public API, or module build target | N/A |
| Artifact lifecycle | Separate root `ai_working/` compaction/rotation and `src/ai_working/` archive flows | 2/5 |
| Governance check | `scripts/archive_ai_working.py --check-freshness` checks nine required docs for presence, one placeholder pattern, and minimum size; it does not check factual freshness | 2/5 |
| Tests | `tests/ai_working/CMakeLists.txt` only declares the minimum CMake version; no module tests are present | 0/5 |
| Benchmarks | README only; no benchmark implementation or measured baseline | 0/5 |
| Security evidence | Secret scanning exists at repository level, but this module's maintenance workflow does not itself establish scan coverage for these artifacts | 1/5 |

*\*Planning maturity is a qualitative 0–5 checklist (0 = no evidence, 5 = repeatable measured controls), not an audit or compliance score. Ratings describe available evidence only.*

The root artifact compactor groups files by filename-derived topic, orders them by modification time, writes compact Markdown and a topic index, and applies age-based archive/cleanup operations (`scripts/ai-working-compact.py`). Its output includes generated dates and source filenames, dates, and sizes; it does not currently provide a content-hash provenance manifest. Separately, `scripts/archive_ai_working.py --archive-stale` selects every unprotected top-level Markdown file in `src/ai_working/`; the name does not imply semantic stale detection. The weekly/manual maintenance workflow runs these operations and the structural document check, but its push path filters do not include `src/ai_working/**` or `scripts/archive_ai_working.py` (`.github/workflows/maintenance-ai-working.yml`).

### Research and gap analysis

The research supports strengthening source traceability, evaluation, and lifecycle controls; it does not justify adding an application runtime or LLM summarizer to this documentation-only module:

- The RAG evaluation survey separates retrieval, generation, and additional requirements, while RAGAS proposes reference-free metrics across retrieval and generated-answer behavior. These are useful design precedents if artifact discovery or generated answers are introduced, but are not direct benchmarks for the current file compactor, which does not implement a RAG pipeline ([Yu et al., 2024](https://arxiv.org/abs/2405.07437); [Es et al., 2023, revised 2025](https://arxiv.org/abs/2309.15217)).
- NIST's Generative AI Profile recommends lifecycle risk management; OWASP's 2025 LLM Top 10 highlights prompt injection, sensitive information disclosure, data/model poisoning, and unbounded consumption. These become direct runtime threat requirements only if an LLM is later given access to artifacts; today the relevant controls are provenance, access/review boundaries, secret scanning, and safe archival ([NIST AI 600-1](https://doi.org/10.6028/NIST.AI.600-1); [OWASP Top 10 for LLM Applications 2025](https://genai.owasp.org/llm-top-10/)).
- NIST SP 800-92's lifecycle-oriented log-management guidance is a useful operational precedent for recording artifact movement and retention decisions, not a claim that the module is subject to that standard ([NIST SP 800-92](https://csrc.nist.gov/pubs/sp/800/92/final)).

**Gap:** structural presence/length checks can pass stale content; archive selection is not semantic; no focused tests exercise the safety properties; no benchmark quantifies scale; and the generated compact files lack content-hash lineage. No current performance result or compliance percentage is available.

## In Progress

- [x] Source inventory and literature-informed v2.5.0/v3.0.0 plan recorded (Target: v2.5.0 planning gate).
- [ ] Maintainer confirms owners, release placement, effort ranges, and whether a structured index is needed (Target: v2.5.0 planning gate).

## Planned Features

### v2.5.0 — Safe, testable documentation tooling (priority: P0)

- [ ] Add focused standard-library Python tests for current archive and compaction behavior, including dry-run non-mutation, same-day name collisions, protected files, empty/large inputs, and failures that must not delete or overwrite artifacts (Target: v2.5.0).
- [ ] Establish a reproducible baseline for governance-check and compaction costs, and capture environment, corpus size, elapsed time, and peak memory before setting absolute performance limits (Target: v2.5.0).
- [ ] If maintainers approve the use case, add a versioned structured artifact manifest containing normalized relative path, topic, content hash, byte size, source modification time, generation time, and lifecycle state; keep source documents authoritative and generated summaries non-normative (Target: v2.5.0).
- [ ] Add manifest/schema and path-containment checks to the focused tests; ensure an invalid manifest or archive collision fails safely without data loss (Target: v2.5.0).
- [ ] Confirm an applicable secret scan and review gate for changed active and archived artifacts; do not treat archive movement as redaction (Target: v2.5.0).

### v3.0.0 — Measured discovery and lifecycle governance (priority: P1, conditional)

- [ ] Build a human-reviewed artifact-discovery corpus with gold source paths and evaluate search/index quality separately from any generated answer quality (Target: v3.0.0).
- [ ] Add a trendable benchmark for 1,000- and 10,000-artifact corpora with recorded artifact sizes, runner, repeated-run p50/p95 latency, and peak memory; after a baseline exists, hold p95 regression to at most 15% on the same runner (Target: v3.0.0).
- [ ] Add explicit artifact lifecycle, provenance, source-authority, and review-state reporting; keep automated freshness findings advisory unless maintainers validate their false-positive rate (Target: v3.0.0).
- [ ] Consider LLM-assisted artifact discovery or summarization only after a separate threat-model and data-boundary approval; require source citations and human review, and do not add a public C++ API without a demonstrated runtime requirement (Target: v3.0.0).

## Implementation Phases

### Phase 1 — Design / API Contract / Evidence Baseline (Wave 1, v2.5.0)

- **Goal:** Keep `ai_working` documentation/tooling-only unless a concrete runtime requirement is approved; define authoritative versus historical artifacts and measurable acceptance criteria.
- **Tasks:** [x] Inspect the `src/`, `include/`, `tests/`, and `benchmarks/` module paths and current maintenance scripts/workflow; [x] compare observed scope with relevant evaluation and risk-management literature; [ ] obtain maintainer approval for the manifest use case and owners.
- **Dependencies:** Repository source, module docs, and governance/release source of truth.
- **Effort:** 1–2 person-days for implementation planning and review; source inventory and research are recorded here.
- **Risk:** The generated developer wiki may be stale or omit source files; it must not override repository source-of-truth documents.
- **Metric / validation:** All four scoped paths are explicitly classified; every roadmap claim is tied to repository evidence or marked as an assumption; maintainer decision is recorded before implementation.

### Phase 2 — Core Tests and Reproducible Baseline (Wave 2, v2.5.0)

- **Goal:** Make existing file-management behavior testable without introducing new dependencies or runtime APIs.
- **Tasks:** Add focused tests for the two Python helpers' existing behavior; test protected-file handling, dry-run, archive collision behavior, empty inputs, deterministic ordering, and non-mutation on rejected operations; measure a fixed representative corpus.
- **Dependencies:** Wave 1 scope decision; tests use existing Python standard library and repository test conventions.
- **Effort:** 4–6 person-days.
- **Risk:** Existing time-based behavior can make tests flaky; isolate fixtures and compare filesystem state instead of relying on wall-clock timing.
- **Metric / validation:** Each listed boundary has a passing test; repeated measurements on the same runner report input count/bytes, elapsed time, and peak memory; publish results before adopting absolute latency budgets.

### Phase 3 — Artifact Index and Safety Controls (Wave 3, v2.5.0)

- **Goal:** Improve discovery and traceability with an optional, bounded manifest while preserving raw artifacts and current authority rules.
- **Tasks:** After maintainer approval, define and validate the manifest fields listed above; generate it deterministically from the active artifact set; validate relative-path containment, schema version, source links, content hashes, and archive collision behavior; attach applicable secret scanning to changed artifacts.
- **Dependencies:** Wave 2 tests and baseline; agreement on manifest consumers and source-of-truth semantics.
- **Effort:** 5–8 person-days.
- **Risk:** A generated index may be mistaken for normative content or silently omit files; fail closed on invalid paths/schema and expose source counts and skipped-file findings.
- **Metric / validation:** 100% of indexed records have required fields and a valid in-repository path; repeat generation over unchanged inputs produces equivalent manifest data; all safety tests pass; no unresolved secret-scanner finding is accepted.

### Phase 4 — Discovery Quality Evaluation (Wave 4, v3.0.0)

- **Goal:** Prove that structured discovery helps agents find the correct authoritative working artifact without overstating model quality.
- **Tasks:** Create at least 50 maintainer-reviewed queries with gold source paths across the active topic groups; measure top-5 source recall and report results by topic; separately score source citation correctness if answer generation is later approved.
- **Dependencies:** Wave 3 manifest, approved corpus access and human-reviewed gold labels.
- **Effort:** 6–10 person-days.
- **Risk:** A small or biased query set can inflate scores; publish corpus composition and treat reference-free metrics as diagnostic rather than a substitute for human labels.
- **Metric / validation:** Top-5 recall is at least 90% overall and no topic falls below 80%; every cited source in any generated answer resolves to an authoritative repository path; report sample size and per-topic results.

### Phase 5 — Lifecycle, Security, and Scale Hardening (Wave 5, v3.0.0)

- **Goal:** Make archival and discovery operations auditable, bounded, and robust as the artifact corpus grows.
- **Tasks:** Record lifecycle transitions and manifest hashes; exercise interrupted operations, malformed metadata, path traversal attempts, duplicate names, and secret-bearing fixtures; benchmark 1,000- and 10,000-artifact corpora over 20 runs on a documented runner.
- **Dependencies:** Wave 3 manifest and Wave 4 corpus/metrics; security review before any LLM-backed operation.
- **Effort:** 5–8 person-days.
- **Risk:** Strict freshness or security checks can create false positives or block legitimate historical content; start advisory, measure findings, and require human review before promotion to blocking gates.
- **Metric / validation:** No test case loses or overwrites source artifacts; zero accepted secret-scanner findings; capture p50/p95 and peak memory; after baseline, p95 regression is no more than 15% on the same runner.

### Phase 6 — Documentation, Operations, and Acceptance (Wave 6, v3.0.0)

- **Goal:** Handoff a supportable governance tool with explicit limits and release evidence.
- **Tasks:** Synchronize module README, architecture, security, performance, audit, and enhancement docs with shipped behavior; document operator recovery and review procedures; confirm changed paths trigger the intended CI checks; obtain maintainer sign-off.
- **Dependencies:** Waves 1–5 and repository-level release/governance approval.
- **Effort:** 3–5 person-days plus review/CI time.
- **Risk:** A successful structural workflow may be mistaken for proof of factual freshness or compliance; preserve that distinction in output and documentation.
- **Metric / validation:** All required module docs pass the existing structural check; focused tests and CI pass on changed paths; zero unreviewed high-risk findings remain; maintainer sign-off recorded.

## Production Readiness Checklist

- [x] Module purpose and documentation-only boundary verified.
- [x] Active compaction and archive flows distinguished from historical artifacts.
- [x] Existing structural freshness check limitations and missing test/benchmark targets recorded.
- [ ] Maintainer approves scope, owner, and manifest use case before implementation.
- [ ] Focused behavior tests and safe-failure checks pass.
- [ ] Secret scanning, artifact provenance, and benchmark evidence are reviewed.
- [ ] Source-of-truth docs and operations guidance are synchronized before v3.0.0 acceptance.

## Known Issues and Limitations

- No C++ implementation, public header, test case, or benchmark target exists in the scoped module directories.
- The freshness check is structural, not semantic or date-based; `--archive-stale` does not identify semantic staleness.
- The root artifact compactor is distinct from the `src/ai_working/` archive helper. A current CI run/result was not verified as part of this documentation task.
- The generated wiki health snapshot is dated 2026-11-02 and reports 1,450 removed and 26 changed sources; it is a staleness signal, not current module status. Root and module source documents remain authoritative.
- The performance values above are proposed test conditions/acceptance thresholds, not measured results.
- Owners, approved release dates, target-specific release gates, and a maintainer-labeled evaluation corpus remain open assumptions.

## Breaking Changes

None planned. Any future runtime/API proposal requires a separate demonstrated use case, design review, and compatibility assessment.
