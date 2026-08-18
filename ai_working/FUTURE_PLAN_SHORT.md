# FUTURE_PLAN_SHORT.md

# ThemisDB Future Plan — Management Summary

**Status:** Draft  
**Date:** 2026-06-01

## Purpose

This document provides a concise management-level summary of the long-term architecture direction for ThemisDB in the areas of RAG, graph, vectors, tensor structures, and LLM/LoRA integration.

## Core Direction

ThemisDB should evolve toward a four-layer architecture:

1. **ANN Frontdoor (HNSW / DiskANN)**  
   Fast semantic candidate retrieval.

2. **Tensor Mid-Layer**  
   Compression, routing, summaries, and structural prioritization.

3. **Graph Truth Layer**  
   Evidence, constraints, provenance, and exact semantic validation.

4. **LLM / LoRA Final Layer**  
   Grounded answer generation and domain adaptation.

## Why This Matters

Traditional RAG systems focus too heavily on retrieving top-k chunks. This becomes inefficient and brittle in systems that require:

- multi-hop reasoning,
- provenance and trust,
- distributed retrieval,
- domain-specific adaptation,
- governance and auditability.

ThemisDB already contains strong building blocks across graph, sharding, training, and tensor-oriented capabilities. The next step is to turn these into a coherent layered architecture.

## Strategic Long-Term Targets

### 1. Unified Knowledge Tensor Layer
A shared tensor-oriented layer connecting graph knowledge, embeddings, document knowledge, process knowledge, adapter knowledge, and provenance.

### 2. Tensor-native Graph Reasoning
Use factorized relation propagation, tensor contraction, and approximate relational inference to accelerate candidate generation and prioritization before exact graph validation.

### 3. Federated / Cross-shard Tensor Summaries
Exchange compact tensor-based shard summaries instead of moving large graph regions across instances. Load exact data only when needed.

## Expected Benefits

- lower latency,
- smaller prompt sizes,
- reduced cross-shard network load,
- better evidence quality,
- more scalable RAG,
- stronger adapter lifecycle management,
- improved auditability.

## Key Risks

- architectural complexity,
- harder debugging due to approximation,
- governance risks if approximate methods are misused,
- maintenance overhead for versioned multi-layer representations.

## Recommended Next Steps

1. Inventory the current architecture by layer.
2. Produce a gap analysis.
3. Identify quick wins.
4. Define implementation issues and roadmap.
5. Align ANN, tensor, graph, and LoRA planning.

## Executive Statement

**ThemisDB should become a layered knowledge retrieval architecture in which ANN finds quickly, tensor structures condense intelligently, graph structures verify exactly, and LLM/LoRA generate on a grounded and auditable basis.**
