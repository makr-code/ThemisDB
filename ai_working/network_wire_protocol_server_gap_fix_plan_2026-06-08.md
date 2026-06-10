# Network Wire Protocol Server Gap Fix Plan (2026-06-08)

## Scope

Erste kleine Gap-Reduktionswelle fuer `src/network/wire_protocol_server.cpp`.

## Quellen
- `src/network/MODULE_GAPS.md`
- `ai_working/MODULE_GAPS_EXECUTION_PLAN_2026-06-08.md`

## Ziel dieser Welle

Nur kleine, testbare Reparaturen mit hoher Sicherheit:
1. sensibles Logging im Startpfad reduzieren
2. mindestens einen klaren Datenrennen-Hotspot entschärfen, falls lokal und risikoarm
3. keine breitflächige Threading-Architekturänderung in dieser Welle

## Nicht Teil dieser Welle
- umfassende Umstellung von Join-/Wait-Modellen
- invasive Lebenszyklusänderungen an I/O-Threads
- größere API- oder Architekturumbauten

## Validierung
- betroffene Network-Fokus-Targets bauen/laufen lassen
- mindestens die Wire-/Network-Focused-Targets für den berührten Pfad prüfen

## Notizen
- Timeout-/Join-Findings bleiben als nächste Welle offen, falls sie eine breitere Lebenszyklusänderung erfordern.
