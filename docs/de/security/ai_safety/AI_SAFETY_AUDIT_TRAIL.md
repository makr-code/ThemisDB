# 📋 AI Session Audit Trail — Schicht 7

> **Erweiterung des bestehenden `AuditLogger`** (HMAC-Chaining bereits implementiert).
>
> Jeder MCP-Tool-Call durch einen KI-Agenten wird als forensisch verwertbarer,
> manipulationssicherer Audit-Event protokolliert.

---

## Übersicht

Der bestehende `AuditLogger` in ThemisDB verwendet HMAC-SHA256-Chaining — jeder
Log-Eintrag enthält den Hash des vorherigen Eintrags. Das macht den Audit-Trail
**tamper-evident**: Nachträgliche Manipulation ist erkennbar.

Schicht 7 fügt **KI-spezifische Audit-Event-Typen** hinzu und sorgt dafür, dass
jeder MCP-Tool-Aufruf durch einen KI-Agenten vollständig und rückverfolgbar
protokolliert wird.

---

## Neue Audit-Event-Typen

| Event-Typ | Beschreibung | Wann |
|---|---|---|
| `AI_TOOL_CALL` | KI-Agent ruft MCP-Tool auf | Jeder Tool-Call |
| `AI_APPROVAL_REQUIRED` | Destructive Op wartet auf Bestätigung | Nach DOG-Klassifikation |
| `AI_APPROVAL_GRANTED` | Operator hat genehmigt | Nach POST /v1/ai/approve |
| `AI_APPROVAL_DENIED` | Operator hat abgelehnt | Nach POST /v1/ai/deny |
| `AI_APPROVAL_EXPIRED` | Token abgelaufen ohne Entscheidung | Nach TTL |
| `AI_OPERATION_BLOCKED` | Operation hardblockt (Env Guard, IntentClassifier) | Sofort |
| `AI_OPERATION_EXECUTED` | Destruktive Operation erfolgreich | Nach Ausführung |
| `AI_SNAPSHOT_CREATED` | Pre-Op-Checkpoint erstellt | Vor jeder genehmigten Op |
| `AI_ROLLBACK_INITIATED` | Rollback eingeleitet | Rollback-Anfrage |
| `AI_ROLLBACK_COMPLETED` | Rollback abgeschlossen | Nach Restore |
| `AI_ROLLBACK_FAILED` | Rollback fehlgeschlagen | Fehler beim Restore |

---

## Beispiel-Log-Einträge

### AI_TOOL_CALL (READ_ONLY)

```json
{
  "event": "AI_TOOL_CALL",
  "timestamp": "2026-04-28T07:15:00.123Z",
  "ai_session_id": "sess-a1b2c3d4",
  "tool": "get_entity",
  "args_summary": {"key": "users:42"},
  "classification": "READ_ONLY",
  "outcome": "executed",
  "duration_ms": 3,
  "hmac_chain": "sha256:abc123..."
}
```

### AI_APPROVAL_REQUIRED (DESTRUCTIVE)

```json
{
  "event": "AI_APPROVAL_REQUIRED",
  "timestamp": "2026-04-28T07:15:05.456Z",
  "ai_session_id": "sess-a1b2c3d4",
  "operation_id": "op-a1b2c3d4-e5f6-7890-abcd-ef1234567890",
  "tool": "delete_entity",
  "args_summary": {"key": "users:42"},
  "classification": "DESTRUCTIVE",
  "intent_type": "DATA_DESTRUCTION",
  "intent_confidence": 0.82,
  "expires_at": "2026-04-28T07:16:05Z",
  "environment": "production",
  "hmac_chain": "sha256:def456..."
}
```

### AI_OPERATION_EXECUTED (nach Approval)

```json
{
  "event": "AI_OPERATION_EXECUTED",
  "timestamp": "2026-04-28T07:15:45.789Z",
  "ai_session_id": "sess-a1b2c3d4",
  "operation_id": "op-a1b2c3d4-e5f6-7890-abcd-ef1234567890",
  "tool": "delete_entity",
  "args_summary": {"key": "users:42"},
  "classification": "DESTRUCTIVE",
  "approved_by": "admin@example.com",
  "approval_reason": "Scheduled data cleanup - JIRA-1234",
  "pre_op_snapshot": "/var/themis/ai-snapshots/snap-20260428T071505Z-op-a1b2c3d4",
  "outcome": "success",
  "duration_ms": 45,
  "hmac_chain": "sha256:ghi789..."
}
```

