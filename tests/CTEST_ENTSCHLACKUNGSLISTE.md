# CTest Entschlackungsliste und Modularisierung tests/<module>

Stand: 2026-06-04
Scope: Tests in `tests/` mit Fokus auf stabile, schnelle und wartbare CTest-Registrierung.

## 0. Fortschritt (aktueller Stand)

- [x] Basispolicy eingefuehrt: `tests/cmake/TestPolicy.cmake`
- [x] Modul-Registrierung eingefuehrt: `tests/cmake/RegisterModuleTests.cmake`
- [x] Pilotmodul `acceleration` migriert: `BackendRegistryStartupFocusedTests`
- [x] Pilotmodul `query` migriert: `QueryEngineFocusedTests`
- [x] Pilotmodul `replication` migriert: `ReplicationHAFocusedTests`
- [x] Pilotmodul `transaction` migriert: `TransactionAuditorFocusedTests`
- [x] Pilotmodul `utils` migriert: `TimestampUtilsFocusedTests`
- [x] Erste modulare CTest-Presets eingefuehrt (`windows-release-module-*`)
- [x] Groesserer zusammenhaengender Utils-Block migriert: RateLimiter, Interfaces, Standalone, FutureInterfaces
- [x] Groesserer zusammenhaengender Transaction-Block migriert: Batcher, Manager, Deadlock, 2PC, Isolation
- [x] Groesserer zusammenhaengender Query-Block migriert: Federation, Continuous Query, Plan, JIT, Routing
- [x] Groesserer zusammenhaengender Sharding-Block migriert: Core/Router/2PC/Gossip/Repair/Chaos/E2E/Interfaces/Phase5
- [~] Groesserer zusammenhaengender Metadata-Block teilweise migriert: SchemaManager, StatisticsCollector, ColumnLineage, CatalogExporter, AuditLog, ConsistencyChecker, IndexRecommender, DistributedMetadataCatalog
- [~] Transaction Copy-First gestartet: relevante `test_*.cpp` wurden nach `tests/transaction/` gespiegelt; Modul-CMake nutzt lokale Dateien
- [~] Metadata Copy-First gestartet: relevante `test_*.cpp` wurden nach `tests/metadata/` gespiegelt; Modul-CMake nutzt lokale Dateien
- [~] Weitere flache Module verdrahtet: `timeseries`, `chimera`, `storage` mit lokalen Modul-CMakeLists und Root-Dispatcher-Anbindung
- [~] Weitere flache Module verdrahtet: `db`, `process` mit lokalen Modul-CMakeLists und Root-Dispatcher-Anbindung
- [~] Weitere flache Module verdrahtet: `analytics`, `geo`, `temporal` mit lokalen Modul-CMakeLists und Root-Dispatcher-Anbindung
- [~] Weitere flache Module verdrahtet: `exporters`, `index`, `security` mit lokalen Modul-CMakeLists und Root-Dispatcher-Anbindung
- [~] Weitere flache Module verdrahtet: `llm`, `graph`, `rag` mit lokalen Modul-CMakeLists und Root-Dispatcher-Anbindung
- [~] Weitere flache Module verdrahtet: `performance` mit lokaler Modul-CMakeLists und Root-Dispatcher-Anbindung
- [~] Query-Gap-Fix bereinigt: `test_pagerank` und `test_query_cancellation` aus Root nach `tests/query/CMakeLists.txt` migriert
- [~] Integration-Gap-Fix bereinigt: focused `integration/test_*.cpp` aus Root nach `tests/integration/CMakeLists.txt` migriert
- [x] Eigenstaendiges AQL-Modul angelegt: `tests/aql/` spiegelt `src/aql`-Kontext; `test_aql_*.cpp` aus `tests/query/` nach `tests/aql/` verschoben
- [x] Gesamtrest modularisiert: verbleibende `test_*.cpp` aus `tests/`-Root per Prefix in `tests/<prefix>/` verschoben; Root nutzt nun Modul-Autodiscovery
- [x] Root-CMake nach Vollmigration stabilisiert: veraltete `tests/test_*.cpp`-Quellenpfade auf modulare Ziele umgeschrieben; `CMake: Configure (windows-release)` laeuft wieder bis `Generating done`
- [x] Root-CMake weiter bereinigt: tote Focus-Blocks mit entfernten Root-Testdateien entfernt (`QW-33` disabled-Block und obsolete `test_inference_engine_register_model_focused`-Registrierung)
- [x] Semantische Konsolidierung Security/Auth umgesetzt: Security-nahe Prefix-Tests nach `tests/security/` verschoben, Quellordner geleert, Validate via Configure + `ctest -N`
- [x] Semantische Konsolidierung Transport umgesetzt: `wire/ws/websocket/quic/udp/socket/transport` nach `tests/network/` verschoben, `tests/network/CMakeLists.txt` auf `test_*.cpp` umgestellt, Validate via Configure + `ctest -N`
- [x] Semantische Konsolidierung HTTP-Stack umgesetzt: `http/http2/http3/cdn` nach `tests/network/` verschoben, Quellordner geleert, Validate via Configure + `ctest -N`
- [x] API/Server-Grenze geschaerft: servernahe `test_api_*` (Gateway/Integration) nach `tests/server/` verschoben; `tests/server/CMakeLists.txt` auf `test_*.cpp` vereinheitlicht; Validate via Configure + `ctest -N`
- [x] API-Modulregistrierung entschlackt: `tests/api/CMakeLists.txt` von doppelten Prefix/Autogen-Loops auf eine konsolidierte `test_*.cpp`-Registrierung reduziert; Validate via Configure + `ctest -N`
- [x] Bulk-Entschlackung weiterer Modul-CMake-Dateien: 32 Restmodule mit doppelten Prefix/Autogen-Loops auf ein einheitliches `test_*.cpp`-Template normalisiert (u.a. auth/security-rest, transport-rest, http-rest); Validate via Configure + `ctest -N`
- [x] Breiter Safe-Autogen-Cleanup umgesetzt: 182 kurze, reine Autogen-Doppel-Module (`# Auto-generated ...` + `# AUTOGEN PREFIX BLOCK`) auf ein einheitliches Single-Loop-Template (`test_*.cpp`) normalisiert; komplexe handgepflegte Module wurden per Guard (Linecount/Pattern) ausgelassen; Validate via Configure + `ctest -N`
- [x] Komplexmodule abgesichert: `query/metadata/transaction/sharding`-Autogenblöcke um Source-Dedupe-Guard erweitert (skip bei bereits explizit verdrahteten Sources), um Doppelbuilds/duplizierte Focus-Targets zu vermeiden; Validate via Configure + `ctest -N`
- [x] Tensor-Core-Bridge-Testduplikat entschlackt: `tests/test_tensor_core_bridge.cpp` als kanonische Quelle festgelegt; `tests/tensor/test_tensor_core_bridge.cpp` auf Forwarding-Shim umgestellt, um Drift zwischen doppelten Testimplementierungen zu vermeiden
- [~] Root-vs-Modul-Duplikataudit gestartet: aktueller No-Build-Scan identifiziert `PAIR_COUNT=1590` bei `ROOT_CANONICAL_COUNT=1565`; naechste Welle priorisiert Kandidaten mit Mehrfachduplikaten (z. B. `schema/metadata`, `retention`, `rotate`, `statistics`)

Validierungsstand:

