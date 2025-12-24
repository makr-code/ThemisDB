# Process Mining in AQL - Research, Best Practices & Implementation Roadmap

**Version:** 1.0  
**Datum:** 24. Dezember 2025  
**Status:** Research & Planning Phase

---

## Executive Summary

Dieses Dokument beschreibt die Integration von Process Mining Funktionalität in ThemisDB's Advanced Query Language (AQL), mit Fokus auf:
1. **Ähnlichkeitsbasierte Prozesserkennung** - Finden ähnlicher Prozessmuster
2. **Ideale Prozessmodelle** - Vergleich mit Soll-Prozessen
3. **Verwaltungsmodelle** - Administrative und Governance-Prozesse
4. **Multi-Model Mining** - Graph-, Vektor- und relationale Ansätze

---

## 1. Wissenschaftliche Grundlagen & Best Practices

### 1.1 Etablierte Process Mining Algorithmen

#### Alpha Miner (van der Aalst et al., 2004)
**Publikation:** "Workflow Mining: Discovering Process Models from Event Logs"
- **Ansatz:** Footprint-basierte Entdeckung von Kausalitäten
- **Stärken:** Findet Parallelität, gut für strukturierte Prozesse
- **Schwächen:** Sensitive gegenüber Rauschen, keine Schleifen
- **ThemisDB Status:** ✅ Implementiert in `process_mining.cpp`

#### Heuristic Miner (Weijters & van der Aalst, 2003)
**Publikation:** "Flexible Heuristics Miner (FHM)"
- **Ansatz:** Frequenz-basierte Dependency-Maße
- **Stärken:** Robust gegen Rauschen und Ausnahmen
- **Schwächen:** Weniger präzise als Alpha
- **ThemisDB Status:** ✅ Implementiert in `process_mining.cpp`

#### Inductive Miner (Leemans et al., 2013)
**Publikation:** "Discovering Block-Structured Process Models from Event Logs"
- **Ansatz:** Divide-and-Conquer mit garantierter Soundness
- **Stärken:** Garantiert korrekte Petri-Netze
- **Schwächen:** Kann zu generisch werden
- **ThemisDB Status:** 🟡 Stub-Implementierung (nutzt Heuristic Miner)

### 1.2 Ähnlichkeitsmetriken für Prozesse

#### Graph-basierte Metriken

**1. Graph Edit Distance (GED)**
- **Publikation:** Dijkman et al. (2011) - "Similarity of Business Process Models"
- **Ansatz:** Minimale Kosten um Graph A in Graph B zu transformieren
- **Komplexität:** NP-vollständig, Approximationen verfügbar
- **Use Case:** Strukturelle Ähnlichkeit zwischen Prozessmodellen

**2. Node/Edge Overlap**
- **Ansatz:** Jaccard-Ähnlichkeit der Aktivitäten und Übergänge
- **Formel:** `similarity = |A ∩ B| / |A ∪ B|`
- **Komplexität:** O(n)
- **Use Case:** Schnelle Grobfilterung

**3. Path-Based Similarity**
- **Ansatz:** Vergleich von Ausführungspfaden
- **Metriken:** Longest Common Subsequence (LCS)
- **Use Case:** Varianten-Erkennung

#### Vektor-basierte Metriken

**1. Activity Embeddings**
- **Publikation:** Evermann et al. (2017) - "Deep Learning in Process Mining"
- **Ansatz:** Word2Vec/BERT-style Embeddings für Aktivitäten
- **ThemisDB Integration:** Nutzt vorhandenen VectorIndex + GNN Embeddings
- **Use Case:** Semantische Ähnlichkeit (z.B. "Genehmigen" ≈ "Freigeben")

**2. Trace2Vec**
- **Ansatz:** Embedding ganzer Prozess-Traces
- **Berechnung:** Aggregation (Mittelwert) der Activity Embeddings
- **Use Case:** Clustering ähnlicher Prozessinstanzen

**3. HNSW Similarity Search**
- **ThemisDB Status:** ✅ Vorhanden in `vector_index.h`
- **Use Case:** Schnelle k-NN Suche über Prozess-Embeddings

#### Verhaltensbasierte Metriken

