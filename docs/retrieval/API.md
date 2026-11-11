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

# Retrieval API scope

The `include/retrieval/retrieval.h` umbrella does not declare the generic RAG API.
For knowledge queries use [`rag::RAGProvider`](../rag/API.md). Artifact/manifest
APIs remain described by the [retrieval module sources](../../src/retrieval/README.md).
