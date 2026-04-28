# ⚡ AI Safety Layer — Schnellreferenz

## Operationsklassen auf einen Blick

| Klasse | Beispiele | Approval? | Snapshot? |
|---|---|---|---|
| 🟢 `READ_ONLY` | get_entity, get_schema, `FOR x RETURN x` | ❌ | ❌ |
| 🔵 `WRITE_SAFE` | put_entity, create_index, `INSERT` | ❌ | ❌ |
| 🟡 `DESTRUCTIVE` | delete_entity, drop_index, `REMOVE @key` | ✅ | ✅ |
| 🔴 `CRITICAL` | `FOR x IN col REMOVE x IN col`, `DROP COLLECTION` | ✅ + Sonderrolle in Prod | ✅ |

---

## Konfiguration: Minimale Produktionssicherung

```yaml
# config/security.yaml
environment:
  name: production
  ai_agent_restrictions:
    block_destructive: true
    require_approval: true
    denied_collections: [users, audit_log, billing, _system]
```

```yaml
# config/ai_ml/llm/modes/default.yaml (agentic mode)
- id: agentic
  safety:
    enabled: true
    require_approval_for: [DESTRUCTIVE, CRITICAL]
    approval_timeout_s: 60
    auto_snapshot: true
```

---

## API-Schnellreferenz

```
GET  /v1/ai/pending-approvals          # Wartende Approvals anzeigen
POST /v1/ai/approve/{operation_id}     # Operation genehmigen
POST /v1/ai/deny/{operation_id}        # Operation ablehnen
POST /v1/ai/rollback/{snapshot_id}     # Rollback ausführen
```

---

## 7 Schichten — Zusammenfassung

| # | Name | Schutz gegen |
|---|---|---|
| 1 | Destructive Operation Guard | KI-Fehlklassifikation, unbeabsichtigte Deletes |
| 2 | Human-in-the-Loop Gate | Autonome Ausführung ohne Operator-Bestätigung |
| 3 | AQL Read-Only Enforcer | Mutations-Queries in `read-only`-deklarierten Tools |
| 4 | IntentClassifier AQL-Awareness | AQL-native Angriffsmuster (REMOVE, DROP) |
| 5 | Pre-Operation Snapshot | Irreversibler Datenverlust ohne Rollback-Option |
| 6 | Environment Isolation Guard | Prod/Dev-Paritätsfehler (Cursor-Vorfall-Muster) |
| 7 | AI Session Audit Trail | Fehlende forensische Rückverfolgbarkeit |

---

## Incident-Sofortmaßnahmen

```bash
# 1. Session des KI-Agenten sperren
curl -X DELETE https://themis:8443/v1/sessions/sess-XXXX -H "Authorization: Bearer $TOKEN"

# 2. Audit-Log der Session abrufen
curl https://themis:8443/v1/ai/audit?session_id=sess-XXXX

# 3. Letzten Snapshot vor Incident finden
ls -lt /var/themis/ai-snapshots/ | head -10

# 4. Rollback ausführen
curl -X POST https://themis:8443/v1/ai/rollback/snap-DATUM-op-ID \
  -d '{"confirmed_by":"dba@example.com","reason":"Incident rollback"}'
```

---

## Implementierungsstand

| Schicht | Status | Ziel |
|---|---|---|
| 1 DOG | 🔴 Geplant | Q3 2026 |
| 2 HILG | 🔴 Geplant | Q3 2026 |
| 3 AQL Validator | 🟢 Implementiert | Q2 2026 ✅ |
| 4 IntentClassifier | 🟢 Implementiert | Q2 2026 ✅ |
| 5 POS Snapshot | 🔴 Geplant | Q3 2026 |
| 6 Env Guard | 🔴 Geplant | Q3 2026 |
| 7 Audit Trail | 🔴 Geplant | Q4 2026 |

🔴 Geplant | 🟡 In Arbeit | 🟢 Implementiert

---

Vollständige Dokumentation: [AI Safety Architecture](AI_SAFETY_ARCHITECTURE.md)
Betriebshandbuch: [Runbook](AI_SAFETY_RUNBOOK.md)