**1. Behavioral Profile (Weidlich et al., 2011)**
- **Konzept:** Weak Order Relations zwischen Aktivitäten
- **Metriken:** Strict Order, Exclusive, Interleaving
- **Publikation:** "Behavioural Profiles for Business Process Models"

**2. Log Replay Fitness**
- **Ansatz:** Token-based Replay (Rozinat & van der Aalst, 2008)
- **Metrik:** `fitness = (consumed - missing) / consumed`
- **ThemisDB Status:** ✅ Implementiert in `checkConformance()`

### 1.3 Verwaltungs- und Governance-Prozesse

#### Häufige Verwaltungsprozesse

**1. Bauanträge (Building Permits)**
- Antragstellung → Vollständigkeitsprüfung → Fachprüfung → Genehmigung
- Meilensteine: M1 (Eingang), M2 (Vollständigkeit), M3 (Bescheid)
- SLA: Typisch 3 Monate nach §34 BauO

**2. Personalverwaltung (HR Processes)**
- Stellenausschreibung → Bewerbung → Interview → Einstellung
- Compliance: DSGVO, AGG, Arbeitsrecht
- Meilensteine: Bewerbungseingang, Vorstellungsgespräch, Vertragsangebot

**3. Beschaffung (Procurement)**
- Bedarfsanforderung → Angebotseinholung → Vergabe → Lieferung
- Compliance: Vergaberecht (GWB, VOB/A)
- Schwellenwerte: EU-Schwellenwerte, nationale Schwellenwerte

**4. Haushaltsplanung (Budget Planning)**
- Bedarfsmeldung → Konsolidierung → Genehmigung → Freigabe
- Zyklus: Jährlich mit Quartalsaktualisierungen
- Genehmigungsstufen: Abteilung → Referat → Leitung

#### Best Practices für Verwaltungsmodelle

**1. Four-Eyes-Principle (Vier-Augen-Prinzip)**
```
Antragsteller → Sachbearbeiter → Genehmiger → [Optional: Zweiter Genehmiger]
```

**2. Meilenstein-basierte Überwachung**
- Definierte Meilensteine mit SLA-Zeiten
- Automatische Eskalation bei Verzug
- Statusberichte für Stakeholder

**3. Compliance-Check-Points**
- Automatische Prüfung rechtlicher Anforderungen
- Dokumentation aller Entscheidungen
- Audit-Trail (vollständig nachvollziehbar)

---

## 2. ThemisDB Process Mining Features (Aktueller Stand)

### 2.1 Implementierte Features (v1.3.0)

✅ **Event Log Extraction**
- Aus Collections: `extractEventLog(collection, config)`
- Aus Graph-Kanten: `extractEventLogFromGraph(edge_collection)`
- Aus Referenzen: `extractEventLogFromReferences(start_collection, refs)`

✅ **Process Discovery**
- Alpha Miner: `runAlphaMiner(log, config)`
- Heuristic Miner: `runHeuristicMiner(log, config)`
- DFG-Erstellung: `createDFG(log)`

✅ **Conformance Checking**
- Token Replay: `checkConformance(log, model)`
- Alignment-basiert: `computeAlignment(log, model)`

✅ **Varianten-Analyse**
- `analyzeVariants(log, top_n)`
- `clusterVariants(log, num_clusters)`

✅ **Performance Enhancement**
- `enhanceWithPerformance(model, log)`
- `detectBottlenecks(process, threshold)`

✅ **Export**
- BPMN 2.0: `exportToBPMN(model)`
- Petri-Netz (PNML): `exportToPNML(model)`

### 2.2 Fehlende Features für Ähnlichkeitssuche

❌ **Pattern Matching**
- Keine direkte Suche nach ähnlichen Prozessmustern
- Keine Vektor-basierte Prozesssuche
- Keine Graph-Isomorphismus-Prüfung

❌ **Ideal Process Comparison**
- Kein Vergleich mit Soll-Modellen
- Keine Gap-Analyse zwischen Ist und Soll

❌ **Multi-Model Integration**
- Graph-, Vektor- und relationale Ansätze isoliert
- Keine hybride Ähnlichkeitssuche

---

## 3. Anforderungsanalyse

### 3.1 Use Case: Ähnliche Prozesse finden

**Szenario:**
Ein Administrator hat einen idealen Genehmigungsprozess definiert:
```
Antrag → Vollständigkeit → Prüfung → Genehmigung → Archivierung
```

