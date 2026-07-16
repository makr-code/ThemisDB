# Issue Set: Fehlende Implementierungen

Stand: 2026-05-25
Quelle:
- ai_working/SPRINT_BACKLOG_MISSING_IMPLEMENTIERUNGEN_2026-05-25.md
- ai_working/TODO_MISSING_IMPLEMENTIERUNGEN_2026-05-25.md

Verwendung:
- Ein Issue pro Block anlegen.
- Labels und Milestones bei Bedarf direkt uebernehmen.

---

## ISSUE-01

Titel:
[DTX] 2PC Phase-2 fuer Remote-Teilnehmer fail-closed verdrahten

Labels:
- area:transaction
- priority:P0
- type:implementation
- risk:consistency

Milestone:
- v2.0.0

Beschreibung:
Aktuell kann der Coordinator in der Phase-2-Zustellung Remote-Teilnehmer ueberspringen, wenn keine Remote-Dispatch-Konfiguration vorhanden ist. Das fuehrt zu inkonsistenten Commit/Abort-Zustaenden und orphaned prepared transactions.

Betroffene Stelle:
- src/transaction/distributed_transaction_manager.cpp
- DistributedTransactionManager::runPhase2Unlocked

Ziel:
- Kein stilles Skip mehr fuer Remote-Teilnehmer.
- Fehlende Dispatch-Konfiguration muss in produktiven Pfaden zu fail-closed fuehren.

Akzeptanzkriterien:
- Bei Remote-Teilnehmern ohne Dispatch wird die Transaktion deterministisch als Fehler/Abort behandelt.
- Telemetrie zaehlt erfolgreiche und fehlgeschlagene Phase-2-Zustellungen.
- Kein warn-only Skip-Pfad mehr im produktiven Modus.

Test-Checklist:
- Unit: callback-less participant ohne dispatcher -> fail-closed.
- Unit: dispatcher throw/false -> Fehlerstatus und sauberer Abort.
- Integration: Multi-node Commit/Abort an alle Teilnehmer zugestellt.
- Regression: bestehende lokale 2PC-Tests bleiben gruen.

Out of Scope:
- Vollstaendige neue RPC-Transportschicht.

---

## ISSUE-02

Titel:
[Wire/Protobuf] AQL/Cursor/Geo/TS/Graph produktiv anbinden statt 501/503

Labels:
- area:themis
- area:network
- priority:P0
- type:implementation
- risk:api-parity

Milestone:
- v2.0.0

Beschreibung:
Im Protobuf-Wire-Server fallen zentrale Handler ohne gesetzte Bridges auf 501/503 zurueck. Dadurch fehlt Feature-Paritaet gegenueber REST/JSON-Wire.

Betroffene Stellen:
- src/themis/wire_protocol_server.cpp
- WireProtocolSession::handle_query_aql
- WireProtocolSession::handle_cursor_next
- WireProtocolSession::handle_cursor_close
- WireProtocolSession::handle_geo_query
- WireProtocolSession::handle_timeseries_query
- WireProtocolSession::handle_graph_traverse

Ziel:
- Standard-Wiring fuer Kernhandler beim Serverstart.
- 501/503 nur noch bei explizit deaktivierten Features.

Akzeptanzkriterien:
- Konfigurierte Instanz verarbeitet AQL/Cursor/Geo/TS/Graph ueber Protobuf erfolgreich.
- Fehlercodes und Fehlermeldungen sind ueber alle Handler konsistent.
- Fallback-Hinweise auf REST bleiben nur fuer explizit deaktivierte Features.

Test-Checklist:
- Integration: Protobuf AQL happy path.
- Integration: Cursor paging inklusive close und TTL.
- Integration: Geo query gegen Testdaten.
- Integration: Graph traversal gegen Testgraph.
- Negativ: Feature deaktiviert -> erwarteter Fehlercode/Message.

Out of Scope:
- Neue Query-Semantik.

---

## ISSUE-03

Titel:
[Wire/JSON] Graph/AQL/Geo auf Startup-validierte Pflichtabhaengigkeiten umstellen

Labels:
- area:network
- priority:P0
- type:implementation
- risk:api-parity

