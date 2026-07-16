[docs](../../README.md) > [en](../README.md) > [themis](./index.md) > [reference](./reference.md)
**Datum:** 2026-04-16
**Status:** review
**Primary (Quelle der Wahrheit):**
- `src/themis/README.md`
- `src/themis/ROADMAP.md`
- `src/themis/FUTURE_ENHANCEMENTS.md`
- `src/themis/AUDIT.md`
- `include/themis/ROADMAP.md`
- `include/themis/FUTURE_ENHANCEMENTS.md`
- `include/themis/AUDIT.md`

**Bezug / Reference:**
- Issue: [MODULE] themis
- Context: Reality check, roadmap verification, and migration notes for Themis primary/secondary docs.

---

# Themis Reality Check & Verification

## Task 1 — Reality Check vs Source Code

- `src/themis/` contains 11 implementation files (`*.cpp`), so it is no longer an empty/planning-only directory.
- Concrete primary drift was corrected in:
  - `src/themis/README.md`
  - `src/themis/AUDIT.md`
  - `include/themis/AUDIT.md`

Remaining documented deviations:
- `src/themis/ARCHITECTURE.md` still includes historical “planned v1.7.0+” migration wording.
- `src/themis/FUTURE_ENHANCEMENTS.md` still includes at least one outdated LZ4-stub constraint reference.

## Task 2 — ROADMAP / FUTURE_ENHANCEMENTS Verification

- `src/themis/ROADMAP.md`: phase structure and implementation status are largely aligned with code reality; open items remain open.
- `include/themis/ROADMAP.md`: technically consistent, but does not yet expose explicit `Known Issues & Limitations` and `Breaking Changes` sections.
- `include/themis/FUTURE_ENHANCEMENTS.md`: includes concrete sections (`Scope`, `Design Constraints`, `Required Interfaces`, `Test Strategy`, `Performance Targets`, `Security / Reliability`) and is implementation-oriented.

## Task 3 — Research Notes and Decisions

Sources reviewed:
- `src/themis/*.cpp`
- `src/network/wire_protocol_v2.cpp`
- `include/themis/runtime_license_gate.h`
- `include/themis/module_signature_verifier.h`

Decisions:
1. Apply only factual primary corrections with direct code evidence.
2. Keep open roadmap items visible instead of force-closing without implementation proof.
3. Track unresolved implementation gaps in the DE report:
   [docs/de/themis/MISSING_IMPLEMENTATIONS.md](../../de/themis/MISSING_IMPLEMENTATIONS.md)