**Ziel:**
Finde alle ähnlichen Prozesse in den Daten, auch wenn sie:
- Abweichende Aktivitätsnamen haben (semantisch ähnlich)
- Zusätzliche oder fehlende Schritte haben
- In einer anderen Reihenfolge ablaufen

**Ähnlichkeitsansätze:**

1. **Graph-Struktur:**
   - Vergleiche Knoten und Kanten
   - Toleriere kleine Abweichungen

2. **Semantik (Vektor):**
   - Embedding der Aktivitätsnamen
   - Finde "Vollständigkeit" ≈ "Vollständigkeitskontrolle" ≈ "Prüfung Vollständigkeit"

3. **Verhalten:**
   - Vergleiche Ausführungsreihenfolgen
   - Erlaubte vs. verbotene Sequenzen

### 3.2 Funktionale Anforderungen

**FR1: Process Pattern Search**
```aql
-- Finde Prozesse ähnlich zum idealen Modell
LET ideal = {
  activities: ["Antrag", "Prüfung", "Genehmigung"],
  edges: [
    {from: "Antrag", to: "Prüfung"},
    {from: "Prüfung", to: "Genehmigung"}
  ]
}

LET similar = PROCESS_FIND_SIMILAR(
  ideal_model: ideal,
  threshold: 0.7,
  method: "hybrid",  -- graph + vector + behavioral
  limit: 10
)

RETURN similar
```

**FR2: Compare with Ideal Process**
```aql
-- Vergleiche Ist-Prozess mit Soll-Modell
FOR case IN process_instances
  LET comparison = PROCESS_COMPARE_IDEAL(
    case.id,
    ideal_model: "bauantrag_standard_v2",
    metrics: ["fitness", "precision", "structural_similarity"]
  )
  FILTER comparison.fitness < 0.8
  RETURN {case: case.id, deviations: comparison.deviations}
```

**FR3: Pattern-based Filtering**
```aql
-- Finde alle Prozesse mit spezifischem Muster
FOR case IN process_instances
  FILTER PROCESS_HAS_PATTERN(case.id, [
    {activity: "Genehmigung", followed_by: "Ablehnung"}
  ])
  RETURN case
```

**FR4: Semantic Activity Matching**
```aql
-- Finde Prozesse mit semantisch ähnlichen Aktivitäten
LET activities = ["Genehmigen", "Freigeben", "Autorisieren"]

FOR case IN process_instances
  LET trace = EXTRACT_TRACE(case.id)
  LET similarity = VECTOR_SIMILARITY(
    trace.activities_embedding,
    activities_embedding
  )
  FILTER similarity > 0.8
  RETURN case
```

### 3.3 Nicht-funktionale Anforderungen

**NFR1: Performance**
- Ähnlichkeitssuche über 100k Prozesse: < 1 Sekunde
- Nutze HNSW-Index für Vektor-Suche
- Caching für häufige Muster

**NFR2: Skalierbarkeit**
- Inkrementelle Indexierung neuer Prozesse
- Parallele Verarbeitung möglich

**NFR3: Erweiterbarkeit**
- Neue Ähnlichkeitsmetriken einfach hinzufügen
- Plugin-Architektur für Custom-Metriken

---

## 4. Lösungsarchitektur

### 4.1 Komponentenübersicht

```
┌─────────────────────────────────────────────────────────────┐
│                    AQL Query Layer                          │
│  PROCESS_FIND_SIMILAR() | PROCESS_COMPARE_IDEAL()          │
│  PROCESS_HAS_PATTERN()  | VECTOR_SIMILARITY()              │
└────────────────────────┬────────────────────────────────────┘
                         │
┌────────────────────────┴────────────────────────────────────┐
│              Process Mining Engine                          │
│  - Pattern Matching                                         │
│  - Similarity Computation (Hybrid)                          │
│  - Ideal Model Comparison                                   │
└────┬──────────────┬──────────────┬─────────────────────────┘
     │              │              │
┌────┴────┐   ┌────┴─────┐   ┌───┴──────┐
│ Graph   │   │ Vector   │   │ Relational│
│ Index   │   │ Index    │   │ Index     │
│ (HNSW)  │   │ (HNSW)   │   │ (B-Tree)  │
└─────────┘   └──────────┘   └───────────┘
```

