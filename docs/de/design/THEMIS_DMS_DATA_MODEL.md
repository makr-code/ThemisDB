# Themis DMS – Datenmodell für Verwaltungsprozesse

**Kategorie:** 🗄️ Datenmodell  
**Version:** v1.0.0  
**Status:** 📋 Spezifikation  
**Datum:** Februar 2026

---

## 📑 Inhaltsverzeichnis

- [Überblick](#-überblick)
- [Kern-Entitäten](#-kern-entitäten)
- [Prozess-Graph-Modell](#-prozess-graph-modell)
- [KI & Collaboration-Erweiterungen](#-ki--collaboration-erweiterungen)
- [AQL-Beispiele](#-aql-beispiele)
- [Graph-Traversal-Queries](#-graph-traversal-queries)
- [Indexstrategie](#-indexstrategie)
- [Schema-Migration](#-schema-migration)

---

## 🎯 Überblick

Das Datenmodell verwendet ThemisDB's Multi-Model-Fähigkeiten, um Verwaltungsprozesse als **Property-Graphen** zu speichern, während Stammdaten relational, Dokument-Embeddings als Vektoren und Zeitreihen für SLA-Tracking genutzt werden.

### Modell-Dimensionen

```
Relational:  STAKEHOLDER, AUTHORITY, ROLE, SLA_DEFINITION, DOCUMENT_METADATA
Graph:       PROCESS → PROCESS_NODE → PROCESS_EDGE (Prozessinstanz als Graph)
             AUTHORITY_GRAPH (Behörden-Hierarchie und Zuständigkeiten)
Vector:      PROCESS_EMBEDDING (für Ähnlichkeitssuche), DOC_EMBEDDING
Geo:         LOCATION (Bauvorhaben-Koordinaten, Zuständigkeitsgrenzen)
Timeseries:  SLA_EVENTS, PROCESSING_TIME_LOG, AUDIT_TRAIL
```

---

## 📦 Kern-Entitäten

### PROCESS (Prozessinstanz)

```sql
-- Haupttabelle für jeden Verwaltungsvorgang
CREATE TABLE process (
    id                UUID        PRIMARY KEY DEFAULT gen_uuid(),
    template_id       UUID        REFERENCES process_template(id),
    process_type      VARCHAR(64) NOT NULL,   -- z.B. 'BAUGENEHMIGUNG_NEUBAU'
    title             VARCHAR(256) NOT NULL,
    reference_number  VARCHAR(32)  UNIQUE,    -- Aktenzeichen z.B. BG-2026-0847
    status            VARCHAR(32)  NOT NULL DEFAULT 'EINGEREICHT',
    -- EINGEREICHT | IN_BEARBEITUNG | WARTEND | IN_REVIEW |
    -- GENEHMIGT | ABGELEHNT | ZURUECKGEZOGEN | ARCHIVIERT

    -- Beteiligte
    applicant_id      UUID        REFERENCES stakeholder(id),
    authority_id      UUID        REFERENCES authority(id),
    lead_processor_id UUID        REFERENCES user_account(id),

    -- Geo-Referenz (für Bauvorhaben, Standorte)
    geo_location      POINT,      -- WGS84 Koordinaten des Objekts
    geo_district_id   UUID        REFERENCES district(id),

    -- Zeitliche Daten
    submitted_at      TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    deadline          TIMESTAMPTZ,
    completed_at      TIMESTAMPTZ,
    sla_definition_id UUID        REFERENCES sla_definition(id),

    -- KI-Feature-Vektor (für Ähnlichkeitssuche)
    embedding         VECTOR(1536),  -- OpenAI ada-002 / Llama Embedding

    -- Flexible Metadaten (prozesstyp-spezifisch)
    metadata          JSONB,
    -- Beispiel Baugenehmigung:
    -- { "object_type": "EFH", "floor_area_sqm": 180,
    --   "floors": 2, "has_basement": true,
    --   "adjacent_protected_zone": false }

    -- Audit
    created_at        TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at        TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    version           INT         NOT NULL DEFAULT 1
);

CREATE INDEX idx_process_status       ON process(status);
CREATE INDEX idx_process_type         ON process(process_type);
CREATE INDEX idx_process_deadline     ON process(deadline) WHERE deadline IS NOT NULL;
CREATE INDEX idx_process_geo          ON process USING GIST(geo_location);
CREATE INDEX idx_process_embedding    ON process USING VECTOR(embedding, 'hnsw');
CREATE INDEX idx_process_submitted    ON process(submitted_at);
```

---

### PROCESS_NODE (Knoten im Prozess-Graph)

```sql
-- Jede Aktivität/Aufgabe/Entscheidung im Prozess-Graph
CREATE TABLE process_node (
    id              UUID        PRIMARY KEY DEFAULT gen_uuid(),
    process_id      UUID        NOT NULL REFERENCES process(id) ON DELETE CASCADE,
    template_node_id UUID       REFERENCES process_template_node(id),

    -- BPMN-Typ des Knotens
    node_type       VARCHAR(32) NOT NULL,
    -- START_EVENT | END_EVENT | USER_TASK | SERVICE_TASK |
    -- GATEWAY_XOR | GATEWAY_AND | GATEWAY_OR |
    -- INTERMEDIATE_EVENT | SUBPROCESS | CALL_ACTIVITY

    title           VARCHAR(256) NOT NULL,
    description     TEXT,

    -- Zustand
    status          VARCHAR(32) NOT NULL DEFAULT 'AUSSTEHEND',
    -- AUSSTEHEND | AKTIV | IN_BEARBEITUNG | WARTEND |
    -- ABGESCHLOSSEN | ABGELEHNT | UEBERSPRUNGEN | FEHLER

    -- Zuweisung
    assignee_id     UUID        REFERENCES user_account(id),
    assignee_role   VARCHAR(64),  -- Fallback: Zuweisung per Rolle

    -- Zeitliche Daten
    started_at      TIMESTAMPTZ,
    completed_at    TIMESTAMPTZ,
    deadline        TIMESTAMPTZ,
    expected_duration_days INT,

    -- Position im Graph (für UI-Layout)
    position_x      FLOAT,
    position_y      FLOAT,

    -- Flexible Knotenmetadaten
    metadata        JSONB,
    -- Beispiel User Task:
    -- { "checklist": ["Lageplan prüfen", "Bauzeichnung prüfen"],
    --   "required_documents": ["UUID1", "UUID2"],
    --   "completion_criteria": "ALL_CHECKLIST_ITEMS" }

    -- Priorität (für Sachbearbeiter-Dashboard-Sortierung)
    priority        INT DEFAULT 50,  -- 1-100, höher = wichtiger

    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at      TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_pnode_process      ON process_node(process_id);
CREATE INDEX idx_pnode_status       ON process_node(status);
CREATE INDEX idx_pnode_assignee     ON process_node(assignee_id) WHERE assignee_id IS NOT NULL;
CREATE INDEX idx_pnode_deadline     ON process_node(deadline) WHERE deadline IS NOT NULL;
```

---

### PROCESS_EDGE (Abhängigkeiten zwischen Knoten)

```sql
-- Gerichtete Kanten im Prozess-Graph (Sequenzflüsse, Abhängigkeiten)
CREATE TABLE process_edge (
    id              UUID        PRIMARY KEY DEFAULT gen_uuid(),
    process_id      UUID        NOT NULL REFERENCES process(id) ON DELETE CASCADE,

    source_node_id  UUID        NOT NULL REFERENCES process_node(id),
    target_node_id  UUID        NOT NULL REFERENCES process_node(id),

    -- Kantentyp (BPMN-konform)
    edge_type       VARCHAR(32) NOT NULL DEFAULT 'SEQUENCE_FLOW',
    -- SEQUENCE_FLOW | MESSAGE_FLOW | ASSOCIATION |
    -- CONDITIONAL_FLOW | DEFAULT_FLOW

    -- Bedingungslogik (für GATEWAY-Ausgänge)
    condition_expr  TEXT,
    -- Beispiel: "application.floor_area_sqm > 200 AND has_basement"
    -- Beispiel: "ai_prediction.approval_score > 0.8"
    condition_type  VARCHAR(32), -- 'EXPRESSION' | 'RULE' | 'AI_BASED'

    -- Status der Kante
    is_active       BOOLEAN NOT NULL DEFAULT FALSE,  -- Wird zur Laufzeit aktiviert
    is_blocked      BOOLEAN NOT NULL DEFAULT FALSE,

    label           VARCHAR(128),  -- Beschriftung im BPMN-Diagramm
    metadata        JSONB,

    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_pedge_process  ON process_edge(process_id);
CREATE INDEX idx_pedge_source   ON process_edge(source_node_id);
CREATE INDEX idx_pedge_target   ON process_edge(target_node_id);
```

---

### PROCESS_EXECUTION (Laufzeit-Zustand)

```sql
-- Snapshot des aktuellen Ausführungszustands (für schnelles Status-Lookup)
CREATE TABLE process_execution (
    id                  UUID        PRIMARY KEY DEFAULT gen_uuid(),
    process_id          UUID        UNIQUE NOT NULL REFERENCES process(id),

    -- Aktuell aktive Knoten (Array, da parallele Ausführung möglich)
    active_node_ids     UUID[]      NOT NULL DEFAULT '{}',

    -- Variablen/Kontext (wird durch Prozessschritte befüllt)
    execution_context   JSONB NOT NULL DEFAULT '{}',
    -- Beispiel: { "received_documents": ["doc1", "doc2"],
    --             "awaiting_external": "GUTACHTER_GTR-22",
    --             "review_result": null }

    -- Statistiken
    total_nodes         INT NOT NULL DEFAULT 0,
    completed_nodes     INT NOT NULL DEFAULT 0,
    progress_percent    FLOAT GENERATED ALWAYS AS
                        (CASE WHEN total_nodes = 0 THEN 0
                         ELSE (completed_nodes::FLOAT / total_nodes * 100) END) STORED,

    -- Token-Zähler (BPMN-Execution-Semantik)
    token_count         INT NOT NULL DEFAULT 1,  -- Parallele Pfade

    last_activity_at    TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at          TIMESTAMPTZ NOT NULL DEFAULT NOW()
);
```

---

### PROCESS_TEMPLATE (Prozessschablone)

```sql
-- Wiederverwendbare Prozess-Definitionen (BPMN-Schablonen)
CREATE TABLE process_template (
    id              UUID        PRIMARY KEY DEFAULT gen_uuid(),
    name            VARCHAR(256) NOT NULL,
    process_type    VARCHAR(64)  UNIQUE NOT NULL,
    description     TEXT,
    version         VARCHAR(16)  NOT NULL DEFAULT '1.0.0',

    -- BPMN-XML-Serialisierung (für Import/Export)
    bpmn_xml        TEXT,

    -- Zugehörige Nodes und Edges werden in separaten Tabellen geführt
    -- (process_template_node, process_template_edge)

    authority_id    UUID        REFERENCES authority(id),
    sla_definition_id UUID      REFERENCES sla_definition(id),

    is_active       BOOLEAN NOT NULL DEFAULT TRUE,
    created_by      UUID        REFERENCES user_account(id),
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at      TIMESTAMPTZ NOT NULL DEFAULT NOW()
);
```

---

### SLA_DEFINITION & SLA_EVENT

```sql
-- SLA-Konfigurationen für verschiedene Prozesstypen
CREATE TABLE sla_definition (
    id              UUID        PRIMARY KEY DEFAULT gen_uuid(),
    process_type    VARCHAR(64)  UNIQUE NOT NULL,
    name            VARCHAR(256) NOT NULL,

    -- Fristen (Werktage)
    total_duration_days     INT NOT NULL,  -- Gesamte Bearbeitungsfrist
    warning_threshold_days  INT NOT NULL,  -- Warnung vor Ablauf (z.B. 5 Tage vorher)
    stage_durations         JSONB,
    -- { "EINGANG": 2, "SACHPRÜFUNG": 20, "FACHPRÜFUNG": 15, "ENTSCHEIDUNG": 5 }

    -- Gesetzliche Grundlage (z.B. LBO §57 – 3 Monate)
    legal_basis     VARCHAR(256),

    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- Zeitreihe: SLA-Ereignisse pro Prozess (für Monitoring und Analyse)
CREATE TIMESERIES sla_events (
    process_id      UUID        NOT NULL,
    event_type      VARCHAR(32) NOT NULL,
    -- STARTED | WARNING_TRIGGERED | DEADLINE_PASSED |
    -- EXTENDED | COMPLETED_ON_TIME | COMPLETED_LATE
    node_id         UUID,
    note            TEXT,
    recorded_at     TIMESTAMPTZ NOT NULL DEFAULT NOW()
) PARTITION BY RANGE(recorded_at);
```

---

### DOCUMENT_ATTACHMENT

```sql
-- Dokumente, die einem Prozess oder Knoten zugeordnet sind
CREATE TABLE document_attachment (
    id              UUID        PRIMARY KEY DEFAULT gen_uuid(),
    process_id      UUID        NOT NULL REFERENCES process(id),
    node_id         UUID        REFERENCES process_node(id),

    filename        VARCHAR(512) NOT NULL,
    mime_type       VARCHAR(128) NOT NULL,
    file_size_bytes BIGINT,
    storage_path    TEXT        NOT NULL,  -- Pfad im ThemisDB-Objektspeicher
    checksum_sha256 CHAR(64),

    -- KI-Klassifizierung
    doc_category    VARCHAR(64),
    -- LAGEPLAN | BAUZEICHNUNG | STATIKNACHWEIS | GUTACHTEN |
    -- ANTRAG | BESCHEID | KORRESPONDENZ | SONSTIGE
    ai_category_confidence FLOAT,

    -- Vektorembedding für semantische Suche
    embedding       VECTOR(1536),

    -- Metadaten (aus OCR/KI extrahiert)
    extracted_metadata JSONB,
    -- { "pages": 12, "detected_keywords": ["Lärmschutz", "DIN 4109"],
    --   "document_date": "2025-12-01" }

    uploaded_by     UUID        REFERENCES user_account(id),
    uploaded_at     TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    version         INT         NOT NULL DEFAULT 1
);

CREATE INDEX idx_doc_process   ON document_attachment(process_id);
CREATE INDEX idx_doc_category  ON document_attachment(doc_category);
CREATE INDEX idx_doc_embedding ON document_attachment USING VECTOR(embedding, 'hnsw');
```

---

## 🕸️ Prozess-Graph-Modell

### Graph-Schema in ThemisDB

```
ThemisDB Graph-Representation:

VERTEX-Typen:
  process        (id, type, status, ...)
  process_node   (id, node_type, status, assignee, ...)
  stakeholder    (id, name, type, ...)
  authority      (id, name, level, ...)
  document       (id, filename, category, ...)

EDGE-Typen:
  HAS_NODE       process → process_node       (1:N)
  DEPENDS_ON     process_node → process_node  (N:M, Ablaufsteuerung)
  ASSIGNED_TO    process_node → stakeholder   (N:1)
  HANDLED_BY     process → authority          (N:1)
  HAS_DOCUMENT   process → document           (N:M)
  ISSUED_BY      process → stakeholder        (Antragsteller)
  RELATED_TO     process → process            (Ähnliche Fälle, Abhängigkeiten)
```

---

## 🤖 KI & Collaboration-Erweiterungen

### KI_PREDICTION

```sql
CREATE TABLE ki_prediction (
    id              UUID        PRIMARY KEY DEFAULT gen_uuid(),
    process_id      UUID        NOT NULL REFERENCES process(id),

    prediction_type VARCHAR(64) NOT NULL,
    -- APPROVAL_PROBABILITY | COMPLETION_TIME |
    -- BOTTLENECK_RISK | ANOMALY_SCORE | DOCUMENT_COMPLETENESS

    score           FLOAT       NOT NULL,  -- 0.0 – 1.0
    confidence      FLOAT,                 -- 0.0 – 1.0
    model_version   VARCHAR(32),
    model_name      VARCHAR(128),

    -- Erklärbarkeit (Explainability)
    feature_importances JSONB,
    -- { "missing_documents": -0.15, "plot_conformity": +0.18,
    --   "similar_cases_approved": +0.25 }

    similar_case_ids UUID[],   -- Referenz-Fälle für diese Vorhersage

    predicted_at    TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    valid_until     TIMESTAMPTZ            -- Vorhersage wird ungültig wenn veraltet
);

CREATE INDEX idx_kipred_process ON ki_prediction(process_id);
CREATE INDEX idx_kipred_type    ON ki_prediction(prediction_type);
```

---

### COLLABORATION_COMMENT

```sql
CREATE TABLE collaboration_comment (
    id              UUID        PRIMARY KEY DEFAULT gen_uuid(),
    process_id      UUID        NOT NULL REFERENCES process(id),
    node_id         UUID        REFERENCES process_node(id),  -- Optional: Knoten-Kontext
    document_id     UUID        REFERENCES document_attachment(id),

    parent_id       UUID        REFERENCES collaboration_comment(id),  -- Thread

    author_id       UUID        NOT NULL REFERENCES user_account(id),
    content         TEXT        NOT NULL,
    content_format  VARCHAR(16) DEFAULT 'markdown',

    -- @-Mentions (werden für Benachrichtigungen ausgewertet)
    mentioned_user_ids UUID[]   DEFAULT '{}',

    is_resolved     BOOLEAN     NOT NULL DEFAULT FALSE,
    is_deleted      BOOLEAN     NOT NULL DEFAULT FALSE,

    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at      TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_comment_process  ON collaboration_comment(process_id);
CREATE INDEX idx_comment_author   ON collaboration_comment(author_id);
```

---

### AUDIT_LOG

```sql
-- Unveränderlicher Audit-Trail für alle Systemaktionen (Compliance, eAkte)
CREATE TIMESERIES audit_log (
    id              UUID        NOT NULL DEFAULT gen_uuid(),
    process_id      UUID        NOT NULL,
    node_id         UUID,
    actor_id        UUID        NOT NULL,  -- Benutzer, der die Aktion ausgeführt hat
    actor_role      VARCHAR(64),

    action          VARCHAR(64) NOT NULL,
    -- PROCESS_CREATED | NODE_STATUS_CHANGED | NODE_ASSIGNED |
    -- DOCUMENT_UPLOADED | COMMENT_ADDED | DECISION_MADE |
    -- SLA_EXTENDED | ESCALATED | EXPORTED | ARCHIVED

    old_value       JSONB,
    new_value       JSONB,

    ip_address      INET,
    user_agent      TEXT,

    recorded_at     TIMESTAMPTZ NOT NULL DEFAULT NOW()
) PARTITION BY RANGE(recorded_at)
  RETENTION PERIOD 10 YEARS;  -- Aufbewahrungsfrist gemäß Archivgesetz
```

---

## 🔍 AQL-Beispiele

### Aktive Aufgaben eines Sachbearbeiters

```aql
FOR node IN process_node
  FILTER node.assignee_id == @user_id
    AND node.status IN ['AKTIV', 'IN_BEARBEITUNG']
  LET proc = FIRST(FOR p IN process FILTER p.id == node.process_id RETURN p)
  LET days_remaining = DATE_DIFF(node.deadline, NOW(), 'days')
  SORT days_remaining ASC
  RETURN {
    node_id:          node.id,
    node_title:       node.title,
    process_id:       proc.id,
    reference_number: proc.reference_number,
    process_type:     proc.process_type,
    status:           node.status,
    deadline:         node.deadline,
    days_remaining:   days_remaining,
    is_overdue:       days_remaining < 0,
    priority:         node.priority
  }
```

### SLA-Überwachung (Team-Übersicht)

```aql
FOR p IN process
  FILTER p.authority_id == @authority_id
    AND p.status NOT IN ['GENEHMIGT', 'ABGELEHNT', 'ARCHIVIERT']
  LET sla = FIRST(FOR s IN sla_definition FILTER s.id == p.sla_definition_id RETURN s)
  LET days_until_deadline = DATE_DIFF(p.deadline, NOW(), 'days')
  LET sla_status =
    CASE
      WHEN days_until_deadline < 0               THEN 'VERLETZT'
      WHEN days_until_deadline <= sla.warning_threshold_days THEN 'RISIKO'
      ELSE 'ON_TRACK'
    END
  COLLECT sla_status_group = sla_status WITH COUNT INTO count
  RETURN { sla_status: sla_status_group, count: count }
```

### Ähnlichkeitssuche (Vector-Search für KI-Insights)

```aql
LET query_embedding = @process_embedding  -- Embedding des aktuellen Prozesses
FOR p IN process
  FILTER p.id != @current_process_id
    AND p.process_type == @process_type
    AND p.status IN ['GENEHMIGT', 'ABGELEHNT']
  LET similarity = SIMILARITY(p.embedding, query_embedding)
  FILTER similarity > 0.75
  SORT similarity DESC
  LIMIT 10
  RETURN {
    id:               p.id,
    reference_number: p.reference_number,
    status:           p.status,
    similarity_score: similarity,
    completed_at:     p.completed_at,
    metadata:         p.metadata
  }
```

### Geo-basierte Zuständigkeitsabfrage

```aql
LET building_location = [9.1829, 48.7758]  -- [lon, lat] Stuttgart Mitte
FOR d IN district
  FILTER CONTAINS_GEO(d.boundary_polygon, building_location)
LET authority = FIRST(FOR a IN authority FILTER a.district_id == d.id RETURN a)
RETURN {
  district_name:  d.name,
  authority_name: authority.name,
  authority_id:   authority.id,
  contact_email:  authority.contact_email
}
```

---

## 🕸️ Graph-Traversal-Queries

### Alle Knoten eines Prozesses traversieren

```aql
-- Tiefensuche ab Start-Knoten eines Prozesses
GRAPH TRAVERSE
  FROM (SELECT id FROM process_node WHERE process_id = @proc_id AND node_type = 'START_EVENT')
  EDGES process_edge (WHERE edge_type = 'SEQUENCE_FLOW')
  VERTICES process_node
  MAX_DEPTH 20
  DIRECTION OUTBOUND
RETURN {
  node_id:     vertex.id,
  title:       vertex.title,
  node_type:   vertex.node_type,
  status:      vertex.status,
  depth:       depth,
  path:        path
}
```

### Kritischer Pfad (längster Pfad zum End-Knoten)

```aql
-- Findet den Pfad mit der längsten Gesamtdauer (Bottleneck-Analyse)
GRAPH ALL_PATHS
  FROM (SELECT id FROM process_node
        WHERE process_id = @proc_id AND node_type = 'START_EVENT')
  TO   (SELECT id FROM process_node
        WHERE process_id = @proc_id AND node_type = 'END_EVENT')
  EDGES process_edge
  VERTICES process_node
LET path_duration = SUM(path.vertices[*].expected_duration_days)
SORT path_duration DESC
LIMIT 1
RETURN {
  critical_path: path.vertices[*].title,
  total_duration_days: path_duration,
  bottleneck_node: path.vertices[ARGMAX(path.vertices[*].expected_duration_days)]
}
```

### Abhängigkeitsnetzwerk zwischen Prozessen

```aql
-- Finde alle Prozesse, die von einem Hauptprozess abhängen (direkt + indirekt)
GRAPH TRAVERSE
  FROM @root_process_id
  EDGES (SELECT * FROM process_edge WHERE edge_type = 'RELATED_TO')
  VERTICES process
  MAX_DEPTH 5
  DIRECTION OUTBOUND
RETURN {
  dependent_process_id: vertex.id,
  reference_number:     vertex.reference_number,
  status:               vertex.status,
  hop_distance:         depth
}
```

---

## 📊 Indexstrategie

| Tabelle | Index | Typ | Zweck |
|---------|-------|-----|-------|
| `process` | `(status, authority_id)` | Composite B-Tree | Dashboard-Abfragen |
| `process` | `geo_location` | GiST | Geo-Proximity-Suche |
| `process` | `embedding` | HNSW Vector | KI-Ähnlichkeitssuche |
| `process` | `(process_type, status)` | Composite B-Tree | Portfolio-Filter |
| `process_node` | `(process_id, status)` | Composite B-Tree | Prozess-Status |
| `process_node` | `(assignee_id, status)` | Composite B-Tree | Meine Aufgaben |
| `process_edge` | `(source_node_id, target_node_id)` | Composite B-Tree | Graph-Traversal |
| `document_attachment` | `embedding` | HNSW Vector | Dokument-Suche |
| `audit_log` | `(process_id, recorded_at)` | Timeseries-Partition | Audit-Abruf |
| `sla_events` | `(process_id, event_type)` | Composite B-Tree | SLA-Monitoring |

---

## 🔄 Schema-Migration

### Von linearem Status-Modell zu Graph-Modell

```sql
-- Phase 1: Neue Tabellen hinzufügen (non-breaking)
-- (Alle oben definierten CREATE TABLE Statements)

-- Phase 2: Für existierende Prozesse Graph-Repräsentation generieren
INSERT INTO process_node (process_id, node_type, title, status)
SELECT
    p.id,
    'USER_TASK',
    'Migrierter Schritt: ' || p.status,
    CASE p.status
        WHEN 'EINGEREICHT'    THEN 'ABGESCHLOSSEN'
        WHEN 'IN_PRÜFUNG'     THEN 'AKTIV'
        WHEN 'GENEHMIGT'      THEN 'ABGESCHLOSSEN'
        WHEN 'ABGELEHNT'      THEN 'ABGESCHLOSSEN'
        ELSE 'ABGESCHLOSSEN'
    END
FROM process p
WHERE NOT EXISTS (SELECT 1 FROM process_node pn WHERE pn.process_id = p.id);

-- Phase 3: Alte status-Spalte als deprecated markieren (nicht löschen)
COMMENT ON COLUMN process.status IS
  'DEPRECATED: Use process_execution.active_node_ids + process_node.status instead.
   Kept for backward compatibility until v2.0.0';
```

---

*Siehe auch:*
- [`docs/de/architecture/DMS_MODERN_ARCHITECTURE.md`](../architecture/DMS_MODERN_ARCHITECTURE.md) – Architekturüberblick
- [`docs/de/integration/KI_ML_INTEGRATION_GUIDE.md`](../integration/KI_ML_INTEGRATION_GUIDE.md) – KI-Integration
- [`docs/de/aql/README.md`](../aql/README.md) – AQL-Referenz
