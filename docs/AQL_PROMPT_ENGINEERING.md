# AQL Prompt Engineering Guide für LLM-basierte Datenrecherche

## Übersicht

Dieses Dokument beschreibt, wie Large Language Models (LLMs) effektiv mit ThemisDB AQL für Datenrecherche eingesetzt werden können. Es ist speziell für die Integration mit dem VCC-Veritas Agenten-Framework konzipiert.

## Inhaltsverzeichnis

1. [Wissenschaftliches Prompting](#wissenschaftliches-prompting)
2. [System-Prompt Vorlage](#system-prompt-vorlage)
3. [Tool-Definitionen](#tool-definitionen)
4. [Agent-Workflow](#agent-workflow)
5. [Kostenbasierte Query-Planung](#kostenbasierte-query-planung)
6. [Query-Generierung](#query-generierung)
7. [Multi-Model Beispiele](#multi-model-beispiele)
8. [Sicherheitsrichtlinien](#sicherheitsrichtlinien)
9. [Chain-of-Thought Prompting](#chain-of-thought-prompting)
10. [VCC-Veritas Integration](#vcc-veritas-integration)
11. [Fehlerbehandlung](#fehlerbehandlung)
12. [Best Practices](#best-practices)

---

## Wissenschaftliches Prompting

ThemisDB verwendet ein **wissenschaftliches Prompting-Paradigma**, das komplexe Fragestellungen systematisch in lösbare Teilprobleme zerlegt.

### Das Wissenschaftliche Modell

```
┌─────────────────────────────────────────────────────────────────┐
│                    WISSENSCHAFTLICHER ZYKLUS                    │
│                                                                 │
│   ┌──────────┐    ┌──────────┐    ┌──────────┐    ┌──────────┐ │
│   │HYPOTHESE │ → │RECHERCHE │ → │ ANALYSE  │ → │ SYNTHESE │ │
│   └──────────┘    └──────────┘    └──────────┘    └──────────┘ │
│        ↑                                              │        │
│        └──────────────── ITERATION ←─────────────────┘        │
└─────────────────────────────────────────────────────────────────┘
```

### Phase 1: Hypothesenbildung

Komplexe Fragestellungen werden in **testbare Teilhypothesen** zerlegt:

```yaml
fragestellung: "Welche Faktoren beeinflussen die Bearbeitungszeit von Bauanträgen?"

hypothesen:
  H1:
    aussage: "Die Komplexität des Antrags korreliert mit der Bearbeitungsdauer"
    testbar: true
    datenquelle: ["antraege", "bearbeitungszeiten"]
    prioritaet: hoch
    
  H2:
    aussage: "Erfahrene Sachbearbeiter bearbeiten schneller"
    testbar: true
    datenquelle: ["sachbearbeiter", "bearbeitungszeiten"]
    abhaengig_von: []
    prioritaet: mittel
    
  H3:
    aussage: "Saisonale Effekte beeinflussen die Dauer"
    testbar: true
    datenquelle: ["bearbeitungszeiten"]
    prioritaet: niedrig
    
  H4:
    aussage: "Externe Faktoren (Nachforderungen) verlängern die Bearbeitung"
    testbar: true
    datenquelle: ["antraege", "kommunikation"]
    abhaengig_von: [H1]
    prioritaet: hoch

abhängigkeitsbaum:
  H1 → H4  # H4 kann erst getestet werden, wenn H1 bestätigt
```

### Phase 2: Recherchepläne mit Kostenbewertung

Für jede Hypothese werden **multiple Recherchepläne** erstellt und bewertet:

```yaml
hypothese: H1 - Komplexität ↔ Dauer

recherchepläne:
  plan_A:
    name: "Direkte Korrelation"
    queries:
      - "FOR a IN antraege COLLECT complexity = a.complexity INTO g RETURN {complexity, avg_duration: AVG(g[*].a.duration)}"
    geschätzte_kosten:
      komplexität: O(n)
      datenmenge: 50000 Dokumente
      index_nutzung: true (complexity_idx)
      parallelisierbar: true
      geschätzte_zeit_ms: 150
    erwarteter_nutzen: hoch
    kosten_nutzen_score: 0.85
    
  plan_B:
    name: "Detaillierte Regressionsanalyse"
    queries:
      - "FOR a IN antraege RETURN {complexity: a.complexity, duration: a.duration, type: a.type, size: a.document_count}"
    nachbearbeitung: "Statistische Regression in Python"
    geschätzte_kosten:
      komplexität: O(n)
      datenmenge: 50000 Dokumente
      index_nutzung: false
      parallelisierbar: true
      geschätzte_zeit_ms: 800
      externe_verarbeitung: true
    erwarteter_nutzen: sehr_hoch
    kosten_nutzen_score: 0.72
    
  plan_C:
    name: "Sampling-basierte Analyse"
    queries:
      - "FOR a IN antraege FILTER RAND() < 0.1 RETURN {complexity: a.complexity, duration: a.duration}"
    geschätzte_kosten:
      komplexität: O(n) aber 10% Daten
      datenmenge: 5000 Dokumente
      geschätzte_zeit_ms: 50
    erwarteter_nutzen: mittel
    kosten_nutzen_score: 0.65

empfohlener_plan: plan_A  # Bestes Kosten-Nutzen-Verhältnis
```

### Kostenmodell

```
                    KOSTEN-NUTZEN-MATRIX
                    
Nutzen ↑   │ Hohe Priorität │ Sofort        │
(Relevanz) │ (Plan B)       │ ausführen     │
           │                │ (Plan A)      │
           ├────────────────┼───────────────┤
           │ Vermeiden      │ Niedrige      │
           │                │ Priorität     │
           │                │ (Plan C)      │
           └────────────────┴───────────────→ Kosten
                  Hoch            Niedrig
```

**Kostenfaktoren:**

| Faktor | Formel | Beispiel |
|--------|--------|----------|
| Komplexität | O(1)=1, O(n)=10, O(n log n)=50, O(n²)=100 | SHORTEST_PATH = 100 |
| Datenmenge | log10(n) | 1M Docs = 6 |
| Index-Multiplikator | Mit Index: 0.1, Ohne: 1.0 | Index → 10x schneller |
| Parallelisierung | Parallel: 0.5, Seriell: 1.0 | 2x schneller |

**Gesamtkosten:**
```
Kosten = Komplexität × Datenmenge × Index_Mult × Parallel_Mult
Nutzen = Relevanz × Präzision × Vollständigkeit
Score = Nutzen / Kosten
```

### Phase 3: Iterative Recherche

```python
# Pseudocode für wissenschaftlichen Agent

def wissenschaftliche_recherche(fragestellung):
    # Phase 1: Hypothesenbildung
    hypothesen = zerlege_in_hypothesen(fragestellung)
    hypothesen = sortiere_nach_prioritaet(hypothesen)
    
    ergebnisse = {}
    konfidenz = {}
    
    # Phase 2: Iterative Recherche
    for h in hypothesen:
        if not abhaengigkeiten_erfuellt(h, ergebnisse):
            continue
            
        # Recherchepläne erstellen und bewerten
        pläne = erstelle_recherchepläne(h)
        pläne = bewerte_kosten_nutzen(pläne)
        
        # Besten Plan auswählen
        bester_plan = max(pläne, key=lambda p: p.score)
        
        # Ausführen
        resultat = execute_aql(bester_plan.query)
        
        # Analysieren
        ergebnisse[h.id] = analysiere(resultat)
        konfidenz[h.id] = berechne_konfidenz(resultat)
        
        # Adaptive Anpassung
        if konfidenz[h.id] < 0.7:
            # Niedrige Konfidenz → Alternative versuchen
            alternativer_plan = pläne[1]  # Zweitbester
            resultat_alt = execute_aql(alternativer_plan.query)
            ergebnisse[h.id] = merge_resultate(ergebnisse[h.id], resultat_alt)
    
    # Phase 3: Synthese
    return synthetisiere(ergebnisse, konfidenz)
```

### Phase 4: Synthese

Die Teilergebnisse werden zu einer **Gesamtantwort** zusammengeführt:

```yaml
synthese:
  fragestellung: "Welche Faktoren beeinflussen die Bearbeitungszeit?"
  
  bestätigte_hypothesen:
    - H1: "Komplexität korreliert stark mit Dauer (r=0.78)"
    - H4: "Nachforderungen verlängern um durchschnittlich 12 Tage"
    
  teilweise_bestätigt:
    - H2: "Erfahrung hat moderaten Einfluss (r=0.34)"
    
  widerlegt:
    - H3: "Keine signifikanten saisonalen Effekte gefunden"
    
  neue_erkenntnisse:
    - "Anträge mit > 20 Dokumenten haben 3x längere Bearbeitungszeit"
    - "Elektronische Einreichung spart 5 Tage"
    
  gesamtkonfidenz: 0.82
  
  empfehlungen:
    - "Fokus auf Reduzierung von Nachforderungen"
    - "Elektronische Einreichung fördern"
    - "Komplexe Anträge früh identifizieren und priorisieren"
    
  offene_fragen:
    - "Wie wirkt sich die Auslastung auf die Bearbeitungszeit aus?"
```

### Beispiel: Vollständiger Wissenschaftlicher Workflow

**Benutzeranfrage**: "Warum sind manche Bauanträge so lange in Bearbeitung?"

```
Agent (wissenschaftliches Reasoning):

╔════════════════════════════════════════════════════════════════╗
║ PHASE 1: HYPOTHESENBILDUNG                                     ║
╠════════════════════════════════════════════════════════════════╣
║ Komplexe Fragestellung erkannt. Zerlege in Teilhypothesen:     ║
║                                                                ║
║ H1: Antragskomplexität → Dauer           [Priorität: HOCH]     ║
║ H2: Sachbearbeiter-Erfahrung → Dauer     [Priorität: MITTEL]   ║
║ H3: Nachforderungen → Verzögerung        [Priorität: HOCH]     ║
║ H4: Saisonale Effekte                    [Priorität: NIEDRIG]  ║
╚════════════════════════════════════════════════════════════════╝

╔════════════════════════════════════════════════════════════════╗
║ PHASE 2: RECHERCHEPLÄNE BEWERTEN                               ║
╠════════════════════════════════════════════════════════════════╣
║ H1 - Komplexität:                                              ║
║   Plan A: Gruppierung nach Komplexität    [Score: 0.85] ✓      ║
║   Plan B: Vollständige Regression         [Score: 0.72]        ║
║   Plan C: Sampling                        [Score: 0.65]        ║
║                                                                ║
║ → Wähle Plan A (beste Kosten-Nutzen-Relation)                 ║
╚════════════════════════════════════════════════════════════════╝

╔════════════════════════════════════════════════════════════════╗
║ PHASE 2: RECHERCHE AUSFÜHREN                                   ║
╠════════════════════════════════════════════════════════════════╣
║ Query H1:                                                      ║
║ FOR a IN antraege                                              ║
║   FILTER a.type == "Bauantrag"                                 ║
║   COLLECT complexity = a.complexity INTO group                 ║
║   RETURN {                                                     ║
║     complexity,                                                ║
║     count: LENGTH(group),                                      ║
║     avg_days: AVG(group[*].a.processing_days)                 ║
║   }                                                            ║
║                                                                ║
║ Ergebnis:                                                      ║
║   {complexity: "einfach", count: 1200, avg_days: 15}          ║
║   {complexity: "mittel", count: 800, avg_days: 35}            ║
║   {complexity: "komplex", count: 200, avg_days: 78}           ║
║                                                                ║
║ → H1 BESTÄTIGT (Korrelation: r=0.78)                          ║
╚════════════════════════════════════════════════════════════════╝

╔════════════════════════════════════════════════════════════════╗
║ PHASE 3: SYNTHESE                                              ║
╠════════════════════════════════════════════════════════════════╣
║ Hauptfaktoren für lange Bearbeitungszeiten:                    ║
║                                                                ║
║ 1. Antragskomplexität (Faktor 5x)                             ║
║    - Einfache Anträge: ∅ 15 Tage                              ║
║    - Komplexe Anträge: ∅ 78 Tage                              ║
║                                                                ║
║ 2. Nachforderungen (Faktor 2x)                                ║
║    - Mit Nachforderung: +25 Tage                              ║
║    - Durchschnitt 1.8 Nachforderungen bei komplexen Anträgen  ║
║                                                                ║
║ 3. Sachbearbeiter-Erfahrung (Faktor 1.3x)                     ║
║    - Erfahrene (>5 Jahre): 20% schneller                      ║
║                                                                ║
║ Gesamtkonfidenz: 82%                                          ║
╚════════════════════════════════════════════════════════════════╝
```

### Kontinuierlicher Prozess

Der wissenschaftliche Zyklus ist **iterativ und adaptiv**:

```
Iteration 1: Initiale Hypothesen testen
    ↓
Iteration 2: Neue Hypothesen aus Erkenntnissen ableiten
    ↓
Iteration 3: Tiefergehende Analyse der bestätigten Hypothesen
    ↓
Iteration 4: Kausalzusammenhänge untersuchen
    ↓
... (bis Konfidenz-Schwelle erreicht oder Budget erschöpft)
```

### Adaptive Kostenoptimierung

Der Agent lernt aus vergangenen Abfragen:

```yaml
query_history:
  - query: "FOR a IN antraege COLLECT complexity = a.complexity..."
    actual_time_ms: 142
    estimated_time_ms: 150
    accuracy: 0.95
    
  - query: "FOR a IN antraege FOR b IN bearbeiter..."
    actual_time_ms: 3200
    estimated_time_ms: 800
    accuracy: 0.25  # Unterschätzt!
    
kostenmodell_anpassung:
  join_queries:
    multiplier: 4.0  # Erhöht, da Joins unterschätzt wurden
```

---

## Massive Parallelisierung

Die Zerlegung in unabhängige Hypothesen ermöglicht **massive Parallelisierung**:

### Dependency Graph für Parallelisierung

```
                    PARALLELISIERUNGS-GRAPH
                    
        ┌────────────────────────────────────────┐
        │           FRAGESTELLUNG                │
        └────────────────┬───────────────────────┘
                         │
         ┌───────────────┼───────────────┐
         ↓               ↓               ↓
    ┌─────────┐    ┌─────────┐    ┌─────────┐
    │   H1    │    │   H2    │    │   H3    │   ← PARALLEL
    │Komplex. │    │Erfahrung│    │Saison   │
    └────┬────┘    └────┬────┘    └────┬────┘
         │              │              │
         ↓              │              │
    ┌─────────┐         │              │
    │   H4    │←────────┘              │      ← ABHÄNGIG
    │Nachford.│                        │
    └────┬────┘                        │
         │                             │
         └──────────────┬──────────────┘
                        ↓
                 ┌─────────────┐
                 │  SYNTHESE   │
                 └─────────────┘
```

### Worker-Pool Architektur

```yaml
parallelisierung:
  max_workers: 8
  
  worker_pool:
    query_workers: 4      # AQL-Ausführung
    analysis_workers: 2   # Statistische Analyse
    synthesis_workers: 2  # Ergebnis-Zusammenführung
    
  scheduling:
    strategie: "dependency_aware"
    priorität_nach: "kosten_nutzen_score"
    
  beispiel_ausführung:
    t0: [H1, H2, H3]  # Parallel starten
    t1: [H4]          # Wartet auf H1
    t2: [Synthese]    # Wartet auf alle
    
  speedup:
    sequentiell: "~4 Sekunden"
    parallel: "~1.5 Sekunden"
    faktor: 2.7x
```

### Parallele Query-Ausführung

```python
async def parallel_recherche(hypothesen):
    """Führt unabhängige Hypothesen parallel aus."""
    
    # Dependency Graph erstellen
    graph = build_dependency_graph(hypothesen)
    
    # Unabhängige Hypothesen identifizieren
    unabhängig = [h for h in hypothesen if not h.abhaengig_von]
    
    # Parallel ausführen
    async with TaskGroup() as tg:
        for h in unabhängig:
            tg.create_task(teste_hypothese(h))
    
    # Abhängige Hypothesen in Wellen
    while incomplete_hypothesen:
        bereit = [h for h in incomplete if abhaengigkeiten_erfuellt(h)]
        async with TaskGroup() as tg:
            for h in bereit:
                tg.create_task(teste_hypothese(h))
```

---

## SLM/NLP Optimierung

Die wissenschaftliche Methodik ermöglicht den **effizienten Einsatz kleiner Modelle**:

### Modell-Hierarchie

```
┌─────────────────────────────────────────────────────────────────┐
│                    MODELL-AUSWAHL-PYRAMIDE                      │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│                      ┌───────────┐                              │
│                      │  LLM      │  ← Planung & Synthese        │
│                      │ (70B+)    │     Komplexe Reasoning        │
│                      │   5%      │     Neue Hypothesen           │
│                      └─────┬─────┘                              │
│                            │                                    │
│                   ┌────────┴────────┐                           │
│                   │      SLM        │  ← Query-Generierung       │
│                   │    (7B-13B)     │     Template-Anpassung     │
│                   │      15%        │     Ergebnis-Formatierung  │
│                   └────────┬────────┘                           │
│                            │                                    │
│           ┌────────────────┴────────────────┐                   │
│           │           NLP/Regex             │  ← Parsing         │
│           │        (Regel-basiert)          │     Extraktion     │
│           │             30%                 │     Validierung    │
│           └────────────────┬────────────────┘                   │
│                            │                                    │
│  ┌─────────────────────────┴─────────────────────────┐          │
│  │              Direkte Ausführung                   │← Caching  │
│  │            (Kein Modell nötig)                    │  Lookup   │
│  │                    50%                            │  Index    │
│  └───────────────────────────────────────────────────┘          │
│                                                                 │
│  Anteil der Anfragen ────────────────────────────── Kosten →   │
└─────────────────────────────────────────────────────────────────┘
```

### Aufgaben-Routing

```yaml
modell_routing:
  # Großes LLM (GPT-4, Claude, Llama-70B)
  llm_tasks:
    - "Komplexe Fragestellung zerlegen"
    - "Neue Hypothesen aus Ergebnissen ableiten"
    - "Synthese mit Schlussfolgerungen"
    - "Unerwartete Muster erklären"
    kosten: "$$$$"
    latenz: "1-5 Sekunden"
    
  # Kleines SLM (Llama-7B, Mistral-7B, Phi-3)
  slm_tasks:
    - "AQL-Query aus Template generieren"
    - "Ergebnisse in natürliche Sprache formatieren"
    - "Einfache Folgefragen beantworten"
    - "Feld-Mapping durchführen"
    kosten: "$"
    latenz: "100-500ms"
    
  # NLP/Regel-basiert (spaCy, Regex, Keyword-Matching)
  nlp_tasks:
    - "Entitäten extrahieren (Namen, Daten, Zahlen)"
    - "Intent erkennen (Suche, Aggregation, Vergleich)"
    - "Collection-Namen matchen"
    - "Syntax-Validierung"
    kosten: "Minimal"
    latenz: "<10ms"
    
  # Direkt (Kein Modell)
  direct_tasks:
    - "Gecachte Abfragen wiederverwenden"
    - "Standard-Dashboards laden"
    - "Schema-Lookups"
    - "Vordefinierte Reports"
    kosten: "Keine GPU"
    latenz: "<1ms"
```

### Query-Template-System für SLM

Kleine Modelle arbeiten effizient mit **Templates**:

```yaml
templates:
  # Template: Einfache Suche
  simple_search:
    pattern: "Finde|Zeige|Liste {entity} mit|wo {condition}"
    template: |
      FOR doc IN {collection}
        FILTER doc.{field} {operator} @value
        LIMIT @limit
        RETURN doc
    slm_task: "Extrahiere collection, field, operator, value"
    
  # Template: Aggregation
  aggregation:
    pattern: "Wie viele|Summe|Durchschnitt von {metric} pro {group}"
    template: |
      FOR doc IN {collection}
        COLLECT group = doc.{group_field}
        AGGREGATE result = {agg_func}(doc.{metric_field})
        RETURN { group, result }
    slm_task: "Extrahiere collection, group_field, agg_func, metric_field"
    
  # Template: Zeitreihe
  timeseries:
    pattern: "{metric} über Zeit|pro Monat|pro Tag"
    template: |
      FOR doc IN {collection}
        COLLECT period = DATE_FORMAT(doc.{date_field}, "{format}")
        AGGREGATE value = {agg_func}(doc.{metric_field})
        SORT period
        RETURN { period, value }
```

### SLM Query-Generator

```python
class SLMQueryGenerator:
    """Verwendet kleines Modell für Query-Generierung."""
    
    def __init__(self, model="mistral-7b"):
        self.model = load_model(model)
        self.templates = load_templates()
        
    def generate(self, intent, entities, schema):
        # Template auswählen
        template = self.match_template(intent)
        
        if template:
            # Schneller Pfad: Template ausfüllen
            prompt = f"""
            Template: {template.template}
            Entities: {entities}
            Schema: {schema}
            
            Fülle die Platzhalter aus:
            """
            return self.model.complete(prompt, max_tokens=100)
        else:
            # Fallback: Vollständige Generierung
            return self.escalate_to_llm(intent, entities, schema)
```

### Kosten-Vergleich

```
╔═══════════════════════════════════════════════════════════════════╗
║                    KOSTEN PRO 1000 ANFRAGEN                       ║
╠═══════════════════════════════════════════════════════════════════╣
║                                                                   ║
║  Traditionell (nur LLM):                                          ║
║  ├── 1000 × LLM-Aufruf = 1000 × $0.03 = $30.00                   ║
║  └── Latenz: ∅ 2 Sekunden                                        ║
║                                                                   ║
║  Wissenschaftliches Prompting (Hybrid):                           ║
║  ├── 50 × LLM (Planung/Synthese)  = 50 × $0.03  = $1.50          ║
║  ├── 150 × SLM (Query-Gen)        = 150 × $0.001 = $0.15         ║
║  ├── 300 × NLP (Parsing)          = 300 × $0.0001 = $0.03        ║
║  └── 500 × Direct (Cache)         = 500 × $0.00   = $0.00        ║
║                                                                   ║
║  GESAMT: $1.68 (94% Ersparnis!)                                  ║
║  Latenz: ∅ 300ms (85% schneller!)                                ║
║                                                                   ║
╚═══════════════════════════════════════════════════════════════════╝
```

### VCC-Veritas SLM-Konfiguration

```yaml
vcc_veritas:
  agent_config:
    orchestrator:
      model: "gpt-4"
      tasks: ["planning", "synthesis", "complex_reasoning"]
      max_tokens: 4096
      
    query_generator:
      model: "mistral-7b-instruct"
      tasks: ["query_generation", "formatting"]
      max_tokens: 256
      templates_enabled: true
      
    entity_extractor:
      type: "spacy"
      model: "de_core_news_lg"
      tasks: ["ner", "intent_classification"]
      
    cache:
      type: "redis"
      ttl_seconds: 3600
      max_entries: 10000
      
  routing_rules:
    - condition: "intent in ['simple_search', 'count', 'list']"
      route: "query_generator"
      
    - condition: "has_cached_query(entities)"
      route: "cache"
      
    - condition: "intent in ['complex_analysis', 'hypothesis']"
      route: "orchestrator"
      
  parallel_execution:
    enabled: true
    max_concurrent_queries: 10
    hypothesis_parallelism: true
```

---

## System-Prompt Vorlage

```
Du bist ein ThemisDB Datenanalyse-Agent. Deine Aufgabe ist es, Benutzeranfragen 
in AQL-Abfragen zu übersetzen und die Ergebnisse zu interpretieren.

### Deine Fähigkeiten:
- Generieren von AQL-Abfragen für ThemisDB
- Analysieren von Multi-Model-Daten (Graph, Vector, Geo, Relational)
- Prozess- und Meilenstein-Analysen
- Verknüpfung von Daten aus verschiedenen Collections

### AQL-Syntax Grundlagen:
- FOR variable IN collection - Iteration über Collection
- FILTER condition - Filterbedingung
- LET variable = expression - Variablenzuweisung
- RETURN expression - Ergebnis zurückgeben
- SORT expression ASC|DESC - Sortierung
- LIMIT offset, count - Ergebnisbegrenzung
- COLLECT variable = expression - Gruppierung

### Verfügbare Funktionskategorien:
- String: LENGTH, CONCAT, UPPER, LOWER, CONTAINS, REGEX_TEST
- Math: ABS, CEIL, FLOOR, SQRT, SUM, AVG, MIN, MAX
- Array: FIRST, LAST, FLATTEN, UNIQUE, SORTED, UNION
- Date: NOW, TODAY, DATE_ADD, DATE_DIFF, WORKDAYS
- Geo: ST_POINT, GEO_DISTANCE, ST_CONTAINS, ST_WITHIN
- Vector: COSINE_SIMILARITY, EUCLIDEAN_DISTANCE, SIMILARITY
- Graph: SHORTEST_PATH, GRAPH_NEIGHBORS, PAGERANK
- Process: MILESTONE_STATUS, PROCESS_CONFORMANCE, SLA_CHECK

### Sicherheitsregeln:
- Generiere NUR lesende Abfragen (keine INSERT, UPDATE, DELETE)
- Verwende LIMIT zur Ergebnisbegrenzung (max 1000)
- Validiere Benutzereingaben mit IS_* Funktionen
- Maskiere sensible Daten mit MASK_* Funktionen
```

---

## Tool-Definitionen

### Tool: execute_aql

Führt eine AQL-Abfrage gegen ThemisDB aus.

```json
{
  "name": "execute_aql",
  "description": "Führt eine AQL-Abfrage aus und gibt die Ergebnisse zurück",
  "parameters": {
    "type": "object",
    "properties": {
      "query": {
        "type": "string",
        "description": "Die AQL-Abfrage"
      },
      "bind_vars": {
        "type": "object",
        "description": "Bind-Variablen für parameterisierte Abfragen"
      }
    },
    "required": ["query"]
  }
}
```

### Tool: get_collections

Listet alle verfügbaren Collections.

```json
{
  "name": "get_collections",
  "description": "Gibt eine Liste aller Collections in der Datenbank zurück",
  "parameters": {
    "type": "object",
    "properties": {}
  }
}
```

### Tool: get_schema

Ermittelt das Schema einer Collection.

```json
{
  "name": "get_schema",
  "description": "Gibt das Schema einer Collection zurück (Feldnamen und Typen)",
  "parameters": {
    "type": "object",
    "properties": {
      "collection": {
        "type": "string",
        "description": "Name der Collection"
      }
    },
    "required": ["collection"]
  }
}
```

### Tool: explain_query

Erklärt den Ausführungsplan einer Abfrage.

```json
{
  "name": "explain_query",
  "description": "Analysiert eine AQL-Abfrage und gibt den Ausführungsplan zurück",
  "parameters": {
    "type": "object",
    "properties": {
      "query": {
        "type": "string",
        "description": "Die zu analysierende AQL-Abfrage"
      }
    },
    "required": ["query"]
  }
}
```

---

## Agent-Workflow

### Schritt 1: Analyse der Benutzeranfrage

Identifiziere:
- Gesuchte Informationen
- Relevante Entitäten (Kunden, Produkte, Prozesse, etc.)
- Beziehungen zwischen Entitäten
- Zeitliche Einschränkungen
- Räumliche Einschränkungen
- Aggregationen (Summe, Durchschnitt, Anzahl)

### Schritt 2: Schema-Erkundung

```
Bevor ich eine Abfrage generiere, muss ich das Datenbankschema verstehen.

1. Welche Collections sind relevant?
2. Welche Felder enthalten diese Collections?
3. Wie sind die Collections verknüpft?
```

Beispiel:
```aql
-- Collections auflisten
RETURN COLLECTIONS()

-- Schema einer Collection erkunden (erste 5 Dokumente)
FOR doc IN customers LIMIT 5 RETURN ATTRIBUTES(doc)
```

### Schritt 3: Query-Generierung

Wähle die passende Abfragestrategie:

| Anfrage-Typ | AQL-Pattern |
|-------------|-------------|
| Einfache Suche | `FOR x IN coll FILTER ... RETURN x` |
| Aggregation | `FOR x IN coll COLLECT ... RETURN` |
| Graph-Traversierung | `FOR v, e IN 1..3 OUTBOUND start GRAPH 'g'` |
| Geo-Suche | `FOR x IN coll FILTER GEO_DISTANCE(...) < r` |
| Vektor-Suche | `FOR x IN coll SORT SIMILARITY(x._embedding, @query_vec, 10)` |
| Join | `FOR a IN coll1 FOR b IN coll2 FILTER a.id == b.ref` |

### Schritt 4: Ausführung und Validierung

```
Ich führe die Abfrage aus und prüfe:
- Sind Ergebnisse vorhanden?
- Entsprechen die Ergebnisse der Anfrage?
- Sind die Datentypen korrekt?
- Gibt es Fehler oder Warnungen?
```

### Schritt 5: Ergebnis-Interpretation

```
Ich interpretiere die Ergebnisse für den Benutzer:
- Zusammenfassung der wichtigsten Erkenntnisse
- Aufzeigen von Mustern oder Anomalien
- Beantwortung der ursprünglichen Frage
- Vorschläge für Folgefragen
```

---

## Query-Generierung

### Einfache Suche

**Benutzer**: "Zeige alle Kunden aus München"

```aql
FOR customer IN customers
  FILTER customer.city == "München"
  RETURN {
    name: customer.name,
    email: MASK_EMAIL(customer.email),
    phone: customer.phone
  }
```

### Aggregation

**Benutzer**: "Wie viele Bestellungen pro Monat?"

```aql
FOR order IN orders
  COLLECT month = DATE_FORMAT(order.created_at, "%Y-%m")
  WITH COUNT INTO count
  SORT month
  RETURN { month, count }
```

### Graph-Traversierung

**Benutzer**: "Wer sind die Kollegen von Max Mustermann?"

```aql
FOR person IN persons
  FILTER person.name == "Max Mustermann"
  FOR colleague, edge IN 1..2 ANY person works_with
    RETURN DISTINCT {
      name: colleague.name,
      department: colleague.department,
      relationship: edge.type
    }
```

### Geo-Suche

**Benutzer**: "Finde Restaurants im Umkreis von 2km"

```aql
LET userLocation = ST_POINT(11.5820, 48.1351)  -- München
FOR restaurant IN restaurants
  FILTER GEO_DISTANCE(restaurant._geometry, userLocation) < 2000
  SORT GEO_DISTANCE(restaurant._geometry, userLocation)
  RETURN {
    name: restaurant.name,
    distance: GEO_DISTANCE(restaurant._geometry, userLocation),
    cuisine: restaurant.cuisine
  }
```

### Vektor-Suche (Semantische Suche)

**Benutzer**: "Finde ähnliche Produkte zu 'Laptop mit langer Akkulaufzeit'"

```aql
LET query_embedding = @query_embedding  -- Vom LLM generiert
FOR product IN products
  LET similarity = COSINE_SIMILARITY(product._embedding, query_embedding)
  FILTER similarity > 0.7
  SORT similarity DESC
  LIMIT 10
  RETURN {
    name: product.name,
    description: product.description,
    similarity: similarity
  }
```

### Prozess-Analyse

**Benutzer**: "Welche Anträge sind überfällig?"

```aql
FOR instance IN _milestone_instances
  FILTER instance.status == "pending"
  FILTER instance.due_date < NOW()
  FOR milestone IN _milestones
    FILTER milestone.id == instance.milestone_id
    FOR vorgang IN vorgaenge
      FILTER vorgang._key == instance.vorgang_id
      RETURN {
        vorgang_id: vorgang._key,
        vorgang_typ: vorgang.type,
        meilenstein: milestone.name,
        faellig_seit: DATE_FORMAT(instance.due_date, "%Y-%m-%d %H:%M"),
        verzoegerung_stunden: (NOW() - instance.due_date) / 3600000,
        ist_kritisch: milestone.is_critical
      }
```

---

## Multi-Model Beispiele

### Kombinierte Abfrage: Graph + Geo + Vector

**Benutzer**: "Finde Experten für KI in meiner Nähe, die mit meinen Kollegen vernetzt sind"

```aql
LET myLocation = ST_POINT(@longitude, @latitude)
LET myConnections = (
  FOR person IN 1..2 OUTBOUND @userId knows
    RETURN person._key
)
LET aiEmbedding = @ai_embedding  -- Embedding für "Künstliche Intelligenz"

FOR expert IN experts
  -- Geo: Im Umkreis von 50km
  FILTER GEO_DISTANCE(expert._geometry, myLocation) < 50000
  
  -- Vector: Expertise in KI (Ähnlichkeit > 0.8)
  LET expertiseSimilarity = COSINE_SIMILARITY(expert.skills_embedding, aiEmbedding)
  FILTER expertiseSimilarity > 0.8
  
  -- Graph: Verbunden mit meinen Kontakten
  LET mutualConnections = (
    FOR connection IN 1..1 OUTBOUND expert._id knows
      FILTER connection._key IN myConnections
      RETURN connection.name
  )
  FILTER LENGTH(mutualConnections) > 0
  
  RETURN {
    name: expert.name,
    expertise_match: expertiseSimilarity,
    distance_km: GEO_DISTANCE(expert._geometry, myLocation) / 1000,
    mutual_connections: mutualConnections,
    contact: MASK_EMAIL(expert.email)
  }
```

### Prozess + Meilensteine + Prognose

**Benutzer**: "Zeige mir den Status aller Bauanträge mit Prognose für Genehmigung"

```aql
FOR antrag IN antraege
  FILTER antrag.type == "Bauantrag"
  FILTER antrag.status != "abgeschlossen"
  
  -- Meilenstein-Status
  LET milestones = (
    FOR mi IN _milestone_instances
      FILTER mi.vorgang_id == antrag._key
      FOR m IN _milestones
        FILTER m.id == mi.milestone_id
        RETURN {
          name: m.name,
          status: mi.status,
          due: mi.due_date,
          reached: mi.reached_at
        }
  )
  
  -- Aktueller Meilenstein
  LET current = FIRST(
    FOR ms IN milestones
      FILTER ms.status == "pending"
      SORT ms.due
      LIMIT 1
      RETURN ms
  )
  
  -- Bisherige Durchlaufzeit
  LET completed = (
    FOR ms IN milestones
      FILTER ms.status == "reached"
      RETURN ms
  )
  
  -- Prognose basierend auf historischen Daten
  LET similar_completed = (
    FOR a IN antraege
      FILTER a.type == "Bauantrag"
      FILTER a.status == "abgeschlossen"
      FILTER a.complexity == antrag.complexity
      RETURN DATE_DIFF(a.created_at, a.completed_at, "days")
  )
  LET avg_duration = AVG(similar_completed)
  LET days_elapsed = DATE_DIFF(antrag.created_at, NOW(), "days")
  LET estimated_remaining = MAX([0, avg_duration - days_elapsed])
  LET estimated_completion = DATE_ADD(NOW(), estimated_remaining, "days")
  
  RETURN {
    antrag_nr: antrag._key,
    antragsteller: antrag.antragsteller,
    eingereicht: DATE_FORMAT(antrag.created_at, "%d.%m.%Y"),
    aktueller_meilenstein: current.name,
    meilenstein_faellig: DATE_FORMAT(current.due, "%d.%m.%Y"),
    fortschritt_prozent: ROUND(100 * LENGTH(completed) / LENGTH(milestones)),
    prognose_fertigstellung: DATE_FORMAT(estimated_completion, "%d.%m.%Y"),
    konfidenz: LENGTH(similar_completed) > 10 ? "hoch" : "niedrig"
  }
```

---

## Sicherheitsrichtlinien

### Read-Only Modus

Der Agent sollte **ausschließlich lesende Abfragen** generieren:

```
ERLAUBT:
- FOR ... RETURN
- FOR ... FILTER ... RETURN
- FOR ... COLLECT ... RETURN

VERBOTEN:
- INSERT
- UPDATE
- REPLACE
- REMOVE
- UPSERT
```

### Parameterisierung

Verwende **immer** Bind-Variablen für Benutzereingaben:

```aql
-- RICHTIG
FOR customer IN customers
  FILTER customer.name == @name
  RETURN customer

-- FALSCH (Injection-Risiko!)
FOR customer IN customers
  FILTER customer.name == "{{user_input}}"
  RETURN customer
```

### Datenmaskierung

Maskiere sensible Daten in Ergebnissen:

```aql
FOR customer IN customers
  RETURN {
    name: customer.name,
    email: MASK_EMAIL(customer.email),      -- max.m***@example.com
    phone: MASK(customer.phone, 0, 4),       -- ****1234
    iban: MASK_IBAN(customer.iban),          -- DE**...****5678
    credit_card: MASK_CREDIT_CARD(customer.cc) -- ****-****-****-1234
  }
```

### Ergebnisbegrenzung

Begrenze immer die Ergebnismenge:

```aql
FOR doc IN large_collection
  LIMIT 100  -- Maximal 100 Ergebnisse
  RETURN doc
```

### Validierung

Validiere Eingaben vor der Verwendung:

```aql
LET email_valid = IS_EMAIL(@email)
LET url_valid = IS_URL(@url)

FILTER email_valid AND url_valid

FOR ...
```

---

## Chain-of-Thought Prompting

### Beispiel: Komplexe Analyse

**Benutzer**: "Welche Kunden haben in den letzten 6 Monaten überdurchschnittlich viel bestellt und sind in der Nähe unserer Lagerhäuser?"

**Agent-Reasoning**:

```
Schritt 1: Analyse der Anfrage
- Zeitraum: Letzte 6 Monate
- Metrik: Bestellvolumen > Durchschnitt
- Räumliche Bedingung: Nähe zu Lagerhäusern
- Benötigte Collections: customers, orders, warehouses

Schritt 2: Durchschnitt berechnen
- Ich muss zuerst den durchschnittlichen Bestellwert berechnen
- Aggregation über alle Bestellungen der letzten 6 Monate

Schritt 3: Lagerhäuser ermitteln
- Standorte der Lagerhäuser für Geo-Abfrage
- Definiere "Nähe" als z.B. 50km Radius

Schritt 4: Query konstruieren
```

```aql
-- Durchschnitt berechnen
LET sixMonthsAgo = DATE_SUBTRACT(NOW(), 6, "months")

LET avgOrderValue = (
  FOR order IN orders
    FILTER order.created_at >= sixMonthsAgo
    COLLECT AGGREGATE total = SUM(order.total)
    RETURN total / LENGTH(FOR o IN orders FILTER o.created_at >= sixMonthsAgo RETURN 1)
)[0]

-- Lagerhäuser
LET warehouseLocations = (
  FOR warehouse IN warehouses
    RETURN warehouse._geometry
)

-- Kunden mit überdurchschnittlichen Bestellungen in Lagernähe
FOR customer IN customers
  -- Bestellungen der letzten 6 Monate
  LET customerOrders = (
    FOR order IN orders
      FILTER order.customer_id == customer._key
      FILTER order.created_at >= sixMonthsAgo
      RETURN order.total
  )
  LET customerTotal = SUM(customerOrders)
  
  -- Nur überdurchschnittliche Kunden
  FILTER customerTotal > avgOrderValue
  
  -- Nähe zu einem Lagerhaus (< 50km)
  LET nearestWarehouse = (
    FOR wh IN warehouses
      LET dist = GEO_DISTANCE(customer._geometry, wh._geometry)
      SORT dist
      LIMIT 1
      RETURN { name: wh.name, distance: dist }
  )[0]
  FILTER nearestWarehouse.distance < 50000
  
  SORT customerTotal DESC
  LIMIT 50
  
  RETURN {
    kunde: customer.name,
    stadt: customer.city,
    bestellwert_gesamt: customerTotal,
    vergleich_durchschnitt: ROUND(customerTotal / avgOrderValue * 100) + "%",
    naechstes_lager: nearestWarehouse.name,
    entfernung_km: ROUND(nearestWarehouse.distance / 1000)
  }
```

---

## VCC-Veritas Integration

### Agentenbasierter Scope

Der VCC-Veritas Agent-Scope für ThemisDB AQL:

```yaml
agent:
  name: "ThemisDB-DataResearch-Agent"
  version: "1.0"
  capabilities:
    - data_query
    - data_analysis
    - process_monitoring
    - geo_analysis
    - semantic_search

tools:
  - name: execute_aql
    scope: read_only
    max_results: 1000
    timeout_ms: 30000
    
  - name: get_collections
    scope: metadata
    
  - name: get_schema
    scope: metadata
    
  - name: explain_query
    scope: analysis

security:
  allowed_operations:
    - SELECT
    - AGGREGATE
    - JOIN
  forbidden_operations:
    - INSERT
    - UPDATE
    - DELETE
    - CREATE
    - DROP
  data_masking:
    - email
    - phone
    - iban
    - credit_card
    - ssn
  rate_limit:
    queries_per_minute: 60
    max_concurrent: 5

context:
  database: "production"
  collections_visible: "all"
  include_system_collections: false
```

### Tool-Aufruf Beispiel

```json
{
  "tool": "execute_aql",
  "parameters": {
    "query": "FOR c IN customers FILTER c.city == @city LIMIT @limit RETURN c",
    "bind_vars": {
      "city": "München",
      "limit": 10
    }
  }
}
```

### Konversations-Beispiel

```
Benutzer: Wie viele offene Anträge haben wir?

Agent (denkt):
1. Der Benutzer fragt nach der Anzahl offener Anträge
2. Ich muss die Collection "antraege" oder "vorgaenge" prüfen
3. "Offen" bedeutet wahrscheinlich status != "abgeschlossen"

Agent (Tool-Aufruf):
{
  "tool": "execute_aql",
  "parameters": {
    "query": "FOR a IN vorgaenge FILTER a.status != 'abgeschlossen' COLLECT WITH COUNT INTO total RETURN total"
  }
}

Ergebnis: [42]

Agent (Antwort):
Es gibt derzeit **42 offene Anträge** im System.

Möchten Sie mehr Details sehen, z.B.:
- Aufschlüsselung nach Antragstyp?
- Überfällige Anträge?
- Bearbeitungsfortschritt?
```

---

## Fehlerbehandlung

### Häufige Fehler und Lösungen

| Fehler | Ursache | Lösung |
|--------|---------|--------|
| `Collection not found` | Falscher Collection-Name | `get_collections` aufrufen |
| `Attribute not found` | Feld existiert nicht | `get_schema` aufrufen |
| `Syntax error` | Ungültige AQL-Syntax | Query vereinfachen |
| `Timeout` | Zu komplexe Abfrage | LIMIT hinzufügen, Index nutzen |
| `Type mismatch` | Falscher Datentyp im Vergleich | Typ-Konvertierung nutzen |

### Fehlerbehandlung im Agent

```
Wenn die Abfrage fehlschlägt:

1. Analysiere die Fehlermeldung
2. Identifiziere die Ursache
3. Korrigiere die Abfrage
4. Versuche erneut (max. 3 Versuche)
5. Bei anhaltendem Fehler: Informiere den Benutzer

Beispiel:
- Fehler: "Collection 'kunden' not found"
- Aktion: get_collections aufrufen
- Ergebnis: Collection heißt "customers"
- Korrektur: "kunden" → "customers"
```

---

## Best Practices

### 1. Schema-First

Immer zuerst das Schema erkunden:

```aql
-- Felder einer Collection
FOR doc IN customers LIMIT 1 RETURN ATTRIBUTES(doc)

-- Beispieldaten
FOR doc IN customers LIMIT 3 RETURN doc
```

### 2. Inkrementelle Abfragen

Komplexe Abfragen schrittweise aufbauen:

```
1. Einfache Abfrage testen
2. Filter hinzufügen
3. Joins hinzufügen
4. Aggregationen hinzufügen
5. Sortierung und Limit
```

### 3. EXPLAIN nutzen

Bei langsamen Abfragen den Ausführungsplan prüfen:

```aql
EXPLAIN FOR doc IN large_collection FILTER doc.status == "active" RETURN doc
```

### 4. Index-Hinweise

Für häufige Abfragen passende Indizes empfehlen:

```
"Die Abfrage wäre schneller mit einem Index auf 'customers.city'. 
Empfehlung: CREATE INDEX idx_customers_city ON customers(city)"
```

### 5. Ergebnisse zusammenfassen

Große Ergebnismengen für den Benutzer aufbereiten:

```
Statt 1000 Zeilen anzuzeigen:
- Top 10 nach Relevanz
- Zusammenfassung mit Aggregationen
- Visualisierungsempfehlung
```

---

## Changelog

| Version | Datum | Änderungen |
|---------|-------|------------|
| 1.0 | 2024-01 | Initiale Version |

---

*Dieses Dokument ist Teil der ThemisDB Dokumentation und wird vom VCC-Veritas Agenten-Framework verwendet.*
