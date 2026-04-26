# LLM+RAID: Verteiltes Inferencing und Batch-Inferencing ueber mehrere ThemisDB-Instanzen

## Hintergrund

Die Analyse der RAID-Sharding-Architektur (`include/sharding/`) und des LLM-Subsystems (`include/llm/`) zeigt, dass alle Bausteine fuer verteiltes Inferencing bereits produktionsreif vorhanden sind — aber noch nicht miteinander verbunden sind.

Dieses Issue beschreibt die 7 konkreten Integrationspunkte und die 6-phasige Umsetzung.

**Bezuege:**
- Issue #4722 — LLM: Erkenntnisse aus llama.cpp/examples (Batching, KV-Sharing, Speculation)
- `docs/de/llm/AI_ECOSYSTEM_SHARDING_ARCHITECTURE.md` — High-Level-Konzept
- `include/llm/LLAMACPP_RAID_DISTRIBUTED_INFERENCE_INSIGHTS.md` — Detailanalyse (dieses Issue)

---

## Aktuelle Situation

Heute laeuft jede ThemisDB-Shard als eigenstaendiger `InferenceEngineEnhanced`-Knoten, aber:

- `LLMAQLHandler::executeInfer()` routet immer lokal, ohne `AdaptiveShardRouter` zu befragen
- `ShardStats` enthaelt keine LLM-Queue-Metriken (`pending_llm_requests`, `avg_queue_ms`)
- `SpeculativeDecoder` kennt nur lokale Draft-Modelle, keine Remote-Draft-Shards
- KV-State-Serialisierung (`llama_state_seq_save/load_file`) ist nicht in `RemoteExecutor` integriert
- Batch-Fan-out nach Domain-Affinitaet fehlt (kein `scatter_gather` fuer LLM-Batches)

---

## Erkenntnisse: Bestehende Bausteine

### 1  LoRA-Domain-Routing via Gossip (PRODUKTIONSREIF — verbinden fehlt)

`AdaptiveShardRouter::updateAdapterCapability()` + `AdapterCapabilityAnnouncement`:

- Jede Shard broadcasted Domain-Score (LEGAL, MEDICAL, TRANSACTION, GEOSPATIAL, …) via GossipProtocol
- `accuracy_delta` und `performance_delta_p99_ms` stehen als Routing-Kriterien bereit
- `LLMAQLHandler` nutzt dies heute nicht — immer lokale Inferenz

**Luecke:** `LLMAQLHandler::executeInfer()` muss `domain_hint` aus AQL-Parametern extrahieren und `routeByAdapterDomain()` aufrufen.

### 2  Batch Fan-out (Pattern vorhanden — nicht fuer LLM genutzt)

`DistributedAnalyticsSharding::executeDistributed()` + `ParallelWriteOptimizer::batchWrite()`:

- Scatter-Gather mit `std::async`-Fan-out ist fuer OLAP-Queries produktionsreif
- Dasselbe Muster auf Batch-Inferenz anwendbar: Requests nach Domain aufteilen, parallel an Shards senden, Ergebnisse mergen
- `scatter_timeout_ms = 30000` und `max_concurrent_shards = 10` bereits konfiguriert

### 3  Cross-Shard Speculative Decoding (Config vorhanden — remote nicht verdrahtet)

`InferenceEngineEnhanced::speculative_draft_model_id`:

- Aus `llama.cpp/examples/speculative.cpp`: n_draft=8-16 Token, Akzeptanzrate 70-85%, Speedup >= 2x
- `speculative_draft_model_id` unterstuetzt theoretisch Remote-Shards als Draft-Quelle
- `RemoteExecutor` wuerde Draft-Request an kleine Shard (z.B. Mistral-7B Q4) schicken, grosse Shard (Mixtral-8x7B) verifiziert

### 4  KV-Praefixe uebergreifend cachen (llama.cpp-Erkenntnisse, neue Integration)

Aus `llama.cpp/examples/save-load-state.cpp`:

- `llama_state_seq_save_file()` / `llama_state_seq_load_file()` serialisiert KV-Zustand
- System-Prompts (>= 256 Token) koennen von Shard 1 berechnet und per `ZeroCopyBuffer` + `RemoteExecutor` an Shard 2/3 transferiert werden
- Einsparung TTFT >= 30% fuer Requests mit identischen System-Prompts
- Einschraenkung: Nur bei identischem Basismodell + Quantisierung