1. `BackendRegistryStartupFocusedTests` baut und laeuft erfolgreich.
2. `QueryEngineFocusedTests` baut und laeuft erfolgreich.
3. `ReplicationHAFocusedTests` baut, zeigt aber aktuell zur Laufzeit einen bestehenden Testfehler in der Suite (nicht Link-/Registrierungsbedingt).
4. `TransactionAuditorFocusedTests` baut und laeuft erfolgreich.
5. `TimestampUtilsFocusedTests` baut und laeuft erfolgreich.
6. Das Preset `windows-release-module-utils` filtert korrekt auf `module:utils` und laeuft erfolgreich.
7. Nach Reconfigure umfasst `windows-release-module-utils` jetzt 7 Tests aus dem migrierten Block.
8. Dabei werden bestehende inhaltliche Fehler in `UtilsStandaloneFocusedTests` und `UtilsFutureInterfacesFocusedTests` sichtbar; die Migration selbst registriert und selektiert diese Tests korrekt.
9. Nach Reconfigure umfasst `windows-release-module-transaction` jetzt 6 Tests aus dem migrierten Block.
10. Dabei wird ein bestehender inhaltlicher Fehler in `TransactionDistributed2PCFocusedTests` sichtbar; die restlichen migrierten Transaction-Tests laufen erfolgreich.
11. Nach Reconfigure umfasst `windows-release-module-query` jetzt 8 Tests aus dem migrierten Block.
12. Dabei werden bestehende inhaltliche Fehler in `QueryFederationShardRoutingTests` sowie ein Laufzeitabsturz in `ContinuousQueryEngineTests` sichtbar; die restlichen migrierten Query-Tests laufen erfolgreich.
13. Das neue Preset `windows-release-module-sharding` ist angelegt und laeuft vollstaendig gruen (21/21 Passed).
14. `RedundancyStrategyFocusedTests`, `StreamProtocolFocusedTests`, `CrossShardTransactionFocusedTests` und `GossipConfigManagerFocusedTests` sind wieder regulär im Build (kein `EXCLUDE_FROM_ALL`) und bestehen im Modul-Preset-Lauf.
15. Der komplette Lauf `ctest --preset windows-release-module-sharding --output-on-failure` wurde erfolgreich validiert (0 Failed, 0 Not Run).
16. Metadata wurde erstmals in `tests/metadata/CMakeLists.txt` ausgelagert; der aktuell extrahierte 9er-Block (`SchemaManager`, `StatisticsCollector`, `StatisticsAutoRefresh`, `ColumnLineage`, `CatalogExporter`, `SchemaAuditLog`, `SchemaConsistencyChecker`, `IndexRecommender`, `DistributedMetadataCatalog`) laeuft vollstaendig gruen.
17. Copy-First-Guard eingebaut: `tests/CMakeLists.txt` schliesst modulare Quellen unter `tests/<module>/test_*.cpp` aus `ALL_TEST_SOURCES` aus, damit kopierte Moduldateien nicht doppelt im monolithischen Aggregat gebaut werden.
18. Copy-First Smoke fuer lokale Metadata-Dateien validiert: `SchemaVersionManagerFocusedTests`, `SchemaConstraintsFocusedTests`, `MetadataSnapshotFocusedTests` laufen gruen.
19. Vollvalidierung des Copy-First-Metadata-Blocks erfolgreich: 21/21 Focus-Tests gruen (`SchemaManager` bis `MetadataSnapshot`) nach lokaler Modulpfad-Umschaltung in `tests/metadata/`.
20. Transaction Copy-First umgesetzt (Dateikopie + lokale Pfadumschaltung in `tests/transaction/CMakeLists.txt`); inhaltliche Fehleranalyse/-behebung fuer dieses Modul ist bewusst auf spaeter verschoben.
21. Neue flache Modulwelle umgesetzt: `tests/timeseries/`, `tests/chimera/` und `tests/storage/` besitzen jetzt eigene Modul-CMakeLists, sind im Root-Dispatcher eingehangen und aus dem monolithischen `ALL_TEST_SOURCES` ausgeschlossen.
22. Weitere flache Modulwelle umgesetzt: `tests/db/` und `tests/process/` besitzen jetzt eigene Modul-CMakeLists, sind im Root-Dispatcher eingehangen und aus dem monolithischen `ALL_TEST_SOURCES` ausgeschlossen; die bisherigen Root-Gap-Fix-Blöcke fuer `db/process` wurden entfernt.
23. Weitere flache Modulwelle umgesetzt: `tests/analytics/`, `tests/geo/` und `tests/temporal/` besitzen jetzt eigene Modul-CMakeLists, sind im Root-Dispatcher eingehangen und aus dem monolithischen `ALL_TEST_SOURCES` ausgeschlossen; der bisherige Root-Gap-Fix-Block fuer `temporal` wurde entfernt.
24. Weitere flache Modulwelle umgesetzt: `tests/exporters/`, `tests/index/` und `tests/security/` besitzen jetzt eigene Modul-CMakeLists, sind im Root-Dispatcher eingehangen und aus dem monolithischen `ALL_TEST_SOURCES` ausgeschlossen; der bisherige Root-Gap-Fix-Block fuer `security` wurde entfernt (inkl. `aql_safety_validator`-Sonderquelle jetzt im Modul-CMake).
25. Weitere flache Modulwelle umgesetzt: `tests/llm/`, `tests/graph/` und `tests/rag/` besitzen jetzt eigene Modul-CMakeLists, sind im Root-Dispatcher eingehangen und aus dem monolithischen `ALL_TEST_SOURCES` ausgeschlossen.
26. Nach Pfad-Rewrite + GPU-Makro-Fix (`tests/gpu/${_name}.cpp`) laeuft `CMake: Configure (windows-release)` wieder ohne `Cannot find source file`.
27. `ctest --preset windows-release -N` validiert die Registrierung ohne `Could not find executable`-/`Cannot find`-Meldungen.
28. Weitere flache Modulwelle umgesetzt: `tests/performance/` besitzt jetzt eine eigene Modul-CMakeLists, ist im Root-Dispatcher eingehangen und aus dem monolithischen `ALL_TEST_SOURCES` ausgeschlossen.
29. Root-Entschlackung Query abgeschlossen: der verbliebene Root-Gap-Fix-Block fuer `test_pagerank` und `test_query_cancellation` wurde entfernt; Registrierung liegt nun konsistent in `tests/query/CMakeLists.txt`.
30. Root-Entschlackung Integration abgeschlossen: der verbliebene Root-Gap-Fix-Block fuer focused `integration/test_*.cpp` (inkl. `test_cross_functional_voice_observability` und `test_process_mining_e2e_focused`) wurde entfernt; Registrierung liegt nun konsistent in `tests/integration/CMakeLists.txt`.
31. AQL-Spiegelung umgesetzt: neues Modul `tests/aql/` mit eigener `CMakeLists.txt`; alle `test_aql_*.cpp` wurden aus `tests/query/` dorthin verschoben und im Root-Dispatcher eingebunden.
32. Root-Spiegelung im Breitenlauf umgesetzt: verbleibende Root-Tests wurden per Prefix in `tests/<prefix>/` migriert (inkl. automatischer Modul-CMake-Erzeugung/-Ergaenzung), Root-Dispatcher in `tests/CMakeLists.txt` auf Auto-Discovery (`*/CMakeLists.txt`) umgestellt und Monolith-Exclude auf generisches `tests/<module>/test_*.cpp` vereinfacht.
33. Semantische Konsolidierung Security/Auth umgesetzt: `test_*.cpp` aus `auth/jwt/jwks/oauth/oauth2/oidc/mfa/saml/rbac/webauthn/mtls/tls/gssapi/ldap/kerberos/pkcs11/pki/keyprovider/hsm/kdf/hkdf` wurden nach `tests/security/` verschoben (Quellordner danach jeweils `0` Dateien), `tests/security/` umfasst jetzt `73` Security-nahe Tests; `Configure` und `ctest -N` bleiben fehlerfrei.
34. Semantische Konsolidierung Transport umgesetzt: `test_*.cpp` aus `wire/ws/websocket/quic/udp/socket/transport` wurden nach `tests/network/` verschoben (Quellordner danach jeweils `0` Dateien), `tests/network/` umfasst jetzt `21` Netzwerk-/Transport-Tests; `tests/network/CMakeLists.txt` registriert konsolidiert ueber `test_*.cpp`; `Configure` und `ctest -N` bleiben fehlerfrei.
35. Semantische Konsolidierung HTTP-Stack umgesetzt: `test_*.cpp` aus `http/http2/http3/cdn` wurden nach `tests/network/` verschoben (Quellordner danach jeweils `0` Dateien), `tests/network/` umfasst jetzt `63` Netzwerk-/Transport-/HTTP-nahe Tests; `Configure` und `ctest -N` bleiben fehlerfrei.
36. API/Server-Grenze geschaerft: `test_api_gateway.cpp`, `test_api_gateway_enhancements.cpp` und `test_api_integration.cpp` wurden von `tests/api/` nach `tests/server/` verschoben; `tests/server/CMakeLists.txt` registriert nun konsolidiert ueber `test_*.cpp`; `tests/api/` behaelt API-kernnahe Tests (`version/routing/interfaces/key/auth/grpc`). `Configure` und `ctest -N` bleiben fehlerfrei.
37. API-Modulregistrierung entschlackt: `tests/api/CMakeLists.txt` nutzt nun einen einzigen Loop auf `test_*.cpp` statt doppelter Prefix-/Autogen-Registrierung; Fokusnamen bleiben stabil als `module_api_<stem>_focused`; `Configure` und `ctest -N` bleiben fehlerfrei.
38. Bulk-Entschlackung Modul-CMake: 32 CMake-Dateien aus konsolidierten Restmodulen (u.a. `auth/jwt/jwks/oauth/...`, `wire/ws/websocket/quic/udp/socket/transport`, `http/http2/http3/cdn`) wurden auf ein einheitliches Registrierungsschema (`test_*.cpp`, ein Loop, `module_<modul>_<stem>_focused`) umgestellt; `Configure` und `ctest -N` bleiben fehlerfrei.
39. Breiter Safe-Autogen-Cleanup: Scriptlauf ueber `tests/*/CMakeLists.txt` (`SCANNED=544`, `UPDATED=182`, `SKIPPED=362`) hat kurze reine Autogen-Doppel-Templates auf Single-Loop normalisiert und komplexe Module mit Zusatzlogik bewusst ausgelassen; `Configure` und `ctest -N` bleiben fehlerfrei.
40. Komplexmodule-Dedupe: In `tests/query/CMakeLists.txt`, `tests/metadata/CMakeLists.txt`, `tests/transaction/CMakeLists.txt` und `tests/sharding/CMakeLists.txt` wurde im jeweiligen AUTOGEN-Loop ein Source-Dedupe-Guard ueber `BUILDSYSTEM_TARGETS` + `get_target_property(... SOURCES)` eingefuehrt; bereits explizit registrierte Test-Sources werden dort nicht mehr als `*_autofocused`-Doppeltarget gebaut.
41. Laufzeitvalidierung der Dedupe-Aenderungen erfolgreich: `test_metadata_snapshot_focused.exe` (15/15 PASS), `test_transaction_isolation_levels_focused.exe` (20/20 PASS), `test_sharding_phase5_focused.exe` (12/12 PASS) und nach gezieltem Build `test_query_federation_routing_focused.exe` (17/17 PASS).
42. Hinweis zur lokalen CTest-Umgebung: `ctest -R` kann in dieser Umgebung aktuell durch fremde Discovery-Binaries vorzeitig abbrechen (z. B. `themis_secidx_tests.exe` / `test_rag_ttft_benchmark.exe`); fuer Modulvalidierung war der robuste Weg daher gezielter Target-Build + direkter GTest-Binarylauf.
43. Harte Verfuegbarkeitsblocker in Modul-Loops behoben: `module_voice_test_voice_api_handler_focused`, `module_wasm_test_wasm_runtime_injector_focused`, `module_whisper_test_whisper_stub_transcribe_bridge_focused`, `module_yaml_test_yaml_config_integration_focused` und `module_zero_test_zero_copy_blob_transfers_focused` bauen nach Wiederherstellung der modulspezifischen Zusatzquellen/-includes wieder erfolgreich.
44. Restbestand in `ctest -N` aktuell weiterhin hoch (vor allem ungebauter Focus-/Autofocus-Bestand): der Missing-Block wird nun in Folgewellen ueber gezielte Batch-Builds pro Prefix reduziert; die 5 zuvor harten Link/Compile-Blocker sind dabei nicht mehr im Missing-Set.
45. Batch-Reduktionswelle #1 ausgefuehrt (erste 120 fehlende Targets aus aktueller Missing-Liste): `98` Targets erfolgreich gebaut, `22` Targets mit echten Build-/Linkfehlern isoliert; `ctest -N` Missing-Eintraege von `1805` auf `1707` reduziert.
46. Restblock der 5 isolierten Hard-Faelle behoben (`module_adalora_test_adalora_tt_bridge_focused`, `module_adaptive_test_adaptive_{cache_integration,cache_phase1,query_cache}_focused`, `module_aql_test_aql_query_validator_focused`): alle 5 Binaries wieder vorhanden (`MISSING_COUNT=0` im Fokusset); global `ctest --preset windows-release -N` weiter reduziert auf `MISSING_EXECUTABLES=1685` (bei `TOTAL_TESTS=4437`).
47. Batch-Reduktionswelle #2 ausgefuehrt (120 Missing-Targets, Einzelziel-Build): `103` Targets erfolgreich gebaut, `17` Targets mit echten Build-/Linkfehlern isoliert; globaler Missing-Count von `1685` auf `1582` reduziert.
48. Batch-Reduktionswelle #3 ausgefuehrt (60 Missing-Targets, Einzelziel-Build unter Skip der bekannten 17 Fehlschlaeger): `53` Targets erfolgreich gebaut, `7` neue Fehlschlaeger isoliert (`columnar/concurrent/context/cpu/cross/cuda`); globaler Missing-Count von `1552` auf `1499` und nach bereinigter Neubewertung weiter auf `1483` reduziert.
49. Batch-Reduktionswelle #4 ausgefuehrt (40 Missing-Targets, Einzelziel-Build unter erweitertem Skip-Set): `31` Targets erfolgreich gebaut, `9` weitere Fehlschlaeger isoliert (`cuda_hnsw_large_k`, `database_domain_auto_labeler`, `delegate_evaluator`, `directx_backend`, `disaster_recovery_manager`, `discourse_engine`, `distributed_catalog`, `distributed_knowledge_integration`, `egov_data_driven`); globaler Missing-Count auf `1452` reduziert.
50. Hotspot-Reparaturwelle #1 umgesetzt: die 9 gezielt isolierten Linkerfehler aus `columnar`, `concurrent`, `context`, `cpu`, `cross`, `cuda` und `database` wurden ueber modulspezifische Zusatzquellen in den jeweiligen `tests/<module>/CMakeLists.txt` verdrahtet; nach Reconfigure (`cmake --preset windows-release`) bauen alle 9 Targets wieder erfolgreich (`FAILED_COUNT=0`), globaler Missing-Count sinkt weiter auf `1443`.
51. Hotspot-Reparaturwelle #2 umgesetzt: `delegate`, `disaster`, `discourse`, `distributed_catalog` und `distributed_knowledge_integration` bauen nach gezielter Zusatzquellen-Verdrahtung und Reconfigure wieder erfolgreich; verbleibender Ausreisser im Cluster ist `module_directx_test_directx_backend_focused`, dort liegt nun ein echter Compile-Blocker im Produktionspfad `src/llm/lora_framework/kernels/directx_kernels.cpp` (kein reines Test-Wiring-Problem mehr). Nach der Welle liegt der globale Stand bei `TOTAL_TESTS=4779`, `MISSING_EXECUTABLES=1438`.
52. DirectX-Produktionsblocker bereinigt: in `src/llm/lora_framework/kernels/directx_kernels.cpp` wurden die lokalen Helper/State-Deklarationen in eine gueltige Reihenfolge gebracht und veraltete API-Nutzung (`DirectXBuffer::Usage`, `bind_uav`/`bind_srv`) auf die aktuelle Descriptor-Table-Bindung (`create_uav/create_srv`, `bind_uav_table/bind_srv_table`) umgestellt. Danach baut `module_directx_test_directx_backend_focused` wieder erfolgreich; globaler Stand: `TOTAL_TESTS=4779`, `MISSING_EXECUTABLES=1437`.
53. Auto-Labeler-Linkerblocker geschlossen: die beiden zuletzt offenen Targets `module_auto_test_auto_labeler_db_fetch_focused` und `module_auto_test_auto_labeler_production_focused` wurden gezielt neu gebaut und linken nun erfolgreich (Binaries vorhanden unter `build-msvc-windows-release/bin/`). Damit ist der Modality-Parser-Cluster im Auto-Modul bereinigt; globaler Stand nach `ctest --preset windows-release -N`: `TOTAL_TESTS=4779`, `MISSING_EXECUTABLES=1426`.
54. ArgumentStore-Cluster geschlossen: die beiden zuvor blockierten Targets `module_argument_test_argument_store_focused` und `module_argument_test_argument_store_standalone_focused` wurden nach transientem DLL-Lock (`LNK1104` auf `themis_base.dll`) erneut gebaut und sind jetzt als Executables vorhanden. Globaler Stand nach `ctest --preset windows-release -N`: `TOTAL_TESTS=4779`, `MISSING_EXECUTABLES=1412`.
55. Binary-Integrity-Linkerblocker geschlossen: fuer `module_binary_test_binary_integrity_focused` wurde die fehlende Mock-Signing-Factory (`createMockSigningService`) testseitig verdrahtet (`tests/binary/mock_signing_service.cpp`, eingebunden nur fuer `test_binary_integrity`). Das Target baut wieder erfolgreich, Binary vorhanden unter `build-msvc-windows-release/bin/`; globaler Stand: `TOTAL_TESTS=4779`, `MISSING_EXECUTABLES=1411`.
56. Byzantine/Chimera-Cluster bereinigt: im byzantine-Modul wurde der fehlende Include-Pfad auf `${THEMIS_ROOT_DIR}/tests` ergaenzt (fuer `byzantine_attacks.h`), wodurch `module_byzantine_test_byzantine_detector_focused` wieder als Binary vorhanden ist. Das chimera-Modul registriert Focus-Targets jetzt nur bei vorhandener externer Headerbasis (`external/chimera/include/chimera/database_adapter.hpp`), wodurch in dieser Umgebung keine Phantom-Executables mehr entstehen. Globaler Stand nach `ctest --preset windows-release -N`: `TOTAL_TESTS=4776`, `MISSING_EXECUTABLES=1407` (davon `CHIMERA_MISSING=0`).
57. Auto-Failover-Recovery-Compileblocker geschlossen: in `tests/auto/test_auto_failover_recovery.cpp` wurden nicht-relokierbare Cluster-Container von `std::vector<MockReplicaNode>` auf `std::deque<MockReplicaNode>` umgestellt; zusaetzlich wurde der `PriorityNode`-Container in `PriorityBasedLeaderElection` ebenfalls auf `std::deque` umgestellt, damit keine impliziten Copy-/Move-Anforderungen fuer `MockReplicaNode` entstehen. `module_auto_test_auto_failover_recovery_focused` baut wieder erfolgreich (Binary vorhanden), globaler Stand: `TOTAL_TESTS=4776`, `MISSING_EXECUTABLES=1406`.
58. ER-Cluster Folgeschritt: `module_er_test_er_diagram_exporter_focused` wurde gezielt gebaut und ist nun als Binary vorhanden (`build-msvc-windows-release/bin/module_er_test_er_diagram_exporter_focused.exe`). Globaler Stand nach `ctest --preset windows-release -N`: `TOTAL_TESTS=4776`, `MISSING_EXECUTABLES=1405`.
59. ER/Storage-Folgeschritt: `module_erasure_test_erasure_coding_backend_focused` wurde gezielt gebaut und ist nun als Binary vorhanden (`build-msvc-windows-release/bin/module_erasure_test_erasure_coding_backend_focused.exe`). Globaler Stand nach `ctest --preset windows-release -N`: `TOTAL_TESTS=4776`, `MISSING_EXECUTABLES=1404`.
60. Error-Cluster Folgeschritt: `module_error_test_error_codes_focused` wurde gezielt gebaut und ist nun als Binary vorhanden (`build-msvc-windows-release/bin/module_error_test_error_codes_focused.exe`).
61. Error-Cluster Folgeschritt: `module_error_test_error_registry_focused` wurde gezielt gebaut und ist nun als Binary vorhanden (`build-msvc-windows-release/bin/module_error_test_error_registry_focused.exe`). Globaler Stand nach beiden Builds: `TOTAL_TESTS=4776`, `MISSING_EXECUTABLES=1402`.
62. Ethics-Plugin-Gating im Modulregister vereinheitlicht: `tests/ethics/CMakeLists.txt` ueberspringt die Focus-Target-Registrierung jetzt konsistent bei deaktiviertem `THEMIS_PLUGIN_ETHICS_AI`. Dadurch entstehen keine `module_ethics_*`-Phantom-Executables mehr (`ETHICS_MODULE_MISSING=0`). Globaler Stand nach Reconfigure + `ctest --preset windows-release -N`: `TOTAL_TESTS=4763`, `MISSING_EXECUTABLES=1389`.
63. Ethical-Cluster Folgeschritt: `module_ethical_test_ethical_guidelines_manager_focused` wurde gezielt gebaut und ist nun als Binary vorhanden (`build-msvc-windows-release/bin/module_ethical_test_ethical_guidelines_manager_focused.exe`). Zwischenstand danach: `TOTAL_TESTS=4763`, `MISSING_EXECUTABLES=1388`.
64. Event-Cluster Folgeschritt: `module_event_test_event_trigger_focused` wurde gezielt gebaut und ist nun als Binary vorhanden (`build-msvc-windows-release/bin/module_event_test_event_trigger_focused.exe`). Globaler Stand nach `ctest --preset windows-release -N`: `TOTAL_TESTS=4763`, `MISSING_EXECUTABLES=1387`.
65. Top-Missing-5er-Welle abgeschlossen: `module_eviction_test_eviction_strategies_focused`, `module_explainability_test_explainability_reason_builder_focused`, `module_explanation_test_explanation_generator_focused`, `module_export_test_export_api_handler_focused` und `module_external_test_external_scheduler_adapter_focused` wurden erfolgreich gebaut und sind als Binaries vorhanden. Fuer zwei echte Linkerblocker wurden modulspezifische Zusatzquellen verdrahtet (`tests/explainability/CMakeLists.txt` bindet `src/rag/explainability_reason_builder.cpp` fuer `test_explainability_reason_builder`; `tests/explanation/CMakeLists.txt` bindet `src/llm/explanation_generator.cpp` fuer `test_explanation_generator`). Globaler Stand nach `ctest --preset windows-release -N`: `TOTAL_TESTS=4763`, `MISSING_EXECUTABLES=1382`.
66. Exporters-Cluster in zwei Mikro-Wellen abgearbeitet: insgesamt 13 Targets (`module_exporters_test_aql_predicate_filter_focused`, `module_exporters_test_arrow_ipc_exporter_focused`, `module_exporters_test_data_augmentation_focused`, `module_exporters_test_export_encryption_focused`, `module_exporters_test_export_format_registry_focused`, `module_exporters_test_format_template_focused`, `module_exporters_test_huggingface_exporter_focused`, `module_exporters_test_huggingface_hub_client_focused`, `module_exporters_test_incremental_exporter_focused`, `module_exporters_test_join_exporter_focused`, `module_exporters_test_jsonl_llm_exporter_focused`, `module_exporters_test_parquet_exporter_focused`, `module_exporters_test_streaming_exporter_focused`) wurden erfolgreich gebaut und als vorhandene Binaries verifiziert. Globaler Stand nach `ctest --preset windows-release -N`: `TOTAL_TESTS=4763`, `MISSING_EXECUTABLES=1369`.
67. Naechste Top-Missing-Welle abgeschlossen: `module_faceted_test_faceted_search_focused`, `module_failover_test_failover_chaos_scenarios_focused`, `module_faiss_test_faiss_gpu_backend_focused`, `module_feature_test_feature_flags_focused`, `module_federated_test_federated_distillation_coordinator_focused` und `module_federated_test_federated_identity_manager_focused` wurden erfolgreich gebaut und als vorhandene Binaries verifiziert. Zwei Linkerblocker wurden ueber modulspezifisches Source-Wiring geloest (`tests/failover/CMakeLists.txt` bindet `src/failover/auto_failover_manager.cpp` fuer `test_failover_chaos_scenarios`; `tests/federated/CMakeLists.txt` bindet `src/distributed_knowledge/federated_distillation_coordinator.cpp` sowie `src/auth/federated_identity_manager.cpp` fuer die entsprechenden Targets). Globaler Stand nach `ctest --preset windows-release -N`: `TOTAL_TESTS=4763`, `MISSING_EXECUTABLES=1363`.
68. Federated/Feedback/Fewshot-Folgewelle abgeschlossen: `module_federated_test_federated_poisoning_detection_focused`, `module_federated_test_federated_privacy_training_focused`, `module_federation_test_federation_admin_focused`, `module_feedback_test_feedback_api_handler_focused`, `module_feedback_test_feedback_collector_focused`, `module_feedback_test_feedback_collector_scaling_focused`, `module_feedback_test_feedback_store_focused` und `module_fewshot_test_fewshot_optimizer_focused` wurden erfolgreich gebaut und als vorhandene Binaries verifiziert. Fuer den einzigen Linkerblocker wurde in `tests/fewshot/CMakeLists.txt` die fehlende Produktionsquelle `src/llm/fewshot_optimizer.cpp` fuer `test_fewshot_optimizer` verdrahtet. Globaler Stand nach `ctest --preset windows-release -N`: `TOTAL_TESTS=4763`, `MISSING_EXECUTABLES=1355`.
69. Flash/Fulltext/Fused-Welle nach Buildsystem-Stabilisierung abgeschlossen: nach frischem Reconfigure (`cmake --preset windows-release --fresh`) und sauberem Neustart des `sccache`-Servers bauen `module_flash_test_flash_attention_correctness_focused`, `module_flash_test_flash_lora_focused`, `module_fulltext_test_fulltext_phrase_fuzzy_focused`, `module_fused_test_fused_kernels_focused` und `module_fused_test_fused_lora_kernels_focused` wieder erfolgreich und sind als Binaries vorhanden. Der einzige lokale Folgeblocker war ein Doppel-Link von `flash_lora.cpp` im Testtarget; dieser wurde in `tests/flash/CMakeLists.txt` durch Entfernen der redundanten Zusatzquelle fuer `test_flash_lora` behoben. Globaler Stand nach `ctest --preset windows-release -N`: `TOTAL_TESTS=4763`, `MISSING_EXECUTABLES=1348`.
70. Flatfile/Fuzz/Fuzzy/GAP008-Folgewelle abgeschlossen: `module_flatfile_test_flatfile_importer_focused`, `module_fuzz_test_fuzz_core_focused`, `module_fuzz_test_fuzz_security_focused`, `module_fuzzy_test_fuzzy_matcher_focused`, `module_gap008_test_gap008_backup_automation_focused` und `module_gap008_test_gap008_observability_focused` wurden erfolgreich gebaut und als vorhandene Binaries verifiziert. Die Builds liefen mit aktivem `sccache`-Server fehlerfrei durch (`Compilations=9`, `Cache write errors=0`), womit die vorherige Cache-Deaktivierung wieder entfallen konnte. Globaler Stand nach `ctest --preset windows-release -N`: `TOTAL_TESTS=4763`, `MISSING_EXECUTABLES=1342`.
71. Gate/GDPR/General/Generic-Folgewelle abgeschlossen: `module_gate_test_gate_result_focused`, `module_gdpr_test_gdpr_and_cross_border_focused`, `module_general_test_general_traversal_focused` und `module_generic_test_generic_plugin_registry_focused` wurden erfolgreich gebaut und als vorhandene Binaries verifiziert. Fuer den GDPR-Linkerblocker wurde in `tests/gdpr/CMakeLists.txt` die fehlende Produktionsquelle `src/governance/gdpr_subject_rights.cpp` fuer `test_gdpr_and_cross_border` verdrahtet (`ErasureReport::toSummaryMap`), und ein transienter Windows-Dateilock (`C1083 Permission denied` auf eine `.obj`) wurde per unveraenderter Retry-Strategie abgefangen. Globaler Stand nach `ctest --preset windows-release -N`: `TOTAL_TESTS=4763`, `MISSING_EXECUTABLES=1338`.
72. Geo-Serienwellen (focused + autofocused) abgeschlossen: der gesamte sichtbare `module_geo_*`-Block wurde in mehreren Mikro- und Bulk-Wellen gebaut (u.a. `aql_st_*`, `geo_*`, `gpu_backend_production`, `rtree_*`, `spatial_*`, `temporal_spatial_*` sowie verbleibende `*_autofocused`-Targets). Dadurch sank der globale Missing-Stand von `1338` auf `1285`.
73. Geometric-Folgewelle repariert: `module_geometric_test_geometric_distances_focused` schlug initial mit `M_PI`/`EXPECT_NEAR`-Compilefehler auf MSVC fehl; in `tests/geometric/test_geometric_distances.cpp` wurde die Pi-Berechnung auf portable `std::acos(-1.0)`-Basis umgestellt. Target baut danach erfolgreich.
74. GGUF-Metadata-Linkerblocker repariert: `module_gguf_test_gguf_metadata_focused` hatte unaufgeloeste `GGUFMetadata`/`ProvenanceRecord`-Symbole; in `tests/gguf/CMakeLists.txt` wurde fuer `test_gguf_metadata` die fehlende Produktionsquelle `src/storage/gguf_metadata.cpp` verdrahtet. Danach bauten `module_gguf_test_gguf_metadata_focused`, `module_global_test_global_transaction_manager_focused`, `module_gnn_test_gnn_embeddings_focused` und `module_gorilla_test_gorilla_focused` erfolgreich. Globaler Stand nach `ctest --preset windows-release -N`: `TOTAL_TESTS=4763`, `MISSING_EXECUTABLES=1279`.
75. Gorilla/Gossip-Folgewelle abgeschlossen: `module_gorilla_test_gorilla_codec_edge_cases_focused`, `module_gorilla_test_gorilla_error_recovery_focused`, `module_gorilla_test_gorilla_probe_focused`, `module_gorilla_test_gorilla_simd_focused`, `module_gossip_test_gossip_config_manager_focused`, `module_gossip_test_gossip_config_manager_focused_focused` und `module_gossip_test_gossip_custom_handler_focused` wurden erfolgreich gebaut und als frische Binaries verifiziert. Globaler Stand nach `ctest --preset windows-release -N`: `TOTAL_TESTS=4763`, `MISSING_EXECUTABLES=1272`.
76. Governance + GPU-Startwelle abgeschlossen: `module_governance_test_governance_compliance_time_window_focused`, `module_governance_test_governance_opa_adapter_focused`, `module_governance_test_governance_policy_hot_reload_focused`, `module_governance_test_governance_policy_simulation_focused`, `module_governance_test_governance_review_scheduler_focused`, `module_gpu_test_gpu_admin_api_focused`, `module_gpu_test_gpu_alerts_focused`, `module_gpu_test_gpu_audit_log_focused`, `module_gpu_test_gpu_cluster_coordinator_focused` und `module_gpu_test_gpu_cluster_topology_focused` wurden erfolgreich gebaut. Globaler Stand nach `ctest --preset windows-release -N`: `TOTAL_TESTS=4763`, `MISSING_EXECUTABLES=1262`.
77. GPU-Folgewelle #2 abgeschlossen: `module_gpu_test_gpu_compression_focused`, `module_gpu_test_gpu_config_focused`, `module_gpu_test_gpu_device_discovery_focused`, `module_gpu_test_gpu_erasure_coding_focused`, `module_gpu_test_gpu_feature_flags_focused`, `module_gpu_test_gpu_graph_cache_focused`, `module_gpu_test_gpu_graph_traversal_focused`, `module_gpu_test_gpu_kernel_validator_focused`, `module_gpu_test_gpu_launcher_focused` und `module_gpu_test_gpu_load_balancer_focused` wurden erfolgreich gebaut. Globaler Stand nach `ctest --preset windows-release -N`: `TOTAL_TESTS=4763`, `MISSING_EXECUTABLES=1252`.
78. GPU-Folgewelle #3 abgeschlossen: `module_gpu_test_gpu_lora_layers_focused`, `module_gpu_test_gpu_memory_management_focused`, `module_gpu_test_gpu_memory_pool_focused`, `module_gpu_test_gpu_metrics_focused`, `module_gpu_test_gpu_mig_manager_focused`, `module_gpu_test_gpu_module_focused`, `module_gpu_test_gpu_olap_accelerator_focused`, `module_gpu_test_gpu_p2p_transfer_focused`, `module_gpu_test_gpu_policy_focused` und `module_gpu_test_gpu_profiler_focused` wurden erfolgreich gebaut. Globaler Stand nach `ctest --preset windows-release -N`: `TOTAL_TESTS=4763`, `MISSING_EXECUTABLES=1242`.
79. GPU-Parity-Blocker beseitigt: `module_gpu_test_gpu_query_accelerator_parity_focused` schlug mit MSVC-Preprozessor-/Syntaxfehlern (`#ifdef` innerhalb `::testing::Values(...)`) fehl. In `tests/gpu/test_gpu_query_accelerator_parity.cpp` wurde die bedingte Groessenliste in eine Hilfsfunktion (`parityInputSizes`) ausgelagert und auf `::testing::ValuesIn(...)` umgestellt; das Target baut danach wieder.
80. GPU-Folgewelle #4 abgeschlossen: `module_gpu_test_gpu_rocm_backend_focused`, `module_gpu_test_gpu_safe_fail_focused`, `module_gpu_test_gpu_safe_fail_module_focused`, `module_gpu_test_gpu_stream_cuda_bridge_focused`, `module_gpu_test_gpu_stream_manager_focused`, `module_gpu_test_gpu_stubs_comprehensive_focused`, `module_gpu_test_gpu_tensor_focused`, `module_gpu_test_gpu_time_slice_scheduler_focused`, `module_gpu_test_gpu_training_loop_focused` und `module_gpu_test_gpu_unified_memory_focused` wurden erfolgreich gebaut. Globaler Stand nach `ctest --preset windows-release -N`: `TOTAL_TESTS=4763`, `MISSING_EXECUTABLES=1230`.
81. Rest-GPU + Graph-Einstieg abgeschlossen: `module_gpu_test_gpu_vector_index_focused`, `module_gpu_test_gpu_vram_allocation_focused`, `module_gpu_test_gpu_vulkan_backend_focused`, `module_gpu_test_gpu_wasm_kernel_sandbox_focused`, `module_graceful_test_graceful_shutdown_focused`, `module_gradient_test_gradient_checkpointing_focused`, `module_graph_test_gpu_traversal_focused`, `module_graph_test_graph_advanced_features_focused`, `module_graph_test_graph_analytics_focused` und `module_graph_test_graph_bfs_fix_focused` wurden erfolgreich gebaut. Globaler Stand nach `ctest --preset windows-release -N`: `TOTAL_TESTS=4763`, `MISSING_EXECUTABLES=1220`.
82. Graph-Folgewelle #2 abgeschlossen: `module_graph_test_graph_distributed_focused`, `module_graph_test_graph_edge_empty_fields_qw45_focused`, `module_graph_test_graph_edge_encryption_focused`, `module_graph_test_graph_index_focused`, `module_graph_test_graph_index_comprehensive_focused`, `module_graph_test_graph_parallel_traversal_focused`, `module_graph_test_graph_query_optimizer_focused`, `module_graph_test_graph_query_rewriter_focused` und `module_graph_test_graph_type_filtering_focused` wurden erfolgreich gebaut.
83. Graph-Watermarking-Linkerblocker behoben: fuer `module_graph_test_graph_watermarking_focused` fehlten die Produktionssymbole aus `GraphWatermark::embed`/`GraphFingerprintDetector::detect`; in `tests/graph/CMakeLists.txt` wurde fuer `test_graph_watermarking` die Zusatzquelle `src/graph/graph_watermark.cpp` zielgenau verdrahtet (focused + autofocused Loop). Danach baut `module_graph_test_graph_watermarking_focused` erfolgreich, und der globale Stand nach `ctest --preset windows-release -N` liegt bei `TOTAL_TESTS=4763`, `MISSING_EXECUTABLES=1210`.
84. Graph-Folgewelle #3 weitgehend abgeschlossen: `module_graph_test_knowledge_graph_reasoner_focused`, `module_graph_test_ontology_manager_focused`, `module_graph_test_path_constraints_semantic_focused`, `module_graph_test_query_explain_focused`, `module_graph_test_rotate_completion_focused` und `module_graph_test_scheduled_edge_refresh_focused` wurden erfolgreich gebaut.
85. Tensor-Fingerprint-Linkerblocker behoben: fuer `module_graph_test_tensor_fingerprint_graph_focused` fehlten die Produktionssymbole aus `TensorFingerprintGraph` und `TensorDeduplicationManager`; in `tests/graph/CMakeLists.txt` wurden fuer `test_tensor_fingerprint_graph` die Zusatzquellen `src/graph/tensor_fingerprint_graph.cpp` und `src/graph/tensor_deduplication_manager.cpp` zielgenau verdrahtet (focused + autofocused Loop). Danach baut `module_graph_test_tensor_fingerprint_graph_focused` erfolgreich. Globaler Stand nach `ctest --preset windows-release -N`: `TOTAL_TESTS=4763`, `MISSING_EXECUTABLES=1203`.
86. Graph-Autofocused-Welle #1 abgeschlossen: `module_graph_test_graph_advanced_features_autofocused`, `module_graph_test_graph_analytics_autofocused`, `module_graph_test_graph_bfs_fix_autofocused`, `module_graph_test_graph_distributed_autofocused`, `module_graph_test_graph_edge_empty_fields_qw45_autofocused`, `module_graph_test_graph_edge_encryption_autofocused`, `module_graph_test_graph_index_autofocused`, `module_graph_test_graph_index_comprehensive_autofocused`, `module_graph_test_graph_parallel_traversal_autofocused` und `module_graph_test_graph_query_optimizer_autofocused` wurden erfolgreich gebaut. Globaler Stand nach `ctest --preset windows-release -N`: `TOTAL_TESTS=4763`, `MISSING_EXECUTABLES=1193`.
87. Rest-Graph + GraphQL-Startwelle abgeschlossen: `module_graph_test_graph_query_rewriter_autofocused`, `module_graph_test_graph_type_filtering_autofocused`, `module_graph_test_graph_watermarking_autofocused`, `module_graphql_test_graphql_focused`, `module_graphql_test_graphql_cache_security_focused`, `module_graphql_test_graphql_error_masking_focused`, `module_graphql_test_graphql_introspection_focused`, `module_graphql_test_graphql_limits_focused`, `module_graphql_test_graphql_multimodel_focused` und `module_graphql_test_graphql_p1_features_focused` wurden erfolgreich gebaut. Globaler Stand nach `ctest --preset windows-release -N`: `TOTAL_TESTS=4763`, `MISSING_EXECUTABLES=1183`.
88. GraphQL-API-Drift behoben: `module_graphql_test_graphql_variables_focused` schlug mit einem Compilefehler gegen die aktuelle Value-API fehl (`Value::int_` existiert nicht mehr). In `tests/graphql/test_graphql_variables.cpp` wurde auf `Value::integer(10LL)` umgestellt; das Target baut danach wieder erfolgreich.
89. GraphQL/gRPC-Folgewelle abgeschlossen: `module_graphql_test_graphql_performance_focused`, `module_graphql_test_graphql_variables_focused`, `module_graphql_test_graphql_ws_handler_focused`, `module_gremlin_test_gremlin_parser_focused`, `module_group_test_group_dek_focused`, `module_grpc_test_grpc_channel_pool_focused` und `module_grpc_test_grpc_observability_focused` wurden erfolgreich gebaut.
90. gRPC-Modul-Wiring erweitert: die gRPC-Focused-Targets benoetigen die `GRPCServer`-Implementierung aus `src/rpc_grpc/grpc_plugin.cpp`; in `tests/grpc/CMakeLists.txt` wurde diese Produktionsquelle deshalb auf Modulniveau in die Target-Erzeugung aufgenommen. Danach bauten auch `module_grpc_test_grpc_plugin_focused`, `module_grpc_test_grpc_plugin_lifecycle_focused`, `module_grpc_test_grpc_server_config_qw42_focused`, `module_grpc_test_grpc_transport_focused`, `module_grpc_test_grpc_web_proxy_bridge_focused` und `module_grpc_test_grpc_web_proxy_handler_focused` erfolgreich. Globaler Stand nach `ctest --preset windows-release -N`: `TOTAL_TESTS=4763`, `MISSING_EXECUTABLES=1170`.


