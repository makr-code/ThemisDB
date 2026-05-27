# Scanner Refactor Plan

## Ziel
- Scanner-Dateien in eine klarere, konsolidierte Struktur überführen.
- Öffentliche Einstiegspunkte behalten, aber neue Canonical-Namen einführen.

## Vorgehen
1. Canonical Package unter `tools/scanners/` anlegen.
2. Aktuelle Scanner-Dateien als Compatibility-Wrappers belassen.
3. Neue, kurze Namen für die wichtigsten Einstiegspunkte bereitstellen.
4. Dokumentation und Referenzen schrittweise auf die neuen Namen umstellen.

## Akzeptanzkriterien
- Es gibt einen klaren, kanonischen Import-/Startpfad für den Scanner.
- Alte Skriptpfade bleiben funktionsfähig.
- Die Struktur ist besser lesbar als die bisherige `gap_scanner_v3*`-Sammlung.