# Prompt Engineering Guide (ThemisDB)

Dieses Dokument definiert verbindliche Prompt-Muster für reproduzierbare, prüfbare KI-gestützte Umsetzung.

## 1. Step-by-step Decomposition (statt "implement feature X")

Jeder Implementierungs-Prompt MUSS in klar getrennte Schritte zerlegt sein:

1. **Scope klären** (Dateien/Module/Out-of-Scope)
2. **Kontext laden** (ROADMAP, FUTURE_ENHANCEMENTS, relevante Header/Tests)
3. **Änderungsplan** (kleinste sichere Inkremente)
4. **Implementierung** (nur vereinbarter Scope)
5. **Validierung** (Build/Test/Lint bzw. dokumentierte Limits)
6. **Review-Zusammenfassung** (Risiken, offene Punkte, Rückbauplan für Stubs)

## 2. Akzeptanzkriterien und Testfälle im Prompt (Pflicht)

Prompts müssen vor der Implementierung messbare Akzeptanzkriterien enthalten:

- Erwartetes Laufzeitverhalten
- Eingaben/Fehlerfälle/Edge Cases
- Nicht-funktionale Ziele (Performance, Sicherheit, Determinismus)
- Explizite Teststrategie (Unit/Integration/Regression)

### Prompt-Template

```text
Task: <konkrete Aufgabe>
Scope: <betroffene Dateien/Namespaces>
Out of Scope: <klare Abgrenzung>
Acceptance Criteria:
- ...
Test Cases:
- ...
Validation Commands:
- ...
Constraints:
- ...
```

## 3. Checkpoint-Strategie für komplexe Agent-Läufe

Bei umfangreichen Änderungen sind verbindliche Checkpoints zu setzen:

- **Checkpoint A:** Analyse abgeschlossen, Plan stabil
- **Checkpoint B:** Kernänderung implementiert, noch nicht finalisiert
- **Checkpoint C:** Validierung abgeschlossen, diff-review bereit

An jedem Checkpoint müssen Scope, Risiken und nächste Schritte explizit bestätigt werden.

## 4. Dokumentations-Enforcement (KI-Pflicht)

Für C++-Änderungen gilt zusätzlich:

- Öffentliche APIs mit Doxygen kommentieren (`@brief`, `@param`, `@return`, `@throws` falls relevant)
- Für Templates zusätzlich `@tparam`; für Concepts zusätzlich `@requires`
- Kommentare erklären **Warum** (Constraint/Trade-off), nicht nur **Was**
- Bei Refactorings bestehende Dokumentation synchron aktualisieren
- Edge-Case-Verhalten in der Doku explizit nennen (z. B. Fehlerpfade, Null-/Empty-Handling)

## 5. Beispiel-Prompts

### 5.1 Thread-sichere Queue

```text
Implementiere eine thread-sichere FIFO-Queue in <module> mit klarer Ownership und ohne raw new/delete.
Acceptance Criteria:
- MPMC-Sicherheit unter Parallelzugriff
- Kein Busy-Wait ohne Begründung
- Definierte Shutdown-Semantik
Test Cases:
- Producer/Consumer Paralleltest
- Shutdown während blockierendem Pop
- Race-Regression für Spurious Wakeups
```

### 5.2 Socket-Handler

```text
Erweitere den Socket-Handler in <module> um Timeout- und Error-Path-Behandlung.
Acceptance Criteria:
- Timeouts führen zu deterministischem Fehlercode
- Ressourcen werden in allen Fehlerpfaden freigegeben
- Keine stillen Fehler
Test Cases:
- Timeout-Simulation
- Verbindungsabbruch während Read/Write
- Mehrfaches Reconnect-Szenario
```

### 5.3 Token-Bucket-Algorithmus

```text
Implementiere einen Token-Bucket-Limiter in <module> mit konfigurierbarer Rate/Burst.
Acceptance Criteria:
- Korrekte Token-Auffüllung über Zeit
- Deterministisches Verhalten bei Grenzwerten
- Thread-safe Nutzung für parallele Requests
Test Cases:
- Burst voll/leer
- Präzision bei kleinen Zeitintervallen
- Gleichzeitige acquire()-Aufrufe
```
