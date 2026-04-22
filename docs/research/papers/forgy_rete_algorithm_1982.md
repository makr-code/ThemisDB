# RETE: A Fast Algorithm for the Many Pattern/Many Object Pattern Match Problem

**Metadaten:**
- Author(en): Charles L. Forgy
- Konferenz/Journal: *Artificial Intelligence*, Vol. 19, No. 1, pp. 17–37
- Jahr: 1982
- Link: [ScienceDirect](https://www.sciencedirect.com/science/article/pii/0004370282900249) · [DOI: 10.1016/0004-3702(82)90024-9](https://doi.org/10.1016/0004-3702(82)90024-9)
- Zitierweise: `forgy1982rete`
- Tags: `expert-system`, `rule-engine`, `rete-algorithm`, `forward-chaining`, `pattern-matching`, `production-rules`, `working-memory`, `agenda`
- ThemisDB-Versionen: v2.1.0+ (planned — `ExpertSystemEngine`, `KnowledgeBase`)
- Status: [ ] Not Started | [ ] Partially Implemented | [x] Fully Implemented  
  *(CEP-NFA-Basis vorhanden; vollständige ExpertSystemEngine geplant Q2 2027 — `src/analytics/ROADMAP.md` Phase 4)*

## 📋 Executive Summary

Der RETE-Algorithmus (lat. *Netz*) von Forgy (1982) ist der grundlegende Algorithmus für effiziente Regelauswertung in Produktionssystemen (Expert Systems). Er löst das N-Pattern/M-Objekt-Matching-Problem durch gemeinsame Zwischenzustandsspeicherung in einem Netzwerk aus `alpha`-Knoten (einfache Bedingungstests) und `beta`-Knoten (Join-Operationen mit partiellen Matches). Im Gegensatz zu naivem Rückruf-Matching ist RETE inkrementell: nur Änderungen am Working Memory (WM) propagieren durch das Netz; unveränderte Fakten erzeugen keine Neuauswertung.

ThemisDB's `ExpertSystemEngine` (analytics-Modul) baut auf dem CEP-NFA-Pattern-Matcher (`cep_engine.cpp`) auf, der eine Rete-ähnliche Zwischenzustandsspeicherung bereits implementiert. Der vollständige RETE-basierte Expert-System-Stack erweitert dies um eine persistente `KnowledgeBase` (ABox-Fakten), Vorwärts-/Rückwärtsverkettung und eine Erklärungskomponente.

## 🎯 Key Findings

- **Alpha-Netzwerk**: Einstufige Bedingungs-Knoten filtern Working-Memory-Elemente nach Klasse, Attribut und Wert. Alpha-Memories speichern alle passenden WM-Elemente — keine Neuauswertung bei unveränderten Objekten.
- **Beta-Netzwerk**: Zwei-Input-Joins verbinden partielle Matches aus alpha-Memories; `beta-memory` speichert alle Tripel-Kombinationen. Gemeinsame Prefixe werden strukturell geteilt (Netz-Topologie = DAG).
- **Inkrementelle Propagation**: `add-wme` und `remove-wme` propagieren nur die Änderung; durchschnittliche Komplexität O(R·M) → O(R + changes) in der Praxis.
- **Konfliktmengenverwaltung (Agenda)**: Aktivierte Regel-Instanzen werden in einer Prioritätsliste gesammelt; Feuerstrategie (LEX, MEA, Salience) wählt nächste Regelinstanz.
- **Rete++ / Rete\***: Nachfolger-Algorithmen verbessern Memory-Verbrauch (Rete++) und unterstützen Negation als Failure (NAF) sowie Aggregationen.
- **Vorwärtsverkettung (Data-Driven)**: WM-Änderungen triggern Regelauswertung; geeignet für Event-getriebene Systeme (CEP-Integration).
- **Rückwärtsverkettung (Goal-Driven)**: Suche von Ziel zu Prämissen rückwärts; geeignet für `queryGoal()` API.

## 🔗 Direct Influence on ThemisDB

### Affected Modules

- [ ] Analytics module → `src/analytics/` — `ExpertSystemEngine` (Rete-ähnlicher Regelausführer über CEP-NFA als Backbone)
- [ ] Analytics module → `src/analytics/` — `KnowledgeBase` (Working Memory: `unordered_multimap<predicate, Fact>`)
- [ ] Analytics module → `src/analytics/cep_engine.cpp` — NFA-Matcher als Alpha-Netz-Äquivalent für Event-Stream-Matching
- [ ] Graph module → `src/graph/` — `KnowledgeGraphReasoner` (Horn-clause Forward-Chaining = Rete-Subset ohne Joins)

### What Was Adopted?

1. **Working Memory (WM)**: `KnowledgeBase` als WM; Fakten als `(subject, predicate, object)` Tripel; `assertFact()` / `retractFact()` als `add-wme`/`remove-wme`-Äquivalent.
2. **Alpha-Tests**: Einfache Prädikats-Bedingungen im YAML-Regelformat werden als Filterprädikate auf der `KnowledgeBase::getFacts(predicate)` API abgebildet.
3. **Beta-Joins**: Horn-Klausel-Konjunktionen implementieren Zwei-Bedingung-Joins; Variablen-Bindungen propagieren durch Klausel-Sequenz.
4. **Agenda**: `ExpertSystemEngine` verwaltet eine Prioritätswarteschlange aktivierter Regelinstanzen; `priority`-Feld im YAML-Regelformat setzt Salience.
5. **Inkrementelle Propagation**: `onCDCEvent()` in `KnowledgeGraphReasoner` triggert Forward-Chaining nur für neue Fakten (nicht Gesamt-Re-Evaluation).
6. **CEP als Alpha-Netz-Erweiterung**: `CEPEngine` NFA-Matcher dient als streaming Alpha-Netz für Event-Strom-Bedingungen; `ExpertSystemEngine` verbindet WM-Fakten mit CEP-Event-Triggern.

### How Was It Adapted?

| Rete-Konzept | ThemisDB Adaptation | Rationale |
|---|---|---|
| Vollständiges Alpha/Beta-Netz | Horn-Klausel-Konjunktionen + CEP-NFA | CEP-NFA bereits produktionsfähig; kein volles Rete-Netz nötig für Horn-Klauseln |
| OPS5-Produktionsformat | YAML Horn-Klausel-Format | YAML zugänglicher für Domain-Experten; YAML-Schema-Validierung |
| Rete++ Memory-Sharing | `std::unordered_multimap` + Predicate-Index | O(log N) Predicate-Lookup; ausreichend für ≤100 Regeln / ≤10k Fakten |
| Negation als Failure (NAF) | v2.2.0 geplant | Komplexitätsreduktion für initiale Version |
| Aggregate-Joins (z. B. COUNT) | via CEP-Aggregationen | CEP-Engine liefert Aggregationen nativ |

### Performance Impact

| Metric | Rete-Theorie | ThemisDB Ziel | Status |
|--------|-------------|---------------|--------|
| Vorwärtsverkettung (10k Fakten, 100 Regeln) | O(changes) inkrementell | ≤ 50 ms | ⏳ Planned Q2 2027 |
| Rückwärtsverkettung (Tiefe ≤ 10) | O(Branching^depth) | ≤ 20 ms | ⏳ Planned Q2 2027 |
| `explain(decision_id)` Proof-Trace | O(Regeltiefe) | ≤ 10 ms | ⏳ Planned Q2 2027 |
| WM-Assertion (10k Fakten) | O(1) inkrementell | ≤ 5 µs/Fakt | ⏳ Planned Q2 2027 |

## ⚠️ Limitations & Open Questions

- **Speicherverbrauch Beta-Memory**: Vollständige RETE-Beta-Memories können O(R × M²) Speicher verbrauchen bei vielen Joins.
  - ThemisDB Lösung: Horn-Klausel-Subset (kein vollständiges Beta-Netz); max. 2 Joins pro Klausel; Faktenlimit 10k.
- **Negation-as-Failure**: Standard-RETE unterstützt keine NAF nativ.
  - ThemisDB Lösung: v2.2.0 geplant; interim: Negation als Absenz-Check über `KnowledgeBase::hasFact(triple)`.
- **Zirkuläre Regeln**: Vorwärtsverkettung kann in Endlosschleifen enden bei zirkulären Regeln.
  - ThemisDB Lösung: `max_cycles`-Limit + Visited-Set für Forward-Chaining; `ConflictError` bei detektierten Zyklen.
- **Rückwärtsverkettung-Tiefe**: Exponentielles Verzweigen bei tiefen Abhängigkeitsgraphen.
  - ThemisDB Lösung: Depth-Limit 10; `CycleDetected`-Fehler.

## 🔬 Validation

- [ ] Code reviewed gegen Forgy (1982) Abschnitte 3–5 (Alpha/Beta-Netz-Konstruktion)
- [ ] Unit tests: WM-Assertion + Forward-Chaining-Fixpunkt; Rückwärtsverkettung 3-Hop; Zirkeldetektion
- [ ] Concurrency-Test: 8 Threads, `assertFact()` + `forwardChain()` gleichzeitig
- [ ] Benchmark: 10k Fakten + 100 Regeln ≤ 50 ms Vorwärtsverkettung
- [ ] Module README verlinkt (`src/analytics/README.md`)
- [ ] implementation_influence index aktualisiert

## 📚 Related Work

- [OWL 2 / Description Logic Handbook](owl2_description_logics_2012.md) — Formale Grundlage für Horn-Klausel-Regelformat
- [TransE / Knowledge Graph Completion](bordes_transe_2013.md) — KG-basierte Faktengewinnung
- [Best Practice: RETE Forward-Chaining Rule Engine](../best_practices/rete_forward_chaining_rule_engine.md)
- [`src/analytics/FUTURE_ENHANCEMENTS.md`](../../../src/analytics/FUTURE_ENHANCEMENTS.md) — Expert System Engine Sektion
- [`src/analytics/ROADMAP.md`](../../../src/analytics/ROADMAP.md) — Phase 4 (ExpertSystemEngine)
- [CLIPS Expert System Shell](https://clipsrules.net/) — Open-Source Rete-Implementierung (Referenzimplementierung)
- [Drools Rule Engine](https://www.drools.org/) — Production-Grade Rete++ Implementierung
- Forgy, C.L. (1979) — *On the Efficient Implementation of Production Systems* (CMU-CS-79-107) — Ursprüngliche Dissertation

## 🔬 Weiterführende Quellen

| Quelle | Relevanz |
|--------|----------|
| Doorenbos (1995) — Production System Techniques | Rete++ (gemeinsame Alpha-Knoten, Speicheroptimierung) |
| Miranker (1987) — TREAT | Alternative zu RETE mit geringerem Speicherbedarf |
| Giarratano & Riley (2005) — Expert Systems | Lehrbuch: CLIPS, Rete, Forward/Backward Chaining |
| McDermott & Forgy (1978) — Production System Languages | Konzeptuelle Grundlagen vor RETE |
| Batory (1985) — LEAPS | Lazy Evaluation für Produktionssysteme |

---
**Last Updated:** 2026-04-22  
**Next Review:** 2026-10-01