72. Tensor-Core-Bridge-Testkonsolidierung abgeschlossen: CMake registriert den Focus-Target `test_tensor_core_bridge` ausschliesslich aus `tests/test_tensor_core_bridge.cpp`; die bisherige Duplikatdatei unter `tests/tensor/test_tensor_core_bridge.cpp` wurde auf einen Forwarding-Shim reduziert, sodass nur noch eine kanonische Testimplementation gepflegt wird und Drift ausgeschlossen ist.
73. Duplikataudit-Fortschritt (No-Build): automatischer Root-vs-Modul-Scan ueber `test_*.cpp` zeigt `PAIR_COUNT=1590`, `ROOT_CANONICAL_COUNT=1565`, `UNIQUE_DUPLICATE_NAMES=1565`; Top-Mehrfachduplikate liegen vorrangig in den Clustern `schema/metadata`, `retention`, `rotate`, `statistics`, die als naechste Entdoppelungswelle vorbereitet werden.

## 1. Zielbild

Wir migrieren von einer stark monolithischen Testregistrierung in `tests/CMakeLists.txt` zu einem modularen Aufbau:

- `tests/<module>/` fuer modulspezifische Unit- und Focus-Tests
- `tests/integration/<module>/` fuer Integrations- und E2E-nahe Tests
- `tests/fixtures/<module>/` fuer modulgebundene Testdaten
- zentrale CTest-Policy: nur real gebaute Targets registrieren

