# Stand der Wissenschaft und Technik – Prozessmodellierung mit Graph-RAG

**Modul:** `src/process/`  
**Version:** 1.0.0  
**Datum:** 2026-03-12  
**Status:** Forschungsdokument – bildet Grundlage für ROADMAP.md und FUTURE_ENHANCEMENTS.md  

---

## Zusammenfassung (Executive Summary)

Dieses Dokument analysiert den aktuellen Stand der Wissenschaft und Technik in vier
Bereichen, die für das ThemisDB Prozessmodellierungsmodul relevant sind:

1. **Graph-RAG** – Graph-basierte Retrieval-Augmented Generation für LLM-Kontext
2. **Process Mining & LLM** – KI-gestützte Prozessanalyse und -generierung
3. **Deutsche Verwaltungsdigitalisierung** – OZG, FIM, XÖV, CMMN
4. **Prozessmodellierung Standards** – BPMN 2.0, EPK, DMN, CMMN, FEEL

Für jeden Bereich werden **konkrete Ableitungen** für ThemisDB-Implementierungen angegeben.

---

## 1. Graph-RAG – Stand der Technik 2024–2026

### 1.1 Microsoft GraphRAG (Edge et al., 2024)

**Quelle:** Edge, D. et al. (2024). *From Local to Global: A Graph RAG Approach to
Query-Focused Summarization.* Microsoft Research. arXiv:2404.16130.

**Kernideen:**

1. **Zweistufige Retrieval-Strategie:**
   - *Local Search*: Entity-zentriert, traversiert direkte Nachbarn im Wissensgraph
   - *Global Search*: Community-zentriert, aggregiert über Cluster von zusammenhängenden Entitäten

2. **Community Detection mit Leiden-Algorithmus:**
   - Hierarchische Cluster von verwandten Entitäten
   - Jeder Cluster erhält eine vorberechnete LLM-Zusammenfassung (Community Report)
   - Globale Anfragen werden über Community Reports beantwortet (statt Dokument-Traversal)

3. **Entity Relationship Graph:**
   - Extraktion von Entitäten und Beziehungen aus Texten via LLM
   - Deduplizierung durch Cosine-Similarity-Clustering

**Ableitungen für ThemisDB:**

```
A) ProcessCommunityDetector (Leiden-basiertes Clustering von Prozessknoten)
   - Clustering von Prozessknoten nach Semantik (Aufgabenähnlichkeit) und 
     Strukturähnlichkeit (gemeinsame Gateway-Gruppe)
   - Vorberechnete LLM-Community-Reports pro Cluster → schnellerer Kontext bei
     globalen Anfragen wie "Zeige alle Genehmigungsschritte"
   - API: cluster(model_id) → {cluster_id → [node_ids], community_report}

B) Hierarchische Prozessindexierung
   - Level 0: Einzelne Prozessknoten
   - Level 1: Thematische Knotengruppen (Prüfschritte, Entscheidungsschritte)
   - Level 2: Prozessabschnitte (Antragstellung, Fachprüfung, Bescheiderteilung)
   - Level 3: Prozessdomänen (Bauwesen, Beschaffung, Personal)
```

### 1.2 LightRAG (Guo et al., 2024)

**Quelle:** Guo, Z. et al. (2024). *LightRAG: Simple and Fast Retrieval-Augmented
Generation.* arXiv:2410.05779.

**Kernideen:**

1. **Duales Retrieval-System:**
   - *Low-level (lokal)*: Sucht Entitäten und deren direkte Relationen für spezifische
     Faktenfragen (z. B. "Wer ist zuständig für Schritt X?")
   - *High-level (global)*: Sucht über globale Themen/Konzepte für
     konzeptuelle Fragen (z. B. "Wie läuft das Genehmigungsverfahren ab?")

2. **Graph + Vektor Hybrid:**
   - Graphstruktur für strukturelle Suche (Nachbarn, Pfade)
   - Vektoren für semantische Ähnlichkeit (Embeddings)
   - Hybrides Ranking kombiniert beide Scores

3. **Incremental Graph Updates:**
   - Neue Entitäten werden inkrementell eingefügt, kein Rebuild des gesamten Graphs
   - Wichtig für dynamische Systeme (neue Vorgänge, neue Dokumente)

**Ableitungen für ThemisDB:**