### 4.2 Neue Komponenten

#### 4.2.1 ProcessPatternMatcher

**Datei:** `include/analytics/process_pattern_matcher.h`

```cpp
class ProcessPatternMatcher {
public:
    struct Pattern {
        std::vector<std::string> activities;
        std::vector<std::pair<std::string, std::string>> edges;
        std::optional<std::vector<float>> embedding;
    };
    
    struct SimilarityResult {
        std::string case_id;
        double similarity_score;
        std::string similarity_method;
        std::map<std::string, double> metric_breakdown;
        std::vector<std::string> matched_activities;
    };
    
    // Find processes similar to pattern
    std::vector<SimilarityResult> findSimilar(
        const Pattern& ideal_pattern,
        double threshold,
        SimilarityMethod method,
        int limit
    );
    
    // Compare process with ideal model
    ConformanceResult compareWithIdeal(
        const std::string& case_id,
        const Pattern& ideal_model
    );
    
private:
    // Similarity methods
    double computeGraphSimilarity(const Pattern& a, const Pattern& b);
    double computeVectorSimilarity(const Pattern& a, const Pattern& b);
    double computeBehavioralSimilarity(const EventLog& log_a, const EventLog& log_b);
    double computeHybridSimilarity(/* ... */);
};
```

#### 4.2.2 ProcessEmbedder

**Datei:** `include/analytics/process_embedder.h`

```cpp
class ProcessEmbedder {
public:
    // Embed a process trace
    std::vector<float> embedTrace(const ProcessTrace& trace);
    
    // Embed activity sequence
    std::vector<float> embedActivities(const std::vector<std::string>& activities);
    
    // Embed entire process model
    std::vector<float> embedProcessModel(const DiscoveredProcess& model);
    
private:
    VectorIndex& vector_index_;
    
    // Use existing GNN embeddings if available
    std::vector<float> getActivityEmbedding(const std::string& activity);
};
```

#### 4.2.3 AdministrativeModels Library

**Datei:** `config/administrative_process_models.yaml`

```yaml
administrative_models:
  - id: bauantrag_standard
    name: "Bauantrag (Standard)"
    domain: "Bauwesen"
    activities:
      - name: "Antragstellung"
        type: "start"
        sla_hours: 0
      - name: "Vollständigkeitsprüfung"
        type: "task"
        sla_hours: 24
      - name: "Fachliche Prüfung"
        type: "task"
        sla_hours: 2160  # 90 Tage
      - name: "Genehmigung"
        type: "end"
        sla_hours: 24
    edges:
      - from: "Antragstellung"
        to: "Vollständigkeitsprüfung"
      - from: "Vollständigkeitsprüfung"
        to: "Fachliche Prüfung"
      - from: "Fachliche Prüfung"
        to: "Genehmigung"
    compliance:
      - rule: "§34 BauO"
        description: "Genehmigung binnen 3 Monaten"
      - rule: "Vier-Augen-Prinzip"
        description: "Prüfung und Genehmigung durch unterschiedliche Personen"
        
  - id: beschaffung_vergaberecht
    name: "Beschaffung (Vergaberecht)"
    domain: "Procurement"
    # ... weitere Modelle
```

---

## 5. Implementierungsroadmap

### Phase 1: Grundlagen (Woche 1-2) ✅ COMPLETED

**Ziel:** Research und Design

- [x] Literaturrecherche abgeschlossen
- [x] Best Practices dokumentiert
- [x] Ähnlichkeitsmetriken definiert
- [x] API-Design erstellt
- [x] Architektur-Diagramme erstellt

### Phase 2: Basis-Implementierung (Woche 3-4)

**Ziel:** Core Pattern Matching

**2.1 ProcessPatternMatcher**
```cpp
// Datei: src/analytics/process_pattern_matcher.cpp
- [ ] Implementiere findSimilar()
- [ ] Implementiere compareWithIdeal()
- [ ] Graph-basierte Ähnlichkeit
- [ ] Tests für Pattern Matching
```

**2.2 Graph Similarity Metrics**
```cpp
// Datei: src/analytics/graph_similarity.cpp
- [ ] Node/Edge Overlap (Jaccard)
- [ ] Path-based Similarity (LCS)
- [ ] Graph Edit Distance (Approximation)
- [ ] Behavioral Profile Comparison
```

