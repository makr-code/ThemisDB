# 🔍 AQL Read-Only Enforcer (Schicht 3)

> **Implementierungsreferenz** für `include/query/aql_safety_validator.h` und
> `src/query/aql_safety_validator.cpp`

---

## Übersicht

Der **AQL Read-Only Enforcer** verhindert, dass ein KI-Agent über das `aql_execute`-Tool
(das in der Mode-YAML als `read-only` deklariert ist) trotzdem schreibende oder
destruktive AQL-Operationen ausführt.

Die Deklaration in der YAML (`description: "Execute a read-only AQL query..."`) ist ohne
Code-Enforcement nur ein Kommentar — der Enforcer macht daraus eine echte Sicherheitsgrenze.

---

## Problem: Silent Trust Gap

```yaml
# config/ai_ml/llm/modes/default.yaml
tools:
  - name: aql_execute
    description: "Execute a read-only AQL query against the ThemisDB instance"  # ← nur Kommentar!
```

```cpp
// src/server/mcp_server.cpp — OHNE Enforcer
json McpServer::toolQuery(const json& args) {
    // Führt JEDE Query aus — auch REMOVE, DROP, TRUNCATE
    auto result = executeAql(query, *query_engine_);  // ← kein Schutz!
    ...
}
```

**Mit Enforcer:**

```cpp
json McpServer::toolQuery(const json& args) {
    // Aktiviert wenn aql_execute aus Mode mit enforce_read_only=true aufgerufen
    if (current_mode_enforces_read_only_) {
        auto validation = AqlSafetyValidator::validate(query);
        if (!validation.is_read_only) {
            return {{"status", "error"},
                    {"message", validation.violation_message},
                    {"detected_mutation", validation.mutation_type},
                    {"position", validation.position}};
        }
    }
    auto result = executeAql(query, *query_engine_);
    ...
}
```

---

## Erkannte Mutationsoperationen

### DML-Writes

| AQL-Keyword | Beispiel | Blockiert |
|---|---|---|
| `INSERT` | `INSERT {name: "x"} INTO users` | ✅ |
| `UPDATE` | `UPDATE "key" WITH {age: 30} IN users` | ✅ |
| `REPLACE` | `REPLACE "key" WITH {name: "y"} IN users` | ✅ |
| `REMOVE` | `REMOVE "key" IN users` | ✅ |
| `UPSERT` | `UPSERT {key: "k"} INSERT {...} UPDATE {...} IN users` | ✅ |

### DDL-Operationen

| Operation | Blockiert |
|---|---|
| `DROP COLLECTION` | ✅ |
| `DROP INDEX` | ✅ |
| `CREATE COLLECTION` | ✅ |
| `TRUNCATE` | ✅ |
| `RENAME COLLECTION` | ✅ |

---

## Geplante API

```cpp
namespace themis::query {

class AqlSafetyValidator {
public:
    struct ValidationResult {
        bool        is_read_only;       ///< true = safe to execute
        std::string mutation_type;      ///< "REMOVE" | "INSERT" | "DROP_COLLECTION" | ...
        std::string violation_message;  ///< Menschenlesbare Fehlermeldung
        size_t      position;           ///< Byte-Offset des Mutations-Keywords in Query
        std::string sanitized_preview;  ///< Query mit maskierten Bind-Vars (für Logs)
    };

    /// Validiert einen AQL-Query-String auf Mutationsfreiheit.
    /// Schnelle tokenbasierte Prüfung (kein vollständiger AST-Parse).
    /// Latenz: < 0.1ms für Queries bis 64 KB.
    static ValidationResult validate(const std::string& aql_query);

    /// Gibt zurück ob der angegebene Mode read-only enforcement erfordert.
    static bool modeEnforcesReadOnly(const std::string& mode_id,
                                     const json& mode_config);
};

} // namespace themis::query
```

---

## Erkennungsalgorithmus

