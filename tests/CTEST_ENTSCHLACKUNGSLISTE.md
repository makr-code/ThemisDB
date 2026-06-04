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
26. Weitere flache Modulwelle umgesetzt: `tests/performance/` besitzt jetzt eine eigene Modul-CMakeLists, ist im Root-Dispatcher eingehangen und aus dem monolithischen `ALL_TEST_SOURCES` ausgeschlossen.
27. Root-Entschlackung Query abgeschlossen: der verbliebene Root-Gap-Fix-Block fuer `test_pagerank` und `test_query_cancellation` wurde entfernt; Registrierung liegt nun konsistent in `tests/query/CMakeLists.txt`.
28. Root-Entschlackung Integration abgeschlossen: der verbliebene Root-Gap-Fix-Block fuer focused `integration/test_*.cpp` (inkl. `test_cross_functional_voice_observability` und `test_process_mining_e2e_focused`) wurde entfernt; Registrierung liegt nun konsistent in `tests/integration/CMakeLists.txt`.
29. AQL-Spiegelung umgesetzt: neues Modul `tests/aql/` mit eigener `CMakeLists.txt`; alle `test_aql_*.cpp` wurden aus `tests/query/` dorthin verschoben und im Root-Dispatcher eingebunden.
30. Root-Spiegelung im Breitenlauf umgesetzt: verbleibende Root-Tests wurden per Prefix in `tests/<prefix>/` migriert (inkl. automatischer Modul-CMake-Erzeugung/-Ergaenzung), Root-Dispatcher in `tests/CMakeLists.txt` auf Auto-Discovery (`*/CMakeLists.txt`) umgestellt und Monolith-Exclude auf generisches `tests/<module>/test_*.cpp` vereinfacht.

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
