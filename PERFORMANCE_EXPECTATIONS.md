# ThemisDB – Performance-Erwartungswerte & Messergebnisse

> Stand: 2026-04-02 | Quellen: `FUTURE_ENHANCEMENTS.md` je Modul, `benchmarks/results_analysis_reports/`, `benchmarks/baselines/`, `benchmarks/VERSION_HISTORY.csv`, `benchmarks/chimera/`
>
> **Benchmark-Plattformen:**
> - Run **20251223** (v1.3.0-baseline): MSVC Release x64, AVX2, 20-Core @ 3.7 GHz, 20 MB L3
> - Run **20251223_085556** (v1.3.3-dev): MSVC Release x64, AVX2, 20-Core @ 3.7 GHz, 20 MB L3
> - Run **20251229_184507** (v1.3.4): Windows x64, 20 Cores @ 3.696 GHz, 20 MB L3-Cache, L1=32KB, L2=256KB

### Lokaler Lauf (2026-04-09, Windows, MSVC Release)

> Quelle: `benchmarks/results/local_20260409_082951/`
>
> Reproduzierbar via Presets: `msvc-ninja-perf` + `windows-perf-workflow` in `CMakePresets.json`.

| Bereich | Benchmark (lokal) | Gemessen | Referenz-Ziel aus diesem Dokument | Status | Hinweis |
|--------|--------------------|----------|-----------------------------------|--------|---------|
| Storage/Allocator | `BM_Allocator_Themis_Small_mean` | 153,979 M ops/s | C-1 L1 Hit-Path: ≥ 5 M ops/s/Core | ✅ | Metrik ist allocator-nah und nicht identisch zum Cache-Hit-Path |
| Storage/Allocator | `BM_Allocator_Themis_Large_mean` | 3,0951 M ops/s | Sustained Write NVMe: ≥ 100.000 ops/s | ✅ | Nicht direkt NVMe-I/O, aber deutlicher Headroom im In-Memory-Pfad |
| Vector | `BM_VectorSearch_efSearch/128/10_mean` | 10,6 ms bei 10 Queries | HNSW Vektor-Suche (CPU): ≥ 5.000 QPS | ⚠️ | Grobe Ableitung ~943 QPS (10 / 0,0106s), kein 1:1 Workload-Match |
| Graph | `PlanGeneration_WithCache/100_mean` | 239 ns | Plan-Cache Lookup P99: < 100 us | ✅ | Plan-Generation mit Cache als Näherung |
| Graph | `PlanGeneration_ShortestPath/100_mean` | 253 ns | Algorithmus-Selektion P99: < 1 ms | ✅ | Plan-Generierung als Näherung für Selektions-Latenz |
| Observability | `BM_RecordCacheHit_mean` | 824 ns (1,23693 M ops/s) | C-5 Admin-API Response: ≤ 5 ms | ✅ | Interne Metrikaufnahme, API-Endpunkt kann höher liegen |

**Einschränkungen lokaler Lauf:**
- `bench_graph_traversal` und `bench_graph_query_optimizer` lieferten valide Teilergebnisse, endeten aber mit Exit-Code 1 nach Teilmenge.
- Huge Pages im lokalen Windows-Lauf nicht verfügbar (`Huge pages not available`).
- Der Vergleich ist als Engineering-Indikator gedacht; SLO-Freigabe nur mit dedizierten E2E-/API-Workloads.

---

## Legende

| Symbol | Bedeutung |
|--------|-----------|
| ✅ | Ziel erfüllt (gemessen ≥ Ziel) |
| ❌ | Ziel nicht erfüllt (gemessen < Ziel) |
| ⚠️ | Partiell / bekannte Regression |
| ❓ | Kein Messwert vorhanden |
| –  | Nicht gemessen in dieser Version |

---

---

## Inhaltsverzeichnis

> **Struktur: Allgemein → Spezifisch (Module) → Rohdaten → Interface-SLOs**

