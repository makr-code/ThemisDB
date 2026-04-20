## Summary
Analyse der Verzahnung zwischen RAID-Sharding-System und LLM-Inferencing zeigt mehrere produktionsreife sowie noch zu implementierende Integrationspunkte. Diese Issue dokumentiert den konkreten Umsetzungsplan für:
- LoRA-Domain-Routing via Gossip-Announcements (PRODUKTIONSREIF)
- Batch-Inferenz Fan-out über mehrere Shards (READY TO IMPLEMENT)
- Cross-Shard Speculative Decoding (DESIGN PHASE)
- KV-Cache-Präfix-Sharing zwischen Shards (ADVANCED)

## Problem
Heute wird Inferencing in ThemisDB immer lokal auf der anfragenden Shard durchgeführt, auch wenn eine andere Shard bessere LoRA-Domänen-Affinität oder bessere Hardware-Kapazität hätte. Das RAID-Sharding-System mit seinen Gossip-Protokollen, `AdaptiveShardRouter` und `RemoteExecutor`-Komponenten bietet die Infrastruktur für intelligentes verteiltes Inferencing, aber die Integration zu `LLMAQLHandler` fehlt.

## Goals
- LoRA-Domain-Routing aktivieren: Requests auf Shard mit höchstem `accuracy_delta` für die Domain leiten
- Batch-Inferenz skalierbar machen: Fan-out über mehrere Shards nach Domain-Affinität
- Speculative Decoding über Shards ermöglichen: kleine Draft-Shard + große Verify-Shard
- KV-Präfix-Sharing implementieren: System-Prompts einmalig berechnen und over Shards verteilen

## Non-Goals
- Vollständiger Refactor der Inference-Pipeline; nur Integrationspunkte aktivieren
- Kein RAID5-Parity-Erasure-Coding für KV-Cache (zu hochfrequent für Parity-Recompute)

## Scope
Betroffene Bereiche:
- `include/llm/` — Integration Points
- `include/sharding/` — Bestehende Komponenten nutzen (AdaptiveShardRouter, RemoteExecutor, ShardStats)
- `src/llm/llm_aql_handler.cpp` — Routing-Logik einbauen
- `benchmarks/bench_llm_raid_pipeline.cpp` — Neue Distributed-Inference-Cases

## Reference Architecture
Siehe ausführliche Dokumentation in `include/llm/LLAMACPP_RAID_DISTRIBUTED_INFERENCE_INSIGHTS.md`:
- Topologie: Jede Shard = Storage + Compute + LLM-Engine
- Routing-Mechanismus via `AdaptiveShardRouter::updateAdapterCapability()` + Gossip
- Remote Execution via `RemoteExecutor` mit mTLS + Circuit Breaker
- Batch-Fan-out nach `DistributedAnalyticsSharding`-Muster
- Speculative Decoding zwischen Draft-Shard und Verify-Shard

## Implementation Phases

### Phase 1: Domain-Routing aktivieren (P1, Target: Q3 2026)
- [ ] `LLMAQLHandler::executeInfer()` extrahiert `domain_hint` aus AQL-Parametern
- [ ] Aufruf `AdaptiveShardRouter::routeByAdapterDomain(domain_hint)` statt lokalem Dispatch
- [ ] Fallback auf lokale Inferenz wenn keine passende Shard hohe Affinität meldet
- [ ] Unit-Test: Mock-Gossip-Announcement → Routing zu höchst-scorender Shard
- [ ] Fehler-Szenarien testen: Keine Shard mit Domain → lokale Inferenz + Metrik

### Phase 2: LLM-Queue-Metriken in ShardStats (P1, Target: Q3 2026)
- [ ] `ShardStats` um Felder erweitern: `pending_llm_requests`, `avg_llm_queue_ms`, `active_lora_adapters`
- [ ] `ContinuousBatchScheduler` schreibt Metriken in `ShardStats`
- [ ] `AdaptiveShardRouter` nutzt `pending_llm_requests` für `LEAST_LOADED`-Routing
- [ ] Integration-Test: 3 Shards unterschiedlich belastet → Routing auf niedrigst-ausgelastete