Milestone:
- v2.0.0

Beschreibung:
Der JSON-Wire-Server liefert bei fehlender Engine-Lage NOT_INTEGRATED-Antworten in Laufzeitpfaden. Fuer produktive Profile sollen diese Features entweder korrekt verdrahtet sein oder der Serverstart fail-fast abbrechen.

Betroffene Stellen:
- src/network/wire_protocol_server.cpp
- WireProtocolServer::Session::handleGraphTraverse
- WireProtocolServer::Session::handleQuery
- WireProtocolServer::Session::handleGeoQuery

Ziel:
- Pflichtabhaengigkeiten fuer aktivierte Opcodes beim Start validieren.
- Keine laufzeitseitigen NOT_INTEGRATED-Pfade in produktiven Profilen.

Akzeptanzkriterien:
- Fehlende QueryEngine/SpatialIndex bei aktivierten Opcodes verhindert Serverstart.
- Bei gueltiger Konfiguration liefern Handler echte Ergebnisse statt NOT_INTEGRATED.

Test-Checklist:
- Startup-Test: fehlende Dependencies -> Start verweigert.
- Integration: Graph/AQL/Geo ueber JSON-Wire liefern valide Antworten.
- Regression: REST-Verhalten unveraendert.

Out of Scope:
- Erweiterung des Wire-Protokolls um neue Opcodes.

---

## ISSUE-04

Titel:
[Backup/Cloud] S3/Azure/GCS Provider von Placeholder auf Produktionspfade umstellen

Labels:
- area:sharding
- area:storage
- priority:P1
- type:implementation
- risk:data-protection

Milestone:
- v2.3.0

Beschreibung:
Mehrere Cloud-Provider-Funktionen sind aktuell placeholder/no-op oder mock-only. Dadurch sind Inventar, Existenzpruefung, Loeschung und Teile des Restore-Lifecycle unvollstaendig.

Betroffene Stelle:
- src/sharding/cloud_backup.cpp

Kernfunktionen:
- S3: deleteObject, listObjects, exists
- Azure: upload, download, deleteObject, listObjects, exists
- GCS: upload, download, deleteObject, listObjects, exists

Ziel:
- SDK- oder verbindliche Produktions-Adapter in allen Kernfunktionen.
- Mock-Mode nur in Testprofilen.

Akzeptanzkriterien:
- Alle Kernfunktionen liefern in Produktionsprofilen echte Provider-Operationen.
- Fehlerbehandlung, Retry und Timeout sind konsistent.
- Keine false-negativen exists/list Standardrueckgaben mehr.

Test-Checklist:
- Integration je Provider: upload/download/delete/list/exists.
- Negativ: Auth-Fehler, Timeout, transienter Netzwerkfehler.
- E2E: Backup erstellen, auflisten, wiederherstellen, loeschen.

Out of Scope:
- Kostenoptimierung und Lifecycle-Policies im Cloud-Konto.

---

## ISSUE-05

Titel:
[PITR] WAL-Replay als verpflichtenden Standardpfad integrieren

Labels:
- area:storage
- priority:P1
- type:implementation
- risk:data-correctness

Milestone:
- v2.1.0

Beschreibung:
PITR kann derzeit ohne gesetztes Replay-Backend erfolgreich enden, aber nur auf Snapshot-Genauigkeit. Das ist fuer echte Point-in-Time-Ansprueche nicht ausreichend.

Betroffene Stelle:
- src/storage/backup_manager.cpp
- BackupManager::performPITR

Ziel:
- WAL-Replay in produktiven Pfaden verpflichtend machen.
- Fehlendes Replay-Backend klar als Betriebsfehler markieren.

Akzeptanzkriterien:
- PITR ohne Replay-Engine liefert keine still degradierte Erfolgsmeldung.
- Replay bis target_time wird in Logs/Metriken nachvollziehbar dokumentiert.

Test-Checklist:
- Integration: Snapshot + WAL-Delta bis target_time.
- Negativ: fehlende Replay-Engine -> klarer Fehler.
- Regression: normales Snapshot-Restore bleibt unveraendert.

