# TODO: Wichtigste fehlende Implementierungen in src/<module>

Stand: 2026-05-25

Basis dieser Liste:
- ai_working/active_stubs.txt (aktuelle offene Stub-IDs 279+)
- ai_working/gap_scan_v3_summary.json (Schwerpunktmodule)
- ai_working/module_gaps/MODULE_GAPS_INDEX.md (Modulpriorisierung)
- Verifikation direkt im Quellcode unter src/

Hinweis: Mehrere Eintraege aus ai_working sind inzwischen ueber Bridge-APIs teilweise aufgeloest. Diese TODO enthaelt nur die wichtigsten, im Code weiterhin funktional offenen Punkte.

## P0 Kritisch (sofort)

- [ ] Distributed 2PC Phase-2 End-to-End fuer Remote-Teilnehmer final verdrahten
  - Funktion: src/transaction/distributed_transaction_manager.cpp::DistributedTransactionManager::runPhase2Unlocked
  - Problem: Ohne konfigurierte remote dispatch/rpc callback wird COMMIT/ABORT fuer Remote-Teilnehmer uebersprungen.
  - Impact: Orphaned prepared transactions, inkonsistenter Clusterzustand.
  - Akzeptanz: Kein Skip-Pfad mehr im Produktivmodus; Remote-Entscheidung wird immer zugestellt oder der Coordinator failt hard/fail-closed.

- [ ] Protobuf-Wire-Server (themis::wire) produktiv anbinden statt 501/503-Fallback
  - Funktionen:
    - src/themis/wire_protocol_server.cpp::WireProtocolSession::handle_query_aql
    - src/themis/wire_protocol_server.cpp::WireProtocolSession::handle_geo_query
    - src/themis/wire_protocol_server.cpp::WireProtocolSession::handle_graph_traverse
    - src/themis/wire_protocol_server.cpp::WireProtocolSession::handle_timeseries_query
  - Problem: Handler fallen ohne gesetzte Bridge-Callbacks auf 501/503 zurueck.
  - Impact: Protobuf-Clients koennen Kernfeatures nicht nutzen.
  - Akzeptanz: Startup-Wiring fuer AQL/Cursor/Geo/TS/Graph vorhanden; keine 501/503 fuer konfigurierte Produktionsinstanzen.

- [ ] JSON-Wire-Server (network::WireProtocolServer) fuer Graph/AQL/Geo voll integrieren
  - Funktionen:
    - src/network/wire_protocol_server.cpp::WireProtocolServer::Session::handleGraphTraverse
    - src/network/wire_protocol_server.cpp::WireProtocolServer::Session::handleQuery
    - src/network/wire_protocol_server.cpp::WireProtocolServer::Session::handleGeoQuery
  - Problem: Bei fehlendem query_engine_/spatial_index_ werden GRAPH_NOT_INTEGRATED / AQL_NOT_INTEGRATED / GEO_NOT_INTEGRATED geliefert.
  - Impact: Native Wire-Clients weichen auf REST aus; Feature-Paritaet fehlt.
  - Akzeptanz: QueryEngine und SpatialIndex als verpflichtende Dependencies fuer Wire-Profile mit diesen Opcodes oder klarer fail-fast beim Serverstart.

## P1 Hoch (naechster Sprint)

- [ ] Cloud-Backup Provider produktiv machen (S3/Azure/GCS)
  - Datei: src/sharding/cloud_backup.cpp
  - Offene Funktionen (wichtigste):
    - S3StorageProvider::deleteObject, ::listObjects, ::exists
    - AzureStorageProvider::upload, ::download, ::deleteObject, ::listObjects, ::exists
    - GCSStorageProvider::upload, ::download, ::deleteObject, ::listObjects, ::exists
  - Problem: Placeholder/no-op oder mock-only Verhalten.
  - Impact: Backup-Lifecycle (Inventar, Existenzpruefung, Loeschung, Restore) in Cloud unvollstaendig.
  - Akzeptanz: Reale SDK-Pfade aktiv (oder verbindlich injizierte Produktions-Callbacks), Integrationstests gegen echte/ephemere Buckets.

- [ ] PITR WAL-Replay standardmaessig integrieren
  - Funktion: src/storage/backup_manager.cpp::BackupManager::performPITR (wal_replay_fn_ Pfad)
  - Problem: Ohne WalReplayFn nur Snapshot-Restore, keine Delta-Replays bis target_time.
  - Impact: PITR-Genauigkeit auf Snapshot-Granularitaet begrenzt.
  - Akzeptanz: WAL-Replay standardmaessig verfuegbar; fehlende Replay-Engine wird als klarer Betriebsfehler behandelt (kein stilles Degradieren).

- [ ] DistributedTrainer: echte AllReduce/Broadcast Verdrahtung in Runtime-Setup
  - Datei: src/llm/lora_framework/distributed_trainer.cpp
  - Funktionen:
    - DistributedTrainer::allreduce_cpu
    - DistributedTrainer::broadcast_cpu
  - Problem: Ohne gesetzte Fn bleibt nur lokale Skalierung bzw. no-op Broadcast.
  - Impact: Multi-Rank-Training divergiert.
  - Akzeptanz: Beim Start der Distributed-Trainingspipeline werden notwendige Collective-Callbacks verpflichtend gesetzt (MPI/Gloo/NCCL je Backend).

## P2 Mittel (nach Stabilisierung)

- [ ] LZ4-Transport im Sharding-Client vervollstaendigen
  - Datei: src/sharding/secure_transport_client.cpp
  - Funktion: compressPayload
  - Problem: LZ4-Pfad als Platzhalter, zstd/uncompressed dominiert.
  - Impact: Hoehere Latenz/Bandbreite bei geeigneten Payloads.

- [ ] Wire-Bridge-Setzer zentral am Serverstart konsolidieren und absichern
  - Dateien:
    - src/themis/wire_protocol_server.cpp (setWire*Fn)
    - src/network/wire_protocol_server.cpp (setNetworkGeoQueryFn)
  - Problem: Setzer vorhanden, aber keine sichtbare globale Initialisierungsstelle in src/.
  - Impact: Laufzeitverhalten abhaengig von impliziter externen Verdrahtung.

## Empfohlene Reihenfolge

1. P0-1 (2PC Phase-2)
2. P0-2 und P0-3 (Wire-Protokoll-Paritaet)
3. P1-1 (Cloud Backup)
4. P1-2 (PITR WAL Replay)
5. P1-3 (DistributedTrainer Collectives)
6. P2 Punkte
