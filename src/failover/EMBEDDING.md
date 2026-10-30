---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Embedding Scope: Failover

## Purpose

This page records the embedding boundary for developers auditing failover
behavior. `src/failover/` and `include/failover/` contain no embedding generation,
vector search, semantic similarity, or embedding-model interface. The module
does not consume vector data to detect node failure or choose a promotion target.

## Decision inputs

The implemented decision path uses health-check outcomes, consecutive-failure
counts, replication-manager quorum and replica-health results, configuration
thresholds, and epoch-fencing tokens. These are control-plane signals, not
embeddings. See [MODELING.md](MODELING.md) for their role and limitations.

## Integration guidance

Do not introduce a vector or embedding dependency to explain, rank, or gate
failover decisions without a separate design and an explicit safety analysis.
Failover must remain governed by validated health, quorum, and fencing contracts;
semantic similarity is not evidence that a node is safe to promote.

## Source basis

- `include/failover/auto_failover_manager.h`
- `include/failover/failover_api_contract.h`
- `src/failover/auto_failover_manager.cpp`

For implemented interfaces, see [API.md](API.md). For trust assumptions, see
[TRUST.md](TRUST.md).