Erfolgskriterien:

1. CTest-Registry ist reproduzierbar und enthaelt keine Phantom-Tests.
2. Modulverantwortliche koennen Testtargets lokal pro Modul bauen/laufen lassen.
3. Root-`tests/CMakeLists.txt` dient nur noch als Dispatcher, nicht als Monolith.
4. Focus-Tests bleiben moeglich, sind aber klar als kurzfristige Stabilisierung markiert.

## 2. Ist-Lage (kurz)

- Sehr grosse Testbasis in `tests/` mit >2000 Dateien.
- Zentrale Registrierung ist stark konzentriert in `tests/CMakeLists.txt`.
- Hoher Anteil an `Focused`-Tests und dedizierten Focus-Executables.
- Auf Windows/modular wird Unified-Discovery teilweise bewusst umgangen.

## 3. CTest-First Grundregeln

1. Kein `add_test(...)` fuer Targets, die nicht gebaut werden.
2. Registrierung immer target-basiert absichern:
   - `if(TARGET <target_name>)`
   - optional generator-expressions mit `$<TARGET_EXISTS:...>`
3. Keine `_NOT_BUILT`-Patterns als dauerhafte CTest-Eintraege.
4. Labels sind verpflichtend und standardisiert: `module`, `tier`, `kind`, `focus`.
5. Jeder neue Test bekommt einen klaren Modulpfad unter `tests/<module>/`.

