# 🏗️ AI Safety Layer — Vollständige Sicherheitsarchitektur

> **Version:** 1.0.0 | **Status:** Geplant / Implementierung Phase 1–4 |
> **Verantwortlich:** Security Engineering Team

## Einleitung: Der Cursor-Vorfall als Lehrbeispiel

Im April 2026 wurde berichtet, dass ein KI-Agent des Cursor-Editors versehentlich eine
Produktionsdatenbank vollständig löschte. Die Ursachenanalyse ergab drei strukturelle Lücken:

1. **Kein Approval-Gate**: Destruktive Operationen wurden ohne Bestätigung ausgeführt
2. **Kein Umgebungsbewusstsein**: Keine Unterscheidung Produktion vs. Entwicklung
3. **Kein automatischer Checkpoint**: Kein Rollback-Punkt vor der Operation

ThemisDB adressiert alle drei Lücken — plus vier weitere — mit einem
**7-Schichten AI Safety Layer**.

---

## Architekturüberblick

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         AI Safety Layer — ThemisDB                          │
│                                                                             │
│   KI-Agent (Cursor/Claude/GPT/Custom)                                       │
│        │                                                                    │
│        ▼                                                                    │
│   ┌──────────────────────────────────────────────────────────────────────┐  │
│   │  MCP Server / AI Orchestrator / LLM API Handler                     │  │
│   └──────────────────────────┬───────────────────────────────────────────┘  │
│                              │                                              │
│        ┌─────────────────────▼────────────────────────┐                    │
│        │          🔍 Schicht 4: IntentClassifier        │ AQL-Awareness     │
│        │   SQL_INJECTION | DATA_EXFILTRATION |          │ DATA_DESTRUCTION  │
│        │   PRIVILEGE_ESCALATION | DATA_DESTRUCTION |    │ SCHEMA_MUTATION   │
│        │   SCHEMA_MUTATION                              │                   │
│        └────────────────────┬─────────────────────────┘                    │
│                             │                                               │
│        ┌────────────────────▼─────────────────────────┐                    │
│        │     🛡️ Schicht 1: Destructive Operation Guard  │                   │
│        │   READ_ONLY | WRITE_SAFE | DESTRUCTIVE | CRITICAL                 │
│        └────────────┬───────────────────┬─────────────┘                    │
│                     │                   │                                   │
│               READ_ONLY          DESTRUCTIVE / CRITICAL                     │
│               WRITE_SAFE                │                                   │
│                     │         ┌─────────▼──────────────┐                   │
│                     │         │  🔒 Schicht 3: AQL      │                   │
│                     │         │  Read-Only Enforcer     │                   │
│                     │         │  (für aql_execute-Mode) │                   │
│                     │         └─────────┬──────────────┘                   │
│                     │                   │                                   │
│                     │         ┌─────────▼──────────────┐                   │
│                     │         │  🌍 Schicht 6: Env.     │                   │
│                     │         │  Isolation Guard        │                   │
│                     │         │  production → BLOCK?    │                   │
│                     │         └─────────┬──────────────┘                   │
│                     │                   │                                   │
│                     │         ┌─────────▼──────────────┐                   │
│                     │         │  🤝 Schicht 2: HILG     │                   │
│                     │         │  Human-in-the-Loop Gate │                   │
│                     │         │  → requires_approval    │                   │
│                     │         └─────────┬──────────────┘                   │
│                     │                   │ (nach Approval)                   │
│                     │         ┌─────────▼──────────────┐                   │
│                     │         │  📸 Schicht 5: Pre-Op   │                   │
│                     │         │  Snapshot (Checkpoint)  │                   │
│                     │         └─────────┬──────────────┘                   │
│                     │                   │                                   │
│        ┌────────────▼───────────────────▼─────────────┐                    │
│        │           ⚙️ Datenbankoperation ausführen      │                   │
│        └────────────────────────┬─────────────────────┘                    │
│                                 │                                           │
│        ┌────────────────────────▼─────────────────────┐                    │
│        │       📋 Schicht 7: AI Session Audit Trail    │                    │
│        │   AI_TOOL_CALL | APPROVAL_REQUIRED |          │                    │
│        │   APPROVAL_GRANTED | OPERATION_EXECUTED       │                    │
│        └──────────────────────────────────────────────┘                    │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Schicht 1: Destructive Operation Guard (DOG)

