---
Author: ThemisDB Contributors
Created: 2026-06-01
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
---

# Public Header Roadmap - user_storage_encrypted

Exact model backend not verifiable from final document text alone.

This derivative roadmap tracks public-header documentation and API review.
Implementation milestones remain in
[`../../src/user_storage_encrypted/ROADMAP.md`](../../src/user_storage_encrypted/ROADMAP.md).

## Current header surface

The operational headers declare types under `themis::plugins::user_storage`.
The separate `user_storage_encrypted_api_contract.h` uses
`themis::user_storage_encrypted`; the relationship between the two contracts
requires maintainer clarification before stability claims are made.

## Follow-up candidates

- [ ] Resolve operational API versus frozen contract discrepancies.
- [ ] Add header compile checks to the documented build/test flow.
- [ ] Document supported platform boundaries and exact public signatures.
- [ ] Review compatibility and ABI guarantees under repository versioning
      policy.

These are documentation/API-review candidates, not delivery commitments or
verified milestones. No layer association is asserted here.

## References

- [Header inventory](README.md)
- [Header architecture](ARCHITECTURE.md)
- [Header enhancement candidates](FUTURE_ENHANCEMENTS.md)
- [Canonical module roadmap](../../src/user_storage_encrypted/ROADMAP.md)