| # | Abschnitt | Typ |
|---|-----------|-----|
| – | Legende | Referenz |
| 1 | Versionshistorie – Kernmetriken | Messung (allgemein) |
| 2–29 | Modul-Spezifische Erwartungswerte | Modul-SLOs |
| 30 | Chimera-Baseline & Suite | Benchmark-Framework |
| 31–32 | Prompt Engineering / Ethics AI | Modul-SLOs |
| 33 | System-Level TPC/YCSB | Benchmarks |
| 34 | CI Regression-Schwellwerte | CI |
| 35 | Bekannte Performance-Lücken | Lücken |
| 36 | Rohdaten: Google Benchmark C++ | Primäre Messungen |
| 37 | Performance-Maßnahmen (GitHub-PR) | Maßnahmen nach Modul |
| 38 | Rohdaten: HTTP-API & Docker Benchmarks | Primäre Messungen |
| 39 | API/Interface Performance-Annahmen | Interface SLOs |

**Hinweis zur Statusbewertung:** Felder mit ❓ sind Erwartungswerte ohne vorliegende Messung.
Typ-Kennung in §39: **[M]** = gemessen · **[Z]** = Ziel · **[I]** = implementiert/bestätigt.

## 1. Versionshistorie – Kernmetriken

> Quelle: `benchmarks/VERSION_HISTORY.csv` + `benchmarks/results_analysis_reports/benchmark_summary.csv`
> Testplattform v1.3.0–v1.3.3: Intel i9-10900K (10C/20T @ 3.70 GHz), 31 GB RAM, WSL2 Linux
> Testplattform v1.3.4: Windows x64, 20 Cores @ 3.696 GHz, 20 MB L3-Cache

| Metrik | Ziel | v1.3.0 | v1.3.1 | v1.3.2 | v1.3.3 | **v1.3.4** | **v1.8.1-rc2** | **v1.8.0 Ziel** | Δ v1.3.0→v1.3.4 | Status |
|--------|------|--------|--------|--------|--------|-----------|-------------------------------|-----------------|-----------------|--------|
| Query Engine Throughput | – | 700 M ops/s | 750 M ops/s | 800 M ops/s | 800 M ops/s | **814,5 M ops/s** | ❓ | **≥ 900 M ops/s** | +16 % | ❓ |
| Vector Insert | – | 280 k/s | 300 k/s | 330 k/s | 340 k/s | **351,4 k/s** | ≈22,8 k/s* | **≥ 600 k/s** | +25 % | ⚠️ |
| Secondary Index Insert | – | 180 k/s | 190 k/s | 210 k/s | 215 k/s | **217,2 k/s** | ❓ | **≥ 1 M/s** | +21 % | ❓ |
| Embedding Cache Hit-Rate | – | – | – | – | – | **155,8 M/s** | ❓ | **≥ 200 M/s** | n/a | ❓ |
| 2PC Throughput | – | – | – | – | – | **6,4 k/s** | ❓ | **≥ 10 k/s** | n/a | ❓ |
| Graph Edge Ops | – | – | – | – | – | **628,7 k/s** | ❓ | **≥ 1 M/s** | n/a | ❓ |
| Timeseries Insert | – | – | – | – | – | **49,0 M pts/s** | ❓ | **≥ 60 M pts/s** | n/a | ❓ |
| Gesamt Benchmark-Tests | – | 450 | 480 | 520 | 780 | **1.078** | 5 Bench-Executables (lokal) | **≥ 1.200** | +140 % | ⚠️ |

---

## 2. Query-Engine – Detailergebnisse

> Quelle: `BENCHMARK_RESULTS.md` (Run 2025-12-18), `benchmark_summary.csv` (Run 2025-12-29)

| Benchmark | Ziel | v1.3.4 Gemessen | v1.8.1-rc2-local (2026-04-09) | Status |
|-----------|------|-----------------|----------------------------|--------|