### Phase 3: Batch-Inferenz Fan-out (P2, Target: Q4 2026)
- [ ] `LLMAQLHandler::executeBatchInfer()` implementieren
- [ ] Batch nach Domain-Affinität splitten (analog `DistributedAnalyticsSharding`)
- [ ] Paralleler Dispatch via `RemoteExecutor::post()` mit `scatter_timeout_ms = 30000`
- [ ] Ergebnisse in Eingangsreihenfolge mergen
- [ ] Benchmark: Batch-Size 64, 4 Shards → Speedup ≥ 2× vs. Single-Shard

### Phase 4: Cross-Shard Speculative Decoding (P2, Target: Q4 2026)
- [ ] `SpeculativeDecoder` erhält `remote_draft_shard_id`-Konfiguration
- [ ] Draft-Requests via `RemoteExecutor::post()` an Draft-Shard
- [ ] Accept-Rate-Telemetrie als Cross-Shard-Metrik in `LLMMetricsCollector`
- [ ] A/B-Test: Lokales vs. Remote-Draft-Modell — Latenz, Akzeptanzrate
- [ ] Performance-Ziel: Accept-Rate ≥ 65%, E2E-Latenz ≤ 1.5× lokale Inferenz

### Phase 5: KV-Präfix-Sharing (P2, Target: Q1 2027)
- [ ] `LLMPrefixCache` Cross-Shard-Export via `llama_state_seq_save_file()` + `ZeroCopyBuffer`
- [ ] `RemoteExecutor` für binären State-Transfer erweitern
- [ ] Empfangende Shard lädt KV-State via `llama_state_seq_load_file()`
- [ ] Modell-Kompatibilitätsprüfung (identische Architektur+Quantisierung) vor Transfer
- [ ] Dokumentation: Einschränkung auf identische Basismodelle

### Phase 6: Embedding-Datenlokalität & Doku (P3, Target: Q1 2027)
- [ ] `LLMAQLHandler::executeEmbed()` nutzt `ShardingManager::GetShardForKey()`
- [ ] Performance-Tests: Embed-Latenz mit/ohne Cross-Shard-Transfer
- [ ] `docs/de/llm/AI_ECOSYSTEM_SHARDING_ARCHITECTURE.md` mit Datenflüssen aktualisieren
- [ ] `benchmarks/bench_llm_raid_pipeline.cpp` um Distributed-Inference-Cases erweitern

## Acceptance Criteria
- Domain-Routing-Latenz: ≤ 5ms Overhead vs. direktes Dispatch
- Batch-Speedup (64 Requests, 4 Shards): ≥ 2× vs. Single-Shard seriell
- Speculative Decoding Accept-Rate: ≥ 65%
- KV-Prefix-Sharing TTFT-Einsparung: ≥ 30% bei gemeinsamen System-Prompts
- Circuit-Breaker-Failover: ≤ 100ms bis Fallback
- Alle neue Metriken in Prometheus + Grafana sichtbar und alarmierbar

## Proposed Initial Parameters
- `max_concurrent_shards = 10` (bestehend, Limit beibehalten)
- `scatter_timeout_ms = 30000` (bestehend)
- `max_tokens_per_batch = 8192` pro Shard (bestehend)
- `speculative_draft_tokens = 8–16` für Remote-Draft

## Risks
| Risiko | Minderung |
|--------|-----------|
| Gossip-Verzögerung → veraltete Domain-Scores | TTL auf Announcements; Health-Check-Intervall anpassen |
| KV-State-Transfer blockiert GPU | Nur bei idleing Shard; max 256 MB |
| Remote-Draft-Latenz eliminiert Speedup | A/B-Test vor Prod; Fallback auf lokal |
| Cross-Shard-Interferenz bei Parallelität | `max_concurrent_shards = 10` als Hard-Limit |

## Deliverables
- Produktionsreife Domain-Routing-Integration
- Batch-Inferenz mit Scatter-Gather-Pattern
- Speculative Decoding über Shards (mit A/B-Telemetrie)
- KV-Präfix-Sharing-Grundlage (mit Kompatibilitätsprüfung)
- Erweiterte Benchmark-Suite für Distributed-Inference
- Aktualisierte Dokumentation in `docs/de/llm/`

## Related Issues
- #4722 — LLM Inference Optimierungen aus llama.cpp/examples
- #0033 — Per-Operation Circuit Breaker (bestehende Blockierung)
