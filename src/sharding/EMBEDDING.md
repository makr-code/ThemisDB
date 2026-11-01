---
Author: "ThemisDB Contributors"
Created: "2026-10-19"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source_Level: level1
SOT_Domain: module-behavior
Canonical_References: `../../include/sharding/consistent_hash.h`, `../../include/sharding/shard_router.h`
Related_Issue: "#7381"
Related_Milestone: DOC-WEEKLY-2026-W44
---

Exact model backend not verifiable from final document text alone.

# Sharding Embedding and Placement Model

> **Assumption note:** The ring and fallback discussion describes the intended placement model and
> public contract; this audit did not establish that all router implementations use one ring,
> virtual-node count, fallback sequence, or locality policy. **TODO:** document these per concrete
> implementation/configuration after source and test verification.

## Purpose

This document explains how the sharding module embeds logical keys and ownership decisions into a topology-aware placement model. It is intended to complement the architecture and API docs by explaining how the module maps data and requests onto a physical shard layout.

## Placement model

The sharding layer uses a virtualized ring model rather than a flat lookup table. Each shard is represented within a consistent hash ring with a larger set of virtual nodes to reduce uneven distribution and reduce hot spots. This is implemented around the topology and routing logic in:

- `consistent_hash.cpp`
- `shard_router.cpp`
- `shard_topology.cpp`
- `locality_aware_router.cpp`

The practical effect is that the module can preserve stable ownership decisions while still adapting to topology change.

## Embedding conventions

The code and contracts model four key layers:

1. logical key / URN / tenant identity,
2. shard ownership decision,
3. replica or fallback route sequence,
4. transactional durability boundary.

The placement decision is not a convenience abstraction; it determines the set of shard operations that are legal for a request. A route decision without a valid target under the current topology is treated as an explicit failure, not as a silent reroute.

## Metadata and identity flow

The sharding stack makes ownership explicit through the URN and shard topology layer. A logical operation includes enough identity to map it to a physical shard, and the router/transaction layers validate the state before forwarding the work. This is essential for distributed operations, especially when data is partially moved due to migration or repair.

## Evaluation of ring behavior

The ring is designed to optimize for:

- deterministic outcomes from the same logical key,
- bounded remapping during membership change,
- minimal cross-shard fan-out during normal routing,
- predictable failover and repair sequences.

The trade-off is that ring-based partitioning does not eliminate load skew in all cases, and topology churn can still create temporary imbalance until the rebalancer reflects the new membership set.

## Fallback and locality assumptions

`AdaptiveShardRouter`, `LocalityAwareRouter`, and the routing fallbacks in `shard_router.cpp` make two important assumptions:

- the primary route is preferred when it is healthy,
- fallback is only valid when it is measured, bounded, and fail-closed when evidence is unavailable.

This is directly aligned with the design constraints in `FUTURE_ENHANCEMENTS.md`: no arbitrary fallback is allowed when the module cannot determine safe routing behavior.

## Operational meaning

For operators, the embedding model means that route decisions should be interpreted as a topology-ownership decision with consequences for replication, availability, and correctness. If the ring or topology becomes inconsistent, the symptom is not merely a slow query; it is a routing failure or a semantics violation in a distributed transaction path.
