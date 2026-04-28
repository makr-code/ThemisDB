# 🛡️ Destructive Operation Guard (DOG) + Human-in-the-Loop Gate (HILG)

> **Schichten 1 & 2 des AI Safety Layer**
>
> Implementierungsreferenz für `include/security/ai_operation_guard.h` und
> `src/security/ai_operation_guard.cpp` sowie die HILG-Integration im MCP-Server.

---

## Übersicht

Der **Destructive Operation Guard** (DOG) klassifiziert jede KI-initiierte Datenbankoperation
**vor** ihrer Ausführung anhand ihres Destruktionspotenzials.

Das **Human-in-the-Loop Gate** (HILG) implementiert den Approval-Workflow für Operationen,
die als `DESTRUCTIVE` oder `CRITICAL` eingestuft werden.

Beide Komponenten sind deterministisch und führen keine LLM-Aufrufe durch.

---

## Operationsklassen

```cpp
enum class OperationClass : uint8_t {
    READ_ONLY   = 0,  // Sichere Leseoperation — direkt ausführen
    WRITE_SAFE  = 1,  // Einzelschreiboperation — direkt ausführen
    DESTRUCTIVE = 2,  // Löschung von Datensätzen — Approval erforderlich
    CRITICAL    = 3,  // Vollbereichsoperation/DDL — Approval + Sonderrolle in Produktion
};
```

### Klassifikationsregeln

#### READ_ONLY
- AQL-Queries ohne Mutationsoperatoren (`FOR...RETURN`, `SELECT`-ähnliche Patterns)
- `get_entity`, `get_schema`, `get_stats`, `list_indexes`
- Alle lesenden MCP-Tools

#### WRITE_SAFE
- `put_entity` mit explizitem Key
- `create_index`
- AQL `INSERT` mit vollständig spezifiziertem Dokument
- AQL `UPDATE FILTER id == @key` (einzelner Datensatz)

#### DESTRUCTIVE
- `delete_entity` (parametrisiertes Single-Key-Delete)
- `drop_index`
- AQL `REMOVE @key IN collection`
- AQL `FOR x IN col FILTER x.id == @id REMOVE x IN col`

#### CRITICAL
- AQL `FOR x IN col REMOVE x IN col` **ohne** `FILTER` → Vollbereichs-Delete
- `DROP COLLECTION`
- `TRUNCATE`
- Jede Operation auf System-Collections (`_system`, `_graphs`, `_analyzers`)
- `drop_index` auf Primary Index

---

## Klassifikationsalgorithmus

```
EINGABE: operation_type (Tool-Name oder "aql_query"), aql_string (optional)

1. Prüfe operation_type:
   - "get_entity", "get_schema", "get_stats", "list_indexes"  → READ_ONLY
   - "put_entity"                                             → WRITE_SAFE
   - "create_index"                                           → WRITE_SAFE
   - "delete_entity"                                          → DESTRUCTIVE
   - "drop_index"                                             → DESTRUCTIVE (base)

2. Wenn operation_type == "query" UND aql_string vorhanden:
   a. Tokenisiere aql_string (case-insensitive)
   b. Enthält Token "REMOVE":
      - Hat vorheriges Token "FOR" und kein "FILTER" im Statement → CRITICAL
      - Sonst → DESTRUCTIVE
   c. Enthält "DROP COLLECTION" oder "TRUNCATE" → CRITICAL
   d. Enthält "DROP INDEX" → DESTRUCTIVE
   e. Enthält "INSERT", "UPDATE", "REPLACE", "UPSERT" → WRITE_SAFE
   f. Sonst → READ_ONLY

3. Für drop_index: Ist column == "_id" oder collection in SYSTEM_COLLECTIONS → CRITICAL

4. Gibt (OperationClass, OperationPreview) zurück
```

---

## Geplante API

```cpp
namespace themis::security {

/// Preview einer klassifizierten Operation (für HILG-Response).
struct OperationPreview {
    std::string tool_name;
    std::string description;
    json        args;               ///< Bereinigt (keine Credentials)
    std::string aql_query;          ///< Nur wenn type == "query"
    std::string target_collection;  ///< Betroffene Collection
    uint64_t    estimated_affected; ///< Geschätzte betroffene Datensätze (0 = unbekannt)
};

/// Rückgabe des Guards an den MCP-Server.
struct GuardDecision {
    OperationClass   op_class;
    OperationPreview preview;
    bool             requires_approval;  ///< = op_class >= DESTRUCTIVE
    std::string      operation_id;       ///< UUID für HILG-Tracking
    std::string      block_reason;       ///< Gesetzt wenn hartes Block (Env Guard)
};

class AiOperationGuard {
public:
    struct Config {
        bool           enabled = true;
        OperationClass approval_threshold = OperationClass::DESTRUCTIVE;
        int            approval_timeout_s = 60;
        bool           auto_snapshot = true;
        std::string    snapshot_dir = "/var/themis/ai-snapshots";
        bool           dry_run_preview = true;

        // Environment Guard
        std::string              environment = "development";
        bool                     block_destructive_in_prod = true;
        std::vector<std::string> denied_collections;
        std::vector<std::string> allowed_collections; // Leer = alle
        std::string              critical_ops_role = "AI_DESTRUCTIVE_PRODUCTION_OPS";
    };

    explicit AiOperationGuard(Config cfg);

    /// Klassifiziert eine Operation und gibt Entscheidung zurück.
    GuardDecision evaluate(
        const std::string& tool_name,
        const json&        args,
        const std::string& ai_session_id,
        const std::string& caller_role = ""
    ) const;

    /// Baut die MCP-Response für "requires_approval".
    json buildRequiresApprovalResponse(const GuardDecision& decision) const;
};

} // namespace themis::security
```