## 4. Zielstruktur im Dateisystem

```text
tests/
  CMakeLists.txt                 # nur Top-Level Dispatcher
  cmake/
    TestPolicy.cmake             # gemeinsame Hilfsfunktionen
    RegisterModuleTests.cmake    # standardisierte Registrierung
  src/
    acceleration/
      CMakeLists.txt
      test_backend_registry_startup.cpp
      ...
    query/
      CMakeLists.txt
      ...
    replication/
      CMakeLists.txt
      ...
    ...
  integration/
    query/
      CMakeLists.txt
      ...
    sharding/
      CMakeLists.txt
      ...
  fixtures/
    query/
    replication/
    ...
```

## 5. Priorisierte Entschlackungsliste

## P0 (sofort, 1-3 Tage)

- [ ] CTest-Registry bereinigen: nur existente Targets registrieren.
- [ ] `_NOT_BUILT`-Registrierungen technisch unterbinden.
- [ ] Label-Konvention festziehen und in allen neuen Registrierungen erzwingen.
- [ ] Top-100 fehleranfaellige Focus-Tests markieren (Keep/Refactor/Drop).

## P1 (kurzfristig, 1-2 Wochen)

- [ ] Root-`tests/CMakeLists.txt` in Modulabschnitte extrahieren.
- [ ] Pro Modul eigenes `tests/<module>/CMakeLists.txt` einfuehren.
- [ ] Gemeinsame Makros in `tests/cmake/TestPolicy.cmake` einziehen.
- [ ] Focus-Targets mit doppelter Funktion identifizieren und zusammenfuehren.

