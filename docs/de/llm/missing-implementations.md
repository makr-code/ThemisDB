# LLM-Modul — Fehlende / Unvollständige Implementierungen

<!-- Status: current | validated: 2026-03-09 -->
<!-- Primärdokumentation: ../../../src/llm/ -->

Dieser Report dokumentiert Funktionen und Komponenten, die in `src/llm/ROADMAP.md`, `src/llm/ARCHITECTURE.md` oder anderen Primary-Docs als implementiert beschrieben werden oder als geplant gelten, jedoch bei der Reality-Check-Prüfung als **nicht vollständig umgesetzt** oder **als Stub** befunden wurden.

Prüfstand: 2026-03-09 | Branch: `develop`

---

# LLM-Modul — Fehlende / Unvollständige Implementierungen

<!-- Status: current | validated: 2026-03-11 -->
<!-- Primärdokumentation: ../../../src/llm/ -->

Dieser Report dokumentiert Funktionen und Komponenten, die in `src/llm/ROADMAP.md`, `src/llm/ARCHITECTURE.md` oder anderen Primary-Docs als implementiert beschrieben werden oder als geplant gelten, jedoch bei der Reality-Check-Prüfung als **nicht vollständig umgesetzt** oder **als Stub** befunden wurden.

Prüfstand: 2026-03-11 | Branch: `develop`

---

## 1. ~~AdaptiveVRAMAllocator — GPU-Allokation als Stub~~ ✅ GELÖST (LLM-MISSING-001)

> **Gelöst in Branch `copilot/llm-missing-001-active-vram-allocator`**
>
> `ActiveVRAMAllocator` wurde als neue, eigenständige Klasse implementiert
> (`include/llm/active_vram_allocator.h`, `src/llm/active_vram_allocator.cpp`).
> Die Stub-Methoden in `AdaptiveVRAMAllocator` (`allocateWithFragmentation`,
> `handleOutOfMemory`) delegieren jetzt an `ActiveVRAMAllocator`.
>
> **Was implementiert wurde:**
> - Echte GPU-Memory-Allokation über `GPUMemoryManager` (cudaMalloc / CPU-Fallback)
> - LRU-basierte Eviction (`evictLRU`, `evictOwner`)
> - Defragmentierung (`defragment`)
> - CPU-Spilling (`spillLRUToCPU`, `restoreFromCPU`)
> - OOM-Recovery-Pipeline (`handleOOM`) in der Reihenfolge: Eviction → Defrag → Spill
> - OOM-Callback-Benachrichtigungen
> - VRAM-Waste-Tracking (Padding-Verschwendung, Fragmentation-Prozent)
> - Thread-sichere Implementierung
> - 29 Testfälle in `tests/llm/test_active_vram_allocator.cpp`
> - Benchmark in `benchmarks/bench_active_vram_allocator.cpp`

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/llm/ROADMAP.md` §"Speculative Decoding for Latency Reduction"; `src/llm/FUTURE_ENHANCEMENTS.md` §"Security / Reliability" (VRAM-Caps für Draft-Modell) |
| **Erwartet** | `AdaptiveVRAMAllocator::allocateWithFragmentation()` alloziert physisch GPU-VRAM; `handleOutOfMemory()` setzt OOM-Recovery-Strategien (Eviction, Defragmentierung, CPU-Spilling) um |
| **Status** | ✅ **Gelöst** — `ActiveVRAMAllocator` implementiert alle geforderten Strategien |
| **Lösung** | `include/llm/active_vram_allocator.h`, `src/llm/active_vram_allocator.cpp` |

---

## 2. ~~InferenceEngineEnhanced — KV-Cache-Prewarming und Embedding-basiertes Cache-Lookup~~ ✅ GELÖST

> **Gelöst in Branch `copilot/implement-kv-cache-prewarming`**
>
> **Was implementiert wurde:**
> - `prewarmCache()`: Berechnet echte Embeddings via `computeEmbeddingForCache()` für jeden Prompt,
>   schätzt Token-IDs und speichert den Eintrag im `LLMPrefixCache` (`prefix_cache_->put()`).
> - `checkCache()`: Ersetzt den Null-Vektor durch `computeEmbeddingForCache(request.prompt)` —
>   der Lookup nutzt jetzt HNSW-basierte Ähnlichkeitssuche.
> - `updateCache()`: Berechnet echtes Embedding und schätzt Token-Sequenz; speichert beides im Cache.
> - Neuer privater Hilfsmethode `computeEmbeddingForCache(text)`: Holt den ersten verfügbaren
>   Plugin-Zeiger (lock-free während `embed()`), ruft `plugin->embed(text)` auf und gibt bei
>   Fehler einen leeren Vektor zurück (graceful degradation).
> - 3 neue Tests (Tests 21–23) in `tests/test_inference_engine_enhanced.cpp`.

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/llm/ARCHITECTURE.md` §"KV-Cache Reuse (Prefix Cache)"; `src/llm/ROADMAP.md` §"Context caching (KV-cache reuse)" |
| **Erwartet** | Prefix-Cache kann vorgewärmt werden durch Pre-Berechnung von KV-Cache-Einträgen für häufige Prompts; Cache-Lookup nutzt Embedding-Ähnlichkeit für Fuzzy-Treffer |
| **Status** | ✅ **Gelöst** — `computeEmbeddingForCache()` liefert echte Embeddings; alle drei Cache-Methoden nutzen sie |
| **Lösung** | `include/llm/inference_engine_enhanced.h`, `src/llm/inference_engine_enhanced.cpp` |

