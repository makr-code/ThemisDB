# Experiment-Protokolle — Algorithm Validation

Dieses Verzeichnis enthält die Protokolle aller Algorithm-Validation-Experimente nach
dem [6-Schritte-Framework](../ALGORITHM_VALIDATION_PROCESS.md).

## Struktur

```
experiments/
├── README.md                         ← Diese Datei
└── <ziel_id>/                        ← Ein Unterverzeichnis pro Ziel-ID
    ├── <kandidat>_<datum>.json       ← Google Benchmark JSON-Rohdaten
    ├── hw_profile.md                 ← Hardware-Profil des Experiment-Runs
    └── protokoll_<kandidat>.md       ← Experiment-Protokoll (Ergebnisse + Empfehlung)
```

## Namenskonventionen

| Element | Format | Beispiel |
|---------|--------|---------|
| Verzeichnis | `<Ziel-ID>` in Kleinbuchstaben | `i_l2distance/` |
| Benchmark JSON | `<kandidat>_YYYYMMDD.json` | `highway_simd_20260715.json` |
| Protokoll | `protokoll_<kandidat>.md` | `protokoll_highway_simd.md` |
| HW-Profil | `hw_profile_<datum>.md` | `hw_profile_20260715.md` (geteilt pro Ziel-ID) |

## Protokoll-Template

```markdown
# Experiment: <Ziel-ID> — <Kandidat>

**Datum:** YYYY-MM-DD  
**Ziel-ID:** <ID>  
**SLO-Zielwert:** <Wert>  
**Baseline-Commit:** <SHA>  
**Kandidat-Commit:** <SHA>  
**Hardware:** [hw_profile_<datum>.md](hw_profile_<datum>.md)

## Ergebnisse

| Metrik | Baseline | Kandidat | Delta | p-Wert | Signifikant? |
|--------|----------|----------|-------|--------|--------------|
| P50-Latenz (ms) | | | | | |
| P95-Latenz (ms) | | | | | |
| P99-Latenz (ms) | | | | | |
| Throughput (ops/s) | | | | | |
| Peak RSS (MB) | | | | | |

**Statistischer Test:** Welch's t-Test, n=<Runs>, α=0.05  
**Effect Size:** Cohen's d = <Wert>

## Interpretation

...

## Empfehlung

- [ ] Adopt — Begründung: ...
- [ ] Reject — Begründung: ...
- [ ] Weiterer Test erforderlich — Was fehlt: ...

## Nächste Schritte

- [ ] ADR erstellen: `docs/research/architecture_decisions/adr_<NNN>_….md`
- [ ] CI-Gate anlegen: `.github/workflows/performance-regression-<modul>-<ziel_id>.yml`
- [ ] Roadmap aktualisieren: `src/<modul>/ROADMAP.md`
```

## Index offener Experimente

| Ziel-ID | Kandidat | Status | Verantwortlich | Target |
|---------|----------|--------|----------------|--------|
| *(noch leer — erste Experimente starten Q3 2026)* | | | | |

---
*Prozess: [ALGORITHM_VALIDATION_PROCESS.md](../ALGORITHM_VALIDATION_PROCESS.md)*