```
EINGABE: aql_query (String)

1. Normalisiere: Strip Kommentare (// ... und /* ... */), konvertiere zu Uppercase

2. Tokenisiere nach Whitespace und AQL-Sonderzeichen

3. Scanne Token-Sequenz:
   a. Token "REMOVE" → violation: mutation_type="REMOVE"
   b. Token "INSERT" (nicht in Unterausdruck) → mutation_type="INSERT"
   c. Token "UPDATE" (nicht "FILTER UPDATE") → mutation_type="UPDATE"
   d. Token "REPLACE" → mutation_type="REPLACE"
   e. Token "UPSERT" → mutation_type="UPSERT"
   f. Sequenz "DROP" + "COLLECTION" → mutation_type="DROP_COLLECTION"
   g. Sequenz "DROP" + "INDEX"      → mutation_type="DROP_INDEX"
   h. Sequenz "CREATE" + "COLLECTION" → mutation_type="CREATE_COLLECTION"
   i. Token "TRUNCATE" → mutation_type="TRUNCATE"

4. Wenn Violation gefunden:
   - position = Byte-Offset des Tokens
   - violation_message = "Read-only enforcement: '{mutation_type}' not permitted
     (position {position}). Use a writable mode for mutation operations."
   - is_read_only = false

5. Sonst: is_read_only = true
```

### Sonderfälle

```
• Bind-Variablen (@key) werden nicht als Keywords behandelt
• Kommentare werden vor der Tokenisierung entfernt
• Quoted Strings werden übersprungen (kein False-Positive bei
  FOR x IN col FILTER x.type == "REMOVE_ME" RETURN x)
• Subqueries werden rekursiv geprüft
```

---

## Aktivierung

### Per Mode-Konfiguration (YAML)

```yaml
# config/ai_ml/llm/modes/default.yaml
tools:
  - name: aql_execute
    description: "Execute a read-only AQL query against the ThemisDB instance"
    enforce_read_only: true       # ← NEU: aktiviert AqlSafetyValidator
    timeout_ms: 10000
```

### Per Programmatic Flag (MCP Server)

```cpp
// In McpServer::toolQuery():
bool enforce_ro = false;

// Prüfe ob das aufrufende Tool als read-only markiert ist
if (args.contains("__tool_name") && args["__tool_name"] == "aql_execute") {
    enforce_ro = AqlSafetyValidator::modeEnforcesReadOnly(
        current_mode_id_, current_mode_config_);
}
```

---

## Fehlerantwort (MCP)

Wenn eine Mutationsoperation in einem read-only enforced Context erkannt wird:

```json
{
  "status": "error",
  "code": "AQL_READ_ONLY_VIOLATION",
  "message": "Read-only enforcement active for tool 'aql_execute'. Mutation operations are not permitted.",
  "details": {
    "detected_mutation": "REMOVE",
    "position": 42,
    "query_preview": "FOR u IN users REMOVE u IN users",
    "suggestion": "Use the 'query' tool in a writable mode (e.g., agentic with appropriate approvals) for mutation operations."
  }
}
```

---

## Testfälle (Geplant: `tests/query/test_aql_safety_validator.cpp`)

| Test-ID | Query | Erwartetes Ergebnis |
|---|---|---|
| ARO-01 | `FOR u IN users RETURN u` | `is_read_only=true` |
| ARO-02 | `FOR u IN users REMOVE u IN users` | `REMOVE violation, pos=19` |
| ARO-03 | `INSERT {name:"x"} INTO users` | `INSERT violation` |
| ARO-04 | `FOR u IN users FILTER u.type == "REMOVE_ME" RETURN u` | `is_read_only=true` (kein False-Positive) |
| ARO-05 | `/* remove comment */ FOR u IN users RETURN u` | `is_read_only=true` (Kommentar ignoriert) |
| ARO-06 | `DROP COLLECTION users` | `DROP_COLLECTION violation` |
| ARO-07 | `FOR u IN users FILTER u.id == @id REMOVE u IN users` | `REMOVE violation` |
| ARO-08 | Bind-Var `@key` enthält "REMOVE" im String | `is_read_only=true` |

---

## Performance-Ziele

| Metrik | Zielwert |
|---|---|
| Validierungslatenz (p99, 1 KB Query) | < 0.1 ms |
| Validierungslatenz (p99, 64 KB Query) | < 2 ms |
| False-Positive-Rate (benigne Queries) | 0% |
| False-Negative-Rate (Mutations-Queries) | 0% |

---

## Roadmap-Verknüpfung

- **ASL-2:** AQL Read-Only Enforcer Implementierung → Q2 2026 (Phase 1 — Kritisch)

Vollständiger Plan: [AI Safety Architecture](AI_SAFETY_ARCHITECTURE.md#implementierungsplan)