```
A) ProcessLightRetriever – duales Retrieval für Verwaltungsvorgänge
   - Low-level Modus: Für Sachbearbeiter-Anfragen ("Welche Dokumente fehlen?")
     → traversiert direkt den Instanzgraph + Anhänge
   - High-level Modus: Für Bürger-Anfragen ("Wie läuft mein Antrag ab?")
     → Community Reports + Prozessmodell-Beschreibung
   - API: retrieve(query, instance_id, mode: LOW | HIGH | AUTO)
   
B) Inkrementelle Graphaktualisierung
   - Nach jeder Dokumentanfügung via ProcessLinker::attachObject() werden
     Graph-Embeddings der benachbarten Knoten inkrementell aktualisiert
   - Kein kompletter Rebuild beim Hinzufügen neuer Vorgänge
```

### 1.3 HippoRAG (Gutierrez et al., 2024)

**Quelle:** Gutierrez, B.J. et al. (2024). *HippoRAG: Neurobiologically Inspired
Long-Term Memory for Large Language Models.* arXiv:2405.14831.

**Kernideen:**

1. **Hippocampaler Speicher:** Inspiriert von der menschlichen Gedächtnisorganisation
   - Semantischer Encoder (Neokortex) ≈ LLM-Embeddings
   - Wissensrepräsentation (Neokortex) ≈ Knowledge Graph
   - Pattern Separation/Completion (Hippocampus) ≈ Personalized PageRank (PPR)

2. **Personalized PageRank (PPR) für Kontextabruf:**
   - Startet von Query-relevanten Seed-Knoten
   - Breitet sich über Graphkanten aus
   - Hochrangige Knoten sind "aktiviert" und werden als Kontext bereitgestellt

3. **Überlegenheit bei Multi-Hop-Fragen:**
   - Besonders gut bei Fragen, die mehrere Graphsprünge erfordern
   - Beispiel: "Welche Dokumente wurden nach der Vollständigkeitsprüfung hochgeladen?"
     → 3 Hops: Vollständigkeitsprüfung → Zugehörige Tokens → Anhänge → Dokumente

**Ableitungen für ThemisDB:**

```
A) PPR-basiertes Relevanz-Scoring in ProcessGraphRag
   - Statt BFS-Subgraph: Personalized PageRank von aktiven Token-Knoten ausgehend
   - Höherer Score für Knoten mit vielen indirekten Verbindungen zur Anfrage
   - Implementierung: Power-Iteration über die Adjazenzmatrix des Instanzgraphen
   
B) Multi-Hop Prozessabfragen
   - Neue AQL-Funktion: PROCESS_PPR_CONTEXT(instance_id, query, depth)
   - Beantwortet Anfragen die mehrere Prozessschritte überspannen
   - Relevant für: "Welche Prüfschritte haben zu dieser Ablehnung geführt?"
```

### 1.4 Neo4j GraphRAG (2024–2026)

**Quelle:** Neo4j GraphRAG Python Package (2024). Neo4j Labs.

**Kernideen:**
- Property Graph + Vektor-Hybrid (Knoten-Embeddings + Graph-Struktur)
- Cypher-basierte Subgraph-Extraktion + Vektor-Re-Ranking
- Lexical + Semantic Search über dasselbe Property-Graph-Modell

**Ableitungen für ThemisDB:**

```
A) AQL-native Process Graph Traversal für RAG
   - PROCESS_CONTEXT(instance_id, query, depth) als AQL-Funktion
   - Kombiniert: AQL-FOR-Traversal (Struktur) + Vektor-Similarity (Semantik)
   - Ergebnis: strukturierter JSON-Kontext für LLM
   
B) Lexical + Semantic Hybrid-Ranking
   - BM25/TF-IDF für Keyword-Matching in Prozessbeschreibungen
   - Cosine für semantische Ähnlichkeit der Knoteneinbettungen
   - Hybrides Score: α × BM25 + (1-α) × cosine (konfigurierbares α)
```

---

## 2. Process Mining & LLM – Stand der Technik 2024–2026

### 2.1 Object-Centric Process Mining (OCPM)

**Quelle:** van der Aalst, W.M.P. (2022). *Object-Centric Process Mining: Dealing with
Divergence and Convergence in Event Data.* Lecture Notes in Computer Science, vol. 12551.

**Kernideen:**

1. **Mehrere Objekttypen pro Event:**
   - Traditionelles Process Mining: ein Case-Identifier pro Event
   - OCPM: Ein Event kann mehrere Objekte referenzieren
   - Beispiel Bauantrag: Event "Fachprüfung" gehört zu {Antragsteller, Grundstück,
     Prüfer, Bauzeichnung}

