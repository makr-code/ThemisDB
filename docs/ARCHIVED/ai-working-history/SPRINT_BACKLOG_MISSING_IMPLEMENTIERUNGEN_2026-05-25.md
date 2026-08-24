# Sprint Backlog: Fehlende Implementierungen

Stand: 2026-05-25
Quelle: ai_working/TODO_MISSING_IMPLEMENTIERUNGEN_2026-05-25.md

## Ziele

- Kritische Integrationsluecken in verteilten Pfaden schliessen.
- Wire-Protokoll-Paritaet zu REST fuer Kernfunktionen herstellen.
- Placeholder-Pfade in Backup/Training auf produktive Pfade umstellen.

## Aufwandsskala

- S: 1-2 PT
- M: 3-5 PT
- L: 6-10 PT
- XL: 11+ PT

## Sprint 1 (Stabilisierung, 2 Wochen)

### BL-01: 2PC Phase-2 Remote-Zustellung fail-closed
- Prioritaet: P0
- Aufwand: M
- Datei/Funktion:
  - src/transaction/distributed_transaction_manager.cpp
  - DistributedTransactionManager::runPhase2Unlocked
- Aufgabe:
  - Skip-Pfad fuer Remote-Teilnehmer eliminieren.
  - Bei fehlender Remote-Dispatch-Konfiguration transaktionalen Fehler erzwingen statt stilles Weiterlaufen.
  - Telemetrie fuer zugestellte/nicht-zugestellte Phase-2-Entscheidungen ergaenzen.
- Abhaengigkeiten:
  - Vorhandene phase2_rpc_fn/remote_phase2_dispatch Konfiguration in Produktionsprofilen.
- Testfaelle:
  - Unit: callback-less participant ohne dispatcher => fail-closed.
  - Unit: dispatcher liefert false/throw => Fehlerstatus und sauberer Abort.
  - Integration: Multi-node COMMIT/ABORT wird fuer alle Teilnehmer zugestellt.
- Definition of Done:
  - Kein stilles Skip von Remote-Teilnehmern mehr.
  - CTest fuer 2PC-Remote-Szenarien gruen.

### BL-02: Protobuf Wire (themis::wire) Kernhandler produktiv verdrahten
- Prioritaet: P0
- Aufwand: L
- Datei/Funktionen:
  - src/themis/wire_protocol_server.cpp
  - handle_query_aql, handle_cursor_next, handle_cursor_close, handle_geo_query, handle_timeseries_query, handle_graph_traverse
- Aufgabe:
  - Standard-Wiring fuer Query/Geo/TS/Graph beim Serverstart herstellen.
  - 501/503 nur noch bei explizit deaktivierten Features erlauben.
  - Fehlercodes und Fehlermeldungen vereinheitlichen.
- Abhaengigkeiten:
  - QueryEngine, Geo/TS/Graph Manager im Server-Setup verfuegbar.
- Testfaelle:
  - Integration: Protobuf-Client AQL happy path.
  - Integration: Cursor paging close/reopen/ttl.
  - Integration: GEO_QUERY und GRAPH_TRAVERSE gegen bekannte Testdaten.
  - Negativ: Feature explizit deaktiviert => konsistente Fehlermeldung.
- Definition of Done:
  - Keine 501/503 Fallbacks bei voll konfigurierter Instanz.
  - Protobuf-Regressionstests im CI-Set gruen.

### BL-03: JSON Wire (network::WireProtocolServer) Feature-Paritaet fuer Graph/AQL/Geo
- Prioritaet: P0
- Aufwand: L
- Datei/Funktionen:
  - src/network/wire_protocol_server.cpp
  - Session::handleGraphTraverse, Session::handleQuery, Session::handleGeoQuery
- Aufgabe:
  - Integrations-Guards vom Laufzeit-Fallback auf Startup-Validierung umstellen.
  - QueryEngine/SpatialIndex bei aktivierten Opcodes als Pflichtabhaengigkeit validieren.
- Abhaengigkeiten:
  - Server-Konfig-Validierung im Bootpfad.
- Testfaelle:
  - Startup-Test: fehlender QueryEngine bei aktivierten Opcodes => Start verweigert.
  - Integration: Wire-Client Graph/AQL/Geo liefert gleiche Ergebnisstruktur wie REST.
- Definition of Done:
  - Kein NOT_INTEGRATED in produktiv konfigurierten Profilen.

## Sprint 2 (Produktivpfade, 2 Wochen)

### BL-04: Cloud Backup Provider finalisieren (S3/Azure/GCS)
- Prioritaet: P1
- Aufwand: XL
- Datei:
  - src/sharding/cloud_backup.cpp