**Datei:** `include/security/ai_operation_guard.h` | `src/security/ai_operation_guard.cpp`

### Operationsklassen

| Klasse | Beschreibung | Beispiele |
|---|---|---|
| `READ_ONLY` | Sichere Leseoperation | `GET entity`, `SELECT`, `FOR x IN col RETURN x` |
| `WRITE_SAFE` | Einzelschreib-/Update-Operation | `PUT entity`, `INSERT` (key angegeben) |
| `DESTRUCTIVE` | Löschung von Datensätzen | `REMOVE @key IN col`, `DELETE entity` |
| `CRITICAL` | Vollbereichsoperation oder Schema-Mutation | `FOR x IN col REMOVE x IN col`, `DROP COLLECTION` |

### Klassifikationsregeln (AQL)

```
AQL-Keyword-Analyse (case-insensitive, tokenisiert):

REMOVE           → mindestens DESTRUCTIVE
DROP COLLECTION  → CRITICAL
DROP INDEX       → DESTRUCTIVE
CREATE COLLECTION→ WRITE_SAFE (reversibel)
TRUNCATE         → CRITICAL
INSERT           → WRITE_SAFE (wenn key explizit)
INSERT + UPSERT  → WRITE_SAFE
UPDATE + FILTER  → WRITE_SAFE
REPLACE          → WRITE_SAFE

Spezialregeln:
• "FOR x IN col REMOVE x IN col" ohne FILTER → CRITICAL (Vollbereich!)
• "FOR x IN col REMOVE x IN col FILTER ..."  → DESTRUCTIVE (begrenzt)
• Drop auf System-Collections (_system, _graphs) → CRITICAL
```

### Integration

```cpp
// McpServer::toolDeleteEntity(), toolDropIndex(), toolQuery()
AiOperationGuard guard(config_);
auto [op_class, preview] = guard.classify(operation);

if (op_class >= OperationClass::DESTRUCTIVE) {
    return guard.requiresApproval(op_class, operation, preview);
}
// ... normale Ausführung
```

**Details:** [Destructive Operation Guard](AI_SAFETY_OPERATION_GUARD.md)

---

## Schicht 2: Human-in-the-Loop Gate (HILG)

**Implementiert in:** `src/server/mcp_server.cpp` | `src/server/http_server.cpp`

### Approval-Flow

```
KI-Agent                    McpServer                  Operator
    │                           │                          │
    │── tools/call ────────────>│                          │
    │   delete_entity            │                          │
    │                           │──classify──> DESTRUCTIVE │
    │                           │                          │
    │<──requires_approval───────│                          │
    │   {                       │                          │
    │     "status": "requires_approval",                   │
    │     "operation_id": "op-uuid",                       │
    │     "preview": {...},                                │
    │     "impact_estimate": {...},                        │
    │     "expires_at": "2026-04-28T07:15:00Z"            │
    │   }                       │                          │
    │                           │──Notification──────────>│
    │                           │  (Slack/Mail/Dashboard)  │
    │                           │                          │
    │                           │<──POST /v1/ai/approve/{id}│
    │                           │   {"approved_by": "...",  │
    │                           │    "reason": "..."}       │
    │                           │                          │
    │                           │──Snapshot────────────────│
    │                           │──Execute─────────────────│
    │                           │──Audit Log───────────────│
    │<──result──────────────────│                          │
```

### Approval-Token-Eigenschaften

- **Format:** UUID v4 mit HMAC-SHA256-Signatur
- **TTL:** Konfigurierbar, Default 60 Sekunden
- **Einmalig:** Token wird nach Verwendung invalidiert
- **Nicht übertragbar:** Token ist an Session-ID und Operation-Hash gebunden

### API-Endpunkte