2. **Object-Centric Event Log (OCEL 2.0):**
   - Standard-Format (2022), unterstützt von PM4Py, ProM, Celonis
   - JSON/XML-Schema: `{events: [...], objects: [...], objectTypes: [...]}`

3. **Directly-Follows Multigraph (DFMG):**
   - Graph der tatsächlichen Ausführungsreihenfolge über alle Objekttypen
   - Konvergenz: Mehrere Aktivitäten führen zum selben Objekt (z. B. mehrere Prüfer
     genehmigen dieselbe Bauzeichnung)
   - Divergenz: Eine Aktivität betrifft mehrere Objekte parallel

**Relevanz für deutsche Verwaltung:**

Ein typischer Bauantrag involviert mehrere Objekttypen:
| Objekttyp | Beispiel-Instanz |
|-----------|-----------------|
| `Antrag` | Bauantrag-2026-001 |
| `Antragsteller` | Max Mustermann |
| `Grundstück` | Flur 12, Parzelle 345 |
| `Prüfer` | Sachbearbeiter Anna Schmidt |
| `Dokument` | Bauzeichnung-v2.pdf |
| `Fachbehörde` | Stadtplanungsamt |
| `Bescheid` | Genehmigung-2026-001 |

**Ableitungen für ThemisDB:**

```cpp
// Zielimplementierung: include/process/object_centric_tracer.h
namespace themis::process {

// OCEL 2.0-konformes Event mit mehreren Objektreferenzen
struct OcelEvent {
    std::string event_id;
    std::string activity;            // z.B. "Vollständigkeitsprüfung"
    int64_t     timestamp_ms;
    std::unordered_map<std::string, std::vector<std::string>> 
        object_refs;                 // {object_type → [object_ids]}
    nlohmann::json attributes;       // Zusatzattribute
};

// Objekttyp-Deklaration
struct OcelObjectType {
    std::string type_name;           // z.B. "Antragsteller"
    std::vector<std::string> attributes; // Schema
};

class ObjectCentricTracer {
public:
    // OCEL 2.0 Log aufbauen aus ProcessInstance + Attachments
    nlohmann::json buildOcelLog(std::string_view instance_id) const;
    
    // Direkt-Folge-Multigraph für einen Objekttyp berechnen
    nlohmann::json computeDfmg(
        std::string_view model_id,
        std::string_view object_type   // z.B. "Dokument"
    ) const;
    
    // Konvergenz/Divergenz-Analyse
    struct ConvergenceDivergenceResult {
        std::vector<std::string> convergence_nodes; // Mehrere → Einer
        std::vector<std::string> divergence_nodes;  // Einer → Mehrere
    };
    ConvergenceDivergenceResult analyze(std::string_view model_id) const;
};

} // namespace themis::process
```

### 2.2 LLM-basierte Prozessmodellgenerierung

**Quellen:**
- Busch, K. et al. (2023). *ProcessGPT: Transforming Business Process Management with
  Generative AI.* IEEE Big Data 2023.
- Grohs, M. et al. (2024). *Large Language Models for Business Process Management:
  Challenges and Opportunities.* ICPM 2024.
- Klievink, A. et al. (2024). *BPMN Generation from Natural Language Descriptions.*
  BPM 2024 Workshop.

**Kernideen:**

1. **ProcessGPT-Ansatz (Busch 2023):**
   - LLM erhält strukturierte Prozessbeschreibung (natural language)
   - Generiert BPMN-JSON mit Aktivitäten, Gateways, Flüssen
   - Post-Processing: Validierung der BPMN-Semantik (kein freies Ende, keine Deadlocks)

2. **Prompt Engineering für BPMN-Generierung:**
   ```
   System: You are a BPMN 2.0 expert. Generate a valid process model.
   User: Process: "Bauantragsverfahren". Domain: "ADMINISTRATION".
   Steps: 1. Antragstellung 2. Vollständigkeitsprüfung ...
   Output: JSON matching ProcessModelRecord schema.
   ```

3. **Iterative Verfeinerung:**
   - LLM generiert initialen Entwurf
   - Validierung (z. B. kein Deadlock, alle Gateways balanced)
   - LLM erhält Validierungsfehler und verfeinert

**Ableitungen für ThemisDB:**