Out of Scope:
- Neue WAL-Formate.

---

## ISSUE-06

Titel:
[LLM/Distributed] AllReduce/Broadcast bei world_size>1 verpflichtend verdrahten

Labels:
- area:llm
- priority:P1
- type:implementation
- risk:training-correctness

Milestone:
- v2.2.0

Beschreibung:
Ohne gesetzte Collectives bleibt distributed_trainer in lokalen Fallbacks (lokale Skalierung/no-op Broadcast). Das fuehrt in echten Multi-Rank-Setups zu Divergenz.

Betroffene Stelle:
- src/llm/lora_framework/distributed_trainer.cpp
- DistributedTrainer::allreduce_cpu
- DistributedTrainer::broadcast_cpu

Ziel:
- world_size > 1 ohne gesetzte Collectives als Konfigurationsfehler behandeln.

Akzeptanzkriterien:
- Kein stiller Fallback bei world_size > 1.
- Klarer Startup-/Init-Fehler ohne Collectives.
- Dokumentierter Wiring-Pfad fuer MPI/Gloo/NCCL.

Test-Checklist:
- Unit: world_size > 1 + kein Callback -> Fehler.
- Integration: Multi-rank Gradienten konsistent nach AllReduce.
- Regression: world_size == 1 bleibt funktionsfaehig.

Out of Scope:
- Implementierung neuer Collective-Backends.

---

## ISSUE-07

Titel:
[Sharding Transport] LZ4-Kompressionspfad vollstaendig implementieren

Labels:
- area:sharding
- priority:P2
- type:implementation
- risk:performance

Milestone:
- v2.1.0

Beschreibung:
Der Transportpfad nutzt vorwiegend zstd/uncompressed, waehrend LZ4 als low-latency Option unvollstaendig ist.

Betroffene Stelle:
- src/sharding/secure_transport_client.cpp
- compressPayload

Ziel:
- LZ4-Kompression und passende Dekompression/Negotiation produktiv verfuegbar.

Akzeptanzkriterien:
- LZ4 kann konfiguriert und erfolgreich ausgehandelt werden.
- End-to-End Payloads sind kompatibel und korrekt dekomprimierbar.

Test-Checklist:
- Unit: LZ4 roundtrip.
- Integration: Mixed cluster zstd/LZ4 Negotiation.
- Performance: Latenzvergleich fuer kleine Payloads.

Out of Scope:
- Kompressions-Autotuning.

---

## ISSUE-08

Titel:
[Wire Bootstrapping] Zentrale Verdrahtung der Bridge-Setter mit Startvalidierung

Labels:
- area:network
- area:themis
- priority:P2
- type:hardening
- risk:operability

Milestone:
- v2.0.0

Beschreibung:
Bridge-Setter sind vorhanden, aber ohne klaren zentralen Bootstrap-Punkt ist das Laufzeitverhalten implizit und fehleranfaellig.

Betroffene Stellen:
- src/themis/wire_protocol_server.cpp (setWire*Fn)
- src/network/wire_protocol_server.cpp (setNetworkGeoQueryFn)

Ziel:
- Eine zentrale Initialisierung fuer alle relevanten Wire-Bridges.
- Konsistente Startvalidierung fuer Pflichtpfade.

Akzeptanzkriterien:
- Startup meldet fehlende Pflichtverdrahtung explizit.
- Wire-Profilverhalten ist reproduzierbar und dokumentiert.

Test-Checklist:
- Startup-Tests fuer vollstaendige und unvollstaendige Verdrahtung.
- Regression: bestehende Wire-Use-Cases bleiben intakt.

Out of Scope:
- Refactoring auf voellig neues DI-Framework.

---

## Optional: Gemeinsame Checkliste fuer alle Issues

- [ ] Doxygen/API-Doku bei oeffentlichen C++ APIs aktualisiert
- [ ] Unit-Tests fuer neue Pfade vorhanden
- [ ] Mindestens ein Integrationstest pro betroffenem Endpunkt/Flow
- [ ] Logging und Metriken fuer Fehlerpfade ergaenzt
- [ ] Kein neuer Legacy-Fallback ohne explizite Human-Freigabe