### 5  LLM-Queue-Metriken in ShardStats (P1-Luecke)

`ShardStats` (in `sharding_interfaces.h`) fehlen heute:
- `pending_llm_requests`
- `avg_llm_queue_ms`
- `active_lora_adapters`

Ohne diese Felder kann `LEAST_LOADED`-Routing nicht auf aktueller LLM-Last basieren.

### 6  RAID-Fehlertoleranz fuer Inferenz-Shards

- RAID0: Circuit Breaker + Fallback-Shard (Warmup-Overhead)
- RAID1: Replica haelt identisches Modell + LoRA, sofortiger Failover via `RemoteExecutor`
- RAID5: `SIMDErasureCoder::computeParitySIMD()` fuer Modell-Weight-Partitionierung (47 GB Mixtral auf 8 Shards) — NICHT fuer KV-Cache-Failover

### 7  Embedding-Datenlokalitaet (Quick Win)

`LLMAQLHandler::executeEmbed()` ruft heute immer lokale Inferenz auf.  
`ShardingManager::GetShardForKey(doc_urn)` wuerde Embed-Request direkt an die Datenshard routen — kein Cross-Shard-Datentransfer noetig.

---

## Implementierungsphasen

### Phase 1 — Design/API-Vertrag (Target: Q3 2026)

- [ ] AQL-Syntax: `INFER 'model' text DOMAIN 'legal'` — Domain-Hint-Parameter definieren
- [ ] `LLMAQLHandler` Interface fuer `executeBatchInfer()` spezifizieren
- [ ] `ShardStats`-Erweiterung API-Vertrag: `pending_llm_requests`, `avg_llm_queue_ms`, `active_lora_adapters`
- [ ] Routing-Entscheidungsbaum dokumentieren: Domain-Routing → LEAST_LOADED → Fallback lokal

### Phase 2 — Core-Implementierung (Target: Q3 2026)

- [ ] `LLMAQLHandler::executeInfer()` extrahiert `domain_hint`, ruft `AdaptiveShardRouter::routeByAdapterDomain()` auf
- [ ] Fallback auf lokale Inferenz wenn kein Shard `accuracy_delta > 0.4` meldet
- [ ] `ContinuousBatchScheduler` schreibt `pending_requests` + `avg_queue_latency_ms` in `ShardStats`
- [ ] `AdaptiveShardRouter` nutzt `pending_llm_requests` fuer `LEAST_LOADED`-Routing
- [ ] `LLMAQLHandler::executeBatchInfer()` mit Domain-Splitting und `std::async`-Fan-out
- [ ] `LLMAQLHandler::executeEmbed()` nutzt `ShardingManager::GetShardForKey()` fuer Daten-Lokalitaet

### Phase 3 — Fehlerbehandlung & Edge Cases (Target: Q4 2026)

- [ ] Circuit-Breaker pro LLM-Operation (`infer`, `embed`, `rag`) — siehe auch Issue #0033
- [ ] Batch-Fan-out: Fehlgeschlagene Shard-Ergebnisse: Teilergebnis oder Retry auf anderer Shard?
- [ ] Gossip-TTL auf `AdapterCapabilityAnnouncement`: Stale-Score-Erkennung
- [ ] Cross-Shard Speculative Decoding: Remote-Draft-Shard konfigurieren + Fallback auf lokales Draft-Modell
  - `config.speculative_draft_model_id = "shard-a:model:mistral-7b-q4"`
  - Accept-Rate-Telemetrie als Cross-Shard-Metrik in `LLMMetricsCollector`
- [ ] Modell-Kompatibilitaetspruefung vor KV-State-Transfer (Architektur + Quantisierung)

### Phase 4 — Tests (Target: Q4 2026)

- [ ] Unit-Test: Mock-Gossip-Announcement → Domain-Routing auf hoechst-scorende Shard
- [ ] Unit-Test: `ShardStats::pending_llm_requests` nach `ContinuousBatchScheduler::submit()`
- [ ] Integration-Test: 3-Shard-Cluster (legal/medical/general) + Domain-Routing-Verifikation
- [ ] Integration-Test: Batch-64-Fan-out, 4 Shards, Ergebnisreihenfolge korrekt
- [ ] Integration-Test: Shard-Ausfall waehrend Batch → Circuit Breaker → Fallback
- [ ] Integration-Test: Remote-Draft-Shard Speculation, Accept-Rate-Messung
- [ ] Embedding-Lokalitaet: Embed-Request landet auf Daten-Shard verifizieren