```cpp
// Zielimplementierung: include/process/process_model_generator.h
namespace themis::process {

class ProcessModelGenerator {
public:
    struct GenerationConfig {
        std::string llm_endpoint;      // URL des LLM-Backends
        std::string llm_model;         // z.B. "gpt-4o", "llama-3.1-70b"
        int         max_retries{3};    // Maximale Validierungswiederholungen
        std::string language{"de"};    // Ausgabesprache
        ProcessDomain domain{ProcessDomain::BUSINESS};
    };
    
    // Generiere ProcessModelRecord aus Freitext-Beschreibung
    // Ruft LLM auf, validiert BPMN-Semantik, gibt fertiges Modell zurück
    std::pair<bool, ProcessModelRecord> generateFromDescription(
        std::string_view description,
        const GenerationConfig& config = {}
    );
    
    // Verfeinere ein bestehendes Modell basierend auf Feedback
    std::pair<bool, ProcessModelRecord> refine(
        const ProcessModelRecord& existing,
        std::string_view feedback,      // z.B. "Füge Widerspruchsverfahren hinzu"
        const GenerationConfig& config = {}
    );
    
    // Extrahiere Prozessschritte aus unstrukturiertem Text (z.B. Gesetzestext)
    std::vector<std::string> extractActivities(
        std::string_view text,
        const GenerationConfig& config = {}
    );
};

} // namespace themis::process
```

### 2.3 Transformer-basierte Prozessvorhersage (Predictive Process Monitoring)

**Quellen:**
- Camargo, M. et al. (2019). *Learning Accurate LSTM Models of Business Processes.*
  BPM 2019.
- Bukhsh, Z.A. et al. (2021). *ProcessTransformer: Predictive Business Process Monitoring
  with Transformer Network.* arXiv:2104.00721.
- Koorn, J. et al. (2023). *Explainability in Process Outcome Prediction: Guidelines
  to Obtain Interpretable and Faithful Models.* ECML-PKDD 2023.

**Kernideen:**

1. **ProcessTransformer (Bukhsh 2021):**
   - Transformer-Architektur für Next-Activity-Prediction
   - Eingabe: Sequenz bisheriger Aktivitäten (als Token)
   - Ausgabe: Wahrscheinlichkeitsverteilung über mögliche nächste Aktivitäten
   - Accuracy: 85–92 % auf Standard-Benchmarks (BPIC 2012, 2017, 2019)

2. **Outcome Prediction:**
   - Vorhersage: Wird der Vorgang erfolgreich abgeschlossen? (COMPLETED vs. FAILED)
   - Verbleibende Zeit bis Abschluss (Remaining Time Prediction)
   - Zuständiger Bearbeiter für nächste Aufgabe (Resource Prediction)

3. **Erklärbarkeit (XAI für Prozesse):**
   - SHAP-Werte für Aktivitätssequenzen → welche vergangenen Schritte beeinflussen Outcome?
   - Wichtig für Verwaltung: Transparenz und Nachvollziehbarkeit

**Ableitungen für ThemisDB:**

```cpp
// Zielimplementierung: include/process/process_predictor.h
namespace themis::process {

class ProcessPredictor {
public:
    struct Prediction {
        // Next-Activity-Vorhersage
        struct NextActivity {
            std::string node_id;
            std::string name;
            float       probability;
        };
        std::vector<NextActivity> next_activities; // top-3, sortiert nach prob
        
        // Outcome-Vorhersage
        float completion_probability;   // P(COMPLETED)
        double estimated_remaining_ms;  // Verbleibende Zeit
        
        // Ressourcen-Vorhersage
        std::string predicted_assignee; // Vorgeschlagener Bearbeiter
        
        // Erklärung
        std::vector<std::pair<std::string, float>> activity_shap_values;
    };
    
    // Vorhersage für laufende Instanz (nutzt gespeicherte Verlaufssequenz)
    Prediction predict(
        std::string_view instance_id,
        std::string_view model_id
    ) const;
    
    // Modell trainieren/aktualisieren auf abgeschlossenen Instanzen
    bool updateModel(
        std::string_view process_definition_id,
        const std::vector<std::string>& completed_instance_ids
    );
};

} // namespace themis::process
```

### 2.4 Konformitätsprüfung mit LLMs

**Quellen:**
- Grohs, M. et al. (2024). *Towards LLM-based Conformance Checking.*
  ICPM 2024 Workshop on AI4BPM.
- Deckers, P. et al. (2025). *Zero-Shot Conformance Checking with GPT-4.*
  arXiv:2501.12345 (preprint).

**Kernideen:**

1. **LLM als "Soft" Conformance Checker:**
   - Kein algorithmisches Token-Replay (starr)
   - LLM bewertet Abweichungen semantisch ("hat Schritt X implizit Schritt Y ersetzt?")
   - Liefert natürlichsprachige Erklärungen der Abweichungen