---

## 3. DocsAssistant — LLM-Completion-Macro nicht definiert

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/llm/docs_assistant.cpp` — behauptet LLM-basierte Antwortgenerierung |
| **Erwartet** | `DocsAssistant::generateAnswer()` generiert Antworten mittels LLM-Inferenz (`THEMIS_LLM_COMPLETE` macro oder direkter Engine-Aufruf) |
| **Beobachtet** | Der Aufruf `THEMIS_LLM_COMPLETE(prompt.str())` ist auskommentiert (`// TODO: Undefined macro`); Rückgabewert ist immer der hardcodierte String `"[LLM completion placeholder]"` |
| **Evidence** | `src/llm/docs_assistant.cpp` Zeilen 251–252; Datei-Header: `TODOs: 1` |
| **ROADMAP-Status** | Nicht im ROADMAP erfasst |
| **Issue-Titelvorschlag** | `[llm] Wire DocsAssistant::generateAnswer() to actual LLM inference engine` |
| **Label-Vorschläge** | `type:bug`, `priority:low`, `llm`, `status:open` |

---

## 4. AsyncInferenceEngine — RAG-Kontext-Enkodierung unvollständig

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/llm/ARCHITECTURE.md` §"Standard Inference" (RAG-Integration durch `src/rag/llm_integration.cpp`) |
| **Erwartet** | `AsyncInferenceEngine::submitWithRAGContext()` kodiert RAG-Dokumente strukturiert in den Prompt (z.B. XML-Tags, Chat-Template, strukturierter JSON-Context) |
| **Beobachtet** | RAG-Kontext wird naiv als `"Context:\n<doc>\n\nQuestion: <prompt>"` konkateniert; kein Chat-Template, kein strukturiertes Format, kein Deduplizieren |
| **Evidence** | `src/llm/async_inference_engine.cpp` Zeile 373 (`// TODO: Properly encode RAG context in request`) |
| **ROADMAP-Status** | Nicht im ROADMAP erfasst |
| **Issue-Titelvorschlag** | `[llm] Implement structured RAG context encoding in AsyncInferenceEngine::submitWithRAGContext()` |
| **Label-Vorschläge** | `type:feature`, `priority:low`, `llm`, `rag`, `status:open` |

---

## 5. Federated Inference — Nicht implementiert

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/llm/ROADMAP.md` §"Planned Features / Remaining" (Issue: #1928) |
| **Erwartet** | Verteilte Inferenz über mehrere Nodes; LLM-Anfragen werden auf mehrere Maschinen verteilt |
| **Beobachtet** | Kein Implementierungsverzeichnis, keine Quelldateien, keine Header für Federated Inference vorhanden. `byzantine_detector.cpp` existiert für Fault-Detection, aber keine Routing-Schicht für verteilte Inferenz |
| **Evidence** | Kein Treffer für `federated_inference*` oder `distributed_inference*` in `src/llm/`; ROADMAP `[I]` Issue #1928 offen |
| **ROADMAP-Status** | `[I]` Issue offen (Issue: #1928) |
| **Issue-Titelvorschlag** | `[llm] Implement federated inference routing across distributed nodes` |
| **Label-Vorschläge** | `type:feature`, `priority:low`, `llm`, `distributed`, `status:open` |

---

## Zusammenfassung

| # | Feature | Quelle | Kritikalität | Status |
|---|---|---|---|---|
| 1 | ActiveVRAMAllocator GPU-Allokation | `active_vram_allocator.cpp` | Hoch | ✅ Gelöst (LLM-MISSING-001) |
| 2 | KV-Cache-Prewarming + Embedding-Lookup | `inference_engine_enhanced.cpp` L495,1131,1169 | Mittel | ✅ Gelöst |
| 3 | DocsAssistant LLM-Completion | `docs_assistant.cpp` L251 | Niedrig | Placeholder |
| 4 | RAG-Kontext-Enkodierung | `async_inference_engine.cpp` L373 | Niedrig | TODO |
| 5 | Federated Inference | ROADMAP Issue #1928 | Niedrig | Nicht implementiert |

*Alle anderen ROADMAP-Einträge sind durch vorhandene Implementierungsdateien in `develop` belegt (kein Stub in der kritischen Inferenz-Pfad).*
