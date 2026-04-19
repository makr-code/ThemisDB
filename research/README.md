# ThemisDB Research & Papers

This directory contains research papers, draft manuscripts, architectural analyses, and design documents for ThemisDB development.

## Contents

### Published & Finalized Papers
- (To be added)

### Research Drafts & Work-in-Progress
- [RAID_SHARDING_LLM_DISTRIBUTED_INFERENCE.md](RAID_SHARDING_LLM_DISTRIBUTED_INFERENCE.md) — RAID sharding strategy and LLM distributed inference integration
- [ACID_CONSTRAINED_RAG_DRAFT.md](ACID_CONSTRAINED_RAG_DRAFT.md) — ACID transaction semantics + RAG integration with measured benchmarks
- [SERIALIZABLE_RAG_UNDER_CONTENTION_DRAFT.md](SERIALIZABLE_RAG_UNDER_CONTENTION_DRAFT.md) — Isolation-aware RAG quality/latency trade-offs under contention
- [QUERY_ENGINE_AQL_GRAPHQL_UNIFICATION_DRAFT.md](QUERY_ENGINE_AQL_GRAPHQL_UNIFICATION_DRAFT.md) — Unified query engine for AQL and GraphQL
- [GOSSIP_AWARE_LORA_ROUTING_DRAFT.md](GOSSIP_AWARE_LORA_ROUTING_DRAFT.md) — Federated LoRA routing via epidemic gossip protocols
- [GOSSIP_DRIVEN_LORA_DOMAIN_ROUTING_DRAFT.md](GOSSIP_DRIVEN_LORA_DOMAIN_ROUTING_DRAFT.md) — Domain-aware LoRA routing with capability gossip and failover
- [LLM_PROCESSING_OPTIMIZATION_PATTERNS.md](LLM_PROCESSING_OPTIMIZATION_PATTERNS.md) — Inference optimization patterns from llama.cpp (batching, speculative decoding, KV-cache)
- [CONTINUOUS_BATCHING_DATABASE_NATIVE_LLM_DRAFT.md](CONTINUOUS_BATCHING_DATABASE_NATIVE_LLM_DRAFT.md) — Scheduler/KV cache trade-offs for DB-native LLM serving
- [COST_AWARE_HYBRID_RETRIEVAL_PLANNING_AQL_DRAFT.md](COST_AWARE_HYBRID_RETRIEVAL_PLANNING_AQL_DRAFT.md) — Cost-based plan selection for lexical+vector+graph retrieval

### Planned Research Topics
- (See drafts above for current work-in-progress topics)

### Research Tooling & Method Notes
- [ARXIV_QUERY_STRATEGY_TOP4_2026-04-19.md](ARXIV_QUERY_STRATEGY_TOP4_2026-04-19.md) — Pre-search strategy and query protocol for four prioritized paper drafts

## Guidelines for Contributors

1. **Naming Convention**: Use descriptive, topic-focused filenames (e.g., `TOPIC_ARCHITECTURE_ANALYSIS.md`).
2. **Structure**: Begin with abstract, include implementation evidence from repo, add measured benchmarks where applicable.
3. **Evidence Anchors**: Reference actual code files, test cases, and benchmark harnesses with line numbers.
4. **Versioning**: Track paper version/status in frontmatter or first section.
5. **Commit Early**: Papers must be committed to git to persist across sessions. Do not rely on working-tree-only edits.

## Status Tracking

Papers should include version and status information:
- **Status**: Draft, In Review, Submitted, Published
- **Last Updated**: Date of most recent meaningful update
- **Target Venue**: (if applicable) VLDB, SIGMOD, ICDE, etc.

---

*Last Updated: 2026-04-19*
