# Algorithm Validation Process — ThemisDB

> **Zweck:** Dieses Dokument kodifiziert das systematische Vorgehen zur Validierung
> bestehender Algorithmen / Methoden in ThemisDB und zur strukturierten Suche nach
> effizienteren Alternativen nach dem Stand der Wissenschaft.
>
> **Vorbild:** Die Einführung von [mimalloc](../../src/performance/ROADMAP.md) in das
> Performance-Modul — reproduzierbar über Benchmarks, CI-Gates und Research-Dokumentation
> abgesichert — gilt als internes Erfolgsbeispiel für diesen Prozess.

---

## Inhaltsverzeichnis

1. [Überblick & Motivation](#1-überblick--motivation)
2. [Schritt 1 — Ziel-ID + SLO fixieren](#2-schritt-1--ziel-id--slo-fixieren)
3. [Schritt 2 — Baseline einfrieren](#3-schritt-2--baseline-einfrieren)
4. [Schritt 3 — Kandidaten aus "State of Science" sammeln](#4-schritt-3--kandidaten-aus-state-of-science-sammeln)
5. [Schritt 4 — Experiment-Design standardisieren](#5-schritt-4--experiment-design-standardisieren)
6. [Schritt 5 — Gates in CI erzwingen](#6-schritt-5--gates-in-ci-erzwingen)
7. [Schritt 6 — Entscheidung dokumentieren](#7-schritt-6--entscheidung-dokumentieren)
8. [Praktische Regel: Definition of "Won"](#8-praktische-regel-definition-of-won)
9. [Prompt-Templates](#9-prompt-templates)
10. [Integration in ThemisDB-Infrastruktur](#10-integration-in-themisdb-infrastruktur)
11. [Beispiele](#11-beispiele)

---

## 1. Überblick & Motivation

Algorithmische Verbesserungen müssen **messbar, reproduzierbar, CI-abgesichert und dokumentiert**
sein, bevor sie als "gewonnen" gelten.  
Gefühlt bessere Algorithmen, Benchmarks ohne Baseline-Vergleich und unkontrollierte Einzel-Experimente
werden **abgelehnt**, da sie die Regression-Erkennung korrumpieren.

Der Prozess besteht aus **sechs Schritten**, die für jede Modul-Optimierung vollständig durchlaufen werden müssen:

```
┌──────────┐   ┌──────────┐   ┌──────────┐   ┌──────────┐   ┌──────────┐   ┌──────────┐
│  Step 1  │──▶│  Step 2  │──▶│  Step 3  │──▶│  Step 4  │──▶│  Step 5  │──▶│  Step 6  │
│ Ziel-ID  │   │ Baseline │   │Kandidaten│   │Experiment│   │CI-Gates  │   │ ADR/Doc  │
│ + SLO    │   │einfrieren│   │sammeln   │   │-Design   │   │erzwingen │   │          │
└──────────┘   └──────────┘   └──────────┘   └──────────┘   └──────────┘   └──────────┘
```

---

## 2. Schritt 1 — Ziel-ID + SLO fixieren

### Was zu tun ist

Vor jeder Optimierungsarbeit muss eine **konkrete Ziel-ID** aus `PERFORMANCE_EXPECTATIONS.md`
(§1.2, Tabellen nach Modul) oder `benchmarks/benchmark_target_mapping.json` identifiziert
und explizit in den folgenden Artefakten referenziert werden:

- `src/<modul>/ROADMAP.md` (als Checkbox-Task mit Target-Quartal)
- `src/<modul>/FUTURE_ENHANCEMENTS.md` (mit Acceptance-Kriterien)
- Dem ADR / Research-Dokument, das die Entscheidung festhält

### Checkliste

- [ ] Ziel-ID aus `PERFORMANCE_EXPECTATIONS.md §1.2` identifiziert (z. B. `Q-SimpleWhere`, `I-L2Distance`, `TS-2`)
- [ ] SLO-Wert (Throughput-Ziel, Latenz P99, Memory-Budget) notiert
- [ ] Checkbox-Task in `src/<modul>/ROADMAP.md` angelegt: `- [ ] <Optimierung> [Ziel-ID: <ID>] (Target: <Q/Jahr>)`
- [ ] Kein "gefühltes" Problem — immer von einem messbaren SLO-Gap ausgehen

### Format für ROADMAP-Tasks

```markdown
- [ ] Ersetze <Algorithmus A> durch <Algorithmus B> für <Hot Path>
      [Ziel-ID: <ID>] [SLO: <Wert>] [Gap: <aktuell vs. Ziel>] (Target: Q3 2026)
```

---

## 3. Schritt 2 — Baseline einfrieren

### Was zu tun ist

Der **aktuelle Algorithmus** muss vollständig dokumentiert und als reproduzierbarer Benchmark
eingefroren werden, bevor irgendeine Alternative getestet wird.

### Artefakte der Baseline

| Artefakt | Speicherort | Inhalt |
|----------|-------------|--------|
| Benchmark-Ergebnis (JSON) | `benchmarks/baselines/<modul>/<id>_baseline.json` | Google Benchmark JSON-Output |
| Hardware-Profil | `benchmarks/baselines/<modul>/<id>_hw_profile.md` | CPU, Kerne, Takt, L3-Cache, DRAM, OS |
| Algorithmus-Beschreibung | `docs/research/FUTURE_ENHANCEMENTS.md` im Modul | Komplexität, Hot Path, bekannte Limitierungen |
| Commit-SHA | Im `<id>_baseline.json` als Metadatum | Exakte Code-Version der Baseline |

### Checkliste

- [ ] Benchmark für Ziel-ID läuft clean durch (`--benchmark_format=json --benchmark_out=<datei>`)
- [ ] Hardware-Profil (CPU-Modell, Kerne, Takt, L3, DRAM, OS, Kernel) dokumentiert
- [ ] Baseline-JSON nach `benchmarks/baselines/<modul>/` eingecheckt
- [ ] Aktueller Algorithmus mit Komplexität (O-Notation), Hot Path und Engpass beschrieben
- [ ] Bekannte Limitierungen notiert (z. B. "Single-threaded", "No SIMD", "malloc pressure")

### Minimal-Format für `<id>_hw_profile.md`

```markdown
# Hardware-Profil — <Ziel-ID> Baseline

**Datum:** YYYY-MM-DD  
**Commit:** <SHA>

| Eigenschaft | Wert |
|-------------|------|
| CPU | Intel/AMD <Modell>, <N> Kerne @ <GHz> |
| L3-Cache | <MB> |
| DRAM | <GB> @ <MHz> |
| OS | Ubuntu <Version> / Linux <Kernel> |
| Compiler | GCC <Version> / Clang <Version> |
| CMake-Flags | `-DCMAKE_BUILD_TYPE=Release -DTHEMIS_ENABLE_MIMALLOC=ON …` |
```

---

## 4. Schritt 3 — Kandidaten aus "State of Science" sammeln

### Was zu tun ist

Mindestens **5 Kandidaten** aus der aktuellen Literatur (2023–heute bevorzugt) werden
systematisch erfasst. Jeder Kandidat erhält einen **Research-Eintrag** im `docs/research/`-System.

### Quellen

| Quelle-Typ | Wo suchen |
|------------|-----------|
| Konferenz-Papers | SIGMOD, VLDB, OSDI, SOSP, NeurIPS, ICML (ArXiv-Preprints akzeptiert) |
| Bibliotheken / Frameworks | GitHub Stars ≥ 1k, produktiv eingesetzt in ClickHouse, RocksDB, DuckDB, PostgreSQL |
| Best Practices | AWS Builder's Library, Google SRE, CNCF Guides |
| Quarterly Landscape | `docs/research/stand_der_technik/` (vierteljährlich aktualisiert) |

### Kandidaten-Steckbrief (pro Kandidat)

Für jeden Kandidaten wird ein Steckbrief angelegt — entweder als:
- `docs/research/papers/<name>_<jahr>.md` (aus `_template_paper.md`), oder
- `docs/research/best_practices/<name>.md` (aus `_template_best_practice.md`)

Der Steckbrief enthält mindestens:

```markdown
| Eigenschaft | Inhalt |
|-------------|--------|
| Quelle | Paper-Titel / Bibliothek-Name + URL |
| Kernidee | Ein Satz: was macht den Algorithmus besser? |
| Erwarteter Vorteil | Latenz, Throughput, Memory — mit Zahlen aus Paper |
| Risiken | Constraints, Portabilität, Lizenzen, Regressions-Risiko |
| Integrationsaufwand | klein (<1 Woche) / mittel (1–4 Wochen) / groß (>4 Wochen) |
| Voraussetzungen | SIMD-Flags, CUDA, externe Libs, ABI-Änderungen? |
```

### Vergleichsmatrix

Nach der Steckbrief-Runde wird eine **Vergleichsmatrix** erstellt:

| Kandidat | Quelle | Erwartete Latenz-Verbesserung | Throughput | Memory Delta | Risiko | Aufwand |
|----------|--------|-------------------------------|------------|--------------|--------|---------|
| A (aktuell / Baseline) | — | 0 % | 100 % | Baseline | — | — |
| B | Paper 2024 | -30 % P99 | +40 % | +10 % | Mittel | Klein |
| C | Lib 2023 | -15 % P99 | +20 % | -5 % | Klein | Klein |
| … | | | | | | |

### Checkliste

- [ ] ≥ 5 Kandidaten aus Literatur 2023–heute identifiziert
- [ ] Für jeden Kandidaten: Research-Dokument erstellt
- [ ] Vergleichsmatrix ausgefüllt
- [ ] Primärkandidat + Fallback-Kandidat benannt
- [ ] Offene Fragen / Abhängigkeiten notiert

---

## 5. Schritt 4 — Experiment-Design standardisieren

### Was zu tun ist

Alle Kandidaten werden unter **identischen Bedingungen** gegen die Baseline gemessen.
Abweichungen vom Standard-Experiment-Design müssen explizit begründet werden.

### Standard-Experiment-Parameter

| Parameter | Wert | Begründung |
|-----------|------|------------|
| Warmup | ≥ 60 s | IEEE Std 2807-2022 |
| Messdauer | ≥ 300 s oder `--benchmark_min_time=5.0s` | Stationäre Phase |
| Wiederholungen | ≥ 5 unabhängige Runs | CI: 95 %-Konfidenzintervall |
| Input-Set | Fixiert (gleiches Dateiformat / Seed / Größe) | Vergleichbarkeit |
| Hardware | Selbe Maschine wie Baseline (oder HW-Profil dokumentiert) | Reproduzierbarkeit |
| Compiler | Selbe Flags wie Baseline | Build-Konsistenz |
| Statistik | Welch's t-Test + Mann-Whitney U für signifikante Unterschiede | IEEE 2807-2022 §4.3 |
| Metriken | **P50, P95, P99** Latenz + Throughput (ops/s) + Peak RSS | Vollständiges Bild |
| Ausreißer-Bereinigung | IQR × 1.5 (Tukey 1977) | Robustheit |

### Benchmark-Aufruf (Template)

```bash
# Candidate benchmark
./build/release/<bench_binary> \
  --benchmark_filter='^<BM_Function>($|/)' \
  --benchmark_min_time=5.0s \
  --benchmark_repetitions=5 \
  --benchmark_format=json \
  --benchmark_out=benchmarks/experiments/<ziel_id>/<kandidat>_<datum>.json

# Statistical comparison
python3 tools/benchmark_compare.py \
  benchmarks/baselines/<modul>/<ziel_id>_baseline.json \
  benchmarks/experiments/<ziel_id>/<kandidat>_<datum>.json \
  --test welch --alpha 0.05 --effect-size cohens-d
```

### Experiment-Protokoll

Pro Experiment wird ein **Protokoll** in `docs/research/experiments/<ziel_id>/` angelegt:

```markdown
# Experiment: <Ziel-ID> — <Kandidat>

**Datum:** YYYY-MM-DD  
**Baseline-Commit:** <SHA>  
**Kandidat-Commit:** <SHA>  
**Hardware:** [Link zu hw_profile.md]

## Ergebnisse

| Metrik | Baseline | Kandidat | Delta | p-Wert | Signifikant? |
|--------|----------|----------|-------|--------|--------------|
| P99-Latenz (ms) | | | | | |
| Throughput (ops/s) | | | | | |
| Peak RSS (MB) | | | | | |

## Interpretation

...

## Empfehlung

[ ] Adopt  [ ] Reject  [ ] Weiterer Test erforderlich
```

### Checkliste

- [ ] Benchmark-Binaries für Baseline + alle Kandidaten gebaut (Release-Build)
- [ ] ≥ 5 Runs pro Kandidat auf selber Hardware wie Baseline
- [ ] Welch's t-Test + Mann-Whitney U berechnet (p < 0.05 gilt als signifikant)
- [ ] P50 / P95 / P99 Latenz + Throughput + Peak RSS dokumentiert
- [ ] Experiment-Protokoll nach `docs/research/experiments/<ziel_id>/` eingecheckt

---

## 6. Schritt 5 — Gates in CI erzwingen

### Was zu tun ist

Jeder Algorithmus-Kandidat, der die Experimente besteht, muss **sofort mit CI-Gates**
abgesichert werden. Ohne Gate kann eine spätere Code-Änderung die Verbesserung
unbemerkt rückgängig machen.

### Gate-Implementierung

```yaml
# .github/workflows/performance-regression-<modul>-<ziel_id>.yml (Beispiel)
- name: Enforce <Ziel-ID> SLO
  run: |
    python3 - << 'PY'
    import json
    from pathlib import Path

    data = json.loads(Path("benchmark_results/<ziel_id>.json").read_text())
    threshold = <SLO_WERT>   # aus PERFORMANCE_EXPECTATIONS.md
    for bench in data["benchmarks"]:
        if bench["name"] == "<BM_Function>":
            measured = bench["items_per_second"]  # oder real_time
            if measured < threshold:
                raise SystemExit(f"SLO-Regression: {measured:.2f} < {threshold:.2f}")
    print(f"SLO erfüllt: {measured:.2f}")
    PY
```

### Mapping in `benchmark_target_mapping.json`

Der neue Benchmark muss in `benchmarks/benchmark_target_mapping.json` eingetragen werden
und mit `tools/verify_benchmark_mapping.py` validiert werden:

```json
"<Ziel-ID>": {
  "label": "<Bezeichnung>",
  "primary_benchmark": "<BM_Function>",
  "file": "<bench_file>.cpp",
  "status": "mapped"
}
```

### Regression-Schwellen

| Threshold-Typ | Grenze | Aktion bei Überschreitung |
|---------------|--------|---------------------------|
| Minor | > 5 % Verschlechterung | WARN in CI-Log |
| Major | > 10 % Verschlechterung | CI-Failure (Block PR) |
| Critical | > 20 % Verschlechterung | CI-Failure + Slack-Alert |

Diese Schwellen werden von `benchmarks/performance_regression_detector.py` ausgewertet.

### Checkliste

- [ ] Workflow-Datei `.github/workflows/performance-regression-<modul>-<ziel_id>.yml` angelegt
- [ ] SLO-Wert im Workflow korrekt aus `PERFORMANCE_EXPECTATIONS.md` übernommen
- [ ] Eintrag in `benchmark_target_mapping.json` ergänzt
- [ ] `python3 tools/verify_benchmark_mapping.py` läuft fehlerfrei durch
- [ ] `performance_regression_detector.py` mit `--fail-on major` konfiguriert
- [ ] Nightly Benchmark Sweep (07-quality_nightly-benchmark-sweep.yml) deckt neue Benchmark-Funktion ab

---

## 7. Schritt 6 — Entscheidung dokumentieren

### Was zu tun ist

Jede abgeschlossene Experiment-Runde — ob Adopt oder Reject — wird als
**Architecture Decision Record (ADR)** dauerhaft festgehalten.

### ADR-Vorlage für Algorithmus-Entscheidungen

```bash
cp docs/research/architecture_decisions/_template_decision.md \
   docs/research/architecture_decisions/adr_<NNN>_<algorithmus>_fuer_<modul>.md
```

Der ADR enthält mindestens:
- **Context:** Ziel-ID, SLO-Gap, Baseline-Ergebnis
- **Considered Options:** Alle Kandidaten aus Schritt 3 mit Vergleichsmatrix
- **Decision:** Adopt / Reject mit quantitativem Begründung (gemessener Vorteil)
- **Consequences:** Positive + Negative + Neutral
- **Validation:** Verweise auf Benchmark-Protokolle und CI-Gate

### Roadmap & FUTURE_ENHANCEMENTS aktualisieren

```markdown
# In src/<modul>/ROADMAP.md
- [x] Ersetze <Algorithmus A> durch <Algorithmus B> für <Hot Path>
      [Ziel-ID: <ID>] [ADR: ADR-<NNN>] (Abgeschlossen: YYYY-MM-DD)

# Bei Reject:
- [~] <Alternative C> getestet, abgelehnt wegen <Begründung>
      [Ziel-ID: <ID>] [ADR: ADR-<NNN>] (Abgeschlossen: YYYY-MM-DD)
```

### Implementation Influence Index aktualisieren

Der Eintrag muss in `docs/research/implementation_influence/README.md` ergänzt werden:

```markdown
| <Paper/Lib-Titel> | Paper/Best Practice | `src/<modul>/` | v<Version>+ | ✅ Implemented | [ref](...) |
```

### Checkliste

- [ ] ADR erstellt unter `docs/research/architecture_decisions/adr_<NNN>_…md`
- [ ] `docs/research/architecture_decisions/decision_log.md` aktualisiert
- [ ] Research-Dokumente für alle adoptierten Kandidaten vollständig
- [ ] `docs/research/implementation_influence/README.md` aktualisiert
- [ ] `src/<modul>/ROADMAP.md` Checkbox auf `[x]` gesetzt
- [ ] `src/<modul>/FUTURE_ENHANCEMENTS.md` Status aktualisiert
- [ ] Modul-README: Abschnitt *Wissenschaftliche Grundlagen & Einflüsse* ergänzt
- [ ] Commit-Prefix: `ref(research): <Algorithmus> adopted/rejected for src/<modul>/`

---

## 8. Praktische Regel: Definition of "Won"

> **Eine Optimierungsidee gilt erst als "gewonnen", wenn alle 6 Schritte abgeschlossen sind.**

| Kriterium | Gate |
|-----------|------|
| Messbare SLO-ID vorhanden | ✅ Schritt 1 |
| Reproduzierbare Baseline existiert | ✅ Schritt 2 |
| ≥ 5 Kandidaten evaluiert | ✅ Schritt 3 |
| Experiment mit Welch's t-Test (p < 0.05) | ✅ Schritt 4 |
| CI-Gate blockiert Regressionen | ✅ Schritt 5 |
| ADR + Research-Doku vollständig | ✅ Schritt 6 |

Kandidaten, die nur 1–5 Schritte bestehen, befinden sich **noch im Experiment-Stadium** und
dürfen nicht als abgeschlossene Optimierungen kommuniziert oder in Release Notes erwähnt werden.

---

## 9. Prompt-Templates

Für die konkrete Nutzung mit Copilot / Ollama / anderen LLMs: → [`PROMPTING_TEMPLATES.md`](PROMPTING_TEMPLATES.md)

---

## 10. Integration in ThemisDB-Infrastruktur

| Infra-Komponente | Rolle im Prozess |
|------------------|-----------------|
| `PERFORMANCE_EXPECTATIONS.md` | Ziel-IDs + SLO-Werte (Schritt 1) |
| `benchmarks/benchmark_target_mapping.json` | Ziel-ID → Benchmark-Mapping (Schritt 5) |
| `tools/verify_benchmark_mapping.py` | Validiert Mapping-Vollständigkeit (Schritt 5) |
| `benchmarks/performance_regression_detector.py` | Regression-Erkennung (Schritt 5) |
| `.github/workflows/07-quality_nightly-benchmark-sweep.yml` | Nightly Sweep (Schritt 5) |
| `.github/workflows/performance-regression-check.yml` | PR-Gate (Schritt 5) |
| `docs/research/papers/` | Kandidaten-Papers (Schritt 3) |
| `docs/research/best_practices/` | Kandidaten-Best-Practices (Schritt 3) |
| `docs/research/architecture_decisions/` | ADR für Entscheidung (Schritt 6) |
| `docs/research/implementation_influence/README.md` | Master-Index (Schritt 6) |
| `docs/research/stand_der_technik/` | Vierteljährliche Kandidaten-Recherche (Schritt 3) |
| `src/<modul>/ROADMAP.md` | Tracking + Status (Schritte 1, 6) |
| `src/<modul>/FUTURE_ENHANCEMENTS.md` | Acceptance-Kriterien (Schritte 1, 6) |

---

## 11. Beispiele

### Beispiel A: mimalloc → Performance-Modul (Erfolgsbeispiel)

| Schritt | Ergebnis |
|---------|----------|
| 1. Ziel-ID | `PERF-MEM-1`: Peak RSS unter sustained load ≤ 4 GB |
| 2. Baseline | tcmalloc / glibc malloc; baseline JSON eingecheckt |
| 3. Kandidaten | mimalloc, jemalloc, tcmalloc (neu), SnakeMalloc, Scudo |
| 4. Experiment | mimalloc: -18 % RSS, +12 % Throughput (p < 0.001, Cohen's d = 1.4) |
| 5. CI-Gate | `THEMIS_ENABLE_MIMALLOC=ON` im nightly sweep; Regression-Gate aktiv |
| 6. Entscheidung | mimalloc adoptiert, jemalloc als Fallback dokumentiert |

→ Implementiert in `src/performance/ROADMAP.md` Phase 1, `benchmarks/performance/`

### Beispiel B: HNSW-Parameter-Tuning → Index-Modul

| Schritt | Ergebnis |
|---------|----------|
| 1. Ziel-ID | `I-TopK`: TopK 5000×50 P99 < 8 ms |
| 2. Baseline | `ef_search=64, M=16`; baseline JSON in `benchmarks/baselines/index/` |
| 3. Kandidaten | ef_search-Sweep, DiskANN, ScaNN (Kandidaten-Papers vorhanden) |
| 4. Experiment | Adaptiver ef-Tuner: -22 % P99 (p < 0.01) |
| 5. CI-Gate | `BM_TopK_5000_50` im Mapping, Regression-Gate 10 % |
| 6. Entscheidung | ADR-001-Extension geplant für Auto-Tuner |

→ Tracking in `src/performance/ROADMAP.md` (WorkloadAdaptiveOptimizer)

---

*Erstellt: 2026-04-22 | Prozess-Version: 1.0*  
*Nächste Review: 2026-07-01 (Q3 quarterly update)*
