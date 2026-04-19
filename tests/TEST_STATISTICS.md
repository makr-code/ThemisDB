> ⚠️ **Historischer Testbericht** – Dieser Bericht beschreibt den Teststand zum Zeitpunkt der Erstellung.
> Für aktuellen Stand: `ctest --preset linux-ninja-release --test-dir build/linux-ninja-release` ausführen.

# Test Statistics

Stand: 2026-03-28

Dieses Dokument trennt drei verschiedene Zaehlebenen, die im Projekt bisher oft vermischt wurden. Die Zahlen sind nur dann belastbar, wenn man immer auf derselben Ebene vergleicht.

## Ebenen

### 1. Source-Level GoogleTest-Deklarationen

Definition:
- Gezaehlt werden deklarierte Testmakros im Quellbaum unter `tests/`.
- Regex: `TEST`, `TEST_F`, `TEST_P`, `TYPED_TEST`, `TYPED_TEST_P`

Bedeutung:
- Das ist ein Inventar der deklarierten Teststellen im Sourcecode.
- Diese Zahl ist **kein** Laufzeit-Pass/Fail-Nenner.
- Sie kann kleiner als die reale Laufzeit-Testzahl sein, weil parameterisierte oder typisierte Tests beim Ausfuehren expandieren.

Aktuelle Zahl:
- 34685 deklarierte GoogleTest-Testmakros

### 2. Runtime GoogleTest-Faelle in gebauten Test-Binaries

Definition:
- Alle gebauten Test-Executables unter `build-msvc-ninja-release/cmake/tests/*.exe`
- Pro Binary wird `--gtest_list_tests` ausgefuehrt
- Gezaehlt werden GoogleTest-Suites und einzelne GoogleTest-Testfaelle

Bedeutung:
- Das ist die sauberste Zahl fuer "wie viele GoogleTests existieren in den bereits gebauten Binaries wirklich?"
- Diese Zahl ist groesser als die Source-Makro-Zahl, wenn Parametrisierung/Typisierung mehrere Laufzeitfaelle erzeugt.

Aktuelle Zahl:
- 466 gebaute Test-Executables im Build-Ordner
- 465 davon liefern GoogleTest-Listen
- 4747 GoogleTest-Suites
- 42299 GoogleTest-Testfaelle

### 3. CTest-Eintraege pro Preset

Definition:
- Gezaehlt werden die von CMake/CTest registrierten Testeintraege (`ctest --preset <preset> -N`)
- Ein CTest-Eintrag ist typischerweise ein Wrapper um ein Test-Binary oder um einen gefilterten Aufruf eines Test-Binaries

Bedeutung:
- Das ist die richtige Zahl fuer CI-/Preset-Gesundheit.
- Diese Zahl darf nicht direkt mit GoogleTest-Fallzahlen verglichen werden.
- Ein einzelner CTest-Eintrag kann 1, 10 oder 1000+ GoogleTest-Faelle enthalten.

Aktuelle Zahl fuer `msvc-ninja-release`:
- 1157 konfigurierte CTest-Eintraege
- 108 davon verweisen aktuell auf fehlende Executables
- 1049 sind im aktuellen Build prinzipiell ausfuehrbar

Aktuelle Zahl fuer `graph-tests-release`:
- 29 konfigurierte CTest-Eintraege
- 0 fehlende Executables
- 29 ausfuehrbar
- letzter validierter Lauf: 29/29 gruen

## Reporting-Regeln

Fuer belastbare Statusmeldungen immer genau eine dieser Formen benutzen:

1. CTest-Preset-Gesundheit
- Beispiel: `graph-tests-release: 29/29 CTest-Eintraege gruen`
- Beispiel: `msvc-ninja-release: 1049 runnable von 1157 konfigurierten CTest-Eintraegen`

2. GoogleTest-Runtime-Inventar
- Beispiel: `42299 GoogleTest-Faelle in 4747 Suites ueber 465 gebaute Test-Binaries`

3. Source-Inventar
- Beispiel: `34685 deklarierte GoogleTest-Makros im tests/-Quellbaum`

Nicht mehr vermischen:
- `1157 CTests` vs. `42299 GoogleTests` vs. `34685 TEST-Makros`
- Das sind drei verschiedene Metriken mit drei verschiedenen Nennern.

## Empfohlene offizielle Kennzahlen

Wenn wir kuenftig "alle Tests gruen" sagen wollen, sollten wir das in zwei sauberen Ebenen berichten:

1. Preset-Ebene
- `Alle CTest-Eintraege im Ziel-Preset sind gruen.`

2. GTest-Ebene
- `Alle gelisteten GoogleTest-Faelle in den gebauten Test-Binaries sind gruen.`

## Reproduzierbarkeit

Das Skript

- `tests/collect_test_statistics.ps1`

liefert dieselben Kennzahlen reproduzierbar fuer den aktuellen Build und die aktuellen Presets.

Aufruf:

```powershell
pwsh -File tests/collect_test_statistics.ps1
```

Optional mit anderen Presets:

```powershell
pwsh -File tests/collect_test_statistics.ps1 -CTestPreset msvc-ninja-release -GraphPreset graph-tests-release -BuildDir build-msvc-ninja-release
```