```
POST /v1/ai/approve/{operation_id}
  Body: {"approved_by": "admin@example.com", "reason": "Maintenance window"}
  Response: {"status": "approved", "execution_id": "..."}

POST /v1/ai/deny/{operation_id}
  Body: {"denied_by": "...", "reason": "..."}
  Response: {"status": "denied"}

GET  /v1/ai/pending-approvals
  Response: [{"operation_id": "...", "tool": "...", "expires_at": "..."}]

POST /v1/ai/rollback/{snapshot_id}
  Body: {"confirmed_by": "...", "reason": "..."}
  Response: {"status": "rollback_initiated", "estimated_seconds": 30}
```

**Details:** [Destructive Operation Guard](AI_SAFETY_OPERATION_GUARD.md)

---

## Schicht 3: AQL Read-Only Enforcer

**Datei:** `include/query/aql_safety_validator.h` | `src/query/aql_safety_validator.cpp`

### Erkannte Mutationsoperationen

| AQL-Keyword | Kategorie | Blockiert |
|---|---|---|
| `INSERT` | DML-Write | Ja (im read-only Mode) |
| `UPDATE` | DML-Write | Ja |
| `REPLACE` | DML-Write | Ja |
| `REMOVE` | DML-Delete | Ja |
| `UPSERT` | DML-Write | Ja |
| `DROP COLLECTION` | DDL | Ja |
| `DROP INDEX` | DDL | Ja |
| `CREATE COLLECTION` | DDL | Ja |
| `TRUNCATE` | DDL | Ja |

### Aktivierung

Der Enforcer ist aktiv, wenn:
- Tool `aql_execute` aus dem Mode-Config aufgerufen wird (markiert als `read-only`)
- Mode-Flag `enforce_read_only: true` gesetzt ist

**Details:** [AQL Read-Only Enforcer](AI_SAFETY_AQL_VALIDATOR.md)

---

## Schicht 4: IntentClassifier — AQL-Erweiterung

**Datei:** `include/security/intent_classifier.h` | `src/security/intent_classifier.cpp`

### Neue IntentType-Werte

```cpp
enum class IntentType {
    LEGITIMATE,
    SQL_INJECTION,
    DATA_EXFILTRATION,
    PRIVILEGE_ESCALATION,
    ANOMALOUS_PATTERN,
    DATA_DESTRUCTION,   // NEU: AQL REMOVE / Bulk-Delete
    SCHEMA_MUTATION,    // NEU: DROP COLLECTION / TRUNCATE
};
```

### AQL-spezifische Feature-Gewichte

| Pattern | Gewicht | Rationale |
|---|---|---|
| `"FOR " + " REMOVE "` | 0.70 | Batch-Delete Pattern |
| `FOR...IN...REMOVE` ohne `FILTER` | 0.90 | Vollbereichs-Delete |
| `DROP COLLECTION` | 0.95 | Irreversibler Schema-Verlust |
| `TRUNCATE` | 0.80 | Irreversibler Datenverlust |
| `REMOVE @` (Bind-Var) | 0.40 | Parametrisiertes Delete (oft legitim) |
| `CREATE COLLECTION` | 0.10 | Meist legitim |
| `DROP INDEX` | 0.55 | Performance-Auswirkung |

**Blockierungsschwellenwert:** Confidence ≥ 0.65 → Operation abgelehnt

**Details:** [IntentClassifier AQL-Erweiterung](AI_SAFETY_INTENT_CLASSIFIER.md)

---

## Schicht 5: Pre-Operation Snapshot (POS)

**Verwendet:** `RocksDBWrapper::createCheckpoint()` | `AdminApiHandler`

### Ablauf

```
1. DOG klassifiziert Operation als DESTRUCTIVE oder CRITICAL
2. HILG erhält Approval vom Operator
3. POS erstellt Checkpoint: storage_->createCheckpoint(snapshot_dir)
4. Snapshot-ID und Pfad werden in Audit-Log geschrieben
5. MCP-Response enthält "pre_operation_snapshot" Pfad
6. Operation wird ausgeführt
7. Bei Fehler: Rollback-Endpoint verfügbar
```

### Konfiguration

```yaml
safety:
  auto_snapshot: true
  snapshot_dir: "/var/themis/ai-snapshots"
  snapshot_retention_days: 7      # Snapshots älter als 7 Tage werden bereinigt
  snapshot_max_size_gb: 100       # Maximale Snapshot-Gesamtgröße
```