**2.3 Process Embeddings**
```cpp
// Datei: src/analytics/process_embedder.cpp
- [ ] Activity Embeddings (nutze VectorIndex)
- [ ] Trace2Vec (Aggregation)
- [ ] Process Model Embeddings
- [ ] Integration mit GNN Embeddings
```

### Phase 3: AQL Integration (Woche 5)

**3.1 Neue AQL Funktionen**

**Datei:** `include/query/functions/process_mining_functions.h`

```cpp
class ProcessFindSimilarFunction : public IFunction {
    // PROCESS_FIND_SIMILAR(ideal, threshold, method, limit)
};

class ProcessCompareIdealFunction : public IFunction {
    // PROCESS_COMPARE_IDEAL(case_id, ideal_model, metrics)
};

class ProcessHasPatternFunction : public IFunction {
    // PROCESS_HAS_PATTERN(case_id, pattern)
};

class ProcessExtractTraceFunction : public IFunction {
    // PROCESS_EXTRACT_TRACE(case_id)
};
```

**3.2 Funktion Registrierung**
```cpp
// Datei: src/query/functions/function_registry.cpp
- [ ] Registriere PROCESS_FIND_SIMILAR
- [ ] Registriere PROCESS_COMPARE_IDEAL
- [ ] Registriere PROCESS_HAS_PATTERN
- [ ] Registriere PROCESS_EXTRACT_TRACE
```

### Phase 4: Administrative Models (Woche 6)

**4.1 Model Library**
```yaml
- [ ] Bauantragsverfahren
- [ ] Personalverwaltung
- [ ] Beschaffungsprozesse
- [ ] Haushaltsplanung
- [ ] Dokumentenfreigabe
```

**4.2 Model Loader**
```cpp
// Datei: src/analytics/administrative_model_loader.cpp
- [ ] YAML Parser für Modelle
- [ ] Model Validation
- [ ] Model Caching
```

**4.3 Compliance Checker**
```cpp
- [ ] SLA-Überwachung
- [ ] Vier-Augen-Prinzip Checker
- [ ] Rechtliche Anforderungen (GWB, DSGVO)
```

### Phase 5: Optimierung (Woche 7-8)

**5.1 Indexierung**
```cpp
- [ ] Process Pattern Index (HNSW)
- [ ] Caching häufiger Muster
- [ ] Inkrementelle Updates
```

**5.2 Performance**
```cpp
- [ ] Parallelisierung (OpenMP)
- [ ] GPU-Beschleunigung (CUDA)
- [ ] Batch-Processing
```

**5.3 Benchmarks**
```cpp
- [ ] Pattern Matching Performance
- [ ] Similarity Computation
- [ ] Large-Scale Tests (100k+ Prozesse)
```

### Phase 6: Testing & Documentation (Woche 9)

**6.1 Tests**
```cpp
// tests/test_process_pattern_matching.cpp
- [ ] Unit Tests für Pattern Matcher
- [ ] Integration Tests für AQL Funktionen
- [ ] End-to-End Tests
```

**6.2 Dokumentation**
```markdown
- [ ] API Referenz
- [ ] Tutorial: Ähnliche Prozesse finden
- [ ] Administrative Model Guide
- [ ] Performance Tuning Guide
```

**6.3 Beispiele**
```aql
- [ ] Bauantrag Conformance Checking
- [ ] Procurement Pattern Discovery
- [ ] HR Process Optimization
```

---

## 6. Beispiel-Use-Cases

### 6.1 Bauantrag: Abweichungen finden

**Szenario:** 
Stadt Berlin hat 10.000 Bauanträge. Finde alle, die vom Standard-Prozess abweichen.

```aql
-- Lade Standard-Modell
LET ideal = LOAD_ADMINISTRATIVE_MODEL("bauantrag_standard")

-- Extrahiere alle Prozess-Traces
FOR case IN bauantraege
  LET trace = PROCESS_EXTRACT_TRACE(case.vorgang_id)
  
  -- Vergleiche mit Ideal
  LET comparison = PROCESS_COMPARE_IDEAL(
    case.vorgang_id,
    ideal_model: ideal,
    metrics: ["fitness", "precision", "duration_deviation"]
  )
  
  -- Filter: Nur Abweichungen
  FILTER comparison.fitness < 0.9 OR comparison.duration_deviation > 30
  
  RETURN {
    vorgang_id: case.vorgang_id,
    fitness: comparison.fitness,
    deviations: comparison.deviations,
    duration_days: comparison.actual_duration / 86400000
  }
```

