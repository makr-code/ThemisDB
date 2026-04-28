# 🔧 AI Safety Layer — Betriebshandbuch

> **Für:** Operatoren, SRE, DBA-Teams
>
> Konfiguration, Monitoring, Incident-Response und Troubleshooting für den
> ThemisDB AI Safety Layer.

---

## Schnellkonfiguration

### Produktion (empfohlen)

```yaml
# config/security.yaml
environment:
  name: production
  ai_agent_restrictions:
    block_destructive: true
    require_approval: true
    denied_collections:
      - users
      - audit_log
      - billing
      - _system
      - _graphs
    require_role_for_critical: AI_DESTRUCTIVE_PRODUCTION_OPS
    max_operations_per_hour: 100
    max_pending_approvals: 10

ai_safety:
  snapshot:
    dir: "/var/themis/ai-snapshots"
    retention_days: 7
    max_total_size_gb: 100
    verify_on_create: true
```

```yaml
# config/ai_ml/llm/modes/default.yaml — Agentic Mode
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

### Staging

```yaml
environment:
  name: staging
  ai_agent_restrictions:
    block_destructive: false
    require_approval: true        # Nur für CRITICAL
    denied_collections:
      - audit_log
      - _system
```

### Entwicklung (minimal)

```yaml
environment:
  name: development
  ai_agent_restrictions:
    block_destructive: false
    require_approval: false       # Kein Approval in Dev
```

---

## Approval-Workflow: Operator-Aktionen

### 1. Pending Approvals anzeigen

```bash
curl -X GET https://themis:8443/v1/ai/pending-approvals \
  -H "Authorization: Bearer $ADMIN_TOKEN"
```

Antwort:

```json
[
  {
    "operation_id": "op-a1b2c3d4-...",
    "tool": "delete_entity",
    "ai_session_id": "sess-a1b2c3d4",
    "classification": "DESTRUCTIVE",
    "preview": {
      "description": "Löscht Datensatz 'users:42'",
      "target_collection": "users",
      "estimated_affected": 1
    },
    "created_at": "2026-04-28T07:15:05Z",
    "expires_at": "2026-04-28T07:16:05Z"
  }
]
```

### 2. Operation genehmigen

```bash
curl -X POST https://themis:8443/v1/ai/approve/op-a1b2c3d4-... \
  -H "Authorization: Bearer $ADMIN_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "approved_by": "dba@example.com",
    "reason": "JIRA-1234: Geplante Bereinigung veralteter User-Datensätze"
  }'
```

### 3. Operation ablehnen

```bash
curl -X POST https://themis:8443/v1/ai/deny/op-a1b2c3d4-... \
  -H "Authorization: Bearer $ADMIN_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "denied_by": "dba@example.com",
    "reason": "Nicht autorisierte Operation — kein Change-Ticket vorhanden"
  }'
```

### 4. Rollback ausführen

```bash
curl -X POST https://themis:8443/v1/ai/rollback/snap-20260428T071505Z-op-a1b2c3d4 \
  -H "Authorization: Bearer $ADMIN_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "confirmed_by": "dba@example.com",
    "reason": "Unerwünschte Massenlöschung — sofortiger Rollback",
    "snapshot_path": "/var/themis/ai-snapshots/snap-20260428T071505Z-op-a1b2c3d4"
  }'
```

---

## Monitoring

### Prometheus-Metriken (Key Indicators)

```promql
# Operationen die Approval erfordern (Rate)
rate(themis_ai_approval_required_total[5m])

# Blockierte Operationen (sollte > 0 sein → Alarm wenn plötzlich 0)
rate(themis_ai_operations_blocked_total[5m])

# Durchschnittliche Approval-Latenz (Reaktionszeit der Operatoren)
histogram_quantile(0.90, rate(themis_ai_approval_latency_seconds_bucket[1h]))

# Rollbacks (Anomalie = sofortige Investigation)
increase(themis_ai_rollback_total[24h])

# Snapshot-Größe (Speicherplanung)
themis_ai_snapshot_total_size_bytes / (1024^3)  # in GB
```

### Grafana-Dashboard

Das AI Safety Dashboard befindet sich unter:
`config/grafana/dashboards/ai-safety-monitoring.json` (geplant, Q3 2026)

Enthält:
- AI-Operation-Klassifikationsverteilung (READ_ONLY / WRITE_SAFE / DESTRUCTIVE / CRITICAL)
- Approval-Queue-Länge über Zeit
- Blocked-Operations-Rate
- Rollback-History
- Snapshot-Speichernutzung
- IntentClassifier-Confidence-Verteilung

### Alerts (Beispiel: Alertmanager)

```yaml
groups:
  - name: ai_safety
    rules:
      - alert: AiCriticalOperationBlocked
        expr: increase(themis_ai_operations_blocked_total{classification="CRITICAL"}[5m]) > 0
        severity: HIGH
        annotations:
          summary: "KI-Agent versuchte CRITICAL-Operation — blockiert"

      - alert: AiRollbackInitiated
        expr: increase(themis_ai_rollback_total[1m]) > 0
        severity: CRITICAL
        annotations:
          summary: "AI Safety Rollback eingeleitet — sofortige Untersuchung"

      - alert: AiApprovalQueueFull
        expr: themis_ai_pending_approvals_count >= 10
        severity: WARNING
        annotations:
          summary: "AI Approval-Queue voll — Operatoren benötigt"

      - alert: AiSnapshotDirFull
        expr: themis_ai_snapshot_total_size_bytes / (1024^3) > 80
        severity: WARNING
        annotations:
          summary: "AI Snapshot-Verzeichnis zu 80% gefüllt"