---

## HILG: Approval-Queue

### Datenstruktur (In-Memory, Thread-safe)

```cpp
struct PendingApproval {
    std::string       operation_id;   // UUID
    std::string       ai_session_id;
    GuardDecision     decision;
    json              operation_args;
    std::string       tool_name;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point expires_at;
    bool              is_executed = false;
};

// In McpServer:
std::unordered_map<std::string, PendingApproval> pending_approvals_;
std::mutex pending_approvals_mutex_;
```

### Ablaufsteuerung

```
1. evaluate() → GuardDecision{requires_approval=true, operation_id="op-uuid"}
2. Operation in pending_approvals_ speichern (mit TTL)
3. MCP-Response: {"status":"requires_approval", "operation_id":"op-uuid", "preview":{...}}
4. KI-Agent erhält Response und wartet (oder informiert Nutzer)
5. Operator ruft POST /v1/ai/approve/op-uuid auf
6. McpServer:
   a. Verifiziert Token-Signatur
   b. Prüft TTL (expired → 410 Gone)
   c. Erstellt Pre-Op-Snapshot
   d. Führt Operation aus
   e. Schreibt AI_OPERATION_EXECUTED in Audit-Log
   f. Gibt execution_result zurück
```

---

## MCP-Response-Formate

### requires_approval (Schritt 3)

```json
{
  "status": "requires_approval",
  "operation_id": "op-a1b2c3d4-e5f6-7890-abcd-ef1234567890",
  "classification": "DESTRUCTIVE",
  "tool": "delete_entity",
  "preview": {
    "description": "Löscht Datensatz mit Key 'users:42' aus Collection 'users'",
    "target_collection": "users",
    "estimated_affected": 1,
    "args": {"key": "users:42"}
  },
  "impact_estimate": {
    "data_loss_risk": "LOW",
    "reversible": true,
    "auto_snapshot": true,
    "snapshot_dir": "/var/themis/ai-snapshots"
  },
  "expires_at": "2026-04-28T07:16:00Z",
  "approve_url": "/v1/ai/approve/op-a1b2c3d4-e5f6-7890-abcd-ef1234567890"
}
```

### Nach Approval — Execution Result

```json
{
  "status": "executed",
  "operation_id": "op-a1b2c3d4-e5f6-7890-abcd-ef1234567890",
  "approved_by": "admin@example.com",
  "pre_operation_snapshot": "/var/themis/ai-snapshots/snap-20260428T071500Z",
  "result": {
    "status": "success",
    "message": "Entity deleted successfully",
    "key": "users:42"
  }
}
```

### Hartes BLOCK (Critical in Produktion ohne Rolle)

```json
{
  "status": "blocked",
  "reason": "CRITICAL operations in production environment require role 'AI_DESTRUCTIVE_PRODUCTION_OPS'",
  "classification": "CRITICAL",
  "operation": "FOR x IN users REMOVE x IN users",
  "environment": "production",
  "contact": "dba-team@example.com"
}
```

---

## Konfiguration

```yaml
# config/ai_ml/llm/modes/default.yaml
modes:
  - id: agentic
    safety:
      enabled: true
      require_approval_for: [DESTRUCTIVE, CRITICAL]
      approval_timeout_s: 60
      dry_run_preview: true
      auto_snapshot: true
      snapshot_dir: "/var/themis/ai-snapshots"
```

```yaml
# config/security.yaml
environment:
  name: production            # production | staging | development
  ai_agent_restrictions:
    block_destructive: true
    require_approval: true
    allowed_collections: []
    denied_collections:
      - users
      - audit_log
      - billing
      - _system
    require_role_for_critical: AI_DESTRUCTIVE_PRODUCTION_OPS
```

---

## Testfälle (Geplant: `tests/security/ai_safety/`)

| Test-ID | Beschreibung | Erwartetes Ergebnis |
|---|---|---|
| DOG-01 | `get_entity` → Klassifikation | `READ_ONLY`, kein Approval |
| DOG-02 | `delete_entity` → Klassifikation | `DESTRUCTIVE`, Approval required |
| DOG-03 | `FOR x IN users REMOVE x IN users` | `CRITICAL`, Approval required |
| DOG-04 | `FOR x IN users FILTER x.id==1 REMOVE x IN users` | `DESTRUCTIVE` |
| DOG-05 | `DROP COLLECTION users` | `CRITICAL` |
| HILG-01 | Approval-Token TTL abgelaufen | `410 Gone` |
| HILG-02 | Token für andere Operation wiederverwendet | `403 Forbidden` |
| HILG-03 | Approval → Execution → Audit-Log | Vollständiger Flow |
| HILG-04 | CRITICAL in Production ohne Rolle | Hartes BLOCK |
| HILG-05 | CRITICAL in Production mit Rolle | Approval möglich |

---

## Roadmap-Verknüpfung

- **Phase 1:** ASL-3 (Dry-Run-Flag) → Q2 2026
- **Phase 2:** ASL-4, ASL-5, ASL-6, ASL-7 → Q3 2026
- **Phase 3:** ASL-8, ASL-9, ASL-10 → Q3 2026

Vollständiger Plan: [AI Safety Architecture](AI_SAFETY_ARCHITECTURE.md#implementierungsplan)