### 6.2 Ähnliche Genehmigungsprozesse finden

**Szenario:**
Finde alle Prozesse, die dem Muster "Antrag → Prüfung → Genehmigung" ähneln.

```aql
-- Definiere Muster
LET pattern = {
  activities: ["Antrag", "Prüfung", "Genehmigung"],
  edges: [
    {from: "Antrag", to: "Prüfung"},
    {from: "Prüfung", to: "Genehmigung"}
  ]
}

-- Suche ähnliche Prozesse
LET similar = PROCESS_FIND_SIMILAR(
  ideal_model: pattern,
  threshold: 0.7,
  method: "hybrid",
  limit: 50
)

-- Gruppiere nach Ähnlichkeit
FOR result IN similar
  COLLECT similarity_range = FLOOR(result.similarity_score * 10) / 10
  INTO group
  
  RETURN {
    similarity_range: similarity_range,
    count: LENGTH(group),
    cases: group[*].result.case_id
  }
```

### 6.3 Compliance Checking mit SLA

```aql
-- Finde alle überfälligen Bauanträge
FOR case IN bauantraege
  LET status = MILESTONE_STATUS(case.vorgang_id)
  
  FILTER status.overdue > 0
  
  -- Hole SLA-Status
  LET sla = SLA_CHECK(case.vorgang_id)
  
  -- Prüfe Conformance
  LET conformance = PROCESS_CONFORMANCE(
    case.vorgang_id,
    "bauantrag_standard"
  )
  
  RETURN {
    vorgang_id: case.vorgang_id,
    overdue_milestones: status.overdue,
    sla_status: sla.status,
    conformance: conformance.conformance_percent,
    next_action: MILESTONE_NEXT(case.vorgang_id).name
  }
```

---

## 7. Wissenschaftliche Publikationen (Referenzen)

### Process Mining Grundlagen

1. **van der Aalst, W. M. P. (2016)**
   - *Process Mining: Data Science in Action*
   - Springer, 2nd Edition
   - ISBN: 978-3662498507

2. **van der Aalst, W. M. P., et al. (2004)**
   - "Workflow Mining: Discovering Process Models from Event Logs"
   - IEEE TKDE, 16(9), 1128-1142

3. **Weijters, A. J. M. M., & van der Aalst, W. M. P. (2003)**
   - "Rediscovering Workflow Models from Event-Based Data using Little Thumb"
   - Integrated Computer-Aided Engineering, 10(2), 151-162

4. **Leemans, S. J. J., et al. (2013)**
   - "Discovering Block-Structured Process Models from Event Logs"
   - BPM 2013, LNCS 8094

### Similarity & Pattern Matching

5. **Dijkman, R., et al. (2011)**
   - "Similarity of Business Process Models: Metrics and Evaluation"
   - Information Systems, 36(2), 498-516

6. **Weidlich, M., et al. (2011)**
   - "Behavioural Profiles for Business Process Models"
   - ECIS 2011

7. **Evermann, J., et al. (2017)**
   - "Predicting Process Behaviour Using Deep Learning"
   - Decision Support Systems, 100, 129-140

### Conformance Checking

8. **Rozinat, A., & van der Aalst, W. M. P. (2008)**
   - "Conformance Checking of Processes Based on Monitoring Real Behavior"
   - Information Systems, 33(1), 64-95

9. **Adriansyah, A., et al. (2011)**
   - "Conformance Checking using Cost-Based Fitness Analysis"
   - EDOC 2011

### Verwaltungsprozesse & E-Government

10. **Janssen, M., & Cresswell, A. M. (2005)**
    - "Enterprise Architecture Integration in E-Government"
    - HICSS 2005

11. **Klischewski, R., & Scholl, H. J. (2006)**
    - "Information Quality as a Common Ground for Key Players in e-Government Integration and Interoperability"
    - HICSS 2006

---

## 8. Erfolgskriterien

### 8.1 Funktionale Kriterien

