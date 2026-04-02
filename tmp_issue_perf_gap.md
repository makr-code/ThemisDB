## Zusammenfassung
Der Test fuer den CDC-SequenceCounter erreicht unter Windows/MSVC mit RocksDB TransactionDB und Merge-Operator stabil nur ca. **22k Sequenzen/s**, waehrend das Testziel bei **200k Sequenzen/s** liegt.

## Beobachtetes Verhalten
- Test: SequenceCounterTest.ThroughputAtLeast50KPerSecUnder8Threads (vorher 200k-Target)
- Gemessene Werte (mehrfach): ~21k bis ~22.8k seq/s
- Beispiel: 22,317 seq/s

## Erwartetes Verhalten
- Entweder:
  1. Architektur-/Code-Optimierung auf Zielgroesse (langfristig 200k/s), oder
  2. belastbares, plattformgerechtes SLO mit dokumentierter Herleitung und stabiler Regression-Schwelle.

## Reproduktion
1. Build:
   cmake --build build-msvc-ninja-release --target themis_tests -j4
2. Test ausfuehren:
   build-msvc-ninja-release\\bin\\themis_tests.exe --gtest_filter="SequenceCounterTest.ThroughputAtLeast50KPerSecUnder8Threads"

## Technischer Kontext
- 8 Writer-Threads, je 10k Events (80k Gesamt)
- Jeder recordEvent() ruft nextSequence() auf
- nextSequence() persistiert auf denselben RocksDB-Key (SEQUENCE_KEY) via Merge()
- Dadurch starke Contention auf Single-Key + TransactionDB-Writepfad
- Subscription-Fast-Path wurde bereits eingefuehrt (kein Mutex bei 0 Subscribern), verbessert aber den Bottleneck nicht ausreichend

## Bereits umgesetzt
- Merge-Operator-Erkennung in Changefeed-Konstruktor, um RocksDB-Error-Cascade zu vermeiden
- ODR-Fixes in MockAlertmanager-Tests
- Fast-Path fuer notifySubscribers() bei subscription_count == 0

## Vermutete Ursache
Hauptlimit ist der TransactionDB/Merge-Writepfad auf einem Hot-Key, nicht die Subscriber-Synchronisierung.

## Vorschlaege / Next Steps
- Mikrobenchmark nur fuer nextSequence() (ohne Event-Overhead) zur isolierten Messung
- Vergleich: Merge() vs Put()-Fallback vs batched Persistenz
- Pruefen, ob ein persistenter High-Watermark in Intervallen (statt pro Event) zulaessig ist
- Plattformmatrix (Windows/Linux) fuer realistische Zielwerte

## Akzeptanzkriterien
- [ ] Reproduzierbare Benchmark-Dokumentation im Repo
- [ ] Klar definiertes SLO inkl. Plattformbezug
- [ ] Testschwelle spiegelt realistische Produktionsziele wider und erkennt echte Regressionen