## P2 (mittelfristig, 2-4 Wochen)

- [ ] Unified + Focus-Duplikate reduzieren (ein Testinhalt, klare Laufmodi).
- [ ] Integrationspfade nach `tests/integration/<module>/` verschieben.
- [ ] Fixture-Pfade je Modul isolieren (keine globalen Temp-Kollisionen).
- [ ] CTest-Presets pro Modul bereitstellen (`query-tests-release`, etc.).

## P3 (laufend)

- [ ] Runtime und Laufzeitkosten je Label monitoren (Top-Langlaeufer).
- [ ] Test-SLA definieren: Unit < 5s, Integration < 60s pro Testziel.
- [ ] Quartalsweise Focus-Tests reviewen und abbauen.

## 6. Konkrete technische Umsetzung

## 6.1 Root-CMake verschlanken

In `tests/CMakeLists.txt` nur noch:

1. globale Includes/TestPolicy
2. `add_subdirectory(<module>)`
3. `add_subdirectory(integration/<module>)`
4. zentrale Optionen/Presets

## 6.2 Registrierung standardisieren

Ein zentrales Makro in `tests/cmake/RegisterModuleTests.cmake`:

- `themis_register_test_target(NAME ... TARGET ... LABELS ... TIMEOUT ...)`
- intern: `if(TARGET ...) add_test(...) endif()`
- optional: automatische Label-Ergaenzung (`module:<name>`, `tier:unit`)

