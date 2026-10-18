# Module Quick Rules for Copilot

Use this as the short operating contract for module work.

## 1. Read the human-readable module layer first
- Start with the module README, FEATURES, ARCHITECTURE, ROADMAP, and current gap state.
- Treat raw JSON and XML as evidence, not as the primary narrative.

## 2. Keep ownership clear by path
- Public API: include/<module>/
- Internal implementation: src/<module>/
- Tests: tests/<module>/
- Benchmarks: benchmarks/<module>/
- External/vendor code: vcpkg, third_party, vendor, external, plugins/private, or equivalent external roots

## 3. Doxygen is part of the source contract
- Public API must be documented with purpose, preconditions, return behavior, errors, and edge cases.
- Missing brief, missing param docs, or missing return docs are compliance findings.

## 4. Evidence must stay concise and structured
- Prefer a small summary, a compact evidence file, and a clear action packet.
- Do not dump raw logs, XML, or huge JSON blobs into the primary module narrative.

## 5. Validate before broadening scope
- Reproduce with the smallest relevant command.
- Then update the module docs and the evidence file in the same change.
- Final state must remain legible to humans and actionable for Copilot.

## 6. Developer-quality docs are not marketing docs
- `FEATURES.md` is capability-facing, not a generic product pitch.
- `ARCHITECTURE.md`, `RESEARCH.md`, `IMPLEMENTATION.md`, `RUNBOOK.md`, and `SECURITY.md` must stay engineering-facing and evidence-based.
- Prefer concrete responsibility, trade-off, failure mode, and source evidence over vague claims like "better", "optimized", or "improved".

## 7. Required doc type expectations
- `FEATURES.md`: problem, role, capabilities, operational value, trade-offs, source references
- `ARCHITECTURE.md`: boundaries, contracts, runtime flow, failure domains, ownership
- `RESEARCH.md` / `EMBEDDING.md`: method, assumptions, metrics, limits, validation path
- `IMPLEMENTATION.md` / `API.md`: scope, contract, edge cases, validation expectations
- `RUNBOOK.md`: failure modes, recoveries, observability, rollback safety
- `SECURITY.md`: trust boundaries, access, key handling, risk posture, incident handling

## Required issue packet
- Scope
- Source Evidence
- What needs to be done
- Validation
- Acceptance criteria

If the module cannot be explained with this structure, treat it as a documentation gap until it is brought back into alignment.