✅ **Pattern Matching:**
- Finde Prozesse mit > 70% Strukturähnlichkeit in < 1 Sekunde
- Unterstütze Graph-, Vektor- und Verhaltens-basierte Suche
- Hybride Metriken kombinierbar

✅ **Ideal Model Comparison:**
- Berechne Fitness, Precision, Generalization
- Identifiziere spezifische Abweichungen
- Gap-Analyse zwischen Ist und Soll

✅ **Administrative Models:**
- Mindestens 5 vordefinierte Verwaltungsprozesse
- SLA-Integration
- Compliance-Checks

### 8.2 Performance-Kriterien

- **100k Prozesse:** Ähnlichkeitssuche < 1 Sekunde
- **1M Prozesse:** Inkrementelles Indexing < 5 Sekunden
- **Pattern Cache Hit Rate:** > 80% bei häufigen Mustern

### 8.3 Qualitätskriterien

- **Code Coverage:** > 85% für neue Komponenten
- **API Stabilität:** Keine Breaking Changes
- **Dokumentation:** Vollständige API-Referenz + Tutorials

---

## 9. Risiken & Mitigationen

### 9.1 Technische Risiken

**Risiko 1: Performance bei großen Graphen**
- **Mitigation:** Approximations-Algorithmen (z.B. A* für GED)
- **Mitigation:** HNSW-Index für Vektor-basierte Suche
- **Mitigation:** Caching häufiger Muster

**Risiko 2: Embedding-Qualität**
- **Mitigation:** Nutze vorhandene GNN-Embeddings
- **Mitigation:** Fine-Tuning auf Domain-Daten
- **Mitigation:** Fallback auf String-basierte Metriken

**Risiko 3: Skalierbarkeit**
- **Mitigation:** Parallele Verarbeitung (OpenMP)
- **Mitigation:** GPU-Beschleunigung optional
- **Mitigation:** Inkrementelle Indexierung

### 9.2 Organisatorische Risiken

**Risiko 1: Unklare Requirements**
- **Mitigation:** Iteratives Design mit Feedback-Schleifen
- **Mitigation:** Konkrete Use Cases als Leitfaden

**Risiko 2: Integration-Komplexität**
- **Mitigation:** Nutze bestehende Komponenten (VectorIndex, GraphIndex)
- **Mitigation:** Klare Interface-Definitionen
- **Mitigation:** Schrittweise Integration

---

## 10. Nächste Schritte

### Sofort (diese Woche)

1. ✅ **Research-Dokument erstellen** (dieses Dokument)
2. ⏳ **ProcessPatternMatcher Interface** definieren
3. ⏳ **Erste AQL Funktion** prototypisch implementieren
4. ⏳ **Administrative Models** YAML-Schema erstellen

### Nächste Woche

5. ⏳ Graph Similarity Metrics implementieren
6. ⏳ Process Embeddings integrieren
7. ⏳ Unit Tests schreiben
8. ⏳ Erste Use Cases testen

### Folgender Sprint

9. ⏳ Performance-Optimierung
10. ⏳ Vollständige AQL Integration
11. ⏳ Dokumentation & Tutorials
12. ⏳ Benchmarks

---

## Appendix A: Glossar

**DFG** - Directly-Follows Graph: Graph der zeigt, welche Aktivitäten direkt aufeinander folgen

**GED** - Graph Edit Distance: Minimale Kosten um einen Graphen in einen anderen zu transformieren

**HNSW** - Hierarchical Navigable Small World: Effizienter Index für Vektor-Ähnlichkeitssuche

**LCS** - Longest Common Subsequence: Längste gemeinsame Teilsequenz zwischen zwei Sequenzen

**Trace** - Prozess-Trace: Sequenz von Aktivitäten für eine spezifische Prozessinstanz

**Conformance** - Übereinstimmung: Wie gut passt ein ausgeführter Prozess zum Modell

**Fitness** - Passung: Anteil der Traces die vom Modell erklärbar sind (Recall)

**Precision** - Präzision: Anteil des Modells der in den Traces vorkommt (Precision)

---

**Status:** ✅ Research Phase abgeschlossen  
**Nächster Schritt:** Implementierung Phase 2  
**Autor:** ThemisDB Team  
**Review Date:** Q1 2026
