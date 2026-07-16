> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

# ARCHITECTURE

## Kontext
- Modul/Ordner: `examples/18_realtime_chat`
- Rolle innerhalb der Gesamtarchitektur von ThemisDB.

## Verantwortlichkeiten
- Bereitstellung der in diesem Ordner enthaltenen Bausteine/Artefakte
- Integration mit benachbarten Schichten (API, Query, Storage, Distributed)
- Einhaltung von Stabilitäts-, Sicherheits- und Wartbarkeitszielen

## Schnittstellen
- Nach außen: über öffentliche APIs/Konfigurationsdateien/CLI-Pfade
- Nach innen: über interne Module desselben Funktionsbereichs

## Nicht-Ziele
- Keine Duplizierung von Logik, die in übergeordneten Core-Modulen verankert ist
- Keine Umgehung zentraler Sicherheits- und Compliance-Vorgaben