```

---

## Incident Response

### Szenario: KI-Agent hat unerwünschte Löschung ausgeführt

```
1. SOFORT: Weitere Operationen des KI-Agenten verhindern
   → Den MCP-Client des KI-Agenten trennen (Session invalidieren)
   → Rate-Limit temporär auf 0 setzen

2. ASSESSMENT: Audit-Log analysieren
   curl -X GET https://themis:8443/v1/ai/audit?session_id=sess-xxx&limit=100
   → Welche Operationen hat die Session ausgeführt?
   → Welche Snapshots wurden erstellt?

3. ROLLBACK: Letzten validen Snapshot identifizieren
   ls -la /var/themis/ai-snapshots/ | sort -k6,7
   → Snapshot vor der problematischen Operation wählen

4. ROLLBACK AUSFÜHREN:
   curl -X POST https://themis:8443/v1/ai/rollback/{snapshot_id} ...

5. VERIFY: Datenintegrität prüfen
   → Betroffene Collections auf Vollständigkeit prüfen
   → Hash-Verifikation des Checkpoints

6. POST-MORTEM:
   → Wie konnte die Operation die Safety Layer passieren?
   → Konfiguration anpassen (denied_collections erweitern?)
   → Audit-Report erstellen
```

### Szenario: Approval-Token abgelaufen, Operation hängt

```
Problem: KI-Agent wartet auf Approval, Token ist abgelaufen.
Lösung:
  1. KI-Agent erhält automatisch "approval_expired"-Response
  2. Agent soll die Operation erneut einreichen
  3. Neuer Approval-Flow startet mit frischem Token
  Falls Agent hängt: Session invalidieren und neu starten.
```

---

## Frequently Asked Questions

**F: Was passiert, wenn der Snapshot-Speicher voll ist?**

A: Die Operation wird abgebrochen und nicht ausgeführt. Der Operator erhält eine
   Fehlermeldung und einen Alert. Der KI-Agent bekommt: `{"status":"error","message":"Pre-operation snapshot failed: insufficient disk space"}`.

**F: Kann der KI-Agent das Safety Layer umgehen?**

A: Nein. Das Safety Layer ist im MCP-Server-Code implementiert und wird vor jeder
   Tool-Ausführung aufgerufen. Der KI-Agent hat keinen Weg, es zu deaktivieren.
   Nur eine Fehlkonfiguration (z.B. `enabled: false`) könnte es deaktivieren.

**F: Wie lange dauert ein Rollback?**

A: Abhängig von der Datenbankgröße. RocksDB-Restore verwendet Hardlinks (O(1) für
   SST-Files). Für typische Produktionsdatenbanken (10–100 GB): 5–60 Sekunden.
   Während des Restores ist die Datenbank nicht verfügbar.

**F: Werden Snapshots verschlüsselt?**

A: In Phase 3 wird TDE (Transparent Data Encryption) auf Snapshot-Verzeichnisse
   angewendet, falls die Produktions-DB verschlüsselt ist. In Phase 1–2: Snapshot
   hat dieselbe Verschlüsselung wie die Quell-DB.

**F: Wie wirkt sich das Safety Layer auf die Performance aus?**

A: Für READ_ONLY und WRITE_SAFE-Operationen: < 0.1ms Overhead (Klassifikation).
   Für DESTRUCTIVE: +Snapshot-Zeit (p99 < 500ms) + Approval-Latenz (menschlich).
   Da destructive Operationen sowieso Approval brauchen, ist der technische Overhead vernachlässigbar.

---

## Troubleshooting

| Problem | Diagnose | Lösung |
|---|---|---|
| Approval-Endpoint antwortet nicht | `GET /v1/health` | MCP-Server neu starten |
| Snapshot schlägt fehl | `df -h /var/themis/ai-snapshots` | Speicherplatz freigeben |
| Zu viele False-Positives (READ_ONLY blockiert) | IntentClassifier-Logs | Schwellenwert erhöhen (intent_classifier.yaml) |
| Approval-Token sofort abgelaufen | `approval_timeout_s` zu niedrig | Wert erhöhen in mode-config |
| CRITICAL-Block trotz richtiger Rolle | Token-Cache | Auth-Cache leeren |

---

## Roadmap-Verknüpfung

Dieses Betriebshandbuch wird mit jeder neuen Phase des AI Safety Layers aktualisiert:

- **Phase 2:** Approval-API-Endpunkte verfügbar → Q3 2026
- **Phase 3:** Rollback-Endpunkt, Snapshot-Cleanup → Q3 2026
- **Phase 4:** Grafana-Dashboard, SIEM-Integration → Q4 2026
