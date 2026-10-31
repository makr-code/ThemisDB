---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: ThemisDB documentation orchestration specialist
Human_Review: false
Human_Author: unknown
Source level: level1
SOT domain: module-behavior
Canonical source references:
  - SECURITY.md
  - TRUST.md
  - ../../include/graph/
  - ../../include/server/graph_api_handler.h
  - ../../include/index/graph_index.h
Related milestone: DOC-WEEKLY-2026-44
AI_Model_Note: Exact model backend not verifiable from final document text alone.
---

# Graph production-integration checklist (non-normative)

This page is an integration review aid, not a source-backed deployment policy or proof of
production readiness. The graph C++ APIs do not, by themselves, establish request authentication,
tenant authorization, startup configuration validation, deployment limits, audit retention, or
service rollback behavior.

Before deploying an integration, the owning service and security teams should define and verify:

- authorization and tenant isolation at the server/API boundary;
- caller-supplied traversal limits appropriate to the workload;
- handling for truncated, timed-out, failed, or partial results;
- accelerator and shard failure behavior for the selected API;
- persistence, transaction, backup, restore, and audit requirements in the owning index/storage
  and deployment components.

Do not infer that these controls are enabled from graph constraint APIs or a server constructor
dependency. See [graph trust boundaries](TRUST.md) and the repository
[security policy](../../SECURITY.md).

**TODO:** the previous document named `THEMIS_PRODUCTION_MODE` and `THEMIS_ENVIRONMENT` as
requirements without a graph-source/configuration reference. The responsible deployment owner
must establish any applicable environment-variable policy.
