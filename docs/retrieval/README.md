---
Author: "ThemisDB Team"
Created: "2026-11-11"
Last Updated: "2026-11-11"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Retrieval module scope

`src/retrieval` contains artifact/manifest and validation facilities, not the
generic knowledge-query executor. See its [source README](../../src/retrieval/README.md).

The endpoint-independent provider/consumer/helper contract is documented under
[RAG](../rag/README.md) and [the internal RAG API](../rag/API.md).