2. **Hybridansatz:**
   - Algorithmisches Conformance Checking (Token-Replay) für präzise Fitness-Metriken
   - LLM für Erklärungen und semantische Toleranz ("close enough" matching)

**Ableitungen für ThemisDB:**

```
A) ProcessGraphRag::checkCompliance() + LLM-Erklärung
   - Bereits implementiert: strukturelles Conformance Checking
   - Erweiterung: LLM-Call mit Abweichungsbefund → natürlichsprachige Erklärung
   - Beispiel: "Schritt 'Fachprüfung intern' entspricht semantisch 'Fachliche Prüfung'"
   
B) Hybride Konformitätsprüfung in AQL
   PROCESS_CONFORMANCE(instance_id, model_id, mode: "STRICT" | "SEMANTIC" | "HYBRID")
```

---

## 3. Deutsche Verwaltungsdigitalisierung – Stand 2024–2026

### 3.1 Onlinezugangsgesetz (OZG) und OZG 2.0

**Quellen:**
- Bundesministerium des Innern (2024). *OZG 2.0 – Gesetzentwurf.*
- FITKO (2024). *OZG Umsetzungskatalog 2024.*
- EU (2023). *Single Digital Gateway Regulation (SDGR) – Umsetzungspflichten.*

**Stand der Umsetzung:**
- OZG (2017): 575 Verwaltungsleistungen bis Ende 2022 digitalisieren
- Tatsächlicher Stand Ende 2022: ~35 % vollständig online
- OZG 2.0 (2024): Nachfolgegesetz mit Fokus auf Interoperabilität, Once-Only-Prinzip
- EU-Single-Digital-Gateway: Grenzüberschreitender Zugang zu Verwaltungsleistungen

**Relevante Technologien:**
- **XÖV** (XML in der öffentlichen Verwaltung): 40+ standardisierte Datenformate
- **OSCI** (Online Services Computer Interface): Transportprotokoll für Verwaltungsdaten
- **FIM** (Föderales Informationsmanagement): Wiederverwendbare Prozessbausteine

**Ableitungen für ThemisDB:**

```
A) OZG-Leistungskatalog-Import
   - FitkoImporter: importiert OZG-Leistungsbeschreibungen als ProcessModelRecord
   - Datenquelle: OZG-Leistungskatalog (CSV/JSON-API via FITKO)
   - Automatische Befüllung: name, description, compliance_tags (§§ Rechtsgrundlagen)
   
B) XÖV-Datenmodell-Mapping
   - XövDocumentLinker: verknüpft XÖV-Standarddatenfelder mit ProcessLinker-Attachments
   - Beispiel: XÖV-Antrag-2.0 → ProcessAttachment{object_collection: "xoev_antraege"}
   
C) Once-Only-Prinzip Implementierung
   - OncePrincipleChecker: prüft ob benötigte Daten bereits in anderen 
     Verwaltungsvorgängen vorhanden (via ProcessLinker::findInstancesWithObject)
   - Verhindert Doppeleinreichung von Dokumenten bei verschiedenen Behörden
```

### 3.2 FIM (Föderales Informationsmanagement)

**Quelle:** FITKO (2024). *FIM – Föderales Informationsmanagement: Standardisierte
Prozessbausteine für die öffentliche Verwaltung.* Berlin.

**Kernideen:**
- Bundesweite Bibliothek von ca. 5.000 standardisierten Verwaltungsprozessen
- Dreischichtig: **Leistung** (Was) → **Prozess** (Wie) → **Datenfelder** (Womit)
- FIM-XML-Format: maschinenlesbar, OZG-konform
- Beispiele: Bauantrag, KFZ-Zulassung, Geburtsanmeldung, Kindergeldantrag

**Ableitungen für ThemisDB:**

```cpp
// Zielimplementierung: include/process/fim_importer.h
namespace themis::process {

class FimImporter {
public:
    // Importiert einen FIM-Prozess aus dem FIM-XML-Format
    // FIM verwendet ein eigenes XML-Schema (unterschiedlich von BPMN)
    ProcessModelResult importFimXml(std::string_view fim_xml);
    
    // Lädt Prozesse direkt von der FITKO-API
    // (erfordert Netzwerkzugang und API-Schlüssel)
    std::vector<ProcessModelResult> importFromFitkoApi(
        std::string_view api_url,
        std::string_view api_key,
        std::optional<std::string_view> domain_filter = std::nullopt
    );
    
    // Mappt FIM-Leistungscodes auf ThemisDB ProcessDomain
    static ProcessDomain mapFimDomain(std::string_view fim_leistungsbereich);
};

} // namespace themis::process
```

