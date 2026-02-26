# Implementierungs-Roadmap – Themis.DocumentManager Modern DMS

**Kategorie:** 📋 Roadmap  
**Version:** v1.0.0  
**Status:** 📋 Planung  
**Datum:** Februar 2026

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#-übersicht)
- [Abhängigkeiten zu aktuellen Komponenten](#-abhängigkeiten-zu-aktuellen-komponenten)
- [Phase 1: Foundation & Data Model](#-phase-1-foundation--data-model)
- [Phase 2: Core Process Engine](#-phase-2-core-process-engine)
- [Phase 3: UI/UX-Layer](#-phase-3-uiux-layer)
- [Phase 4: KI/ML-Integration](#-phase-4-kiml-integration)
- [Phase 5: Collaboration & Real-time](#-phase-5-collaboration--real-time)
- [Phase 6: Hardening & Production](#-phase-6-hardening--production)
- [Migrations-Strategie](#-migrations-strategie)
- [Ressourcenplanung](#-ressourcenplanung)
- [Risiken & Mitigationen](#-risiken--mitigationen)

---

## 🎯 Übersicht

Diese Roadmap beschreibt die schrittweise Transformation des Themis.DocumentManager von einem "einfachen Document Browser" zu einem **modernen, KI-gestützten, Graph-basierten DMS** für komplexe Verwaltungsprozesse.

### Zeitplan-Übersicht

```
2026                Q2              Q3              Q4
│──────────────────┤───────────────┤───────────────┤──────────────
│ Phase 1          │ Phase 2       │ Phase 3       │
│ Foundation       │ Process       │ UI/UX         │
│ (8 Wochen)       │ Engine (10W)  │ (10 Wochen)   │
│                  │               │               │ Phase 4-6
│                  │               │               │ KI + Collab
│                  │               │               │ (parallel, Q4)

MVP (Phase 1+2): Mitte Q3 2026
Beta (Phase 1-3): Ende Q4 2026
Production (alle Phasen): Q2 2027
```

### Ziel-Metriken

| Metrik | Aktuell | Ziel (Phase 6) |
|--------|---------|---------------|
| Max. unterstützte Prozesstypen | ~5 linear | Unbegrenzt (Template-basiert) |
| Parallele Workstreams pro Fall | 0 | 10+ |
| Durchschn. SLA-Einhaltungsrate | k.A. | ≥ 95% |
| KI-Klassifizierungs-Genauigkeit | 0% | ≥ 90% |
| Gleichzeitige Nutzer | k.A. | 500+ |
| Antwortzeit (Dashboard) | k.A. | < 200ms |

---

## 🔗 Abhängigkeiten zu aktuellen Komponenten

### Themis.DocumentManager – Bestehende Komponenten

| Komponente | Status | Verwendung im neuen DMS |
|-----------|--------|------------------------|
| **Timeline-View** | ✅ Vorhanden | Erweitern zu `TimelineWithBranches` (parallele Pfade) |
| **Gantt-View** | ✅ Vorhanden | Direkt wiederverwendbar für Portfolio-View |
| **Graph-View** | ✅ Vorhanden | Basis für `ProcessGraphCanvas` (BPMN-Erweiterung) |
| **Geo-View** | ✅ Vorhanden | Direkt wiederverwendbar (GeoMiniMap-Wrapper) |
| **ERD-View** | ✅ Vorhanden | Für Schema-Visualisierung in Admin-Bereich |
| **Query Editor** | ✅ Vorhanden | Basis für `NLQueryInterface` (LLM-Layer drüber) |
| **LLM-Integration** | ✅ llama.cpp | Direkt nutzbar für alle KI-Features |

### ThemisDB Core – Benötigte Features

| Feature | Status | Benötigt für |
|---------|--------|-------------|
| **Graph-Traversal AQL** | ✅ Implementiert | Prozess-Graph-Queries |
| **Vector-Search (HNSW)** | ✅ Implementiert | KI-Ähnlichkeitssuche |
| **Geo-Queries (PROXIMITY, CONTAINS_GEO)** | ✅ Implementiert | Zuständigkeits-Routing |
| **Timeseries** | ✅ Implementiert | SLA-Tracking, Audit-Log |
| **ACID Multi-Model** | ✅ Implementiert | Konsistente Prozess-Updates |
| **WebSocket/Push** | ⚠️ Zu prüfen | Real-time Collaboration |
| **Row-Level Security** | ⚠️ Zu prüfen | Mandanten-Trennung |

---

## 📐 Phase 1: Foundation & Data Model

**Dauer:** 8 Wochen  
**Ziel:** Solides Datenbankschema und Backend-Foundation

### Aufgaben

- [ ] Datenbankschema implementieren (`THEMIS_DMS_DATA_MODEL.md`)
  - [ ] `process` Tabelle mit Geo + Vector Feldern
  - [ ] `process_node` und `process_edge` Tabellen
  - [ ] `process_execution` (Laufzeit-Zustand)
  - [ ] `process_template` und Template-Nodes/Edges
  - [ ] `sla_definition` und `sla_events` (Timeseries)
  - [ ] `document_attachment` mit Embedding-Spalte
  - [ ] `ki_prediction` Tabelle
  - [ ] `collaboration_comment` Tabelle
  - [ ] `audit_log` Timeseries
- [ ] Indizes erstellen (HNSW, GiST, Composite B-Trees)
- [ ] Initiale Prozess-Templates für häufige Verwaltungstypen
  - [ ] `BAUGENEHMIGUNG_NEUBAU`
  - [ ] `BAUGENEHMIGUNG_UMBAU`
  - [ ] `GEWERBEERLAUBNIS`
  - [ ] `WOHNRAUMGENEHMIGUNG`
  - [ ] `BESCHWERDE_ALLGEMEIN`
- [ ] REST-API für Kern-CRUD-Operationen (Process, Node, Edge, Document)
- [ ] Migrations-Script: Bestehende Prozesse → neues Graph-Schema
- [ ] Unit-Tests für Datenmodell-Constraints
- [ ] Datenbankschema-Dokumentation aktualisieren

**Akzeptanzkriterien:**
- AQL-Queries aus `THEMIS_DMS_DATA_MODEL.md` laufen korrekt
- Migrations-Script migriert 100% der bestehenden Datensätze verlustfrei
- Alle Tabellen haben vollständige Indizes und FK-Constraints

---

## ⚙️ Phase 2: Core Process Engine

**Dauer:** 10 Wochen  
**Ziel:** Funktionierende Prozess-Ausführungsengine (BPMN-kompatibel)

### Aufgaben

#### Prozess-Execution-Engine
- [ ] BPMN-Token-Semantik implementieren
  - [ ] Start-Event: Token erzeugen
  - [ ] Task: Token konsumieren + neues Token nach Abschluss
  - [ ] XOR-Gateway: Token-Routing basierend auf Bedingung
  - [ ] AND-Gateway: Token-Split (parallele Ausführung) und Token-Join (Synchronisation)
  - [ ] OR-Gateway: Mindestens-ein-Pfad Semantik
  - [ ] End-Event: Token konsumieren, Prozess abschließen
- [ ] Bedingungsauswertung (Condition Expression Engine)
  - [ ] Einfache Vergleiche: `metadata.floor_area > 200`
  - [ ] Logische Verknüpfungen: AND, OR, NOT
  - [ ] AQL-basierte Bedingungen (für komplexe Fälle)
- [ ] Dynamische Prozessanpassung zur Laufzeit
  - [ ] Neuen Knoten einfügen (z.B. Nachforderung)
  - [ ] Neuen Pfad aktivieren (z.B. Eskalationspfad)
  - [ ] Knoten-Reassignment

#### Process-Management-API
- [ ] `POST /processes` – Prozess aus Template instanziieren
- [ ] `GET /processes/:id/graph` – Vollständiger Prozess-Graph
- [ ] `POST /processes/:id/nodes/:nodeId/complete` – Knoten abschließen
- [ ] `POST /processes/:id/nodes/:nodeId/request-info` – Nachforderung
- [ ] `POST /processes/:id/nodes/:nodeId/reassign` – Neu zuweisen
- [ ] `POST /processes/:id/nodes/:nodeId/escalate` – Eskalieren
- [ ] `GET /processes` – Liste mit Filter/Sort/Pagination
- [ ] WebSocket-Endpoint für Echtzeit-Status-Updates

#### SLA-Engine
- [ ] SLA-Deadline-Berechnung (Werktage, Feiertage)
- [ ] Automatische SLA-Warnungen (Cronjob: täglich)
- [ ] SLA-Verletzungs-Protokollierung in `sla_events`

#### Tests
- [ ] Unit-Tests für Token-Semantik (alle Gateway-Typen)
- [ ] Integrations-Tests für komplette Prozessdurchläufe
- [ ] Performance-Tests: 1000 gleichzeitige Prozesse

**Akzeptanzkriterien:**
- Alle BPMN-Gateway-Typen korrekt implementiert
- Szenario 1 (parallele Gutachten) läuft korrekt durch
- SLA-Warnungen werden rechtzeitig ausgelöst

---

## 🎨 Phase 3: UI/UX-Layer

**Dauer:** 10 Wochen  
**Ziel:** Vollständiges, responsives Frontend nach `SCREEN_LAYOUTS.md`

### Aufgaben

#### Layout 1: Process Execution View
- [ ] 3-Spalten-Layout (Kontext + Graph + Aufgaben)
- [ ] `ProcessGraphCanvas` (SVG-basiert, interaktiv)
  - [ ] BPMN-Knotentypen als unterschiedliche Shapes
  - [ ] Status-basierte Farbkodierung
  - [ ] Zoom/Pan
  - [ ] Klick/Doppelklick/Rechtsklick-Interaktion
- [ ] `ProcessNodeCard` mit `ProcessActionPanel`
- [ ] `SLACountdown` mit Warnzuständen
- [ ] `DocumentPreview` (PDF-Viewer integriert)
- [ ] `CommentThread` (Basis-Version)

#### Layout 2: Process Portfolio View
- [ ] Kanban-Ansicht (Drag-Drop zwischen Status-Spalten)
- [ ] Listen-Ansicht (sortierbar, filterbar)
- [ ] Gantt-Ansicht (bestehende Komponente erweitern)
- [ ] Filter-Panel (Typ, Status, Sachbearbeiter, Zeitraum)
- [ ] `SLADashboard`-Widget

#### Layout 3: Graph Explorer
- [ ] Globale Knowledge-Graph-Visualisierung
- [ ] Filter für Knotentypen, Status, Zeitraum
- [ ] Cluster-Erkennung und -Zusammenfassung
- [ ] Path-Finder (zwei Knoten → kürzester Pfad)
- [ ] Heatmap-Overlay (Verweildauer)

#### Layout 4: Batch Operations
- [ ] Selektions-Panel mit Kriterien-Builder
- [ ] Vorschau-Liste mit Paginierung
- [ ] Batch-Aktions-Auswahl + Bestätigung (2FA für kritische Aktionen)
- [ ] `BatchProgressIndicator`

#### Navigation
- [ ] `AdaptiveSidebar` (Desktop/Tablet/Mobile)
- [ ] Responsive Breakpoints (alle 6 Stufen)
- [ ] Keyboard-Navigation + Shortcuts
- [ ] WCAG 2.1 AA Compliance-Check

**Akzeptanzkriterien:**
- Alle 4 Layouts funktional und responsiv
- Barrierefreiheits-Audit bestanden (axe-core oder Lighthouse)
- User-Test mit 3 Sachbearbeitern aus echten Behörden

---

## 🤖 Phase 4: KI/ML-Integration

**Dauer:** 8 Wochen (parallel zu Phase 5)  
**Ziel:** Alle KI-Features aus `KI_ML_INTEGRATION_GUIDE.md`

### Aufgaben

#### Dokumenten-Klassifizierung
- [ ] LLM-Prompt-Template für Dokumenten-Klassifizierung
- [ ] OCR-Pipeline für Scans (Tesseract oder PaddleOCR)
- [ ] Vollständigkeitsprüfung nach Prozesstyp
- [ ] `AIInsightsPanel` im UI

#### Prozessschritt-Empfehlung
- [ ] Ähnlichkeitssuche via HNSW (Vector-Search)
- [ ] Approval-Probability-Score (Gradient Boosting)
- [ ] Initialer Trainings-Datensatz aus Demo-Daten
- [ ] `PredictionScore`-Widget

#### Assignee-Optimierung
- [ ] Workload-Berechnungs-API
- [ ] Kompetenz-Matching (process_type vs. user.competencies)
- [ ] Vorschlags-Integration im `ProcessActionPanel`

#### Natural Language Query
- [ ] Text-zu-AQL Prompt-Template
- [ ] AQL-Sicherheits-Validator
- [ ] `NLQueryInterface`-Komponente
- [ ] Follow-up-Fragen-Unterstützung

#### Anomalieerkennung
- [ ] Z-Score-basierte Zeitreihen-Anomalieerkennung
- [ ] `AnomalyAlert`-Komponente
- [ ] Konfigurierbare Schwellenwerte je Prozesstyp

#### Tests
- [ ] Klassifizierungs-Genauigkeit: ≥ 90% auf Test-Datensatz
- [ ] NLQ-Korrektheit: ≥ 85% korrekte AQL-Generierung auf Test-Set
- [ ] Latenz-Tests: LLM-Antwort < 3s (8B-Modell, Q4)

---

## 💬 Phase 5: Collaboration & Real-time

**Dauer:** 6 Wochen (parallel zu Phase 4)  
**Ziel:** Real-time-Collaboration-Features

### Aufgaben

- [ ] WebSocket-Server für Echtzeit-Events
  - [ ] `PROCESS_NODE_STATUS_CHANGED`
  - [ ] `COMMENT_ADDED`
  - [ ] `ASSIGNMENT_CHANGED`
  - [ ] `DOCUMENT_UPLOADED`
- [ ] `CollaborationPresence` (Online-Indikatoren)
- [ ] `MentionSelector` (@-Mentions mit Autocomplete)
- [ ] `NotificationCenter` mit Prioritäts-Filterung
- [ ] Push-Benachrichtigungen (Browser-Push API)
- [ ] Konflikt-Auflösung (Optimistic Locking für Knotenstatus)
- [ ] E-Mail-Benachrichtigungen (Digest + Sofort-Alerts)

---

## 🔒 Phase 6: Hardening & Production

**Dauer:** 8 Wochen  
**Ziel:** Production-Readiness, Sicherheit, Performance

### Aufgaben

#### Sicherheit
- [ ] Row-Level-Security (Mandanten-Trennung)
- [ ] OAuth2/OIDC Integration (Keycloak / SAML für Behörden)
- [ ] Audit-Log Signing (tamper-evident)
- [ ] Penetration-Test
- [ ] BSI IT-Grundschutz Compliance-Check

#### Performance
- [ ] Query-Optimierung (EXPLAIN ANALYZE für alle Haupt-Queries)
- [ ] Caching-Layer (häufige Dashboard-Daten)
- [ ] Graph-Visualisierung: Virtualisierung für > 100 Knoten
- [ ] Load-Testing: 500 gleichzeitige Benutzer

#### Betrieb & Monitoring
- [ ] Prometheus/Grafana-Dashboard für DMS-Metriken
- [ ] Health-Checks für alle Services (LLM, DB, API)
- [ ] Backup/Restore-Verfahren für Prozessdaten
- [ ] Disaster-Recovery-Dokumentation

#### Dokumentation
- [ ] Benutzerhandbuch (Sachbearbeiter, Supervisor, Admin)
- [ ] API-Dokumentation (OpenAPI 3.1)
- [ ] Betriebsdokumentation (Installation, Konfiguration, Update)

---

## 🔄 Migrations-Strategie

### Von "Simple Linear" zu "Complex Graph" Model

Die Migration erfolgt in 4 Stufen ohne Downtime:

#### Stufe 1: Parallel-Betrieb (Phase 1)
```
Bestehende Prozesse:  lineares Status-Modell (weiterhin lesbar)
Neue Prozesse:        direkt im Graph-Modell
Migration:            Läuft im Hintergrund (low-priority)
```

#### Stufe 2: Backward-Compatible Schema (Phase 1)
```sql
-- Status-Feld bleibt erhalten (deprecated, aber lesbar)
-- Neues Graph-Schema wird parallel befüllt
-- Trigger: Bei jeder Status-Änderung → auch Graph aktualisieren
```

#### Stufe 3: Graph als Primary Source of Truth (Phase 2)
```
Ab Phase 2:  Alle neuen Writes gehen primär in Graph-Tabellen
            lineares status-Feld wird calculated/derived
Lesekompatibilität:  status-Feld wird aus Graph abgeleitet:
    status = active_node ? 'IN_BEARBEITUNG' : completion_status
```

#### Stufe 4: Deprecation (Phase 6)
```
lineares status-Feld: READ-ONLY, scheduled for removal in v3.0.0
Alle Clients müssen auf Graph-API migriert sein
```

### Datenmigrations-Script

```sql
-- Erstelle minimalen Graph-Repräsentation für alle legacy-Prozesse
INSERT INTO process_node (process_id, node_type, title, status, created_at)
SELECT
    p.id AS process_id,
    'USER_TASK' AS node_type,
    'Migrierter Hauptschritt' AS title,
    CASE p.legacy_status
        WHEN 'EINGEREICHT'   THEN 'AKTIV'
        WHEN 'IN_PRÜFUNG'    THEN 'IN_BEARBEITUNG'
        WHEN 'GENEHMIGT'     THEN 'ABGESCHLOSSEN'
        WHEN 'ABGELEHNT'     THEN 'ABGESCHLOSSEN'
        ELSE 'AKTIV'
    END AS status,
    p.created_at
FROM process p
WHERE NOT EXISTS (
    SELECT 1 FROM process_node pn WHERE pn.process_id = p.id
)
ON CONFLICT DO NOTHING;

-- Erstelle START und END Events
INSERT INTO process_node (process_id, node_type, title, status, created_at)
SELECT
    p.id, 'START_EVENT', 'Antrag eingereicht', 'ABGESCHLOSSEN', p.submitted_at
FROM process p
WHERE NOT EXISTS (
    SELECT 1 FROM process_node pn
    WHERE pn.process_id = p.id AND pn.node_type = 'START_EVENT'
);
```

---

## 👥 Ressourcenplanung

### Team-Zusammensetzung

| Rolle | Anzahl | Phasen | Verantwortlichkeit |
|-------|--------|--------|-------------------|
| Backend-Entwickler (ThemisDB/C++) | 2 | 1, 2, 6 | Process Engine, DB-Schema, API |
| Frontend-Entwickler | 2 | 3, 5 | UI-Komponenten, Real-time |
| KI/ML-Ingenieur | 1 | 4 | LLM-Integration, ML-Modelle |
| UX-Designer | 1 | 3 | UI-Design, User-Testing |
| DevOps/SRE | 1 | 6 | Deployment, Monitoring |
| QA-Ingenieur | 1 | 2–6 | Tests, Barrierefreiheit |

### Hardware (Empfohlen, Entwicklung + Staging)

```yaml
development:
  ram: 32GB
  cpu: 8-core
  gpu: RTX 4090 (für LLM-Entwicklung)
  storage: 500GB NVMe

staging:
  ram: 64GB
  cpu: 16-core
  gpu: A10G oder RTX 4090
  storage: 2TB NVMe

production:
  ram: 128GB
  cpu: 32-core
  gpu: 2× A100 80GB (LLM 70B + Embedding parallel)
  storage: 10TB NVMe RAID
```

---

## ⚠️ Risiken & Mitigationen

| Risiko | Wahrscheinlichkeit | Impact | Mitigation |
|--------|-------------------|--------|-----------|
| **LLM-Performance on-premise unzureichend** | Mittel | Hoch | 8B-Modell als Fallback; KI-Features optional |
| **Migration bestehender Prozesse verlustreich** | Niedrig | Hoch | Dry-Run-Migration auf Kopie vor Produktion |
| **BPMN-Engine-Komplexität unterschätzt** | Mittel | Mittel | Scope Phase 2 auf Core-Features; OR-Gateway später |
| **WebSocket-Skalierung bei 500+ Nutzern** | Mittel | Mittel | Redis PubSub als Backend; Load-Balancer |
| **Datenschutz-Anforderungen (OZG, DSGVO)** | Niedrig | Sehr hoch | Rechtliche Review in Phase 6; BSI-Konformität |
| **User-Acceptance im Verwaltungskontext** | Mittel | Hoch | Frühzeitige User-Tests mit echten Sachbearbeitern |
| **Bias in KI-Routing-Empfehlungen** | Mittel | Hoch | Fairness-Audit in Phase 4; monatliches Monitoring |

---

## 🔗 Verknüpfte Dokumente

| Dokument | Relevanz |
|----------|----------|
| [`docs/de/architecture/DMS_MODERN_ARCHITECTURE.md`](architecture/DMS_MODERN_ARCHITECTURE.md) | Architektur-Grundlagen |
| [`docs/de/design/THEMIS_DMS_DATA_MODEL.md`](design/THEMIS_DMS_DATA_MODEL.md) | Datenmodell (Phase 1) |
| [`docs/de/design/SCREEN_LAYOUTS.md`](design/SCREEN_LAYOUTS.md) | UI-Spezifikation (Phase 3) |
| [`docs/de/design/COMPONENTS_AND_WIDGETS.md`](design/COMPONENTS_AND_WIDGETS.md) | Komponentenliste (Phase 3) |
| [`docs/de/design/WORKFLOW_INTERACTION_PATTERNS.md`](design/WORKFLOW_INTERACTION_PATTERNS.md) | User Journeys (Phase 3) |
| [`docs/de/integration/KI_ML_INTEGRATION_GUIDE.md`](integration/KI_ML_INTEGRATION_GUIDE.md) | KI-Features (Phase 4) |
| [`examples/modern_dms_workflow_scenarios.md`](../../examples/modern_dms_workflow_scenarios.md) | Konkrete Szenarien |

---

## Production Readiness Checklist

### Phase 1–2 (MVP)
- [ ] Alle Kern-Tabellen erstellt und indiziert
- [ ] BPMN XOR/AND Gateway implementiert und getestet
- [ ] SLA-Engine läuft
- [ ] REST-API vollständig dokumentiert (OpenAPI)
- [ ] Migrations-Script getestet auf Produktions-Kopie

### Phase 3 (Beta)
- [ ] Alle 4 Layouts funktional
- [ ] WCAG 2.1 AA Barrierefreiheits-Audit bestanden
- [ ] User-Acceptance-Test mit ≥ 3 Verwaltungsmitarbeitern
- [ ] Mobile-Layouts getestet auf iOS + Android

### Phase 4–5 (KI+Collab)
- [ ] KI-Klassifizierung ≥ 90% Accuracy auf Test-Set
- [ ] NLQ korrekte Antworten ≥ 85%
- [ ] WebSocket funktioniert bei 100+ gleichzeitigen Verbindungen
- [ ] Alle KI-Entscheidungen werden im Audit-Log protokolliert

### Phase 6 (Production)
- [ ] Penetration-Test bestanden
- [ ] Load-Test: 500 gleichzeitige Nutzer ohne Degradation
- [ ] Backup/Restore-Prozedur dokumentiert und getestet
- [ ] BSI IT-Grundschutz Basis-Compliance
- [ ] Benutzerhandbücher für alle Rollen fertig
