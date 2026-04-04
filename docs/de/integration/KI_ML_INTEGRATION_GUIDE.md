# KI/ML Integration Guide – Themis.DocumentManager

**Kategorie:** 🤖 KI-Integration  
**Version:** v1.0.0  
**Status:** 📋 Spezifikation  
**Datum:** Februar 2026

---

## 📑 Inhaltsverzeichnis

- [Übersicht & Prinzipien](#-übersicht--prinzipien)
- [LLM-Integration](#-llm-integration)
- [Dokumenten-Klassifizierung](#-dokumenten-klassifizierung)
- [Prozessschritt-Empfehlung](#-prozessschritt-empfehlung)
- [Anomalieerkennung](#-anomalieerkennung)
- [Natural Language Query Interface](#-natural-language-query-interface)
- [ML-Modelle](#-ml-modelle)
- [Datenschutz & Compliance](#-datenschutz--compliance)
- [Deployment-Architektur](#-deployment-architektur)
- [Evaluation & Monitoring](#-evaluation--monitoring)

---

## 🎯 Übersicht & Prinzipien

### Design-Prinzipien für KI im Verwaltungskontext

| Prinzip | Beschreibung |
|---------|-------------|
| **DSGVO-first** | Alle KI-Modelle laufen on-premise, kein Datentransfer zu externen APIs |
| **Human-in-the-Loop** | KI empfiehlt, Mensch entscheidet – keine vollautomatischen Verwaltungsentscheidungen |
| **Erklärbarkeit** | Jede KI-Empfehlung hat eine nachvollziehbare Begründung (Explainability) |
| **Auditierbarkeit** | Alle KI-Empfehlungen werden mit Modellversion + Konfidenz protokolliert |
| **Degradation** | System funktioniert vollständig auch ohne KI (Graceful Degradation) |
| **Bias-Awareness** | Regelmäßige Überprüfung auf Diskriminierung in Routing/Scoring |

### KI-Funktionen im Überblick

```
┌────────────────────────────────────────────────────────────────────────┐
│                      Themis DMS KI-Stack                               │
├─────────────────────┬──────────────────────┬───────────────────────────┤
│  DOKUMENTEN-        │  PROZESS-            │  ANALYTICS                │
│  INTELLIGENZ        │  INTELLIGENZ         │                           │
│                     │                      │                           │
│  • Klassifizierung  │  • Routing-Empfehlung│  • Anomalieerkennung      │
│  • OCR + Extraktion │  • Nächster Schritt  │  • SLA-Risikovorhersage   │
│  • Vollständigkeit  │  • Assignee-Vorschlag│  • Throughput-Prognose    │
│  • Duplikaterkennung│  • Eskalations-Radar │  • Process-Mining         │
├─────────────────────┴──────────────────────┴───────────────────────────┤
│                      FOUNDATION LAYER                                  │
│  • LLM: Llama 3.1 (8B/70B) on-premise                                 │
│  • Embeddings: nomic-embed-text-v1.5 (local)                           │
│  • Vector Store: ThemisDB native HNSW                                  │
│  • Timeseries-Anomaly: Z-Score + Isolation Forest                      │
└────────────────────────────────────────────────────────────────────────┘
```

---

## 🧠 LLM-Integration

### Technologie-Stack

```yaml
# Empfohlene Modell-Konfiguration (DSGVO-konform, on-premise)

llm_primary:
  model: "llama-3.1-8b-instruct"      # Für Chat, Klassifizierung, NLQ
  provider: "llama.cpp"               # ThemisDB bereits mit llama.cpp integriert
  quantization: "Q4_K_M"             # ~5GB RAM
  context_window: 8192
  temperature: 0.1                    # Konsistente Outputs für Klassifizierung
  
llm_heavy:
  model: "llama-3.1-70b-instruct"    # Für komplexe NLQ und Analyse
  provider: "llama.cpp"
  quantization: "Q4_K_M"            # ~40GB RAM (GPU-empfohlen)
  
embeddings:
  model: "nomic-embed-text-v1.5"    # 768-dimensional, lokal
  provider: "ollama"
  batch_size: 32
```

### LLM-Pipeline-Architektur

```
Eingehende Anfrage
       ↓
  [Input-Sanitizer]          -- Entfernt persönliche Daten für Prompt
       ↓
  [Context-Builder]          -- Holt relevante Daten aus ThemisDB
       ↓
  [Prompt-Template-Engine]   -- Befüllt strukturierte Prompts
       ↓
  [LLM via llama.cpp]        -- Lokale Inferenz
       ↓
  [Output-Parser]            -- Strukturierte JSON-Ausgabe
       ↓
  [Confidence-Evaluator]     -- Bewertet Zuverlässigkeit
       ↓
  [Audit-Logger]             -- Protokolliert Anfrage + Antwort + Konfidenz
       ↓
  Strukturiertes Ergebnis an UI
```

---

## 📄 Dokumenten-Klassifizierung

### Funktionsbeschreibung

Beim Upload von Dokumenten (PDF, DXF, IFC, DOCX) wird automatisch:
1. Text via OCR extrahiert (falls Scan)
2. Dokumenttyp klassifiziert
3. Pflichtfelder extrahiert (Datum, Namen, Aktenzeichen)
4. Vollständigkeit gegen Prozesstyp geprüft

### Prompt-Template: Dokumenten-Klassifizierung

```
System:
Du bist ein Sachbearbeiter-Assistent für deutsche Verwaltungsbehörden.
Klassifiziere das folgende Dokument und extrahiere strukturierte Informationen.
Antworte ausschließlich als gültiges JSON.

User:
Dokument-Text (erste 2000 Zeichen):
---
{document_text}
---

Prozesskontext:
- Prozesstyp: {process_type}
- Erwartete Dokumente: {expected_docs}

Klassifiziere dieses Dokument und gib folgendes JSON zurück:
{
  "doc_category": "EINER_DER_TYPEN",
  "confidence": 0.0-1.0,
  "document_date": "ISO-Datum oder null",
  "document_reference": "Aktenzeichen/Referenz oder null",
  "key_entities": ["Liste relevanter Personen/Firmen/Adressen"],
  "detected_issues": ["Liste von Problemen, z.B. unleserlich, unvollständig"],
  "covers_requirement": "NAME_DES_ANFORDERUNGSPUNKTS oder null"
}

Mögliche Kategorien: LAGEPLAN, BAUZEICHNUNG, STATIKNACHWEIS, 
GUTACHTEN_UMWELT, GUTACHTEN_LAERM, ANTRAG, BESCHEID, 
NACHFORDERUNGSSCHREIBEN, VOLLMACHT, EIGENTUEMERNACHWEIS, SONSTIGE
```

### Vollständigkeitsprüfung

```python
# Pseudocode: Vollständigkeitsprüfung nach Dokumenten-Upload
def check_completeness(process_id: str, process_type: str) -> dict:
    required_docs = REQUIRED_DOCS_BY_TYPE[process_type]
    uploaded_docs = db.query(
        "SELECT doc_category, ai_category_confidence FROM document_attachment "
        "WHERE process_id = ? AND ai_category_confidence > 0.7",
        [process_id]
    )
    
    uploaded_categories = {d.doc_category for d in uploaded_docs}
    missing = set(required_docs) - uploaded_categories
    
    return {
        "is_complete": len(missing) == 0,
        "missing_documents": list(missing),
        "uploaded_documents": list(uploaded_categories),
        "completeness_score": len(uploaded_categories) / len(required_docs)
    }
```

---

## 🎯 Prozessschritt-Empfehlung

### Funktionsbeschreibung

Nach jedem abgeschlossenen Prozessschritt empfiehlt das KI-System:
1. **Nächste Aktion** (Abschließen, Nachfordern, Eskalieren)
2. **Optimalen Assignee** für den nächsten Schritt
3. **Risikobewertung** des aktuellen Zustands

### Empfehlungs-Score-Berechnung

```aql
-- AQL: Berechnung des Empfehlungs-Scores für "Abschließen" vs. "Nachfordern"
LET current_process = FIRST(FOR p IN process WHERE p.id == @process_id RETURN p)
LET similar_approved = (
    FOR p IN process
      FILTER p.process_type == current_process.process_type
        AND p.status == 'GENEHMIGT'
        AND SIMILARITY(p.embedding, current_process.embedding) > 0.8
      LIMIT 50
      RETURN p
)
LET similar_rejected = (
    FOR p IN process
      FILTER p.process_type == current_process.process_type
        AND p.status == 'ABGELEHNT'
        AND SIMILARITY(p.embedding, current_process.embedding) > 0.8
      LIMIT 50
      RETURN p
)
LET doc_completeness = FIRST(
    SELECT completeness_score FROM document_completeness_view
    WHERE process_id = @process_id
)
RETURN {
    approve_score: (
        LENGTH(similar_approved) / (LENGTH(similar_approved) + LENGTH(similar_rejected)) * 0.6
        + doc_completeness.completeness_score * 0.4
    ),
    evidence: {
        similar_approved_count: LENGTH(similar_approved),
        similar_rejected_count: LENGTH(similar_rejected),
        doc_completeness: doc_completeness.completeness_score,
        top_similar_cases: similar_approved[0..2][*].reference_number
    }
}
```

### Assignee-Optimierung

```python
# Pseudocode: Optimaler Assignee für einen Prozessschritt
def recommend_assignee(
    node: ProcessNode,
    process: Process
) -> list[AssigneeRecommendation]:
    
    candidates = db.query("""
        SELECT u.id, u.name,
               AVG(pn.completed_at - pn.started_at) AS avg_duration,
               COUNT(*) AS completed_similar,
               COUNT(open_tasks.id) AS current_load
        FROM user_account u
        JOIN process_node pn ON pn.assignee_id = u.id
        LEFT JOIN process_node open_tasks ON (
            open_tasks.assignee_id = u.id
            AND open_tasks.status IN ('AKTIV', 'IN_BEARBEITUNG')
        )
        WHERE u.role = :required_role
          AND pn.node_type = :node_type
          AND pn.status = 'ABGESCHLOSSEN'
        GROUP BY u.id, u.name
    """, node.required_role, node.node_type)
    
    max_load = max(c.current_load for c in candidates) or 1
    scores = []
    for c in candidates:
        score = (
            0.4 * (c.completed_similar / 100)          # Erfahrung
            + 0.4 * (1 - c.current_load / max_load)    # Verfügbarkeit
            + 0.2 * (1 / (c.avg_duration.days + 1))    # Geschwindigkeit
        )
        scores.append(AssigneeRecommendation(
            user_id=c.id,
            name=c.name,
            score=score,
            reasoning=f"Erfahrung: {c.completed_similar} ähnliche Fälle, "
                      f"Auslastung: {c.current_load} offene Tasks"
        ))
    
    return sorted(scores, key=lambda x: x.score, reverse=True)[:3]
```

---

## ⚠️ Anomalieerkennung

### Zeitreihen-basierte Anomalieerkennung

**Ziel:** Proaktive Erkennung von ungewöhnlich langen Bearbeitungszeiten bevor SLA verletzt wird.

```python
# Pseudocode: Z-Score basierte Anomalieerkennung
def detect_processing_time_anomaly(
    process_id: str,
    node_type: str,
    current_duration_days: float
) -> AnomalyResult:
    
    # Baseline aus ähnlichen historischen Fällen (Vector-Search)
    process_embedding = db.get_process_embedding(process_id)
    baseline_durations = db.query("""
        SELECT pn.TIMESERIES_AGG(
            audit_log,
            'node_duration_days',
            node_type = :node_type
        ) AS duration
        FROM process p
        JOIN process_node pn ON pn.process_id = p.id
        WHERE pn.node_type = :node_type
          AND pn.status = 'ABGESCHLOSSEN'
          AND SIMILARITY(p.embedding, :embedding) > 0.75
        LIMIT 100
    """, node_type=node_type, embedding=process_embedding)
    
    if len(baseline_durations) < 10:
        return AnomalyResult(is_anomaly=False, reason="Insufficient baseline data")
    
    durations = [d.duration for d in baseline_durations]
    mean = statistics.mean(durations)
    std = statistics.stdev(durations)
    
    z_score = (current_duration_days - mean) / std if std > 0 else 0
    
    return AnomalyResult(
        is_anomaly=abs(z_score) > 2.5,
        z_score=z_score,
        baseline_mean_days=mean,
        baseline_std_days=std,
        current_duration_days=current_duration_days,
        severity='CRITICAL' if z_score > 4 else 'WARNING' if z_score > 2.5 else 'OK',
        reason=f"Bearbeitungszeit {current_duration_days:.1f}d vs. Ø {mean:.1f}d (±{std:.1f}d)"
    )
```

### Muster-basierte Anomalieerkennung

Erkennung von **strukturellen Anomalien** im Prozess-Graph:

```aql
-- Finde Prozesse mit ungewöhnlicher Knoten-Sequenz (verglichen mit Template)
FOR p IN process
  FILTER p.status == 'IN_BEARBEITUNG'
  LET template_path = GRAPH_PATHS(process_template_graph, p.template_id)
  LET actual_path   = GRAPH_PATHS(process_graph, p.id)
  LET deviation = GRAPH_EDIT_DISTANCE(template_path, actual_path)
  FILTER deviation > 3  -- Mehr als 3 Abweichungen vom Template
  RETURN {
    process_id:       p.id,
    reference_number: p.reference_number,
    graph_deviation:  deviation,
    anomaly_type:     'STRUCTURAL_DEVIATION'
  }
```

---

## 💬 Natural Language Query Interface

### Architektur

```
Text-Eingabe
     ↓
[Intent-Klassifizierer]      -- Suche / Analyse / Aktion / Frage
     ↓
[Entity-Extraktor]           -- Prozesstyp, Zeitraum, Ort, Status, Person
     ↓
[AQL-Generator (LLM)]        -- Text → AQL-Query
     ↓
[AQL-Validator]              -- Sicherheitsprüfung, Schema-Validierung
     ↓
[ThemisDB AQL-Executor]      -- Query ausführen
     ↓
[Response-Formatter (LLM)]   -- Ergebnis in natürliche Sprache übersetzen
     ↓
Strukturierte Antwort an UI
```

### Prompt-Template: Text-zu-AQL

```
System:
Du bist ein AQL-Query-Generator für ThemisDB.
Konvertiere natürlichsprachliche Anfragen in valide AQL-Queries.
Das Schema ist:

Tabellen: process, process_node, process_edge, document_attachment,
          stakeholder, authority, ki_prediction, collaboration_comment

Wichtige Felder:
- process.process_type: VARCHAR ('BAUGENEHMIGUNG_NEUBAU', etc.)
- process.status: VARCHAR ('IN_BEARBEITUNG', 'GENEHMIGT', 'ABGELEHNT', ...)
- process.geo_location: POINT (für PROXIMITY-Funktionen)
- process.embedding: VECTOR (für SIMILARITY-Funktionen)
- process.submitted_at: TIMESTAMPTZ

AQL-Funktionen verfügbar:
- SIMILARITY(vec1, vec2) → float
- PROXIMITY(point, [lon,lat]) → meters
- DATE_DIFF(d1, d2, 'days') → int
- DATE_SUB(date, n, 'month') → date

Antworte NUR mit der AQL-Query, kein erläuternder Text.
Nutze immer parametrisierte Werte (@param) statt Literal-Injection.

User:
{natural_language_query}
```

### Sicherheits-Validierung

```python
FORBIDDEN_AQL_KEYWORDS = {
    'DROP', 'DELETE', 'TRUNCATE', 'ALTER', 'CREATE',
    'INSERT', 'UPDATE', 'GRANT', 'REVOKE'
}

def validate_generated_aql(aql_query: str, user_role: str) -> ValidationResult:
    # 1. Gefährliche Operationen verbieten
    query_upper = aql_query.upper()
    for keyword in FORBIDDEN_AQL_KEYWORDS:
        if keyword in query_upper:
            return ValidationResult(valid=False,
                reason=f"Verbotenes Keyword: {keyword}")
    
    # 2. Berechtigungscheck: Darf Rolle auf alle referenzierten Tabellen zugreifen?
    referenced_tables = extract_table_references(aql_query)
    for table in referenced_tables:
        if not has_read_permission(user_role, table):
            return ValidationResult(valid=False,
                reason=f"Keine Leseberechtigung für: {table}")
    
    # 3. LIMIT erzwingen (Performance-Schutz)
    if 'LIMIT' not in query_upper:
        aql_query += '\nLIMIT 100'
    
    return ValidationResult(valid=True, sanitized_query=aql_query)
```

---

## 📈 ML-Modelle

### Modell 1: Approval Probability (Genehmigungswahrscheinlichkeit)

```
Typ:            Gradient Boosting (XGBoost)
Training-Daten: Historische abgeschlossene Prozesse (min. 500 je Prozesstyp)
Update-Zyklus:  Monatlich (Retrain bei > 5% Performance-Drift)

Features:
  Dokumenten-Features:
    - completeness_score (0-1)
    - doc_freshness_days (Alter ältesten Dokuments)
    - has_required_certifications (boolean[])
  
  Prozess-Features:
    - process_type (one-hot encoded)
    - complexity_score (Anzahl Knoten im Graph)
    - applicant_history_score (frühere Genehmigungsrate)
  
  Geo-Features:
    - is_in_protection_zone (boolean)
    - distance_to_next_protected_zone_m (float)
    - urban_density_score (float)
  
  Ähnlichkeits-Features:
    - similar_approved_ratio (Anteil genehmigter ähnlicher Fälle)
    - nn_similarity_score (Ähnlichkeit zu nächstem Nachbar)
    - cluster_approval_rate (Rate im Prozess-Cluster)

Target: binary (0=ABGELEHNT, 1=GENEHMIGT)

Evaluation-Metriken:
  - AUC-ROC ≥ 0.85
  - Precision bei Threshold 0.7 ≥ 0.80
  - Calibration Error ≤ 0.05
```

### Modell 2: SLA-Risk-Score

```
Typ:            Random Forest Regressor
Ziel:           Vorhersage: Wird SLA eingehalten? (Tage bis Deadline)

Features:
  - current_processing_day (Bearbeitungstag)
  - pending_nodes_count (Anzahl offener Knoten)
  - blocking_nodes_count (Anzahl blockierter Knoten)
  - external_dependencies (Boolean)
  - similar_case_avg_duration (aus Vector-Search)
  - assignee_avg_throughput (Sachbearbeiter-Geschwindigkeit)
  - current_week_load (Teamauslastung diese Woche)

Output:
  - estimated_completion_days: float
  - sla_at_risk: boolean
  - risk_score: 0.0-1.0
```

### Modell 3: Assignment Optimization

```
Typ:            Multi-Armed Bandit (Thompson Sampling) mit Kontext
Ziel:           Optimale Sachbearbeiter-Zuweisung

Kontext-Features:
  - process_type
  - estimated_complexity
  - required_specialization
  - deadline_urgency

Reward-Signal:
  - Bearbeitungszeit (kürzere = höherer Reward)
  - Qualitätsbewertung (Nachforderungsrate = negativer Reward)
  - SLA-Einhaltung (eingehalten = positiver Reward)

Update: Online-Learning nach jedem abgeschlossenen Fall
```

---

## 🔒 Datenschutz & Compliance

### DSGVO-Anforderungen

| Anforderung | Umsetzung |
|-------------|-----------|
| **Datensparsamkeit** | LLM-Prompts enthalten keine personenbezogenen Daten (Pseudonymisierung) |
| **Zweckbindung** | KI-Modelle nur für definierte Zwecke trainiert und eingesetzt |
| **Transparenz** | Jede KI-Entscheidung mit Begründung im Audit-Log |
| **Widerspruchsrecht** | Sachbearbeiter kann KI-Empfehlung ignorieren/überschreiben |
| **On-Premise** | Alle Modelle lokal, kein Datentransfer zu Cloud-APIs |

### Pseudonymisierung für LLM-Prompts

```python
def pseudonymize_for_llm(process_context: dict) -> tuple[dict, dict]:
    """Ersetzt personenbezogene Daten durch Platzhalter für LLM-Prompts."""
    mapping = {}
    pseudonymized = process_context.copy()
    
    # Antragsteller-Namen
    if 'applicant_name' in pseudonymized:
        placeholder = f"PERSON_{gen_short_id()}"
        mapping[placeholder] = pseudonymized['applicant_name']
        pseudonymized['applicant_name'] = placeholder
    
    # Adressen (nur Bezirk/PLZ behalten)
    if 'address' in pseudonymized:
        pseudonymized['address'] = f"PLZ {pseudonymized['address']['zip']}, {pseudonymized['address']['district']}"
    
    return pseudonymized, mapping  # mapping für Re-Identifikation in Antworten
```

---

## 🚀 Deployment-Architektur

### Service-Layout

```
┌─────────────────────────────────────────────────────────────────┐
│                    ThemisDB Server                              │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │                 KI-Service-Layer                          │  │
│  │                                                          │  │
│  │  ┌───────────────┐  ┌──────────────┐  ┌──────────────┐  │  │
│  │  │  LLM-Worker   │  │ ML-Inference │  │  Embedding   │  │  │
│  │  │ (llama.cpp)   │  │ (XGBoost etc)│  │  Service     │  │  │
│  │  │  Port: 8081   │  │  Port: 8082  │  │  Port: 8083  │  │  │
│  │  └───────────────┘  └──────────────┘  └──────────────┘  │  │
│  │                                                          │  │
│  │  ┌───────────────────────────────────────────────────┐   │  │
│  │  │              AI-Orchestrator                      │   │  │
│  │  │  - Routing: welcher Service für welche Anfrage   │   │  │
│  │  │  - Caching: häufige Anfragen zwischenspeichern   │   │  │
│  │  │  - Fallback: bei Fehler → Basis-Empfehlung       │   │  │
│  │  └───────────────────────────────────────────────────┘   │  │
│  └──────────────────────────────────────────────────────────┘  │
│                                                                 │
│  ┌──────────────────┐  ┌────────────────────────────────────┐  │
│  │  ThemisDB Core   │  │  Vector Store (HNSW in ThemisDB)   │  │
│  │  (AQL Engine,    │  │  - Process Embeddings              │  │
│  │   Graph, etc.)   │  │  - Document Embeddings             │  │
│  └──────────────────┘  └────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

### Hardware-Anforderungen

| Konfiguration | RAM | GPU | Eignung |
|--------------|-----|-----|---------|
| **Minimal** | 16 GB | keine | LLM 8B Q4, langsam |
| **Standard** | 32 GB | RTX 4090 (24GB) | LLM 8B schnell, Embedding-Service |
| **Enterprise** | 128 GB | 2× A100 (80GB each) | LLM 70B, parallele Inferenz |

---

## 📊 Evaluation & Monitoring

### KI-Performance-Metriken

```aql
-- KI-Performance-Dashboard: Prediction-Accuracy über Zeit
FOR pred IN ki_prediction
  FILTER pred.prediction_type == 'APPROVAL_PROBABILITY'
    AND pred.predicted_at >= DATE_SUB(NOW(), 3, 'month')
  LET actual_process = FIRST(FOR p IN process WHERE p.id == pred.process_id RETURN p)
  FILTER actual_process.status IN ['GENEHMIGT', 'ABGELEHNT']
  LET predicted_approved = pred.score >= 0.5
  LET actually_approved  = actual_process.status == 'GENEHMIGT'
  COLLECT
    week = DATE_TRUNC(pred.predicted_at, 'week'),
    correct = (predicted_approved == actually_approved)
  WITH COUNT INTO count
  RETURN {
    week:     week,
    correct:  correct,
    count:    count
  }
```

### Drift-Detection

Bei signifikantem Performance-Drift (> 5% AUC-ROC-Verschlechterung über 4 Wochen):
1. **Automatische Warnung** an System-Administrator
2. **Modell-Retrain** mit aktuellen Daten (letzten 12 Monate)
3. **A/B-Vergleich** neues vs. altes Modell auf Hold-Out-Daten
4. **Manuelle Freigabe** des neuen Modells durch Administrator

---

*Siehe auch:*
- [`docs/de/architecture/DMS_MODERN_ARCHITECTURE.md`](../architecture/DMS_MODERN_ARCHITECTURE.md) – Architekturüberblick
- [`docs/de/design/THEMIS_DMS_DATA_MODEL.md`](../design/THEMIS_DMS_DATA_MODEL.md) – KI-Vorhersage-Tabellen
- [`docs/de/llm/`](../llm/) – ThemisDB LLM-Integration