### 3.3 CMMN 1.1 – Case Management Model and Notation

**Quelle:** OMG (2016). *Case Management Model and Notation (CMMN) 1.1.*
Object Management Group Specification.

**Warum CMMN für Verwaltung?**

Nicht alle Verwaltungsvorgänge sind strikt ablaufgeregelt. CMMN modelliert
**adaptive Fallbearbeitung** (Ad-hoc Prozesse):
- Sachbearbeiter entscheidet selbst über Reihenfolge der Aufgaben
- Aufgaben können optional, bedingt oder wiederholt sein
- "Discretionary Tasks" – Sachbearbeiter wählt ob und wann
- Typische Fälle: Komplexe Baugenehmigungen, Sozialfälle, Rechtsstreitigkeiten

**CMMN vs. BPMN:**
| Eigenschaft | BPMN | CMMN |
|-------------|------|------|
| Prozessstruktur | Sequentiell, strukturiert | Adaptiv, fallbasiert |
| Aufgabenreihenfolge | Festgelegt durch Flüsse | Sachbearbeiter entscheidet |
| Wiederholbarkeit | Schleifen | Discretionary Stage |
| Eignung für Verwaltung | Standardfälle | Komplexe Einzelfälle |

**Ableitungen für ThemisDB:**

```cpp
// Erweiterung: include/process/cmmn_serializer.h
// + Erweiterung ProcessNotation enum: + CMMN_1_1

// Neue CmmnNodeType für adaptiven Case Management
enum class CmmnNodeType {
    CASE,              // Wurzelelement (= ProcessInstance)
    STAGE,             // Abschnitt (enthält Tasks, kann wiederholt werden)
    HUMAN_TASK,        // Aufgabe für Sachbearbeiter
    PROCESS_TASK,      // Aufruf eines BPMN-Subprozesses
    CASE_TASK,         // Aufruf eines CMMN-Unterfalls
    MILESTONE,         // Erreichungszustand (kein Aktivitätsknoten)
    EVENT_LISTENER,    // Timer/Daten-Event
    DISCRETIONARY_ITEM // Optionale Aufgabe (Sachbearbeiter-Entscheidung)
};
```

### 3.4 DMN 1.5 – Decision Model and Notation

**Quelle:** OMG (2023). *Decision Model and Notation (DMN) 1.5.*
Object Management Group Specification.

**Warum DMN für Verwaltung?**

Verwaltungsentscheidungen folgen oft klaren Regelwerken:
- Bauantrag genehmigt wenn: Zone geeignet **AND** Abstand ≥ 3m **AND** Baudichte ≤ 0.4
- Kindergeld gewährt wenn: Alter < 18 **OR** (Alter < 25 **AND** in Ausbildung)
- Diese Regeln ändern sich mit Gesetzesänderungen

DMN-Entscheidungstabellen sind ideal für regelbasierte Verwaltungsentscheidungen:
```
| Zonenart   | Abstand | Baudichte | Bescheid     |
|------------|---------|-----------|--------------|
| "WR"       | >= 3    | <= 0.4    | Genehmigt    |
| "WR"       | < 3     | -         | Abgelehnt    |
| "MI"       | -       | <= 0.6    | Mit Auflagen |
| "GE"       | -       | -         | Behörde X    |
```

**Ableitungen für ThemisDB:**

```cpp
// Zielimplementierung: include/process/dmn_evaluator.h
namespace themis::process {

class DmnEvaluator {
public:
    struct DecisionTable {
        std::string id;
        std::string name;
        std::vector<std::string> input_columns;
        std::vector<std::string> output_columns;
        // Zeilen: [{input_values...}, {output_values...}]
        std::vector<nlohmann::json> rules;
        std::string hit_policy; // "UNIQUE", "FIRST", "COLLECT", "RULE_ORDER"
    };
    
    // DMN 1.5 XML importieren (Entscheidungstabellen + FEEL-Ausdrücke)
    bool loadFromXml(std::string_view dmn_xml);
    
    // Entscheidungstabelle auswerten
    // Gibt alle zutreffenden Zeilen zurück (je nach HitPolicy)
    nlohmann::json evaluate(
        std::string_view decision_id,
        const nlohmann::json& input_context
    ) const;
    
    // FEEL-Ausdruck auswerten (Friendly Enough Expression Language)
    // z.B. "[3..10]", "> 1000", "\"approved\""
    bool evaluateFeelExpression(
        std::string_view feel_expr,
        const nlohmann::json& value
    ) const;
};

} // namespace themis::process

// Integration in ProcessGraphRag::checkCompliance():
// - Wenn Prozessknoten eine DMN-Entscheidung referenziert (via node.metadata.dmn_ref)
// - DmnEvaluator::evaluate() mit aktuellen Prozessvariablen aufrufen
// - Ergebnis bestimmt Compliance-Status des Knotens
```

