---
Author: ThemisDB Contributors
Created: 2026-10-31
Creation_Date_Note: Creation date of this rewritten page; the original file creation date is unknown.
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: themisdb-doc-orchestrator
Human_Review: false
Human_Author: unknown
Source_Level: level1
SOT_Domain: api-contract
Canonical_References:
  - src/security/ROADMAP.md
  - include/security/key_provider.h
  - include/security/security_api_contract.h
Related_Issue: "#7379"
Related_Milestone: DOC-WEEKLY-2026-W44
---

Exact model backend not verifiable from final document text alone.

# Public security API roadmap

This file is a navigation note, not an independent delivery plan. The
implementation roadmap and current status are maintained in
[`src/security/ROADMAP.md`](../../src/security/ROADMAP.md); public API facts are
defined by the headers under this directory.

Earlier copies of this page listed proposed interface additions as in progress
even though equivalent or differently named operations already exist in current
headers. Those entries are not current commitments. In particular, consult
`key_provider.h` and `security_api_contract.h` for their actual declarations,
and verify implementation status in the module roadmap and source.