## 6.3 Focus-Policy

Focused-Tests nur wenn mindestens eines gilt:

1. Link-/ABI-Sonderfall
2. besonders schweres Fixture
3. reproduzierbarer Flaky-Schutz

Jeder Focus-Test bekommt Kommentarfelder:

- Grund fuer Focus-Target
- Exit-Kriterium fuer Rueckfuehrung in modulare Standardtests

## 7. Modul-Migrationsreihenfolge (empfohlen)

1. `acceleration`
2. `query`
3. `replication`
4. `transaction`
5. `sharding`
6. `security`
7. `storage`
8. `llm`
9. Restmodule nach Volumen/Flakiness

Begruendung:

- hohe Testdichte
- viele Focus-Targets
- grosse Wirkung auf CTest-Laufzeit und Stabilitaet

## 8. CTest-Preset-Strategie

Neue Presets (zusetzlich zu bestehenden):

1. `windows-release-module-acceleration`
2. `windows-release-module-query`
3. `windows-release-module-replication`
4. `windows-release-fast-dev` (nur unit + stabile labels)
5. `windows-release-failed-rerun` (letzte Fehlerliste)

Leitlinie:

- Entwicklerlauf: schnell und modulfokussiert
- Gate-Lauf: stabil, reproduzierbar, ohne Not-Run-Rauschen

