# IntelliSense vs Gap Scanner: Best-Practice Policy

Datum: 2026-05-21

## 1. Ziel

Die Tools beantworten unterschiedliche Fragen und muessen kombiniert, nicht gegeneinander gestellt werden:

- IntelliSense/Compiler: Build-semantische Wahrheit (Syntax, Typen, Includes, ABI-nahe Probleme).
- clang-tidy/cppcheck: semantische Codequalitaet und Modern-C++-Regeln.
- Gap Scanner v3: architektur- und risikoorientierte Pattern-Heuristik (Debt, OOP-Design, Uninitialized-Muster, Determinismus etc.).

## 2. Prioritaetsmodell (verbindlich)

### P0 (Merge-Blocker)

- Neue Compiler- oder IntelliSense-Errors in geaenderten Dateien.
- Reproduzierbare Critical-Scanner-Funde in sicherheits- oder korrektheitsnahen Kategorien:
  - security
  - input_validation
  - query_correctness
  - distributed_consistency
  - concurrency
  - memory

### P1 (vor Merge loesen oder explizit freigeben)

- Neue High-Scanner-Funde in den oben genannten Kategorien.
- clang-tidy-Warnungen mit unmittelbarem Fehlerrisiko (z. B. UB-nahe Muster, uninitialisierte Nutzung).

### P2 (Roadmap/Backlog)

- Design- und Debt-orientierte Findings ohne akutes Laufzeit-/Sicherheitsrisiko:
  - oop_design
  - observability
  - performance_patterns
  - audit_logging

## 3. Triage-Regeln fuer Scanner-Funde

1. Nur Delta gegen Baseline bewerten, nicht absolute Gesamtsumme.
2. Pro PR nur geaenderte Dateien als Pflichtbereich.
3. Kategorie unknown aktiv abbauen:
   - kurzfristig als nicht-blockierend markieren,
   - mittelfristig regelbasiert in bekannte Kategorien ueberfuehren.
4. Ein Fund ist erst blockierend, wenn reproduzierbar (mindestens ein konkreter Codepfad/Dateiverweis).

## 4. Workflow pro PR

1. Lokale Build-Semantik pruefen (CMake/Compiler + IntelliSense).
2. clang-tidy/cppcheck auf geaenderte Dateien laufen lassen.
3. Gap-Scanner modulweise ausfuehren, Delta gegen Baseline bilden.
4. Findings nach P0/P1/P2 labeln.
5. PR nur freigeben, wenn:
   - keine neuen P0,
   - P1 geloest oder mit dokumentierter Ausnahme freigegeben,
   - P2 im Backlog verlinkt.

## 5. Metriken fuer Reporting

Pflichtmetriken pro PR:

- compiler_errors_new
- intellisense_errors_new
- clang_tidy_high_risk_new
- scanner_critical_new (by category)
- scanner_high_new (by category)
- scanner_unknown_new

Zielbild:

- P0 = 0
- P1 trendend gegen 0
- unknown-Anteil sinkt kontinuierlich

## 6. Konfigurationskonsistenz

IntelliSense muss den Build spiegeln:

- C++ Standard: C++20
- compile_commands.json aus aktivem Preset
- gleiche Defines/Include-Pfade wie CMake-Generator

Damit sinken False Positives und der Kontrast zwischen IDE- und Scanner-Sicht wird fachlich belastbar.
