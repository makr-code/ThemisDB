# 📸 Pre-Operation Snapshot (POS) — Schicht 5

> **Automatische RocksDB-Checkpoints** vor jeder KI-initiierten destruktiven Operation.
>
> Verwendet: `RocksDBWrapper::createCheckpoint()` | `AdminApiHandler`

---

## Übersicht

Bevor ThemisDB eine vom DOG als `DESTRUCTIVE` oder `CRITICAL` klassifizierte
KI-Operation ausführt, erstellt es einen **synchronen RocksDB-Checkpoint**.
Dieser Checkpoint ermöglicht einen vollständigen Datenbank-Rollback, falls die
Operation unerwünschte Auswirkungen hatte.

**Kernprinzip:** Kein destruktiver KI-Befehl wird ausgeführt, ohne vorher einen
sicheren Rückkehrpunkt zu schaffen.

---

## Warum synchron?

Ein asynchroner Snapshot könnte nach Beginn der Operation abgeschlossen werden —
dann wäre er kein echter Rollback-Punkt mehr. RocksDB-Checkpoints verwenden
Hardlinks (O(1) auf modernen Dateisystemen) und sind typischerweise in < 500ms
abgeschlossen — selbst bei 100 GB Datenbanken.

```
Latenz-Budget:
  Checkpoint erstellen: p99 < 500 ms
  Operation ausführen:  variabel
  Gesamtlatenz (Approval-Flow): bereits 60s+ → Snapshot-Overhead vernachlässigbar
```

---

## Ablauf

```
┌─────────────────────────────────────────────────────────┐
│  Destructive/Critical Operation genehmigt               │
└────────────────────────┬────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────┐
│  1. Snapshot-Verzeichnis generieren                     │
│     snap_dir = snapshot_dir + "/snap-" + iso_timestamp  │
│     Beispiel: /var/themis/ai-snapshots/snap-20260428    │
│              T071500Z-op-a1b2c3d4                       │
└────────────────────────┬────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────┐
│  2. RocksDB-Checkpoint erstellen                        │
│     storage_->createCheckpoint(snap_dir)                │
│     Fehler → Operation ABGEBROCHEN, Error zurück        │
└────────────────────────┬────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────┐
│  3. Snapshot-Metadata in Audit-Log schreiben            │
│     Event: AI_SNAPSHOT_CREATED                          │
│     {operation_id, snap_dir, timestamp, size_bytes}     │
└────────────────────────┬────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────┐
│  4. Operation ausführen                                 │
└────────────────────────┬────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────┐
│  5. MCP-Response mit Snapshot-Referenz                  │
│     {"pre_operation_snapshot": snap_dir, ...}           │
└─────────────────────────────────────────────────────────┘
```

---

## Rollback-Flow

```
POST /v1/ai/rollback/{snapshot_id}
  Body: {
    "confirmed_by": "dba@example.com",
    "reason": "Unerwünschte Massenlöschung rückgängig machen",
    "snapshot_path": "/var/themis/ai-snapshots/snap-20260428T071500Z-op-a1b2c3d4"
  }

Ablauf:
  1. Snapshot-Pfad validieren (existiert? Hash-Prüfung?)
  2. Anfragesteller-Rolle prüfen (erfordert DBA_ROLLBACK oder höher)
  3. Audit-Log: AI_ROLLBACK_INITIATED
  4. storage_->restoreFromCheckpoint(snapshot_path)
     - DB wird geschlossen
     - Checkpoint-Verzeichnis wird als neue DB verwendet
     - DB wird neu geöffnet
  5. Audit-Log: AI_ROLLBACK_COMPLETED
  6. Response: {"status": "rollback_completed", "restored_from": snapshot_path}
```

---

## Konfiguration

```yaml
# config/ai_ml/llm/modes/default.yaml
modes:
  - id: agentic
    safety:
      auto_snapshot: true
      snapshot_dir: "/var/themis/ai-snapshots"
      snapshot_retention_days: 7
      snapshot_max_total_gb: 100
```

```yaml
# config/security.yaml
ai_safety:
  snapshot:
    dir: "/var/themis/ai-snapshots"
    retention_days: 7
    max_total_size_gb: 100
    cleanup_schedule: "0 3 * * *"   # Cron: täglich 03:00 Uhr
    verify_on_create: true           # Hash-Verifikation nach Checkpoint
```

---

## Snapshot-Naming-Schema

```
/var/themis/ai-snapshots/
  snap-{ISO8601}-op-{operation_id_prefix}/
  │
  ├── CURRENT
  ├── MANIFEST-000001
  ├── OPTIONS-000001
  └── *.sst (Hardlinks auf originale SST-Dateien)

Beispiel:
  snap-20260428T071500Z-op-a1b2c3d4/
```

---

## Retention & Cleanup

Der **Snapshot Cleanup Job** (geplant in `src/maintenance/`) läuft planmäßig und bereinigt
Snapshots nach Ablauf der Retention-Policy:

```
1. Scanne snapshot_dir nach snap-* Verzeichnissen
2. Prüfe Alter (> retention_days → löschen)
3. Prüfe Gesamtgröße (> max_total_size_gb → älteste löschen)
4. Protokolliere jede Löschung in Audit-Log
5. Behalte Snapshots die manuell als "protected" markiert wurden
```

**Wichtig:** Snapshots zu laufenden Rollback-Vorgängen dürfen nicht gelöscht werden.

---

## Metriken

```
themis_ai_snapshot_created_total        Counter: Erstellte Snapshots
themis_ai_snapshot_duration_seconds     Histogram: Checkpoint-Erstellungszeit
themis_ai_snapshot_size_bytes           Gauge: Aktuelle Snapshot-Größe
themis_ai_rollback_total                Counter: Durchgeführte Rollbacks
themis_ai_snapshot_cleanup_deleted_total Counter: Gereinigte Snapshots
```

---

## Fehlerszenarien

| Szenario | Verhalten |
|---|---|
| Checkpoint-Verzeichnis nicht beschreibbar | Operation ABGEBROCHEN, Fehlermeldung zurück |
| Nicht genug Speicherplatz für Checkpoint | Operation ABGEBROCHEN, Admin-Alert |
| Checkpoint-Erstellung schlägt fehl (DB-Fehler) | Operation ABGEBROCHEN |
| Rollback-Snapshot nicht gefunden | `404 Not Found` |
| Rollback-Snapshot beschädigt (Hash-Fehler) | `500` + forensisches Log |
| DB-Restore schlägt fehl | Notfall-Shutdown + Operator-Alert |

---

## Testfälle (Geplant: `tests/security/ai_safety/test_ai_snapshot.cpp`)

| Test-ID | Beschreibung | Erwartetes Ergebnis |
|---|---|---|
| POS-01 | Snapshot vor DESTRUCTIVE-Operation | Verzeichnis erstellt, Audit-Event vorhanden |
| POS-02 | Snapshot-Pfad in MCP-Response | `pre_operation_snapshot` Key vorhanden |
| POS-03 | Rollback nach Deletion | Daten wiederhergestellt |
| POS-04 | Snapshot bei Speicherplatzmangel | Operation abgebrochen, kein Datenverlust |
| POS-05 | Retention-Cleanup | Alte Snapshots gelöscht, aktuelle behalten |

---

## Roadmap-Verknüpfung

- **ASL-8:** Pre-Op-Snapshot-Hook → Q3 2026 (Phase 3)
- **ASL-10:** Rollback-Endpoint → Q3 2026 (Phase 3)
- **ASL-11:** Snapshot-Cleanup-Job → Q3 2026 (Phase 3)
