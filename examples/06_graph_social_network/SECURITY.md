# SECURITY

## Scope
- Modul/Ordner: `examples/06_graph_social_network`
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
