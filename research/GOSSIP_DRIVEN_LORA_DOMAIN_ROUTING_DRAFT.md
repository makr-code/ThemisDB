# Gossip-Driven LoRA Domain Routing for Distributed Inference Fabrics

**Status**: Draft  
**Version**: 0.1  
**Last Updated**: 2026-04-19  
**Target Venue**: Middleware, ICDCS, SoCC

---

## I. Abstract

Dieses Paper beschreibt ein domänenbewusstes Routing für verteilte Inferenz, das über Gossip propagierte Adapter-Fähigkeiten nutzt. Requests werden auf Shards mit bester Domänenaffinität und vertretbarer Tail-Latenz gelenkt. Der Beitrag ist ein reproduzierbares Entscheidungsmodell aus Accuracy- und Performance-Signalen.

## II. Problem Statement

Statisches Sharding ignoriert, dass LoRA-Adapter auf einzelnen Shards unterschiedliche Domänenqualität besitzen. Dadurch gehen Genauigkeit und SLOs verloren. Gesucht ist ein leichtgewichtiges, fehlertolerantes Routing mit schneller Konvergenz ohne zentrale Koordination.

## III. Research Questions

1. Wie schnell konvergiert Capability-Wissen über Gossip bei Clustergrößen 4-64?
2. Welche Qualitäts- und Latenzgewinne bringt Domain-Routing gegenüber Hash-Only-Routing?
3. Welche Robustheit zeigt das Verfahren bei stalen Announcements oder Teilausfällen?
4. Wie sensibel sind Ergebnisse gegenüber Schwellenwerten und TTL?

## IV. Repository Evidence Registry

- E1: `include/sharding/adaptive_shard_router.h`
- E2: `include/distributed_knowledge/adapter_capability_announcement.h`
- E3: `include/sharding/remote_executor.h`
- E4: `tests/test_sharding_gossip.cpp`
- E5: `tests/test_gossip_custom_handler.cpp`
- E6: `research/GOSSIP_AWARE_LORA_ROUTING_DRAFT.md`
- E7: `research/RAID_SHARDING_LLM_DISTRIBUTED_INFERENCE.md`

## V. Measurement Plan

- Vergleich: Hash Routing vs Domain-aware Routing.
- Lastprofile: gemischte Domänenanfragen mit zeitlich driftender Verteilung.
- Metriken:
  - Domain-hit-rate
  - End-to-end p95/p99
  - Qualitätsscore-Differenz pro Domäne
  - Routing-Overhead und Fehlerrate unter Ausfällen

## VI. Claim Boundaries

**Unterstützte Claims:**
- Router- und Capability-Strukturen sind vorhanden.
- Gossip-basierte Tests existieren.

**Deferred Claims:**
- Großskalige Konvergenzdaten (N > 32).
- Produktionsnahe Störungsserien (partition, churn).

## VII. Next Milestones

- M1: Parameterstudie (TTL, score thresholds)
- M2: Stale-info und failover Experimente
- M3: Abgleich Qualität vs Latenzbudget
- M4: v0.2 mit formaler Entscheidungsfunktion