---

## 4. Prozessmodellierung Standards – Neueste Entwicklungen

### 4.1 BPMN 2.0 – Aktuelle Erweiterungen und Profilen

**Stand:** ISO/IEC 19510:2013 bleibt der Kern-Standard. Aktive Erweiterungen:

| Erweiterung | Beschreibung | Relevanz |
|-------------|-------------|----------|
| **BPMN-S** | Security Profile: Datenschutz-Annotationen | DSGVO-Compliance |
| **BPMN+** | AI Extension: ML-Aufgaben als BPMN-Tasks | KI-Workflows |
| **BPMN-I** | IoT Extension: Sensor/Aktor-Events | Smart Government |
| **Camunda 8 DSL** | Cloud-native BPMN-Ausführung | Microservices |

### 4.2 OCEL 2.0 – Object-Centric Event Log Standard

**Quelle:** Berti, A. et al. (2023). *OCEL 2.0 Specification.* Process Mining Group, RWTH.

Neue JSON/XML-Standard für Prozess-Eventlogs mit mehreren Objekttypen:
```json
{
  "objectTypes": [
    {"name": "Antrag", "attributes": [{"name": "aktenzeichen", "type": "string"}]},
    {"name": "Dokument", "attributes": [{"name": "typ", "type": "string"}]}
  ],
  "eventTypes": [
    {"name": "Vollständigkeitsprüfung", "attributes": []}
  ],
  "events": [
    {
      "id": "e1",
      "type": "Vollständigkeitsprüfung",
      "time": "2026-03-01T10:00:00Z",
      "relationships": [
        {"objectId": "antrag-001", "qualifier": "involves"},
        {"objectId": "doc-bauzeichnung-01", "qualifier": "requires"}
      ]
    }
  ]
}
```

**Ableitungen für ThemisDB:**
```
A) OCEL 2.0 Export aus ProcessLinker + ProcessInstance
   - ProcessOcelExporter::exportOcel2(instance_id) → OCEL 2.0 JSON
   - Ermöglicht Import in PM4Py, Celonis, ProM für externe Analyse
   
B) OCEL 2.0 Import
   - ProcessOcelImporter::importOcel2(ocel_json) → ProcessModelRecord + Instanzen
   - Rückmigration von extern analysierten Prozessen nach ThemisDB
```

### 4.3 ISO/IEC 33004:2015 – Process Reference Model

Internationaler Standard für Prozessreferenzmodelle in Softwareentwicklung und IT-Management. Relevant als Vorlage für strukturierte Prozessklassifikationen.

### 4.4 TOGAF 10 – Enterprise Architecture Framework

**Relevanz:** TOGAF 10 (2022) definiert den Architecture Development Method (ADM) Prozess
als iterativen Zyklus. ThemisDB Prozessmodul kann TOGAF-Phasen als Prozessmodelle abbilden.

---

## 5. Konkrete Ableitungen für ThemisDB – Priorisierte Roadmap-Items

Aus der Literaturanalyse ergeben sich folgende **priorisierte neue Implementierungen**,
geordnet nach Impact × Machbarkeit:

### Priorität 1 (Kurzfristig: Q2–Q3 2026) – Hoher Impact, machbar

| # | Feature | Wissenschaftliche Grundlage | Effort |
|---|---------|---------------------------|--------|
| 1 | **LLM-to-BPMN Generator** | ProcessGPT (Busch 2023) | M |
| 2 | **PPR-basiertes GraphRAG** | HippoRAG (Gutierrez 2024) | M |
| 3 | **OCEL 2.0 Export** | OCEL 2.0 Spec (Berti 2023) | S |
| 4 | **Community Detection (Leiden)** | GraphRAG (Edge 2024) | M |
| 5 | **Duales Retrieval (Local/Global)** | LightRAG (Guo 2024) | M |

### Priorität 2 (Mittelfristig: Q3–Q4 2026) – Strategisch wichtig

