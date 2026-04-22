# OWL 2 Web Ontology Language & Description Logic Handbook

**Metadaten:**
- Author(en): (1) W3C OWL Working Group — OWL 2 Specification; (2) Franz Baader, Diego Calvanese, Deborah McGuinness, Daniele Nardi, Peter F. Patel-Schneider (Eds.) — *The Description Logic Handbook*
- Konferenz/Journal: (1) W3C Recommendation; (2) Cambridge University Press
- Jahr: (1) 2012 (OWL 2); (2) 2003 (DL Handbook, 2nd ed. 2007)
- Link: [OWL 2 W3C Rec.](https://www.w3.org/TR/owl2-overview/) · [DL Handbook (Cambridge)](https://www.cambridge.org/core/books/description-logic-handbook/F78EB90B6F45D33E34B8DD94CEE9FAC6) · [arXiv overview](https://arxiv.org/abs/1201.4089)
- Zitierweise: `w3c2012owl2`; `baader2003dlhandbook`
- Tags: `ontology`, `owl`, `description-logic`, `knowledge-representation`, `semantic-constraints`, `reasoning`, `knowledge-graph`
- ThemisDB-Versionen: v2.1.0+ (planned — `OntologyManager`, `KnowledgeGraphReasoner`)
- Status: [ ] Not Started | [ ] Partially Implemented | [x] Fully Implemented  
  *(Planned: Q3 2026 — see `src/graph/ROADMAP.md` Phase 6)*

## 📋 Executive Summary

**OWL 2** ist die führende W3C-Empfehlung zur maschinenlesbaren Wissensrepräsentation. Es basiert auf der **Beschreibungslogik SROIQ(D)** und bietet vollständige Ausdrucksmächtigkeit für Konzepthierarchien, Rolleneigenschaften (Transitivität, Symmetrie, Inversität) und Datenbeschränkungen. ThemisDB benötigt OWL-lite-Konstrukte für die `OntologyManager`-Komponente, die semantische Pfad-Constraints im Graph-Modul und Wissensrepräsentation im Knowledge-Graph-Reasoner implementiert.

**Description Logic (DL)** — insbesondere die Family `ALC`, `SHOIN(D)` und `SROIQ(D)` — bildet die formale Grundlage für Ontologie-Reasoning: transitive Konzept-Inklusion (`C ⊑ D`), Rollenhierarchien (`R ⊑ S`), und Einschränkungen (`∃R.C ⊑ D`). Das DL-Handbook (Baader et al.) ist die Standardreferenz für Tableau-Algorithmen und Reasoning-Komplexität.

## 🎯 Key Findings

- **OWL-lite (SHIF(D))**: Subontologie für Horn-regel-kompatibles Reasoning; `isA()` transitive Abschluss-Berechnung in polynomialer Zeit (`ALC` ⊑ PSPACE-complete; Horn-DL ⊑ P).
- **Rollenhierarchien**: `rdfs:subPropertyOf` ermöglicht Kanten-Typvererbung; `owl:TransitiveProperty` liefert transitiven Abschluss über `isA()`-Lookups.
- **Klasseninklusion**: `owl:subClassOf` modelliert Konzepthierarchien; `owl:equivalentClass` erlaubt Aliase.
- **Tableau-Algorithmus**: Standardverfahren für ALC-Reasoning; Tiefe ≤ 20 Hops für ThemisDB-Anwendungsfälle.
- **OWL 2 Profile**: `OWL 2 EL` (polynomiales Reasoning), `OWL 2 QL` (SPARQL-kompatibel), `OWL 2 RL` (Regelbasiert über Forward-Chaining) — ThemisDB verwendet OWL 2 RL für den Forward-Chaining-Pfad.
- **RDF/RDFS**: Untermenge von OWL; `rdf:type`, `rdfs:subClassOf`, `rdfs:domain/range` sind Basis für `KnowledgeBase`-Tripel-Format.

## 🔗 Direct Influence on ThemisDB

### Affected Modules

- [ ] Graph module → `src/graph/` — `OntologyManager` (OWL-lite Konzepthierarchie, `isA()`, `allowedEdgeTypes()`)
- [ ] Graph module → `src/graph/` — `PathConstraints::addSemanticConstraint()` (OWL-based edge-type validation)
- [ ] Graph module → `src/graph/` — `KnowledgeGraphReasoner` (Horn-clause Forward-Chaining = OWL 2 RL Subset)
- [ ] Analytics module → `src/analytics/` — `KnowledgeBase` (RDF-Triple-Format: subject, predicate, object)
- [ ] RAG module → `src/rag/` — `OntologyAwareRetriever` (Konzept-Expansion via `isA()`)

### What Was Adopted?

1. **OWL-lite Konzepthierarchie**: `OntologyManager` speichert Konzepte als DAG (`subClassOf`-Kanten); `isA()` traversiert Vorfahren-Kette bis Tiefe 20 — direkt aus OWL 2 EL Subclass-Reasoning.
2. **`rdf:type`-Kanten-Typ-Validierung**: `allowedEdgeTypes(srcClass, dstClass)` prüft OWL `rdfs:domain` und `rdfs:range` Axiome für jeden Kanten-Typ.
3. **OWL 2 RL Forward-Chaining**: `KnowledgeGraphReasoner` implementiert Horn-Klausel-Subset von OWL 2 RL; leitet neue Tripel aus existierenden Fakten ab (z. B. `rdfs:subClassOf` + `rdf:type` → abgeleitetes `rdf:type`).
4. **`(subject, predicate, object)` Tripel**: `KnowledgeBase`-Faktenformat folgt RDF-Terminologie; Prädikate entsprechen OWL-Rollen.
5. **JSON/YAML-Schema**: OntologieLadung via JSON (als OWL/JSON-LD-Subset) oder YAML; `OntologyManager::loadFromJson()` parst `@type`, `subClassOf`, `allowedRelations`.

### How Was It Adapted?

| OWL/DL Konzept | ThemisDB Adaptation | Rationale |
|---|---|---|
| Vollständiges OWL 2 SROIQ(D) | OWL-lite (SHIF Subset) | Reasoning-Komplexität auf P/PolyTime für Produktionsbetrieb |
| Tableau-Algorithmus | BFS über Ancestor-DAG + LRU-Cache | ≤5 µs/Abfrage; kein allgemeines Tableau nötig für DAG-Hierarchien |
| RDF/SPARQL | Internes Triple-Format + AQL-Extension | Kein SPARQL-Server; AQL-Integration direkter |
| OWL `owl:sameAs` (Entitätsfusion) | Nicht implementiert (v2.1.0) | Komplexität zu hoch für initiale Phase |
| OWL 2 RL vollständig | Horn-Klausel-Subset (ThemisDB Regelformat) | YAML-Regelformat einfacher zu schreiben als OWL 2 RL Syntax |

### Performance Impact

| Metric | OWL 2 EL Garantie | ThemisDB Ziel | Status |
|--------|-------------------|---------------|--------|
| Concept Subsumption | Polynomiale Zeit | `isA()` ≤ 5 µs (LRU-cached) | ⏳ Planned Q3 2026 |
| Ontologie-Load (10k Konzepte) | — | ≤ 100 ms | ⏳ Planned Q3 2026 |
| Forward-Chaining (1M Tripel) | — | ≤ 2 s kalt; ≤ 50 ms incremental | ⏳ Planned Q4 2026 |

## ⚠️ Limitations & Open Questions

- **OWL DL Decidability**: Vollständiges OWL 2 DL-Reasoning ist NEXPTIME-complete — für große Ontologien nicht geeignet.
  - ThemisDB Lösung: OWL-lite (Horn-DL) beschränkt auf polynomiales Reasoning; kein Tableau.
- **Open World Assumption (OWA)**: OWL nimmt an, dass nicht bekannte Fakten unbekannt (nicht falsch) sind — ContraSt zu Datenbanken.
  - ThemisDB Lösung: `KnowledgeBase` verwendet Closed-World-Semantik für bekannte Entitäten; OWA nur für Inferenz-Tripel.
- **ABox vs. TBox**: OWL unterscheidet Terminologiewissen (TBox) von Faktenwissen (ABox).
  - ThemisDB Lösung: `OntologyManager` = TBox; `KnowledgeBase` = ABox; `KnowledgeGraphReasoner` verbindet beide.
- **Zyklische Ontologien**: Zirkuläre `subClassOf`-Ketten (A ⊑ B ⊑ A) sind in OWL äquivalent (`owl:equivalentClass`).
  - ThemisDB Lösung: Zirkeldetektion bei `OntologyManager::build()`; Äquivalenzklassen werden zu einem Repräsentanten zusammengefasst.

## 🔬 Validation

- [ ] Code reviewed against OWL 2 EL spec (transitive subsumption correctness)
- [ ] Unit tests: `isA()` mit 3-Hop-Hierarchie; `allowedEdgeTypes()` Korrektheit; Zyklusdetektion
- [ ] Benchmark: Ontologie-Load 10k Konzepte ≤ 100 ms; `isA()` ≤ 5 µs
- [ ] Module README verlinkt (`src/graph/README.md`)
- [ ] implementation_influence index aktualisiert

## 📚 Related Work

- [Knowledge Graph Embeddings — Bordes et al. (2013)](bordes_transe_2013.md) — Link Prediction über KG
- [GraphRAG — Edge et al. (2024)](graphrag_edge_2024.md) — KG-augmentiertes RAG
- [HippoRAG — Gutierrez et al. (2024)](hipporag_gutierrez_2024.md) — PPR über KG
- [Best Practice: OWL-lite Ontology Constraints](../best_practices/owl_lite_ontology_constraints.md)
- [`src/graph/FUTURE_ENHANCEMENTS.md`](../../../src/graph/FUTURE_ENHANCEMENTS.md) — Ontology-based Semantic Constraints + KG Reasoning
- [`src/graph/ROADMAP.md`](../../../src/graph/ROADMAP.md) — Phase 6 + Phase 7
- [W3C OWL 2 Primer](https://www.w3.org/TR/owl2-primer/)
- [W3C OWL 2 RL Profile](https://www.w3.org/TR/owl2-profiles/#OWL_2_RL)

## 🔬 Weiterführende Quellen

| Quelle | Relevanz für ThemisDB |
|--------|----------------------|
| Horrocks et al. (2006) — OWL-Lite Reasoning | Komplexitätsanalyse Horn-DL |
| Pérez-Urbina et al. (2010) — Tractable Ontology Reasoning | Polynomial-time DL-Lite/EL |
| Kazakov et al. (2014) — ELK Reasoner | State-of-the-Art OWL 2 EL Reasoner (Open Source) |
| Motik et al. (2009) — OWL 2 RL Semantics | Forward-Chaining-Regelformat |
| Pan et al. (2017) — Exploiting Linked Data / KG | KG-Integration in Applikationen |

---
**Last Updated:** 2026-04-22  
**Next Review:** 2026-10-01