### Phase 5 — Performance/Hardening (Target: Q1 2027)

- [ ] KV-Praefixe Cross-Shard: `llama_state_seq_save/load_file` + `ZeroCopyBuffer` + `RemoteExecutor` Binary-Transfer
- [ ] Benchmark: Batch-64, 4 Shards vs. Single-Shard seriell — Speedup >= 2x bestaetigt
- [ ] Benchmark: Domain-Routing Latenz-Overhead <= 5 ms
- [ ] Benchmark: KV-Praefixe Sharing TTFT-Einsparung >= 30%
- [ ] RAID5 Modell-Weight-Partitionierung fuer grosse Modelle (Mixtral-8x7B, 47 GB) evaluieren
- [ ] `SIMDErasureCoder` AVX2-Paritaet fuer Weight-Chunks (nicht KV-Cache) messen

### Phase 6 — Dokumentation & Abnahme (Target: Q1 2027)

- [ ] `docs/de/llm/AI_ECOSYSTEM_SHARDING_ARCHITECTURE.md` mit konkreten Datenfluessen aktualisieren
- [ ] Benchmark-Suite in `benchmarks/bench_llm_raid_pipeline.cpp` um Distributed-Inference-Cases erweitern
- [ ] RUNBOOK: Cross-Shard-Inference-Debugging, Circuit-Breaker-Monitoring, Gossip-Health
- [ ] Alle Metriken in Grafana-Dashboard: `domain_routing_fallback_total`, `batch_fan_out_latency_ms`, `speculation_accept_rate`, `kv_prefix_hit_rate`

---

## Akzeptanzkriterien

| Metrik                                            | Ziel                                  |
|---------------------------------------------------|---------------------------------------|
| Domain-Routing-Latenz-Overhead                    | <= 5 ms vs. direktes lokales Dispatch |
| Batch-Inferenz Speedup (64 Requests, 4 Shards)   | >= 2x vs. Single-Shard seriell       |
| Cross-Shard Speculative Decoding Accept-Rate      | >= 65 %                               |
| E2E-Latenz mit Remote-Draft-Shard                 | <= 1.5x lokale Inferenz              |
| KV-Praefixe Sharing TTFT-Einsparung              | >= 30 % (System-Prompt >= 256 Token) |
| Embedding-Lokalitaetsrouting Latenz-Overhead      | <= 10 % vs. Remote-Embed             |
| Circuit-Breaker-Failover bei Shard-Ausfall        | <= 100 ms bis Fallback-Request       |

---

## Betroffene Subsysteme

- `src/llm/llm_aql_handler.cpp` — Domain-Routing, Batch-Fan-out, Embedding-Lokalitaet
- `include/sharding/sharding_interfaces.h` — ShardStats LLM-Felder
- `include/llm/speculative_decoder.h` — Remote-Draft-Shard-Konfiguration
- `include/sharding/remote_executor.h` — Binary-State-Transfer fuer KV-Praefixe
- `benchmarks/bench_llm_raid_pipeline.cpp` — Distributed-Inference-Benchmarks
- `include/llm/LLAMACPP_RAID_DISTRIBUTED_INFERENCE_INSIGHTS.md` — Detaildokumentation

---

## Bekannte Risiken

| Risiko                                         | Minderung                                               |
|------------------------------------------------|---------------------------------------------------------|
| Gossip-Verzoegerung — stale Domain-Scores      | TTL auf Announcements; Fallback auf lokale Inferenz     |
| KV-State-Transfer blockiert GPU-Memory         | Nur bei idle Shard; Groesse <= 256 MB Limit             |
| Remote-Draft-Shard Latenz eliminiert Speedup   | A/B-Test vor Prod; Fallback auf lokales Draft-Modell    |
| Per-op Circuit-Breaker fehlt in LLMAQLHandler  | Issue #0033 bereits erstellt — als Blocker markieren    |
| Batch-Splitting-Heuristik zu aggressiv         | Batchgroesse per Domain-Hint konfigurierbar halten      |