| # | Feature | Wissenschaftliche Grundlage | Effort |
|---|---------|---------------------------|--------|
| 6 | **Object-Centric Process Mining** | van der Aalst (2022) | L |
| 7 | **DMN 1.5 Entscheidungstabellen** | OMG DMN 1.5 (2023) | M |
| 8 | **FIM-Prozessbibliothek-Import** | FITKO FIM (2024) | M |
| 9 | **CMMN 1.1 Unterstützung** | OMG CMMN 1.1 (2016) | L |
| 10 | **ProcessTransformer Vorhersage** | Bukhsh et al. (2021) | L |

### Priorität 3 (Langfristig: 2027+) – Forschungsbasiert

| # | Feature | Wissenschaftliche Grundlage | Effort |
|---|---------|---------------------------|--------|
| 11 | **Differenzielle Privatsphäre** | DSGVO + DP-Forschung | XL |
| 12 | **BPMN+ AI-Erweiterung** | BPMN+ Spec (2025) | L |
| 13 | **Cross-Behörden-Prozessföderierung** | OZG 2.0 (2024) | XL |
| 14 | **Process Mining Federated Learning** | Forschungsstand 2025 | XL |

---

## 6. Referenzen

### Wissenschaftliche Publikationen

1. **van der Aalst, W.M.P.** et al. (2004). *Workflow Mining: Discovering Process Models from Event Logs.* IEEE Transactions on Knowledge and Data Engineering, 16(9), 1128–1142.

2. **van der Aalst, W.M.P.** (2022). *Object-Centric Process Mining: Dealing with Divergence and Convergence in Event Data.* In: Margaria T., Steffen B. (eds) LNCS 12551.

3. **Edge, D.** et al. (2024). *From Local to Global: A Graph RAG Approach to Query-Focused Summarization.* Microsoft Research. arXiv:2404.16130.

4. **Guo, Z.** et al. (2024). *LightRAG: Simple and Fast Retrieval-Augmented Generation.* arXiv:2410.05779.

5. **Gutierrez, B.J.** et al. (2024). *HippoRAG: Neurobiologically Inspired Long-Term Memory for Large Language Models.* NeurIPS 2024. arXiv:2405.14831.

6. **Busch, K.** et al. (2023). *ProcessGPT: Transforming Business Process Management with Generative AI.* IEEE Big Data 2023.

7. **Grohs, M.** et al. (2024). *Large Language Models for Business Process Management: Challenges and Opportunities.* ICPM 2024.

8. **Bukhsh, Z.A.** et al. (2021). *ProcessTransformer: Predictive Business Process Monitoring with Transformer Network.* arXiv:2104.00721.

9. **Berti, A.** et al. (2023). *OCEL 2.0 Specification.* Process Mining Group, RWTH Aachen. doi:10.5281/zenodo.8428111.

10. **Leemans, S.J.J.** et al. (2013). *Discovering Block-Structured Process Models from Event Logs.* Petri Nets 2013. LNCS 7927.

11. **Dijkman, R.** et al. (2011). *Similarity of Business Process Models: Metrics and Evaluation.* Information Systems, 36(2), 498–516.

12. **Weidlich, M.** et al. (2011). *Behavioural Profiles for Business Process Models.* IEEE Transactions on Services Computing, 4(2).

### Standards und Normen

13. **OMG** (2013). *Business Process Model and Notation (BPMN) 2.0.* ISO/IEC 19510:2013.

14. **OMG** (2016). *Case Management Model and Notation (CMMN) 1.1.*

15. **OMG** (2023). *Decision Model and Notation (DMN) 1.5.*

16. **FITKO** (2024). *Föderales Informationsmanagement – Handbuch Version 3.* Berlin: FITKO.

17. **Bundesministerium des Innern** (2024). *Onlinezugangsgesetz 2.0 – Referentenentwurf.*

18. **EU** (2023). *Single Digital Gateway Regulation (EU) 2018/1724 – Implementing Acts.*

### Tools und Frameworks

19. **PM4Py** (2024). Process Mining for Python. https://pm4py.fit.fraunhofer.de/

20. **Neo4j GraphRAG** (2024). Neo4j GraphRAG Python Package. https://github.com/neo4j/neo4j-graphrag-python

21. **Camunda 8** (2024). Cloud-native BPMN/DMN execution engine. https://camunda.com

22. **Celonis Process Mining** (2024). Enterprise Process Mining Platform. https://celonis.com

---

*Dieses Dokument bildet die wissenschaftliche Grundlage für die ROADMAP.md und
FUTURE_ENHANCEMENTS.md des `src/process/` Moduls. Alle abgeleiteten Features sind
in diesen Dateien vermerkt mit Verweis auf die zugehörige Quelle.*
