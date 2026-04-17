---
type: enhancement
labels: ["type:enhancement", "module:distributed_knowledge", "priority:low", "status:open", "queue/copilot"]
milestone: v2.0.0
parent: DK-0-EPIC
session: 9
---

# [DK-8] distributed_knowledge: Performance-Benchmarks & Hardening

## Aufgabe

Sicherstellen, dass die Federation-Operationen die definierten Latenz-Targets
einhalten und das System unter kontinuierlicher Last (50 Runden / 1h) kein
Memory-Leak produziert. Alle vier Komponenten werden mit realistischen
N-Shard-Konfigurationen benchmarkt.

## Abgrenzung

| In Scope | Out of Scope |
|---|---|
| Benchmark: `triggerAggregation()` N=64 Shards × 100 Gradient-Keys | GPU-Beschleunigung der Aggregation |
| Benchmark: `FederatedRAGMerger::merge()` N=16 Shards × 50 Docs | Netzwerk-Latenz-Messung (liegt in observability/) |
| Benchmark: `handleInboundSummary()` Dedup-Throughput | Distributed Load Test (echter Netzwerk-Stack) |
| Benchmark: `publishFeedback()` End-to-End-Dispatch | Profiling der LLM-Inferenz |
| Memory-Leak-Test: 50 Runden ohne wachsende RSS | Automatische Performance-Regression-Erkennung |
| `benchmarks/bench_distributed_knowledge.cpp` (neu) | CI-Integration der Benchmarks (→ Future) |

## Idee / Konzept

Performance-Budgets sind **bindend** — nicht aspirational. Sie wurden aus
Produktionsanforderungen abgeleitet:

| Warum das Target? | Woher kommt es? |
|---|---|
| `triggerAggregation()` ≤ 500 ms | Federation läuft asynchron, aber Operator wartet auf Bestätigung |
| `merge()` ≤ 20 ms | Liegt im kritischen Pfad einer RAG-Query (p99-Budget: 100 ms) |
| `handleInboundSummary()` ≥ 10k msg/s | Gossip-Bus kann bei 100 Shards schnell 10k+ Nachrichten/s erzeugen |
| `publishFeedback()` ≤ 1 ms | Liegt im synchronen `recordFeedback()`-Pfad |

### Memory-Leak-Kriterium

```
50 Runden × {3 Shards, 100 Keys, 384-dim Embeddings}
RSS vor Runde 1:  R₀
RSS nach Runde 50: R₅₀
Assertion: R₅₀ - R₀ ≤ 5 MB
```

Hintergrund: `pending_gradients_`-Map wird nach jeder Runde geleert (`advanceRound()`).
Der Dedup-LRU-Cache hat fixe Größe (10.000 Einträge). Beide müssen stabil sein.

## Technische Details

### Benchmark-Datei: `benchmarks/bench_distributed_knowledge.cpp`

```cpp
// Struktur (Google Benchmark oder einfacher Chrono-Harness)
BENCH(BM_TriggerAggregation_N64) {
    // Setup: 64 MockShards mit je 100 Gradient-Keys
    // Measure: triggerAggregation() inkl. DP-Noise
    // Assert: p99 ≤ 500ms
}

BENCH(BM_FederatedRAGMerge_N16x50) {
    // Setup: 16 ShardRetrievalResult mit je 50 Docs
    // Measure: FederatedRAGMerger::merge()
    // Assert: p99 ≤ 20ms
}

BENCH(BM_FeedbackDedup_Throughput) {
    // Setup: CrossShardFeedbackSync mit 384-dim Embeddings
    // Measure: handleInboundSummary() × 100k Nachrichten
    // Assert: throughput ≥ 10k msg/s
}

BENCH(BM_PublishFeedback_Latency) {
    // Setup: CrossShardFeedbackSync + Mock-Gossip-Handler
    // Measure: publishFeedback() End-to-End
    // Assert: p99 ≤ 1ms
}
```

### Optimierungen falls Targets verfehlt werden

**`triggerAggregation()` zu langsam:**
- FedAvg-Aggregation parallelisieren (`std::transform + std::reduce`)
- DP-Noise-Generierung: `std::normal_distribution` ist bereits schnell,
  aber `mt19937_64` kann durch `RDRAND` ersetzt werden wenn vorhanden

**`merge()` zu langsam:**
- `std::sort` durch `std::partial_sort` ersetzen (nur top_k sortieren)
- Dedup-`unordered_set` durch Bitset ersetzen wenn doc_ids numerisch

**`handleInboundSummary()` zu langsam:**
- LRU-Cache durch `absl::flat_hash_set` ersetzen
- Embedding-Dim-Check in Fast-Path auslagern (nur wenn `validate_embedding_dim=true`)

### Memory-Leak-Test

```cpp
TEST(DistributedKnowledge, NoMemoryLeak_50Rounds) {
    size_t rss_before = getCurrentRSS(); // via /proc/self/status auf Linux
    for (int i = 0; i < 50; ++i) {
        submitGradientsAndAggregate(coordinator, 3_shards, 100_keys);
    }
    size_t rss_after = getCurrentRSS();
    EXPECT_LE(rss_after - rss_before, 5 * 1024 * 1024); // 5 MB
}
```

## Performance-Targets (bindend)

| Operation | Konfiguration | P99-Target | Messmethode |
|---|---|---|---|
| `triggerAggregation()` | N=64, 100 Keys | ≤ 500 ms | Chrono-Harness, 100 Iterationen |
| `FederatedRAGMerger::merge()` | N=16, 50 Docs | ≤ 20 ms | Chrono-Harness, 1000 Iterationen |
| `handleInboundSummary()` | 384-dim Embeddings | ≥ 10k msg/s | Throughput über 100k Nachrichten |
| `publishFeedback()` | Mock-Handler | ≤ 1 ms | Chrono-Harness, 10k Iterationen |
| Memory (50 Runden) | 3 Shards, 100 Keys | ≤ 5 MB RSS-Wachstum | `/proc/self/status` |

## Abhängigkeiten

- **Vorbedingung:** DK-6 (Integrationstest grün), DK-7 (Admin-API vollständig)
- **Keine** weiteren Blockaden

## Erfolgskriterien

- [ ] `benchmarks/bench_distributed_knowledge.cpp` vorhanden
- [ ] `triggerAggregation()` N=64, 100 Keys: p99 ≤ 500 ms (Benchmark-Ergebnis dokumentiert)
- [ ] `FederatedRAGMerger::merge()` N=16, 50 Docs: p99 ≤ 20 ms
- [ ] `handleInboundSummary()` Throughput: ≥ 10k msg/s bei 384-dim Embeddings
- [ ] `publishFeedback()` p99 ≤ 1 ms
- [ ] Memory-Leak-Test: RSS-Wachstum nach 50 Runden ≤ 5 MB
- [ ] Alle Performance-Ziele mit konkreten Messwerten in `CHANGELOG.md` dokumentiert
- [ ] Keine Regressions in bestehenden Test-Suites
- [ ] Falls ein Target verfehlt: Optimierung aus den genannten Vorschlägen angewandt
  und neues Messergebnis dokumentiert

## Definition of Done

Alle 4 Performance-Targets erreicht. Messergebnisse in
`src/distributed_knowledge/CHANGELOG.md` unter `v0.2.0 Performance` eingetragen.
Memory-Leak-Test grün.