- Offene Kernfunktionen:
  - S3: deleteObject, listObjects, exists
  - Azure: upload, download, deleteObject, listObjects, exists
  - GCS: upload, download, deleteObject, listObjects, exists
- Aufgabe:
  - SDK-basierte Implementierungen oder verbindliche Produktions-Adapter pro Provider.
  - Mock-Mode strikt auf Testprofile begrenzen.
  - Einheitliches Retry/Timeout/Fehler-Mapping einbauen.
- Abhaengigkeiten:
  - SDK-Verfuegbarkeit in Buildprofilen und Secrets fuer Integrationsumgebung.
- Testfaelle:
  - Integration je Provider: upload/download/delete/list/exists.
  - Chaos/Retry: transienter Fehler, Auth-Fehler, Netzwerk-Timeout.
  - E2E: Backup erstellen, auflisten, wiederherstellen, loeschen.
- Definition of Done:
  - Kein Placeholder/no-op Verhalten im Produktionsmodus.
  - Provider-Matrix in Testreport gruen.

### BL-05: PITR mit WAL-Replay als Standardpfad
- Prioritaet: P1
- Aufwand: M
- Datei/Funktion:
  - src/storage/backup_manager.cpp
  - BackupManager::performPITR
- Aufgabe:
  - WalReplayFn in produktiven Pfaden verpflichtend machen.
  - Fehlende Replay-Engine als Betriebsfehler kennzeichnen.
  - Zeitfenstergrenzen und Replay-Metriken dokumentieren.
- Abhaengigkeiten:
  - Verfuegbarer WAL Reader/Replay Service.
- Testfaelle:
  - Integration: Snapshot + WAL Delta bis target_time.
  - Negativ: fehlende Replay-Engine => klarer Fehler statt stilles Degradieren.
- Definition of Done:
  - PITR erzielt target_time-korrektes Restore in Testmatrix.

### BL-06: DistributedTrainer Collectives verpflichtend verdrahten
- Prioritaet: P1
- Aufwand: M
- Datei/Funktionen:
  - src/llm/lora_framework/distributed_trainer.cpp
  - allreduce_cpu, broadcast_cpu
- Aufgabe:
  - world_size > 1 ohne Collective-Callbacks nicht mehr zulassen.
  - Runtime-Pruefung und klare Fehlermeldung beim Start.
- Abhaengigkeiten:
  - MPI/Gloo/NCCL-Backend Initialisierung.
- Testfaelle:
  - Unit: world_size > 1 + no callback => Fehler.
  - Integration: Multi-rank gradients konsistent nach allreduce.
- Definition of Done:
  - Kein stilles lokales Skalieren/no-op bei verteiltem Training.

## Sprint 3 (Optimierung/Hardening)

### BL-07: LZ4 Pfad in secure_transport_client aktivieren
- Prioritaet: P2
- Aufwand: S
- Datei/Funktion:
  - src/sharding/secure_transport_client.cpp::compressPayload
- Aufgabe:
  - LZ4 Zweig plus Dekompressionspfad und Negotiation vervollstaendigen.
- Testfaelle:
  - Kompatibilitaetstests LZ4 vs zstd/uncompressed.

### BL-08: Wire Bridge Wiring zentralisieren
- Prioritaet: P2
- Aufwand: M
- Dateien:
  - src/themis/wire_protocol_server.cpp
  - src/network/wire_protocol_server.cpp
- Aufgabe:
  - zentrale Bootstrapping-Stelle fuer setWire*Fn und setNetworkGeoQueryFn schaffen.
  - Startvalidierung fuer Pflicht-Handler aufnehmen.
- Testfaelle:
  - Startup-Regressionen fuer alle Wire-Profile.

## Abhaengigkeiten uebergreifend

- BL-01 ist blocker fuer verteilte Konsistenz.
- BL-02 und BL-03 koennen parallel laufen, benoetigen aber abgestimmte Fehlersemantik.
- BL-04 benoetigt Build-/Secret-Infrastruktur und kann parallel zu BL-05 laufen.
- BL-06 kann parallel zu BL-05 umgesetzt werden.

## Vorschlag Reihenfolge nach Risiko

1. BL-01
2. BL-02
3. BL-03
4. BL-05
5. BL-06
6. BL-04
7. BL-07
8. BL-08

## Tracking Felder (fuer Jira/GitHub Issues)

Pro Backlog-Item erfassen:
- Owner
- Ziel-Sprint
- Status (Todo/In Progress/Blocked/Done)
- PR-Link
- Testnachweis (CTest Filter + Ergebnis)
- Rollback-Plan
