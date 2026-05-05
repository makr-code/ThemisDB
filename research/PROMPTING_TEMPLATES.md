# Prompting Templates — Algorithmus-Validierung ThemisDB

> **Zweck:** Fertige Prompt-Templates für die strukturierte Analyse von Algorithmen und
> Methoden in ThemisDB — sowohl modul-spezifisch als auch modulübergreifend.
>
> **Prozess-Kontext:** Diese Templates sind Schritt 3–4 des
> [Algorithm Validation Process](ALGORITHM_VALIDATION_PROCESS.md) zugeordnet.
> Die Ergebnisse aus diesen Prompts fließen direkt in Experiment-Protokolle,
> Research-Dokumente und ADRs ein.

---

## Inhaltsverzeichnis

1. [Template A — Modul-spezifische Analyse](#template-a--modul-spezifische-analyse)
2. [Template B — Cross-Module-Review](#template-b--cross-module-review)
3. [Ausgabe-Checkliste (für beide Templates)](#ausgabe-checkliste-für-beide-templates)
4. [Ausgabe-Verarbeitung](#ausgabe-verarbeitung)
5. [Beispiel: Template A ausgefüllt — `src/index/` + `I-L2Distance`](#beispiel-template-a-ausgefüllt--srcindex--i-l2distance)
6. [Beispiel: Template B ausgefüllt — `src/index/`, `src/query/`, `src/cache/`](#beispiel-template-b-ausgefüllt--srcindex-srcquery-srccache)
7. [Routing-Hinweise (Ollama vs. Copilot Cloud)](#routing-hinweise-ollama-vs-copilot-cloud)

---

## Template A — Modul-spezifische Analyse

**Verwendung:** Für eine einzelne Ziel-ID in einem Modul.  
**Routing:** `@ollama` (C++ Boilerplate/Analyse) — siehe [Routing-Hinweise](#routing-hinweise-ollama-vs-copilot-cloud)

```
Analysiere Modul `<src/modul>` für Ziel-ID `<Ziel-ID>` aus PERFORMANCE_EXPECTATIONS.md.

### Kontext

- Ziel-ID: <Ziel-ID>
- SLO-Zielwert: <Wert, z. B. "P99 < 8 ms" oder "> 1 M ops/s">
- Aktueller Messwert (Baseline): <gemessener Wert oder "nicht gemessen">
- SLO-Gap: <z. B. "-22 %" oder "noch nicht messbar">
- Betroffene Dateien: <Hauptimplementierungs-Dateien, z. B. "src/index/hnsw_index.cpp">
- Relevante Benchmarks: <z. B. "benchmarks/bench_vector_search.cpp: BM_TopK_5000_50">

### Was ich brauche

1. **Aktuelle Methode + Hot Path + Komplexität**
   - Beschreibe den aktuellen Algorithmus und seine zeitliche/räumliche Komplexität.
   - Identifiziere den Hot Path (welche Funktion/Schleife dominiert die Laufzeit?).
   - Benenne bekannte Engpässe (z. B. cache misses, Speicherbandbreite, Branching).

2. **Relevante Benchmarks + aktuelle Messwerte**
   - Welche Google-Benchmark-Funktionen messen diesen Hot Path direkt?
   - Gibt es Proxy-Metriken, wenn kein direkter Benchmark existiert?
   - Tabelle: BM-Funktionsname | Letzter bekannter Wert | Zielwert | Status

3. **Top-5 State-of-Science-Kandidaten (2023–heute bevorzugt)**
   - Für jeden Kandidaten:
     - Quelle: Titel + Konferenz/Journal + Jahr + URL/ArXiv
     - Kernidee (1 Satz)
     - Erwarteter Vorteil: Latenz-Delta, Throughput-Delta, Memory-Delta (aus Paper-Claims)
     - Risiken: Portabilität, Lizenzen, ABI-Änderungen, Regressions-Risiko
     - Integrationsaufwand: klein (<1 Woche) / mittel (1–4 Wochen) / groß (>4 Wochen)

4. **Vergleichsmatrix**
   Tabelle:
   | Kandidat | Quelle | Latenz-Delta | Throughput | Memory | Risiko | Aufwand |
   (Baseline immer als Zeile 0 / Referenz)

5. **Empfehlung**
   - Primärkandidat: <Name> — Begründung in 3–5 Sätzen
   - Fallback-Kandidat: <Name> — wann/warum als Alternative

6. **Konkrete Akzeptanzkriterien**
   - Messbare SLO-Verbesserung: "<Kandidat> muss P99 um ≥ X % verbessern oder Throughput um ≥ Y % steigern"
   - Kein Memory-Rückschritt > Z %
   - Test-Coverage: Unit + Integration + Benchmark erforderlich
   - CI-Gate: Regression-Threshold ≤ 10 %

7. **Teststrategie**
   - Unit-Tests: Was muss getestet werden? (Korrektheit, Edge Cases, Thread Safety)
   - Integration-Tests: Welche Downstream-Module werden beeinflusst?
   - Benchmark-Funktionen: Welche neuen `BM_*` Funktionen brauchen wir?

8. **CI-Gate-Vorschlag**
   - Workflow-Datei-Name
   - SLO-Threshold im Python-Gate-Script
   - Mapping-Eintrag für `benchmark_target_mapping.json`

9. **Roadmap-Tasks im Checkbox-Format**
   - [ ] Task 1 (Target: Q3 2026)
   - [ ] Task 2 (Target: Q4 2026)
   - …

### Einschränkungen

- Keine theoretischen Überlegungen ohne Quellen — jeder Kandidat braucht eine citable Quelle.
- Keine Kandidaten ohne Open-Source-Implementierung (Apache 2.0 / MIT bevorzugt).
- ABI-inkompatible Änderungen müssen explizit als Risiko markiert werden.
- Platform-spezifische Kandidaten (CUDA-only, AVX-512-only) müssen CPU-Fallback haben oder abgelehnt werden.
```

---

## Template B — Cross-Module-Review

**Verwendung:** Für modulübergreifende Bottleneck-Analyse und Synergien.  
**Routing:** Copilot Cloud (Architekturentscheidungen, Cross-cutting concerns) — siehe [Routing-Hinweise](#routing-hinweise-ollama-vs-copilot-cloud)

```
Führe einen Cross-Module-Review für die Module `<Modul A>`, `<Modul B>`, `<Modul C>` durch.

### Kontext

- Module: <Komma-separierte Liste, z. B. "src/index/, src/query/, src/cache/">
- Gemeinsame Ziel-IDs (falls bekannt): <z. B. "I-L2Distance, Q-SimpleWhere, C-1">
- Bekannte Symptome: <z. B. "Hohe P99-Latenz unter last, lock contention in Cache">
- Zeitraum: <z. B. "Nächste 90 Tage (Q3 2026)">

### Was ich brauche

1. **Gemeinsame Bottlenecks**
   Analysiere, welche systemischen Engpässe alle oder mehrere der genannten Module teilen:
   - Allocator-Druck (malloc/free Hotspots, Heap-Fragmentierung)
   - Scheduling/Thread-Pinning (false sharing, NUMA-Crossing)
   - Locking (shared_mutex vs. lock-free, critical section size)
   - Serialisierung (Protobuf, JSON, Arrow — Overhead auf Hot Path)
   - I/O-Latenz (RocksDB sync, WAL writes, network round-trips)
   - Memory-Bandbreite (cache-line thrashing, cross-socket DRAM)
   Für jeden Bottleneck: betroffene Module, geschätzte Auswirkung auf Ziel-IDs.

2. **Synergien und Trade-offs zwischen Modulen**
   - Welche Optimierungen in Modul A helfen auch Modul B? (z. B. mimalloc global)
   - Welche Optimierungen in einem Modul können ein anderes verschlechtern? (z. B. aggressive Caching vs. GC-Druck)
   - Gibt es Interfaces zwischen den Modulen, die umgestaltet werden sollten?

3. **Einheitliche KPI-Suite**
   Schlage eine KPI-Suite vor, die alle genannten Module abdeckt:
   | KPI | Zielwert | Benchmark-Funktion | Modul |
   Mindest-KPIs: P99 Latenz, Throughput (ops/s), RSS Peak, CPU-Utilization %, Tail under load (P99.9)

4. **Priorisierte 90-Tage-Roadmap**
   Priorisierte Liste von Optimierungsaufgaben für alle Module:
   Für jedes Item:
   - Owner-Modul
   - Ziel-ID
   - Benchmark-Mapping (Funktion + Datei)
   - Go/No-Go-Kriterium (messbare Bedingung für "done")
   - Abhängigkeiten (was muss vorher fertig sein?)
   - Geschätzter Aufwand (S/M/L)

5. **State-of-Science-Empfehlungen (Cross-cutting)**
   - Welche modulübergreifenden Technologien/Algorithmen sollten evaluiert werden?
     (z. B. io_uring für alle I/O-Pfade, RCU für alle Read-Heavy-Locks, DMA für RocksDB)
   - Für jeden Vorschlag: Quelle, erwarteter Vorteil für alle betroffenen Module, Risiken.

6. **Modulübergreifende Experiment-Koordination**
   - Welche Experimente müssen sequentiell laufen (Abhängigkeiten)?
   - Welche können parallel durchgeführt werden?
   - Welche gemeinsamen Benchmark-Infrastruktur-Änderungen brauchen wir?

7. **Roadmap-Tasks im Checkbox-Format (nach Modul)**
   ### `src/<modul_a>/`
   - [ ] Task 1 [Ziel-ID: <ID>] (Target: Q3 2026)
   - [ ] Task 2 [Ziel-ID: <ID>] (Target: Q4 2026)

   ### `src/<modul_b>/`
   - [ ] Task 1 [Ziel-ID: <ID>] (Target: Q3 2026)

   ### Cross-Cutting
   - [ ] Task [Module: alle] (Target: Q3 2026)

### Einschränkungen

- Keine vagen Empfehlungen ("verbessere die Performance") — alles muss messbar sein.
- Jede Empfehlung braucht eine konkrete Ziel-ID oder einen Vorschlag für eine neue Ziel-ID.
- Risiken zwischen Modulen (z. B. ABI-Brüche, API-Änderungen) müssen explizit bewertet werden.
- Lock-Hierarchie-Änderungen müssen mit Deadlock-Analyse begleitet werden.
```

---

## Ausgabe-Checkliste (für beide Templates)

Nach Erhalt der LLM-Ausgabe folgende Punkte prüfen, bevor Ergebnisse weiterverarbeitet werden:

- [ ] Jeder Kandidat hat eine citable Quelle (ArXiv-ID / DOI / GitHub-URL)
- [ ] Alle erwarteten Vorteile sind aus Paper-Claims oder Benchmarks belegt (nicht geschätzt)
- [ ] Lizenz jedes Kandidaten geprüft (Apache 2.0 / MIT bevorzugt; GPL abgelehnt)
- [ ] Portabilitäts-Anforderungen geprüft (Linux x86-64 + ARM64 Pflicht; Windows + macOS optional)
- [ ] Aufwandsschätzungen realistisch (Cross-Check mit bisherigen ThemisDB-Implementierungen)
- [ ] Roadmap-Tasks haben konkrete Target-Quartale
- [ ] CI-Gate-Vorschlag enthält konkrete Python-Snippet oder Workflow-Datei-Namen

---

## Ausgabe-Verarbeitung

Nach Ausfüllen des Templates und Erhalt der LLM-Antwort:

```
1. Research-Dokumente anlegen
   cp research/papers/_template_paper.md \
      research/papers/<kandidat>_<jahr>.md
   # (oder best_practices/ je nach Quelle)

2. Experiment-Protokoll vorbereiten
   mkdir -p research/experiments/<ziel_id>/
   # Protokoll-Format: siehe ALGORITHM_VALIDATION_PROCESS.md Schritt 4

3. Kandidaten-Benchmarks bauen & messen
   # Schritt 4 aus ALGORITHM_VALIDATION_PROCESS.md

4. ADR erstellen (nach Experiment)
   cp research/architecture_decisions/_template_decision.md \
      research/architecture_decisions/adr_<NNN>_<kandidat>_fuer_<modul>.md

5. Roadmap aktualisieren
   # Checkboxen in src/<modul>/ROADMAP.md setzen
```

---

## Beispiel: Template A ausgefüllt — `src/index/` + `I-L2Distance`

```
Analysiere Modul `src/index/` für Ziel-ID `I-L2Distance` aus PERFORMANCE_EXPECTATIONS.md.

### Kontext

- Ziel-ID: I-L2Distance
- SLO-Zielwert: L2Distance 1000×512 P99 < 2 ms
- Aktueller Messwert (Baseline): P99 ~3.4 ms (BM_L2Distance_1000_512, Run 20260413)
- SLO-Gap: -41 % (3.4 ms vs. 2 ms Ziel)
- Betroffene Dateien: src/index/simd_distance.cpp, include/index/distance_functions.h
- Relevante Benchmarks: benchmarks/bench_vector_search.cpp: BM_L2Distance_1000_512, BM_CosineDistance_1000_512

### Was ich brauche

1. Aktuelle Methode + Hot Path + Komplexität
   [Template-Text wie oben]

[… restliche Punkte wie im Template …]
```

**Erwartete Ausgabe (Auszug):**

```markdown
## 3. Top-5 State-of-Science-Kandidaten

| # | Kandidat | Quelle | Kernidee | Latenz-Delta | Aufwand |
|---|----------|--------|----------|--------------|---------|
| 1 | Intel oneDNN L2 Kernel | Intel (2024) | Optimierte AVX-512/AMX Kernel für L2-Distanz | -35 % P99 | Klein |
| 2 | RaBitQ (Binary Quantization) | arXiv 2404.04696 (2024) | 1-bit Quantisierung für ANN-Kandidaten-Screening | -60 % (approx) | Mittel |
| 3 | SIMD-everywhere (sve/neon) | github.com/simd-everywhere | Portabler SIMD-Wrapper für ARM + x86 | -20 % P99 (ARM) | Klein |
| 4 | Highway (Google) | arXiv 2310.04426 (2023) | Runtime-dispatch SIMD, AVX-512 + SVE + NEON | -25 % P99 | Klein |
| 5 | Faiss GPU IVF | Johnson et al. TPAMI 2019 | GPU-beschleunigte Batch-L2 (nur für GPU-Nodes) | -80 % P99 (GPU) | Groß |

## 5. Empfehlung

**Primär:** Highway (Google) — liefert -25 % P99 ohne GPU-Abhängigkeit, 
Runtime-dispatch unterstützt AVX-512 + ARM NEON, Apache 2.0, vcpkg-kompatibel.

**Fallback:** SIMD-everywhere — einfachere API, geringerer Vorteil, aber 
minimales Integrationsrisiko.
```

---

## Beispiel: Template B ausgefüllt — `src/index/`, `src/query/`, `src/cache/`

```
Führe einen Cross-Module-Review für die Module `src/index/`, `src/query/`, `src/cache/` durch.

### Kontext

- Module: src/index/, src/query/, src/cache/
- Gemeinsame Ziel-IDs: I-L2Distance, Q-SimpleWhere, C-1, C-4
- Bekannte Symptome: P99-Spike unter concurrency, lock contention in AdaptiveQueryCache,
  hohe malloc/free-Frequenz in Batch-Query-Pfad
- Zeitraum: Q3 2026 (nächste 90 Tage)

[… restliche Punkte wie im Template …]
```

**Erwartete Ausgabe (Auszug):**

```markdown
## 1. Gemeinsame Bottlenecks

| Bottleneck | Betroffene Module | Geschätzte Auswirkung |
|------------|-------------------|-----------------------|
| malloc/free Hotspot (Batch-Pfad) | index, query, cache | +15–30 % P99 unter Last |
| shared_mutex Contention (Cache) | cache, query | P99.9 Spike ×3–5× vs. P99 |
| RocksDB WriteBatch Serialisierung | storage (→ cache, index) | ~2 ms overhead pro Flush |

## 4. Priorisierte 90-Tage-Roadmap

| Priorität | Modul | Aufgabe | Ziel-ID | Go/No-Go | Aufwand |
|-----------|-------|---------|---------|----------|---------|
| 1 | cross-cutting | mimalloc global aktivieren für alle Batch-Pfade | PERF-MEM-1 | RSS -15 % | S |
| 2 | cache | AdaptiveQueryCache L1 auf lock-free Skip-List migrieren | C-4 | P99.9 < 5× P99 | M |
| 3 | index | Highway SIMD für L2/Cosine-Kernel | I-L2Distance | P99 -25 % | S |
| 4 | query | Batch-Allocator im Executor (arena per Query) | Q-SimpleWhere | P99 -10 % | M |
```

---

## Routing-Hinweise (Ollama vs. Copilot Cloud)

| Prompt-Typ | Modell | Begründung |
|------------|--------|------------|
| Template A für C++ Implementierungs-Details (Hot-Path, Komplexität, Code) | `@ollama` (z. B. `deepseek-coder-v2:16b`) | C++ Boilerplate/Analyse → lokales Modell bevorzugt |
| Template A für Paper-Recherche und Kandidaten-Auswahl | Copilot Cloud | Benötigt aktuelles Wissen (ArXiv 2024/2025) |
| Template B (Cross-Module, Architektur) | Copilot Cloud | Architekturentscheidungen und Security-Review |
| Roadmap-Tasks im Checkbox-Format generieren | `@ollama` | Repetitive Strukturierung → lokales Modell |
| ADR-Formulierung (Adopt/Reject) | Copilot Cloud | Qualitätsurteil erfordert Reasoning |

**VS Code Workspace-Setting für automatisches Routing:**

```jsonc
{
  "ollamaBridge.delegationMode": "auto",
  "ollamaBridge.themisDbRules": true,
  "ollamaBridge.defaultModel": "deepseek-coder-v2:16b"
}
```

> **Routing-Hint für Agent-Automation:**
> ```
> // ROUTING HINT: ollama-local
> // Model: deepseek-coder-v2:16b
> // Reason: Algorithm analysis for C++ module — Template A
> ```

---

*Erstellt: 2026-04-22 | Template-Version: 1.0*  
*Prozess-Kontext: [ALGORITHM_VALIDATION_PROCESS.md](ALGORITHM_VALIDATION_PROCESS.md)*
