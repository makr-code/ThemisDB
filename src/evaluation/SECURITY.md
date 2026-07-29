# Evaluation Module Security

<!-- Status: current | validated: 2026-07-29 -->
<!-- Links: README.md · ARCHITECTURE.md · AUDIT.md · PRODUCTION_REQUIREMENTS.md -->

## Scope

Security posture for the partially delivered EPIC 2 evaluation module. Runtime sources,
tests, and benchmark entry points now exist, but production promotion remains blocked
until policy/error hardening and executable evidence are complete.

## Security Posture (Current)

- evaluation contracts and runtime-owned sources are present for the currently shipped EPIC 2 surfaces
- planner and approximation behavior must remain explicit, machine-readable, and fail-closed
- no unsupported claims are made about full module-wide runtime hardening
- production enablement remains gated by focused tests, benchmark evidence, and documented acceptance criteria

## Primary Risks to Track

1. approximation-policy bypasses or silent downgrades in future downstream integrations
2. inconsistent validation between benchmark, metrics, artifact-lifecycle, and planner inputs
3. degraded-mode behavior that returns truth-bearing results without graph-verified finalization
4. hardware/profile or manifest-mismatch paths that are not surfaced clearly to operators

## Required Controls for Phase Advancement

- explicit contract-level input validation and failure semantics
- deterministic tests for policy, fallback, and approximation boundaries
- benchmark-backed regression gates for planner and evaluation paths
- operator-visible diagnostics for policy decisions, fallback behavior, and evidence blockers

## References

- `src/evaluation/PRODUCTION_REQUIREMENTS.md`
- `src/evaluation/MODULE_EVIDENCE.md`
- `src/evaluation/ROADMAP.md`
