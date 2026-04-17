# SECURITY

## Scope
- Modul/Ordner: `examples/18_realtime_chat`
- Sicherheitsrelevante Funktionen, Konfigurationen und Datenflüsse in diesem Verzeichnis.

## Bedrohungsmodell (Kurz)
- Unvalidierte Eingaben
- Unzureichende Autorisierung/Authentisierung
- Leakage sensibler Daten in Logs/Artefakten
- Fehlkonfiguration bei Deployment/Runtime

## Mindestanforderungen
- Eingaben strikt validieren und Fehler explizit behandeln
- Geheimnisse niemals im Klartext ablegen
- Security-relevante Änderungen mit Tests absichern
- Abhängigkeiten regelmäßig auf Schwachstellen prüfen

## Incident & Meldung
- Sicherheitsfunde gemäß Root-`SECURITY.md` melden und behandeln.