## 9. Messbare KPIs

1. Anteil Focus-Tests an allen add_test-Eintraegen
2. Anteil Not-Run im CTest-Result
3. Median Testdauer pro Label
4. Anzahl CMake-Zeilen in Root-`tests/CMakeLists.txt`
5. Anzahl Module mit eigener Test-CMake

Zielwerte (erste Ausbaustufe):

- Focus-Anteil -30%
- Not-Run gegen 0 in Standardpresets
- Root-CMake um mindestens 40% reduziert

## 10. Sofort umsetzbare 10 Schritte

1. Inventarliste aus `tests/CMakeLists.txt` nach Modul gruppieren.
2. `tests/cmake/TestPolicy.cmake` anlegen.
3. `tests/cmake/RegisterModuleTests.cmake` anlegen.
4. Erste 3 Module (`acceleration`, `query`, `replication`) als Unterordner-CMake anlegen.
5. Je Modul 20-50 add_test-Registrierungen aus Root auslagern.
6. `if(TARGET ...)`-Guards bei allen ausgelagerten Registrierungen erzwingen.
7. Focus-Kommentarpflicht fuer neue Focus-Targets einfuehren.
8. Modul-Presets in `CMakePresets.json` ergaenzen.
9. `ctest -N` Delta vor/nach Migration dokumentieren.
10. Woechentliches Cleanup-Ritual fuer neue `_NOT_BUILT` oder unlabelled Tests.

## 11. Risiken und Gegenmassnahmen

- Risiko: kurzfristige Build-Instabilitaet durch CMake-Umbau.
  - Gegenmassnahme: Migration in kleinen Modulpaketen, jeweils mit CMake-Tool Build+CTest-Check.

- Risiko: Focus-Tests werden unkoordiniert neu erzeugt.
  - Gegenmassnahme: Focus-Policy + Review-Checkliste.

- Risiko: Laufzeit steigt trotz Modularisierung.
  - Gegenmassnahme: Label-basierte Presets und aktive Langlaeuferpflege.

## 12. Definition of Done (DoD)

- [ ] Root-`tests/CMakeLists.txt` ist Dispatcher statt Monolith.
- [ ] Mindestens 5 Kernmodule liegen unter `tests/<module>/CMakeLists.txt`.
- [ ] Standardpresets liefern keine `_NOT_BUILT`-Eintraege.
- [ ] Focus-Targets sind dokumentiert und reduziert.
- [ ] CTest-KPIs werden regelmaessig berichtet.
