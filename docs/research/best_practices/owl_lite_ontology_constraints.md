# OWL-lite Ontology-based Semantic Constraint Checking

**Metadaten:**
- Source: W3C OWL 2 Recommendation (2012); Baader et al. — *The Description Logic Handbook* (2003/2007); Industry practice in ontology-driven databases (Stardog, GraphDB, Apache Jena)
- URL: [W3C OWL 2 RL](https://www.w3.org/TR/owl2-profiles/#OWL_2_RL) · [Apache Jena Inference](https://jena.apache.org/documentation/inference/) · [Stardog Reasoning](https://docs.stardog.com/inference-engine/)
- Tags: `ontology`, `owl-lite`, `constraint-checking`, `knowledge-graph`, `semantic-reasoning`, `path-constraints`, `horn-clauses`
- ThemisDB-Versionen: v2.1.0+ (planned — `OntologyManager`, `PathConstraints`, `KnowledgeGraphReasoner`)
- Status: [ ] Identified | [ ] Partially Adopted | [x] Fully Adopted  
  *(Planned Q3-Q4 2026 — `src/graph/ROADMAP.md` Phase 6)*

## 📋 Summary

Ontologie-basierte semantische Constraints ermöglichen es, Graphpfade und Fakten gegen eine formale Domänen-Ontologie zu validieren. Statt hartkodierter Regeln wird ein externes Schema (OWL-lite) geladen, das Konzepthierarchien (`subClassOf`), Rollen-Eigenschaften (`domain`, `range`, `transitive`) und Restriktionen definiert. Constraint-Checks werden dann durch Lookup in dieser Ontologie statt durch imperativen Code ausgeführt — erweiterbar ohne Recompilierung.

Für ThemisDB bedeutet dies: `OntologyManager` lädt eine OWL-lite/YAML-Schemadatei; `PathConstraints::addSemanticConstraint()` prüft jeden Graphkanten-Typ gegen die Ontologie; `KnowledgeGraphReasoner` leitet neue Fakten über Horn-Klausel-Subset (OWL 2 RL) ab.

## 🎯 Core Principles

1. **Konzepthierarchie als DAG**: Klassen (`owl:Class`) und ihre Unterklassen (`owl:subClassOf`) bilden einen gerichteten azyklischen Graphen (DAG); `isA(A, B)` = DAG-Vorfahren-Suche.
2. **Rollen-Einschränkungen**: Jede Kante (Prädikaten/Relation) hat `rdfs:domain` (erlaubte Quellklasse) und `rdfs:range` (erlaubte Zielklasse); Verletzungen = Constraint-Violation.
3. **Transitive Abschlüsse via LRU-Cache**: `isA()` traversiert Ancestors-DAG; Ergebnis wird in LRU-Cache (1 000 Einträge) gespeichert — amortisiert O(1) bei wiederholten Anfragen.
4. **Prune-First-Strategie**: BFS/DFS-Traversal prüft `allowedEdgeTypes()` vor Expansion jedes Frontiers — ungültige Kanten werden nicht verfolgt, niemals erst post-hoc verworfen.
5. **Graceful Degradation**: Unbekannte Konzept-IDs werden als `unconstrained` behandelt (WARN-Log, keine Exception) — Produktionsbetrieb wird nicht unterbrochen.
6. **Immutable nach `build()`**: `OntologyManager` ist nach Konstruktion read-only; keine Write-Locks während Traversal; voll thread-safe für concurrent reads.
7. **Round-Trip-Serialisierung**: `toJson()` / `toYaml()` ermöglichen Hot-Reload ohne Neustart.

## 🔗 Adoption in ThemisDB

### Affected Modules

- `src/graph/` — `OntologyManager`: OWL-lite DAG-Loader; `isA()` + `allowedEdgeTypes()` API
- `src/graph/` — `PathConstraints::addSemanticConstraint()`: Prune-first BFS mit Ontologie-Prüfung
- `src/graph/` — `KnowledgeGraphReasoner`: Horn-Klausel-Forward-Chaining = OWL 2 RL Subset
- `src/rag/` — `OntologyAwareRetriever`: Entity-Expansion via `isA()` für Oberbegriff-Retrieval
- `src/analytics/` — `KnowledgeBase`: RDF-Triple-Format; `predicate` entspricht OWL-Rolle

### What Was Adopted?

**1. OWL-lite JSON/YAML Schema-Format**
```yaml
# ontology.yaml — OWL-lite Konzepthierarchie
concepts:
  - id: LegalEntity
    subClassOf: []
  - id: NaturalPerson
    subClassOf: [LegalEntity]
  - id: Organization
    subClassOf: [LegalEntity]
  - id: PublicBody
    subClassOf: [Organization]

relations:
  - id: hasParty
    domain: Case
    range: LegalEntity
  - id: ruledBy
    domain: Case
    range: Statute
    transitive: false
  - id: subjectTo
    domain: LegalEntity
    range: Regulation
    transitive: true
```

**2. `OntologyManager` API (geplant)**
```cpp
auto onto = std::make_shared<OntologyManager>();
onto->loadFromYaml("schemas/legal_ontology.yaml");
onto->build();  // finalisiert DAG, berechnet transitive Ancestor-Sets

// isA() — O(1) mit LRU-Cache
assert(onto->isA("NaturalPerson", "LegalEntity") == true);
assert(onto->isA("PublicBody", "LegalEntity") == true);

// allowedEdgeTypes() — gibt erlaubte Relationen zurück
auto allowed = onto->allowedEdgeTypes("Case", "Statute");
// → {"ruledBy"}

// Constraint-Check in PathConstraints
constraints.addSemanticConstraint(onto.get(), OntologyRuleset::STRICT);
auto paths = constraints.findConstrainedPaths("case_001", "statute_42", 10);
auto violations = constraints.lastViolations();
// violations enthält {edge_id, expected_class, actual_class} Einträge
```

**3. Prune-First BFS-Integration**
```cpp
// In PathConstraints::findConstrainedPaths() — PRUNE-FIRST-Logik
for (const auto& neighbor : graph.getNeighbors(current)) {
    const auto edgeType = graph.getEdgeType(current, neighbor.id);
    const auto srcClass = graph.getNodeClass(current);
    const auto dstClass = graph.getNodeClass(neighbor.id);

    // Ontologie-Check VOR Expansion (prune-first)
    if (ontology_ && ruleset_ == OntologyRuleset::STRICT) {
        auto allowed = ontology_->allowedEdgeTypes(srcClass, dstClass);
        if (allowed.find(edgeType) == allowed.end()) {
            // Ontologie-Verletzung: Kante nicht verfolgen
            recordViolation(current, neighbor.id, edgeType, allowed);
            continue;  // nicht expandieren
        }
    }
    frontier.push(neighbor);
}
```

**4. LoRA-Plausibility-Score als neuronales Komplement**

Symbolische OWL-Constraints werden durch neuronales Soft-Scoring ergänzt:
```cpp
// Symbolisch: Kante erlaubt laut Ontologie?
bool symbolic_ok = onto->allowedEdgeTypes(src, dst).count(edge_type) > 0;

// Neural: Wie plausibel ist die Kante laut LoRA-Adapter?
float lora_score = reasoner.applyLoRAScore(chain, "legal_domain_v1");
// → 0.0..1.0

// Kombiniert: STRICT = nur symbolisch; SOFT = beide
bool accept = (ruleset == STRICT) ? symbolic_ok
                                  : (symbolic_ok && lora_score >= min_score);
```

### Deviations & Rationale

| OWL-Standard | ThemisDB Abweichung | Begründung |
|---|---|---|
| SPARQL-CONSTRUCT für Inferenz | Horn-Klausel-YAML-Format | Einfacher für Domain-Experten; keine SPARQL-Abhängigkeit |
| SWRL (Semantic Web Rule Language) | Eigenes YAML-Regelformat | SWRL zu komplex; ThemisDB-Format ausreichend für Horn-Klauseln |
| OWL Full Open World Assumption | Closed World für ABox-Fakten | Datenbank-Semantik; unbekannte Fakten = false |
| Dedizierter OWL-Reasoner (Pellet, HermiT) | Interner DAG-Traversal | Keine externe Abhängigkeit; ausreichend für OWL-lite Subset |

## ⚠️ Trade-offs & Limitations

- **OWL-lite Einschränkung**: Kein volles OWL DL (keine Cardinality-Restrictions, keine nominals, keine complex class expressions).
  - Rationale: Polynomiales Reasoning; produktionstauglich.
- **Statische Ontologie**: Nach `build()` keine inkrementellen Schema-Änderungen ohne Hot-Reload.
  - Mitigation: `toJson()`/`toYaml()` + `loadFromJson()` für Hot-Reload via Admin-API.
- **Kein SPARQL-Endpoint**: Keine standardisierte Abfrage-Schnittstelle für Drittanwendungen.
  - Mitigation: AQL-Extension für Ontologie-Abfragen geplant (v2.2.0).
- **Domain/Range-Verletzungen**: Strenger STRICT-Modus kann legitime Kanten ablehnen wenn Ontologie unvollständig.
  - Mitigation: WARN-Modus (log + accept) als konfigurierbare Alternative.

## 🔬 Validation

- [ ] `OntologyManager::isA()` validiert gegen OWL EL Subsumptions-Korrektheit (Beispiel-Ontologien)
- [ ] Prune-First: Verifikation dass Violations-Zählung 0 für konformes Graphbeispiel
- [ ] Performance: `isA()` ≤ 5 µs mit LRU-Cache (1000 Einträge, warm)
- [ ] Hot-Reload-Test: JSON round-trip; `loadFromJson()` nach `toJson()` = gleiche DAG-Topologie
- [ ] Module README verlinkt (`src/graph/README.md`)
- [ ] implementation_influence index aktualisiert

## 📚 Related

- [Paper: OWL 2 / Description Logic Handbook](../papers/owl2_description_logics_2012.md)
- [Paper: TransE / Knowledge Graph Completion](../papers/bordes_transe_2013.md)
- [Best Practice: RETE Forward-Chaining Rule Engine](rete_forward_chaining_rule_engine.md)
- [Best Practice: Multi-LoRA Adapter Routing](multi_lora_adapter_routing.md)
- [`src/graph/FUTURE_ENHANCEMENTS.md`](../../../src/graph/FUTURE_ENHANCEMENTS.md) — Ontology-based Semantic Constraints
- [Apache Jena GenericRuleReasoner](https://jena.apache.org/documentation/inference/#rules)
- [Stardog Integrity Constraint Validation](https://docs.stardog.com/ontologies/integrity-constraint-validation)

---
**Last Updated:** 2026-04-22
