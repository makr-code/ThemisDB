# Geo public-header documentation roadmap

## Scope

This file is limited to the public declarations under
`include/geo/`. The active module delivery
roadmap, including runtime implementation and test evidence, is
`src/geo/ROADMAP.md`.

## Current documentation rule

- Treat each header declaration and its Doxygen comments as the API evidence.
- When a public signature or contract changes, synchronize
  `src/geo/API.md` and the focused
  implementation/contract tests where applicable.
- Do not infer API stability, performance targets, runtime configuration, or
  completed test coverage from the presence of a header alone.

## Open documentation check

- [ ] Before the next public geo contract change, review header/API-guide
  consistency and add migration notes if the change is breaking. Target:
  next public API change.

No separate public-header feature or delivery status is asserted here. Runtime
plans and evidence remain in the canonical module roadmap.

---

**AI_Used:** true · **AI_Models:** unknown · **AI_Contributor:** GitHub Copilot coding agent · **Human_Review:** false · **Human_Author:** ThemisDB Contributors
Exact model backend not verifiable from final document text alone.
