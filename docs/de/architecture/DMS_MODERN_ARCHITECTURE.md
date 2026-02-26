# Moderne DMS-Architektur für komplexe Verwaltungsprozesse

**Kategorie:** 🏛️ Architektur  
**Version:** v1.0.0  
**Status:** 📋 Spezifikation  
**Datum:** Februar 2026

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#-übersicht)
- [Anforderungen moderner DMS](#-anforderungen-moderner-dms)
- [Sequential vs. Graph-basierte Prozessmodellierung](#-sequential-vs-graph-basierte-prozessmodellierung)
- [Multi-Model-Integration](#-multi-model-integration)
- [KI/ML-Use-Cases für Verwaltungsprozesse](#-kiml-use-cases-für-verwaltungsprozesse)
- [Best-Practices aus Enterprise-DMS](#-best-practices-aus-enterprise-dms)
- [ThemisDB-Stärken im DMS-Kontext](#-themisdb-stärken-im-dms-kontext)
- [Referenzen](#-referenzen)

---

## 🎯 Übersicht

Das **Themis.DocumentManager** wird von einem einfachen Document-Browser zu einem vollständigen, **KI-gestützten, Graph-basierten DMS** für komplexe Verwaltungsprozesse weiterentwickelt. Diese Spezifikation beschreibt die Architektur, die notwendig ist, um reale, nicht-lineare Verwaltungsvorgänge zu unterstützen.

### Kernprinzipien

| Prinzip | Beschreibung |
|---------|-------------|
| **Graph-first** | Verwaltungsprozesse sind Graphen, nicht lineare Sequenzen |
| **Multi-Model** | Relational + Graph + Vector + Geo + Timeseries in einer Engine |
| **KI-augmentiert** | ML für Klassifizierung, Routing, Anomalieerkennung, Vorhersage |
| **Echtzeit-kollaborativ** | Multi-User, Real-time-Updates, @-Mentions |
| **Standard-konform** | BPMN 2.0, OZG, XÖV, WCAG 2.1 |

---

## 📋 Anforderungen moderner DMS

### 1. Graph-basierte Prozessmodellierung

Real-world Verwaltungsvorgänge sind hochgradig vernetzt. Ein Baugenehmigungsantrag beispielsweise löst parallel Prüfvorgänge bei Statikbüro, Umweltamt und Tiefbauamt aus – diese sind ihrerseits voneinander abhängig.

**Anforderungen:**
- BPMN 2.0-konforme visuelle Modellierung mit Gateways, Parallel-Pfaden, Conditional Routing
- Runtime-Prozess-Graphen: Jeder instanziierte Prozess wird als Graph gespeichert
  - Node = Aktivität (Aufgabe, Entscheidung, Ereignis)
  - Edge = Abhängigkeit (sequenziell, parallel, bedingt)
- Dynamische Prozessanpassung zur Laufzeit (neue Abhängigkeiten, Nachforderungen)
- Prozess-Templates (Schablonen) für wiederkehrende Verwaltungsabläufe

### 2. Nicht-lineare Abhängigkeiten

```
Lineares Modell (unzureichend):
Antrag → Prüfung → Genehmigung → Archiv

Graph-Modell (realistisch):
                    ┌─ Statikprüfung ─────────────┐
Antrag ─┬─────────→┤                              ├─→ Gesamtentscheidung → Archiv
        │          └─ Umweltverträglichkeit ──────┤
        │                                         │
        └─ Tiefbauamt ────────────────────────────┘
```

### 3. Cross-Domain-Prozesse

Bürger durchlaufen mehrere Behördenstellen gleichzeitig:
- Parallele Workstreams bei verschiedenen Ämtern
- Abhängigkeiten zwischen Behörden (z.B. Umweltgutachten blockiert Baugenehmigung)
- Zentralisierte SLA-Überwachung über alle beteiligten Stellen

### 4. Netzwerk-Effekte

- Ein Hauptantrag kann N Unteranträge auslösen
- Unteranträge haben eigene Abhängigkeiten und Sub-Prozesse
- Graph-Traversal ermöglicht Analyse der Gesamtkomplexität

### 5. Echtzeit-Monitoring

- SLA-Überwachung: Fristen pro Prozess und global
- Engpass-Erkennung: Identifikation von überlasteten Sachbearbeitern/Ämtern
- Ressourcenauslastung: Kapazitätsplanung über alle Prozesse

### 6. Historische Analyse & Lernen

- Vector-Search für ähnliche Fälle aus der Vergangenheit
- Process-Mining aus abgeschlossenen Prozessen
- Prozessoptimierung durch Mustererkennung

---

## 🔄 Sequential vs. Graph-basierte Prozessmodellierung

### Sequential Model (Traditionell)

```
Status: ENUM (EINGEREICHT | IN_PRÜFUNG | GENEHMIGT | ABGELEHNT | ARCHIVIERT)

Vorteile:
+ Einfach zu implementieren
+ Klare Zustandsmaschine
+ Geringer Speicherverbrauch

Nachteile:
- Keine Parallelität abbildbar
- Keine bedingten Verzweigungen
- Keine Sub-Prozesse
- Keine Rücksprünge
- Kein Kontext für KI-Analysen
```

### Graph Model (Modern)

```
PROCESS_NODE: {id, type, status, assignee, deadline, metadata}
PROCESS_EDGE: {source, target, type, condition, weight}

Vorteile:
+ Beliebige Komplexität abbildbar
+ Parallele Workstreams nativ unterstützt
+ Bedingte Routing-Logik
+ Vollständiger Audit-Trail als Graph
+ KI kann Graph-Features für Vorhersagen nutzen
+ Graph-Traversal für komplexe Analysen

Nachteile:
- Höhere Systemkomplexität
- Erfordert Graph-fähige Datenbank (ThemisDB ✓)
```

### BPMN 2.0 Elemente im ThemisDB-Graph

| BPMN-Element | ThemisDB-Node-Type | Beschreibung |
|-------------|-------------------|-------------|
| Start Event | `EVENT_START` | Prozessbeginn (Antrag eingereicht) |
| End Event | `EVENT_END` | Prozessende (Genehmigt/Abgelehnt) |
| Task | `TASK` | Menschliche oder automatische Aufgabe |
| Service Task | `TASK_SERVICE` | Automatisierte Systemaufgabe |
| User Task | `TASK_USER` | Aufgabe für Sachbearbeiter |
| Exclusive Gateway | `GATEWAY_XOR` | Entweder-oder-Entscheidung |
| Parallel Gateway | `GATEWAY_AND` | Parallele Ausführung |
| Inclusive Gateway | `GATEWAY_OR` | Mindestens-ein-Pfad |
| Intermediate Event | `EVENT_INTERMEDIATE` | Nachforderung, Frist, etc. |
| Sub-Process | `SUBPROCESS` | Eingebetteter Unterprozess |

---

## 🗄️ Multi-Model-Integration

ThemisDB bietet einzigartige Stärken durch seine native Multi-Model-Architektur, die alle relevanten DMS-Datenperspektiven in einer Engine vereint.

### Datenmodell-Dimensionen

```
┌──────────────────────────────────────────────────────────────────┐
│                    ThemisDB Multi-Model DMS                      │
├──────────────┬───────────────────────────────────────────────────┤
│   RELATIONAL │ Stammdaten: Bürger, Ämter, Sachbearbeiter, Rollen │
│              │ Prozess-Metadaten, SLA-Definitionen               │
├──────────────┼───────────────────────────────────────────────────┤
│    GRAPH     │ Prozessinstanz-Graph (Nodes + Edges)              │
│              │ Abhängigkeitsnetzwerk zwischen Prozessen          │
│              │ Organisationsstruktur (Behörden-Hierarchie)       │
├──────────────┼───────────────────────────────────────────────────┤
│    VECTOR    │ Dokumenten-Embeddings für Ähnlichkeitssuche       │
│              │ Prozess-Fingerprint für ähnliche Fälle            │
│              │ Semantische Suche über Beschreibungen             │
├──────────────┼───────────────────────────────────────────────────┤
│      GEO     │ Standort-bezogene Zuständigkeiten                 │
│              │ Baugenehmigungen: Parzellen, Gebäude, Zonen       │
│              │ Umweltdaten: Schutzgebiete, Lärm-Isophones        │
├──────────────┼───────────────────────────────────────────────────┤
│  TIMESERIES  │ SLA-Tracking: Fristen, Bearbeitungszeiten         │
│              │ Performance-Metriken über Zeit                    │
│              │ Audit-Log-Zeitstempel für Compliance              │
└──────────────┴───────────────────────────────────────────────────┘
```

### Beispiel: Baugenehmigung (Multi-Model-Query)

```aql
-- Finde alle offenen Baugenehmigungen in Stuttgart mit Geo-Filter,
-- die ähnlich zu einem Referenzfall sind (Vector), und zeige den
-- aktuellen Graph-Status sowie SLA-Risiko

WITH ref_case AS (
  SELECT embedding FROM process_documents WHERE id = 'CASE-2024-001'
),
similar_cases AS (
  SELECT
    p.id,
    p.process_type,
    p.status,
    SIMILARITY(p.embedding, ref_case.embedding) AS similarity_score
  FROM processes p, ref_case
  WHERE p.process_type = 'BAUGENEHMIGUNG'
    AND p.status != 'ABGESCHLOSSEN'
    AND PROXIMITY(p.geo_location, [9.1829, 48.7758]) <= 5000  -- 5km Radius Stuttgart
    AND SIMILARITY(p.embedding, ref_case.embedding) > 0.75
  ORDER BY similarity_score DESC
  LIMIT 20
),
graph_status AS (
  GRAPH TRAVERSE FROM similar_cases.id
  EDGES process_edges
  NODES process_nodes
  WHERE node.status IN ('AKTIV', 'WARTEND', 'ÜBERFÄLLIG')
  RETURN {process_id, current_node, blocking_nodes}
)
SELECT
  sc.id,
  sc.process_type,
  sc.similarity_score,
  gs.current_node,
  gs.blocking_nodes,
  TIMESERIES_AGG(audit_log, 'deadline_risk', INTERVAL '7 days') AS sla_risk
FROM similar_cases sc
JOIN graph_status gs ON sc.id = gs.process_id
JOIN LATERAL TIMESERIES audit_timeline ON audit_timeline.process_id = sc.id
```

### Hybride Indexstrategie für DMS

| Datentyp | Index-Typ | Zweck |
|----------|-----------|-------|
| Prozess-Text | Fulltext + Vector | Semantische Suche, KI-Klassifizierung |
| Geo-Koordinaten | R-Tree/GeoHash | Zuständigkeits-Lookup, Proximity-Filter |
| Graph-Edges | Adjacency-List | Traversal, Pfad-Analyse |
| Zeitstempel | B-Tree + Timeseries | SLA-Berechnung, Trend-Analyse |
| Dokument-Hash | Hash-Index | Duplikat-Erkennung |

---

## 🤖 KI/ML-Use-Cases für Verwaltungsprozesse

### 1. Automatische Dokumentenklassifizierung

**Problem:** Eingehende Dokumente (E-Mail, Fax, Portal-Upload) müssen manuell einer Kategorie zugewiesen werden.

**Lösung:** Zero-Shot-Classification via LLM (lokal, DSGVO-konform)

```
Input: Dokument-Text
Output: {
  category: "ANTRAG_BAUGENEHMIGUNG" | "BESCHWERDE" | "ANFRAGE_INFORMATION" | ...
  confidence: 0.94,
  suggested_workflow: "BAUGENEHMIGUNG_STANDARD",
  required_documents: ["Lageplan", "Bauzeichnung", "Statiknachweis"],
  estimated_duration_days: 45
}
```

**ThemisDB-Integration:** Fulltext-Index für schnellen Dokumenten-Lookup + Vector-Embedding für Ähnlichkeitsprüfung mit Referenzfällen.

### 2. Intelligentes Routing

**Problem:** Sachbearbeiter-Zuweisung erfolgt manuell, führt zu ungleicher Auslastung.

**Lösung:** Multi-Kriterien-Optimierung

```
Score = w1 * Expertise(sachbearbeiter, prozesstyp)
      + w2 * (1 - CurrentLoad(sachbearbeiter))
      + w3 * HistoricalPerformance(sachbearbeiter, ähnliche_fälle)
      + w4 * AvailableCapacity(sachbearbeiter, deadline)
```

**ThemisDB-Integration:** Graph-Traversal für Organisationsstruktur + Timeseries für Auslastungshistorie + Relational für Kapazitätsdaten.

### 3. Anomalieerkennung

**Problem:** Ungewöhnlich lange Bearbeitungszeiten werden zu spät erkannt.

**Lösung:** Zeitreihen-basierte Anomalieerkennung

```
Baseline: Median + Standardabweichung aus ähnlichen Fällen (Vector-Search)
Alert-Trigger:
  - Bearbeitungszeit > Baseline + 2σ
  - Prozess in WARTEND-Status > SLA-Threshold
  - Aufeinanderfolgende Ablehnungen in Graph-Pfad (Muster)
```

**ThemisDB-Integration:** Timeseries für Bearbeitungszeit-Tracking + Vector-Search für Baseline-Kalibrierung anhand ähnlicher historischer Fälle.

### 4. Predictive Analytics – Genehmigungswahrscheinlichkeit

**Problem:** Bürger und Sachbearbeiter wissen nicht, wie wahrscheinlich eine Genehmigung ist.

**Lösung:** ML-Modell basierend auf historischen Fällen

```
Features:
- Prozesstyp, Antragssteller-Geschichte
- Vollständigkeit der Unterlagen
- Ähnlichkeit zu genehmigten/abgelehnten Fällen (Vector-Distance)
- Geografische Lage (Schutzzone, Bebauungsplan)
- Aktueller Prozess-Graph-Zustand

Output:
- approval_probability: 0.78
- key_risk_factors: ["fehlender_laermschutznachweis", "grenzabstand"]
- recommended_actions: ["Lärmschutzgutachten beauftragen (erhöht P um +0.15)"]
```

### 5. Natural Language Query Interface

**Problem:** Komplexe Suchanfragen erfordern AQL-Kenntnisse.

**Lösung:** LLM-basiertes Text-zu-AQL-System

```
Benutzer: "Zeige mir alle genehmigten Wohnhausbau-Anträge in Stuttgart 
           der letzten 6 Monate, die Lärmschutz-Probleme hatten"

Generiertes AQL:
  FOR p IN processes
    FILTER p.type == "BAUGENEHMIGUNG_WOHNHAUS"
      AND p.status == "GENEHMIGT"
      AND p.location.city == "Stuttgart"
      AND p.completed_date >= DATE_SUB(NOW(), 6, "month")
      AND "laermschutz" IN p.issue_tags
    RETURN p
```

---

## 🏢 Best-Practices aus Enterprise-DMS

### Alfresco (Enterprise DMS)

| Feature | Adaption für ThemisDB-DMS |
|---------|--------------------------|
| Content Repository | ThemisDB Relational + Document Store |
| Workflow Engine (Activiti) | ThemisDB Graph-Prozessmodell |
| Full-Text Search (Solr) | ThemisDB Native Fulltext + Vector-Index |
| Metadata Model | ThemisDB Flexible JSON-Schema |
| Version Control | ThemisDB MVCC + Audit-Log |

**Übernahme:** Alfresco-Konzept der "Aspects" (dynamische Metadaten-Erweiterungen) als JSON-Felder in ThemisDB-Prozessknoten.

### Apache Airflow / DAG-basierte Prozessmodellierung

| Konzept | Adaption |
|---------|----------|
| DAG (Directed Acyclic Graph) | ThemisDB Graph-Prozessinstanz (mit zyklischer Erweiterung für Rücksprünge) |
| Operator | PROCESS_NODE mit Typ-Attribut |
| Dependency | PROCESS_EDGE mit Bedingung |
| XCom (Datenaustausch) | Edge-Metadata in ThemisDB |
| Task State | NODE_STATUS ENUM |

**Unterschied:** Airflow verarbeitet technische Workflows; ThemisDB-DMS verarbeitet menschlich-getriebene Verwaltungsprozesse mit Genehmigungen, Fristen und Kollaboration.

### Microsoft SharePoint – Collaboration Patterns

- **Dokumenten-Co-Authoring:** Real-time-Bearbeitung von Dokumenten (adaptiert via WebSocket-Layer)
- **@-Mentions in Kommentaren:** Direkte Benachrichtigung von Sachbearbeitern
- **Versionierung:** Jede Änderung als versionierter Snapshot in ThemisDB
- **Berechtigungssystem:** Rollenbasiert (Sachbearbeiter, Supervisor, Bürger, Lesezugriff)

### Notion / Obsidian – Knowledge Graph UI

- **Bidirektionale Links:** Verwandte Prozesse, Dokumente, Personen sind verknüpft
- **Graph-View:** Visuelle Exploration des Prozess-Netzwerks
- **Inline-Kommentare:** Kommentare direkt an Prozessknoten anheften
- **Kanban-Board:** Prozesse nach Status sortiert anzeigen

---

## 🚀 ThemisDB-Stärken im DMS-Kontext

### Warum ThemisDB und kein klassisches DMS?

| Capability | Klassisches DMS (z.B. Alfresco) | ThemisDB-DMS |
|-----------|--------------------------------|--------------|
| Graph-Prozesse | Extern (Activiti/Camunda) | Nativ |
| Vector-Search | Extern (Elasticsearch/Pinecone) | Nativ |
| Geo-Queries | Extern (PostGIS) | Nativ |
| Timeseries-SLA | Extern (InfluxDB) | Nativ |
| AQL-Queries | Proprietär (CMIS, AlfrescoQL) | Offen, erweiterbar |
| Multi-Model-Joins | Nicht möglich (System-Grenzen) | Native Hybrid-Queries |
| DSGVO-Compliance | Aufwendige Konfiguration | On-Premise, kein Cloud-Pflicht |

### Konkrete Vorteile

1. **Keine Systemgrenzen:** Graph, Vector, Geo, Timeseries in einer AQL-Query
2. **Konsistenz:** ACID über alle Modelle, keine Eventual-Consistency-Probleme
3. **Einfache Betriebsführung:** Ein System statt 5+ Spezialsysteme
4. **Sovereign Cloud:** Vollständig on-premise, DSGVO-konform ohne externe Abhängigkeiten

---

## 📚 Referenzen

- [BPMN 2.0 Spezifikation](https://www.omg.org/spec/BPMN/2.0/) (OMG)
- [OZG – Onlinezugangsgesetz](https://www.onlinezugangsgesetz.de/)
- [XÖV – XML in der öffentlichen Verwaltung](https://www.xoev.de/)
- [WCAG 2.1 Accessibility Guidelines](https://www.w3.org/TR/WCAG21/)
- [Apache Airflow DAG-Konzept](https://airflow.apache.org/docs/apache-airflow/stable/core-concepts/dags.html)
- [Alfresco Content Services Architecture](https://docs.alfresco.com/content-services/latest/develop/software-architecture/)
- ThemisDB AQL-Dokumentation: `docs/de/aql/README.md`
- ThemisDB Multi-Model-Architektur: `docs/de/architecture/architecture_multi_model.md`

---

*Siehe auch:*
- [`docs/de/design/THEMIS_DMS_DATA_MODEL.md`](../design/THEMIS_DMS_DATA_MODEL.md) – Konkretes AQL-Datenmodell
- [`docs/de/integration/KI_ML_INTEGRATION_GUIDE.md`](../integration/KI_ML_INTEGRATION_GUIDE.md) – KI/ML-Integration
- [`docs/de/IMPLEMENTATION_ROADMAP.md`](../IMPLEMENTATION_ROADMAP.md) – Umsetzungsplan
