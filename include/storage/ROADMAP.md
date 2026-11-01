---
Author: "ThemisDB Contributors"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References:
  - "../../src/storage/ROADMAP.md"
  - "../../ROADMAP.md"
  - "tiered_storage.h"
  - "federated_blob_router.h"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Storage header roadmap

The canonical storage delivery and status plan is
[`../../src/storage/ROADMAP.md`](../../src/storage/ROADMAP.md), with the
repository maturity aggregate in [`../../ROADMAP.md`](../../ROADMAP.md).
This companion file does not maintain a separate API-stability or delivery
status.

Earlier copies of this file described `federated_blob_router.h` as planned even
though that header is present, and described other proposed headers without
current implementation evidence. Do not treat those old entries as active
work. The current public surface is indexed in [`README.md`](README.md).

TODO: add a header-specific roadmap item only when it is linked to an issue,
has an owner/acceptance criteria, and is reflected in the canonical module
roadmap.
