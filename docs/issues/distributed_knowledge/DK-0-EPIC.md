---
type: epic
labels: ["type:enhancement", "module:distributed_knowledge", "priority:high", "status:open", "queue/copilot"]
milestone: v2.0.0
---

# [DK-EPIC] Distributed Knowledge — RAID-5-Sharding der ThemisDB-Intelligenz

## Aufgabe

ThemisDB hat RAID-Sharding für **Daten** vollständig implementiert. Die Lernschichten
(`LoRA`, `RLAIF`, `ContinuousLearningOrchestrator`) sind shard-lokal — jeder Shard
lernt isoliert. Dieses Epic verbindet die vorhandenen Lern- und Transportkomponenten
zu einem **verteilten Intelligenzsystem**: Optimierungseinsichten fließen shard-übergreifend,
ohne dass Rohdaten die Shard-Grenzen überschreiten.

## Abgrenzung

| In Scope | Out of Scope |
|---|---|
| Verbindung vorhandener Komponenten (kein Neu-Training) | Neue ML-Algorithmen jenseits FedAvg/FedProx |
| 4 Verbindungsebenen A–D (Gossip, LoRA-Fed, RAG-Fed, RLAIF-Fed) | Neue Infrastruktur (kein neuer Gossip-Transport) |
| DP-Schutz auf Gradient-Ebene | Homomorphic Encryption (Post-Quantum-Schutz liegt in security/) |
| Admin-API + Privacy-Audit-Log | Full Observability Stack (liegt in observability/) |
| Performance-Benchmarks für Federation-Overhead | GPU-Beschleunigung der Aggregation |

## Idee / Konzept

**RAID-5 für Wissen:** Analog zu RAID-5 (Parität ohne vollständige Replikation)
aggregiert FedAvg lokale Gradienten zu einem globalen Delta — keiner kennt das
vollständige Trainingsset eines anderen. Der Unterschied zu RAID-1 (volle Replikation):
kein Shard sendet seinen vollständigen Adapter, sondern nur einen DP-verrauschten
Gradientenvektor. Das globale Delta ist die "Parität" des Wissenssystems.

```
RAID-5 Parität:   XOR(Block₁, Block₂, … Blockₙ)  = Parität
FedAvg-Parität:   Σₖ (nₖ/N) · θₖ + N(0,σ²)       = GlobalDelta
```

Vier Verbindungsebenen realisieren die vollständige Infrastruktur:
- **A — Adapter-Gossip:** Routing-Qualität sofort, kein Training nötig
- **B — Federated LoRA:** Kern des verteilten Lernens, FedAvg + DP
- **C — Federated RAG:** LLM sieht Wissen aller Shards bei jeder Anfrage
- **D — Federated RLAIF:** DBA-Feedback auf einem Shard verbessert alle Shards

## Technische Grundlagen

| Forschungsreferenz | Anwendung in ThemisDB |
|---|---|
| McMahan et al. (2017) FedAvg | `LoRAFederationCoordinator` Aggregation |
| Dwork & Roth (2014) DP Gaussian | `applyDifferentialPrivacy()` σ-Berechnung |
| Li et al. (2020) FedProx | Heterogene Shard-Workloads (Security vs. Schema) |
| Cormack et al. (2009) RRF | `FederatedRAGMerger` Rang-Fusion |

## Sub-Issues (Implementierungsreihenfolge)

| Issue | Titel | Session | Abhängigkeiten | Priorität |
|---|---|---|---|---|
| [DK-1](./DK-1-build-tests.md) | Build-System & Unit-Tests | 1 | — | 🔴 Kritisch |
| [DK-2](./DK-2-layer-a-gossip.md) | Layer A — Adapter-Gossip-Integration | 2+3 | DK-1 | 🔴 Hoch |
| [DK-3](./DK-3-layer-b-fedlora.md) | Layer B — Federated LoRA | 4 | DK-1, DK-2 | 🔴 Hoch |
| [DK-4](./DK-4-layer-c-rag.md) | Layer C — Federated RAG Merge | 5 | DK-1, DK-2 | 🟠 Mittel |
| [DK-5](./DK-5-layer-d-rlaif.md) | Layer D — Federated RLAIF | 6 | DK-1 | 🟠 Mittel |
| [DK-6](./DK-6-integration-tests.md) | End-to-End Integration & Privacy | 7 | DK-2…DK-5 | 🔴 Hoch |
| [DK-7](./DK-7-admin-api.md) | Admin-API + Privacy-Audit-Log | 8 | DK-3, DK-6 | 🟠 Mittel |
| [DK-8](./DK-8-performance.md) | Performance-Benchmarks & Hardening | 9 | DK-6, DK-7 | 🟡 Niedrig |

## Erfolgskriterien (Epic-Ebene)

- [ ] Alle 8 Sub-Issues closed
- [ ] Kein Shard-Klartext verlässt den Shard — nachgewiesen durch Integrationstest
- [ ] FedAvg-Runde auf 3 Shards: globales Delta verbessert oder hält Accuracy (kein Rückschritt)
- [ ] Federated RAG Recall@10 ≥ +15 % gegenüber shard-lokalem Retrieval
- [ ] DBA-Feedback von Shard A erreicht `RLAIFTrainer` auf allen anderen Shards
- [ ] Domain-Routing via Gossip: Precision@3 ≥ 80 % für `domain_hint`-Queries
- [ ] DP-Budget-Monitoring: Runde 51 nach 50×ε=0.1 wird abgelehnt
- [ ] Admin-API liefert `GET /admin/federation/stats` mit korrektem `current_round`
- [ ] Performance: `triggerAggregation()` N=64 ≤ 500 ms · `merge()` N=16 ≤ 20 ms
- [ ] Docs-lint sauber für alle neuen Markdown-Dokumente

## Referenz-Dokumente

- `docs/en/research/DISTRIBUTED_KNOWLEDGE_FEDERATION.md` — Forschungspaper EN
- `docs/de/research/VERTEILTES_WISSEN_FEDERATION.md` — Forschungspaper DE
- `src/distributed_knowledge/ARCHITECTURE.md` — Modularchitektur
- `src/distributed_knowledge/ROADMAP.md` — Session-Plan mit Akzeptanzkriterien
- `src/distributed_knowledge/FUTURE_ENHANCEMENTS.md` — Post-v1.0-Roadmap