### AI_OPERATION_BLOCKED (Environment Guard)

```json
{
  "event": "AI_OPERATION_BLOCKED",
  "timestamp": "2026-04-28T07:15:10.000Z",
  "ai_session_id": "sess-b5c6d7e8",
  "tool": "query",
  "query_preview": "FOR u IN users REMOVE u IN users",
  "classification": "CRITICAL",
  "block_reason": "CRITICAL operations in production require role 'AI_DESTRUCTIVE_PRODUCTION_OPS'",
  "environment": "production",
  "intent_type": "DATA_DESTRUCTION",
  "intent_confidence": 0.92,
  "hmac_chain": "sha256:jkl012..."
}
```

---

## AI-Session-ID

Jede KI-Agenten-Sitzung erhält eine eindeutige `ai_session_id` (UUID v4), die:

- beim ersten MCP-Tool-Call der Session generiert wird
- über alle Tool-Calls der Session erhalten bleibt
- im Audit-Trail alle Aktionen der Session verknüpft
- in der Standard-Session des MCP-Clients gespeichert wird

Dies ermöglicht die **vollständige Rekonstruktion einer KI-Agent-Sitzung** aus
dem Audit-Log — unverzichtbar für forensische Analysen nach Incidents.

---

## Datenschutz: Argument-Sanitisierung

Die `args_summary`-Felder enthalten **keine Klartext-Bind-Variablen** mit
sensiblen Daten. Stattdessen wird eine Sanitisierungsfunktion angewendet:

```
Sanitisierungsregeln:
• Keys/IDs werden vollständig protokolliert (notwendig für Forensik)
• Values > 256 Zeichen werden auf Länge+SHA256-Hash reduziert
• Felder mit Namen aus der PII-Erkennungsliste werden maskiert
• Credentials/Tokens werden vollständig entfernt (→ "[REDACTED]")
```

---

## Retention & Compliance

Der AI-Audit-Trail unterliegt denselben Retention-Policies wie der allgemeine Audit-Log:

```yaml
# config/compliance/audit/ai_audit_config.yaml (Erweiterung)
ai_audit:
  retention_days: 365      # 1 Jahr (GDPR Art. 5 Abs. 1e)
  export_format: jsonl     # Für SIEM-Integration
  alert_on_events:
    - AI_OPERATION_BLOCKED
    - AI_ROLLBACK_INITIATED
    - AI_ROLLBACK_FAILED
  grafana_dashboard: "ai-safety-audit"
```

---

## SIEM-Integration

Der AI-Audit-Trail ist als JSON-Lines-Stream exportierbar und kompatibel mit:

- **Elasticsearch/OpenSearch** (Kibana-Dashboard vorhanden)
- **Splunk** (via JSON-Importer)
- **IBM QRadar** (via JSON Log Source)
- **Microsoft Sentinel** (via Custom Connector)

Alerting-Beispiel (Elasticsearch):

```json
{
  "rule": {
    "name": "AI_OPERATION_BLOCKED in Production",
    "query": "event:AI_OPERATION_BLOCKED AND environment:production",
    "severity": "HIGH",
    "action": "pagerduty_alert"
  }
}
```

---

## Testfälle (Geplant: `tests/security/ai_safety/test_ai_audit_trail.cpp`)

| Test-ID | Beschreibung | Erwartetes Ergebnis |
|---|---|---|
| AAT-01 | GET-Operation → AI_TOOL_CALL mit classification=READ_ONLY | Event vorhanden |
| AAT-02 | DELETE → AI_APPROVAL_REQUIRED → AI_OPERATION_EXECUTED | 3 Events, verkettet |
| AAT-03 | Blocked-Operation → AI_OPERATION_BLOCKED | Event mit block_reason |
| AAT-04 | HMAC-Chain-Integrität nach 100 Events | Alle HMACs valide |
| AAT-05 | Session-ID-Konsistenz über 10 Tool-Calls | Gleiche session_id |
| AAT-06 | Rollback → AI_ROLLBACK_INITIATED + AI_ROLLBACK_COMPLETED | 2 Events |
| AAT-07 | PII-Maskierung in args_summary | Sensible Felder maskiert |

---

## Roadmap-Verknüpfung

- **ASL-12:** AI-Session-Audit-Trail im `AuditLogger` → Q4 2026 (Phase 4)
