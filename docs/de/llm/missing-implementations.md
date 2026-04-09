# LLM-Modul — Fehlende / Unvollständige Implementierungen

<!-- Status: current | validated: 2026-04-09 -->
<!-- Primärdokumentation: ../../../src/llm/ -->

Dieser Report dokumentiert Funktionen und Komponenten, die in `src/llm/ROADMAP.md`, `src/llm/ARCHITECTURE.md` oder anderen Primary-Docs als implementiert beschrieben werden oder als geplant gelten, jedoch bei der Reality-Check-Prüfung als **nicht vollständig umgesetzt** oder **als Stub** befunden wurden.

Prüfstand: 2026-04-09 | Branch: `develop`

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

> **Gelöst in Branch `copilot/implement-kv-cache-prewarming`** (Audit-Pass 2: 2026-03-11)
>
> **Was implementiert wurde:**
> - `prewarmCache()`: Berechnet echte Embeddings via `computeEmbeddingForCache()` für jeden Prompt,
>   schätzt Token-IDs und speichert den Eintrag im `LLMPrefixCache` mit dem Prompt-Text als Schlüssel.
> - `checkCache()`: Nutzt `request.prompt` als Cache-Schlüssel + echtes Embedding → HNSW-Fuzzy-Matching
>   für semantisch ähnliche Prompts. Gibt `cached->generated_text` zurück (die echte Modellantwort).
> - `updateCache()`: Speichert Prompt als Schlüssel, echtes Embedding und `response.text` als
>   `generated_text` in `PrefixCacheEntry`.
> - `computeEmbeddingForCache(text)`: Holt ersten verfügbaren Plugin-Zeiger (lock-free während `embed()`),
>   graceful degradation bei Fehler.
> - `estimateTokenSequence(text)`: Gemeinsamer statischer Helfer (4 chars ≈ 1 Token BPE-Heuristik).
> - `PrefixCacheEntry::generated_text` (neu): Speichert die tatsächlich generierte Antwort;
>   `put()` erhält neuen optionalen Parameter `generated_text`.
> - 3 neue Tests (Tests 21–23) in `tests/test_inference_engine_enhanced.cpp`.
> - `InferenceEngineEnhancedFocusedTests` CMake-Target in `tests/CMakeLists.txt`.

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/llm/ARCHITECTURE.md` §"KV-Cache Reuse (Prefix Cache)"; `src/llm/ROADMAP.md` §"Context caching (KV-cache reuse)" |
| **Erwartet** | Prefix-Cache kann vorgewärmt werden durch Pre-Berechnung von KV-Cache-Einträgen für häufige Prompts; Cache-Lookup nutzt Embedding-Ähnlichkeit für Fuzzy-Treffer |
| **Status** | ✅ **Vollständig gelöst** (Audit-Pass 2: SHA256-as-response-text-Bug behoben, Prompt als Cache-Schlüssel, `generated_text` korrekt gespeichert und zurückgegeben) |
| **Lösung** | `include/llm/llm_prefix_cache.h`, `src/llm/llm_prefix_cache.cpp`, `include/llm/inference_engine_enhanced.h`, `src/llm/inference_engine_enhanced.cpp` |

---

## 3. DocsAssistant — LLM-Completion-Macro ✅ BEHOBEN (2026-03-11)

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/llm/docs_assistant.cpp` — behauptet LLM-basierte Antwortgenerierung |
| **Erwartet** | `DocsAssistant::generateAnswer()` generiert Antworten mittels LLM-Inferenz |
| **Beobachtet (behoben)** | `generateAnswer()` ruft jetzt `THEMIS_LLM_GENERATE(prompt.str())` auf, wenn `THEMIS_ENABLE_LLM` gesetzt und `EmbeddedLLMManager::instance().isInitialized()` wahr ist. Andernfalls wird eine aussagekräftige Fehlermeldung zurückgegeben statt `"[LLM completion placeholder]"`. |
| **Evidence** | `src/llm/docs_assistant.cpp` Zeilen 249–256 |
| **Status** | ✅ Behoben (2026-03-11) |

---

## 4. AsyncInferenceEngine — RAG-Kontext-Enkodierung ✅ BEHOBEN (2026-03-11)

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/llm/ARCHITECTURE.md` §"Standard Inference" (RAG-Integration durch `src/rag/llm_integration.cpp`) |
| **Erwartet** | `AsyncInferenceEngine::submitRAG()` kodiert RAG-Dokumente strukturiert in den Prompt |
| **Beobachtet (behoben)** | Strukturiertes XML-Tag-Format: `<system>`, `<context><document index="N" source="..." relevance="...">`, `<question>`, `<answer>` — kompatibel mit modernen instruction-tuned Modellen. Custom `context_template` mit `{{CONTEXT}}`/`{{QUERY}}`-Platzhaltern wird ebenfalls unterstützt. |
| **Evidence** | `src/llm/async_inference_engine.cpp` Zeilen 369–428 |
| **Status** | ✅ Behoben (2026-03-11) |

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
| 3 | DocsAssistant LLM-Completion | `docs_assistant.cpp` L251 | Niedrig | ✅ Behoben (2026-03-11) |
| 4 | RAG-Kontext-Enkodierung | `async_inference_engine.cpp` L373 | Niedrig | ✅ Behoben (2026-03-11) |
| 5 | Federated Inference | ROADMAP Issue #1928 | Niedrig | Nicht implementiert |

*Alle anderen ROADMAP-Einträge sind durch vorhandene Implementierungsdateien in `develop` belegt (kein Stub in der kritischen Inferenz-Pfad).*
