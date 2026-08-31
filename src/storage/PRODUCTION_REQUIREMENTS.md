> **Status:** 2026-06-01 – mit aktuellem Storage-Code (`wal_storage.cpp`, `backup_manager.cpp`, `pitr_manager.cpp`, `security_signature.cpp`) abgeglichen.

# ThemisDB Storage Module - Production Requirements

## Zweck und Geltungsbereich

Dieses Dokument ist der **kanonische Referenzpunkt für produktive Mindestanforderungen** des Storage-Moduls.
Es definiert verbindliche Anforderungen für Durability, WAL/Replay-Verhalten, Backup/PITR-Konfiguration, Integritätssicherung und Blob-Backend-Absicherung.

## Dokumentabgrenzung (Canonical Split)

- **`src/storage/PRODUCTION_REQUIREMENTS.md` (dieses Dokument):** verpflichtende Produktionsanforderungen (MUST/MUST NOT), Sicherheitsannahmen, Betriebsgrenzen.
- **`src/storage/README.md`:** Funktionsübersicht, Architekturkontext, API- und Nutzungsbeispiele.
- **`src/storage/ROADMAP.md`:** Lieferphasen, offene/abgeschlossene Features, Readiness-Planung.
- **`src/storage/FUTURE_ENHANCEMENTS.md`:** mittelfristige/langfristige Erweiterungen und Forschungsfelder.

## Verbindliche Durability-Anforderungen

- **MUST:** WAL (`wal_storage.cpp`) in Produktionsdeployments aktiviert; kein In-Memory-Only-Betrieb ohne explizite Deklaration.
- **MUST:** WAL-Replay-Pfad nach Neustart vollständig durchlaufen; partielle Replays führen zu deterministischem Fehler.
- **MUST:** Backup-Manager (`backup_manager.cpp`) mit konfiguriertem Backup-Ziel und Zeitplan; keine impliziten Defaults.
- **MUST:** PITR-Manager (`pitr_manager.cpp`) konfiguriert, wenn Point-in-Time-Recovery erforderlich; Backup-Retention-Window explizit dokumentiert.
- **MUST NOT:** Storage-Fehler nach Write-Commit stillschweigend ignorieren; Fehler müssen propagiert werden.

## Verbindliche Sicherheitsanforderungen

### 1) Integritätssicherung

- **MUST:** `security_signature.cpp` aktiv für kritische Daten-Schreibpfade; Signatur-Prüfung bei Lese-Operationen aktiv.
- **MUST:** `SecuritySignatureManager` mit einem persistenten RocksDB-Backend initialisieren; In-Memory-Fallback ist nur mit explizitem Opt-in für Tests oder klar deklarierte ephemere Läufe zulässig.
- **MUST:** Storage-Audit-Logging für Write-/Delete-Operationen aktiv.
- **MUST NOT:** Integritätsprüfungen für Produktions-Schreibpfade deaktivieren.
- **MUST NOT:** `SecuritySignatureManager(nullptr)` als stillschweigenden Produktions-Downgrade verwenden.

### 2) Blob-Backend-Absicherung

- **MUST:** Blob-Backend (S3/GCS/Azure/Filesystem) mit explizit gesetzten Credentials konfiguriert; leere/Default-Credentials werden nicht akzeptiert.
- **MUST:** Blob-Backend-Verbindungsfehler werden als expliziter Fehler propagiert; kein Silent-Fallback auf lokalen Speicher ohne Konfiguration.
- **MUST NOT:** Unverschlüsselte Blob-Übertragung in Produktionsdeployments verwenden.

### 3) Adaptive Compaction

- **MUST:** `adaptive_compaction.cpp` mit konfiguriertem I/O-Budget und Scheduling; unkontrollierte Compaction kann Produktions-Latenz beeinträchtigen.

## Betriebsgrenzen (aktuelles Storage-Verhalten)

- Backup-Retention-Window muss mit RPO/RTO-Anforderungen des Deployments abgestimmt sein.
- S3/GCS/Azure-Backends erfordern konfigurierte Retry- und Timeout-Policies; Standard-HTTP-Timeouts ohne Konfiguration sind unzureichend.
- Storage-Compaction ist I/O-intensiv; Compaction-Scheduling außerhalb von Peak-Zeiten empfohlen.

## Minimaler Produktions-Check (Audit-fähig)

- [ ] WAL aktiviert (kein In-Memory-Only ohne Deklaration)
- [ ] WAL-Replay nach Neustart vollständig
- [ ] Backup-Manager mit explizitem Backup-Ziel und Zeitplan konfiguriert
- [ ] PITR konfiguriert wenn RPO-Anforderungen dies verlangen
- [ ] Security-Signature-Pfad aktiv
- [ ] `SecuritySignatureManager` läuft gegen RocksDB oder ein ausdrücklich dokumentiertes test-only Fallback
- [ ] Blob-Backend-Credentials explizit gesetzt
- [ ] Storage-Audit-Logging aktiv
- [ ] Produktionsmodus via `THEMIS_PRODUCTION_MODE` oder `THEMIS_ENVIRONMENT` gesetzt

## Review / Sourcecode-Audit-Nachweis

### Betroffene Dateien im Review

- `src/storage/PRODUCTION_REQUIREMENTS.md`
- `src/storage/wal_storage.cpp`
- `src/storage/backup_manager.cpp`
- `src/storage/pitr_manager.cpp`
- `src/storage/security_signature.cpp`
- `src/storage/adaptive_compaction.cpp`
- `src/storage/blob_backend_s3.cpp`
- `src/storage/blob_backend_gcs.cpp`
- `src/storage/blob_backend_azure.cpp`