**Details:** [Pre-Operation Snapshot](AI_SAFETY_SNAPSHOT.md)

---

## Schicht 6: Environment Isolation Guard

**Konfiguration:** `config/security.yaml`

### Umgebungsklassen

| Umgebung | `block_destructive` Default | Approval erforderlich | CRITICAL blockiert |
|---|---|---|---|
| `production` | `true` | Immer | Ja, außer Rolle `AI_DESTRUCTIVE_PRODUCTION_OPS` |
| `staging` | `false` | Bei `CRITICAL` | Nein, mit Approval |
| `development` | `false` | Nein | Nein |

### Konfigurationsbeispiel

```yaml
environment:
  name: production
  ai_agent_restrictions:
    block_destructive: true
    require_approval: true
    allowed_collections: []        # Leer = alle erlaubt (wird durch denied eingeschränkt)
    denied_collections:
      - users
      - audit_log
      - billing
      - _system
    require_role_for_critical: AI_DESTRUCTIVE_PRODUCTION_OPS
```

**Details:** [Environment Isolation Guard](AI_SAFETY_ENVIRONMENT.md)

---

## Schicht 7: AI Session Audit Trail

**Verwendet:** `AuditLogger` (bestehend, HMAC-Chaining)

### Neue Audit-Event-Typen

| Event | Beschreibung |
|---|---|
| `AI_TOOL_CALL` | Jeder MCP-Tool-Aufruf durch KI-Agent |
| `AI_APPROVAL_REQUIRED` | Operation wartet auf Bestätigung |
| `AI_APPROVAL_GRANTED` | Operator hat genehmigt |
| `AI_APPROVAL_DENIED` | Operator hat abgelehnt |
| `AI_APPROVAL_EXPIRED` | Token abgelaufen ohne Entscheidung |
| `AI_OPERATION_EXECUTED` | Destruktive Operation durchgeführt |
| `AI_SNAPSHOT_CREATED` | Pre-Op-Snapshot erstellt |
| `AI_ROLLBACK_INITIATED` | Rollback eingeleitet |

### Beispiel-Log-Eintrag

```json
{
  "event": "AI_TOOL_CALL",
  "timestamp": "2026-04-28T07:15:00.123Z",
  "ai_session_id": "sess-a1b2c3d4",
  "tool": "delete_entity",
  "args": {"key": "users:42"},
  "classification": "DESTRUCTIVE",
  "intent_type": "DATA_DESTRUCTION",
  "intent_confidence": 0.82,
  "approval_required": true,
  "approval_token": "tok-e5f6g7h8",
  "approved_by": "admin@example.com",
  "approval_reason": "Scheduled data cleanup",
  "pre_op_snapshot": "/var/themis/ai-snapshots/snap-20260428T071500Z",
  "outcome": "executed",
  "duration_ms": 45,
  "hmac": "sha256:..."
}
```

**Details:** [AI Session Audit Trail](AI_SAFETY_AUDIT_TRAIL.md)

---

## Implementierungsplan

### Phase 1 — Kritische Fixes (Priorität: SOFORT)

| ID | Aufgabe | Datei | Ziel |
|---|---|---|---|
| ASL-1 | AQL `REMOVE`/`DROP` im IntentClassifier | `src/security/intent_classifier.cpp` | Q2 2026 |
| ASL-2 | AQL Read-Only Enforcer für `aql_execute`-Tool | `src/query/aql_safety_validator.cpp` | Q2 2026 |
| ASL-3 | Dry-Run-Flag in `toolDeleteEntity()` + `toolDropIndex()` | `src/server/mcp_server.cpp` | Q2 2026 |

### Phase 2 — DOG + HILG (Priorität: HOCH)

| ID | Aufgabe | Datei | Ziel |
|---|---|---|---|
| ASL-4 | `AiOperationGuard` Klassifikations-Engine | `src/security/ai_operation_guard.cpp` | Q3 2026 |
| ASL-5 | Approval-Queue + In-Memory-Store | `src/server/mcp_server.cpp` | Q3 2026 |
| ASL-6 | `POST /v1/ai/approve/{id}` + `GET /v1/ai/pending-approvals` | `src/server/http_server.cpp` | Q3 2026 |
| ASL-7 | `safety:`-Sektion in Mode-YAML auswerten | `src/server/mcp_server.cpp` | Q3 2026 |

