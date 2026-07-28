# Memory Management Policy (RAII & Ownership)

Datum: 2026-07-28
Status: Active
Bezug: Verbindliche Ownership- und Lifetime-Richtlinie fuer AI-gestuetzte C++-Aenderungen
Primary (Quelle der Wahrheit): include/**, src/**, ai_context/COPILOT_INSTRUCTIONS.md

Diese Richtlinie beschreibt verpflichtende Ownership- und Lifetime-Regeln für KI-gestützte C++-Änderungen.

## Kernprinzipien

1. **RAII first**: Ressourcenlebenszeit an Objekte binden.
2. **Kein manuelles `new`/`delete`** im Normalfall.
3. **Eindeutige Ownership** pro Ressource definieren.
4. **Exception Safety** für alle Ressourcenpfade sicherstellen.

## Ownership-Regeln

- Exklusive Ownership: `std::unique_ptr`
- Geteilte Ownership nur mit begründetem Bedarf: `std::shared_ptr`
- Beobachtende Referenz ohne Ownership: `std::weak_ptr`, `std::span`, `std::string_view`, `const&`

## API-Vertragsanforderungen

- Öffentliche APIs dokumentieren Ownership und Lifetime explizit.
- Return-Werte und Fehlerpfade dürfen keine Ressourcenlecks erzeugen.
- Threading-Pfade müssen Lock-Lifetime klar durch RAII-Konstrukte kapseln (`std::lock_guard`, `std::unique_lock`).

## Review-Checkpunkte

- Gibt es einen klaren Owner für jede Ressource?
- Wird bei Fehlern/Exceptions sauber freigegeben?
- Sind Nicht-Owner-Typen (`span`/`string_view`) korrekt und sicher verwendet?
- Wurde die API-Dokumentation bei Verhaltensänderungen synchron aktualisiert?
