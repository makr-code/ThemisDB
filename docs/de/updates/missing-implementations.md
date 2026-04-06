# Updates-Modul — Fehlende / Unvollständige Implementierungen

<!-- Status: current | validated: 2026-04-06 | updated: 2026-03-20 -->
<!-- Primärdokumentation: ../../../src/updates/ -->

Dieser Report dokumentiert Funktionen und Komponenten, die in `src/updates/ROADMAP.md`, `src/updates/README.md` oder anderen Primary-Docs als implementiert beschrieben werden oder als geplant gelten, jedoch bei der Reality-Check-Prüfung als **nicht vollständig dokumentiert**, **fehlerhaft referenziert** oder **teilweise umgesetzt** befunden wurden.

Prüfstand: 2026-03-20 | Branch: `develop`

---

## 1. README.md — Veraltete Dateireferenzen — ✅ Behoben

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/updates/README.md` §"Relevant Interfaces" (Tabelle Zeilen 9–14) |
| **Erwartet** | Tabelle zeigt korrekte, existierende Quelldateien für die Kern-Interfaces |
| **Beobachtet** | 4 Dateien referenziert, die **nicht existieren**: `schema_migrator.cpp`, `update_manager.cpp`, `version_tracker.cpp`, `migration_registry.cpp` |
| **Evidence** | `ls src/updates/*.cpp` — keine dieser Dateien vorhanden. Tatsächliche Dateien: `in_place_schema_migrator.cpp`, `coordinated_update_manager.cpp`, `cluster_update_manager.cpp`, `update_history_logger.cpp`, `dependency_resolver.cpp` |
| **Status** | ✅ **Behoben** — Tabelle korrigiert (2026-03-20) |
| **Lösung** | `src/updates/README.md` §"Relevant Interfaces": 4 veraltete Einträge durch 5 korrekte Einträge ersetzt |
| **Issue-Titelvorschlag** | `[updates] README.md Relevant Interfaces table references non-existent files` |
| **Label-Vorschläge** | `type:documentation`, `priority:medium`, `updates`, `status:resolved` |

---

## 2. ROADMAP.md — Falsche CI-Workflow-Pfade — ✅ Behoben

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/updates/ROADMAP.md` (6 CI-Workflow-Referenzen) |
| **Erwartet** | CI-Pfade zeigen auf die tatsächlichen Workflow-Dateien im Repository |
| **Beobachtet** | 5 von 7 CI-Workflow-Pfade fehlen die Unterverzeichnis-Präfixe (z.B. `02-feature-modules/storage/`) |
| **Evidence** | `find .github/workflows -name "binary-delta-patches-ci.yml"` → `.github/workflows/02-feature-modules_storage_binary-delta-patches-ci.yml` (nicht `.github/workflows/binary-delta-patches-ci.yml`) |
| **Betroffene Pfade** | `canary-deployments-ci.yml` → `.github/workflows/04-release/`; `binary-delta-patches-ci.yml` → `.github/workflows/02-feature-modules/storage/`; `parallel-file-downloads-ci.yml` → `.github/workflows/02-feature-modules/storage/`; `dependency-resolution-engine-ci.yml` → `.github/workflows/02-feature-modules/`; `multi-tenant-update-scheduling-ci.yml` → `.github/workflows/02-feature-modules/` |
| **Status** | ✅ **Behoben** — Alle 5 Pfade korrigiert; 2 fehlende CI-Referenzen ergänzt (2026-03-20) |
| **Lösung** | `src/updates/ROADMAP.md`: Korrekte Unterverzeichnis-Pfade eingetragen; Referenzen für `manifest-database-file-deletion-ci.yml` und `distributed-cluster-updates-ci.yml` neu hinzugefügt |
| **Issue-Titelvorschlag** | `[updates] ROADMAP.md CI workflow paths are missing subdirectory prefixes` |
| **Label-Vorschläge** | `type:documentation`, `priority:low`, `updates`, `status:resolved` |

---

## 3. ROADMAP.md — Fehler bei Anzahl der Test-Targets — ✅ Behoben

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/updates/ROADMAP.md` §Completed: "9 standalone focused test targets added to tests/CMakeLists.txt" |
| **Erwartet** | Anzahl stimmt mit der Liste überein |
| **Beobachtet** | Text sagt "9" aber listet 10 Targets: UpdatesProduction, BlueGreen, CanaryRollout, BinaryDeltaPatches, CoordinatedUpdateManager, InPlaceSchemaMigrator, NotificationWebhook, PreflightHealthCheck, SchemaMigrationTester, ParallelFileDownloads |
| **Evidence** | Manuelle Zählung der Listeneinträge in der Klammer |
| **Status** | ✅ **Behoben** — Korrigiert auf "10 standalone focused test targets" (2026-03-20) |
| **Lösung** | `src/updates/ROADMAP.md`: Text von "9" auf "10" korrigiert |
| **Issue-Titelvorschlag** | `[updates] ROADMAP.md test target count off-by-one (9 vs 10)` |
| **Label-Vorschläge** | `type:documentation`, `priority:low`, `updates`, `status:resolved` |

---

## 4. Kubernetes Operator Integration — ⚠️ Offen (kein Issue-Tracker-Eintrag benötigt)

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/updates/ROADMAP.md` §"Long-term (6-12 months)" / Phase 4 |
| **Erwartet** | `[!]` Kubernetes operator integration (rolling update coordination) — Issue #2483 |
| **Beobachtet** | Kein Code in `src/updates/` oder `include/updates/` für Kubernetes-Integration vorhanden. Die `ClusterUpdateManager`-Klasse ist transport-agnostisch (Callbacks), aber kein K8s-Operator implementiert. |
| **Evidence** | `grep -r "kubernetes\|k8s\|operator" src/updates/ include/updates/` → keine Treffer |
| **Status** | `[!]` **Offen** — bewusst als langfristiges Ziel markiert (Status `unclear`). Nicht blockierend für Production. |
| **Roadmap-Status** | `[!]` in ROADMAP.md §Long-term — kein kurzfristiger Handlungsbedarf |
| **Issue-Titelvorschlag** | `[updates] Kubernetes operator integration for rolling update coordination (Issue #2483)` |
| **Label-Vorschläge** | `type:feature`, `priority:low`, `updates`, `status:planned` |

---

## 5. FUTURE_ENHANCEMENTS.md — Veraltete Metadaten — ✅ Behoben

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/updates/FUTURE_ENHANCEMENTS.md` (Footer + Status-Header) |
| **Erwartet** | Footer und Status-Header spiegeln den aktuellen Entwicklungsstand wider |
| **Beobachtet** | Footer: "Last Updated: February 2026", "Module Version: v1.5.x", "Next Review: v1.6.0 Release" — steht im Widerspruch zu v1.8.0 Release und März 2026 Stand. Status-Header: `validated: 2026-04-06` |
| **Evidence** | `src/updates/ROADMAP.md` §"Current Status": v1.7.0 als Production-ready, v1.8.0 multi-tenant fertig |
| **Status** | ✅ **Behoben** — Footer auf v1.8.0 / März 2026 aktualisiert; validated-Datum auf 2026-03-20 (2026-03-20) |
| **Lösung** | `src/updates/FUTURE_ENHANCEMENTS.md`: Footer-Zeilen und Status-Header korrigiert |

---

## Zusammenfassung

| # | Befund | Quelle | Kritikalität | Status |
|---|---|---|---|---|
| 1 | README.md veraltete Dateireferenzen (4 Dateien) | `src/updates/README.md` | **Mittel** | ✅ Behoben (2026-03-20) |
| 2 | ROADMAP.md CI-Workflow-Pfade falsch (5 Pfade) | `src/updates/ROADMAP.md` | Niedrig | ✅ Behoben (2026-03-20) |
| 3 | ROADMAP.md Test-Target-Zählfehler (9 statt 10) | `src/updates/ROADMAP.md` | Niedrig | ✅ Behoben (2026-03-20) |
| 4 | Kubernetes Operator-Integration fehlt | ROADMAP §Long-term | Niedrig | ⚠️ Offen (langfristig) |
| 5 | FUTURE_ENHANCEMENTS.md veraltete Metadaten | `src/updates/FUTURE_ENHANCEMENTS.md` | Niedrig | ✅ Behoben (2026-03-20) |

*Alle kritischen und mittleren Items sind behoben. Das offene Kubernetes-Item ist bewusst als langfristiges Ziel markiert (`[!]`) und nicht produktionsblockierend.*