### Phase 3 — POS + Environment (Priorität: MITTEL)

| ID | Aufgabe | Datei | Ziel |
|---|---|---|---|
| ASL-8 | Pre-Op-Snapshot-Hook | `src/server/mcp_server.cpp` | Q3 2026 |
| ASL-9 | `environment:`-Konfiguration laden + auswerten | `src/server/mcp_server.cpp` | Q3 2026 |
| ASL-10 | `POST /v1/ai/rollback/{snapshot_id}` | `src/server/http_server.cpp` | Q3 2026 |
| ASL-11 | Snapshot-Cleanup-Job (Retention-Policy) | `src/maintenance/` | Q3 2026 |

### Phase 4 — Audit + LoRA-Classifier (Priorität: MITTEL)

| ID | Aufgabe | Datei | Ziel |
|---|---|---|---|
| ASL-12 | AI-Session-Audit-Trail im `AuditLogger` | `src/utils/audit_logger.cpp` | Q4 2026 |
| ASL-13 | LoRA-Adapter für IntentClassifier (IMPL-A2) | `src/security/intent_classifier.cpp` | Q4 2026 |
| ASL-14 | Chaos-Tests: Destruktiver KI-Agent | `tests/security/ai_safety/` | Q4 2026 |
| ASL-15 | Grafana-Dashboard für AI Safety Metriken | `config/grafana/` | Q4 2026 |

---

## Designentscheidungen

### Warum synchroner Snapshot (kein async)?

Der Pre-Operation Snapshot wird **synchron** vor der Ausführung erstellt. Asynchrone
Alternativen wurden verworfen, weil:
- Ein async Snapshot könnte nach Beginn der Operation fertiggestellt werden → kein echter Rollback-Punkt
- RocksDB Checkpoints sind O(1) (Hardlinks) und typischerweise < 500ms
- Die Latenz ist akzeptabel, da destruktive Operationen sowieso auf Approval warten

### Warum HMAC-gebundene Approval-Tokens?

Approval-Tokens sind an `(session_id, operation_hash, timestamp)` gebunden, um:
- Token-Replay-Angriffe zu verhindern (ein Token = eine Operation)
- Operationssubstitution zu verhindern (Token kann nicht für andere Op verwendet werden)
- Session-Hijacking-Angriffe zu erschweren

### Warum kein LLM-basierter Guard (nur Rules)?

Phase 1–3 verwenden regelbasierte Klassifikation, weil:
- Determinismus ist kritisch — ein KI-Guard der selbst LLM-Aufrufe macht, ist inakzeptabel
- Niedrige Latenz (< 1ms) gegenüber LLM-Inferenz (100ms–10s)
- LoRA-Adapter als Drop-In-Ersatz ist in Phase 4 geplant (IMPL-A2)

---

## Metriken

| Metrik | Prometheus-Label | Zielwert |
|---|---|---|
| KI-Operationen total | `ai_operations_total{class="*"}` | — |
| Blockierte Operationen | `ai_operations_blocked_total` | < 5% false positives |
| Approval-Rate | `ai_approvals_granted_total` | Monitoring |
| Durchschn. Approval-Latenz | `ai_approval_latency_seconds` | < 120s |
| Snapshot-Erstellungszeit | `ai_snapshot_duration_seconds` | p99 < 2s |
| Rollbacks | `ai_rollbacks_total` | Monitoring (Anomalie = Alert) |

---

## Referenzen

- [Golem.de: Cursor-KI-Vorfall (April 2026)](https://www.golem.de/news/cursor-ki-agent-loescht-aus-versehen-produktiv-datenbank-2604-208074.html)
- [MCP Protocol Documentation](../../../apis/MCP_PROTOCOL_SUPPORT.md)
- [ThemisDB Security Architecture](../README.md)
- [IntentClassifier STUB/SIMULATION NOTE](../../../../src/security/intent_classifier.cpp)
- [src/security/ROADMAP.md — Phase 5: AI Safety](../../../../src/security/ROADMAP.md)
