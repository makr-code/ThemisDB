---
status: stale
doc_version: "1.3.0"
validated: "2025-12-22"
---

# Development Documentation — Struktur

**Stand:** 6. April 2026  
**Version:** v1.3.0  
**Kategorie:** 🛠️ Development

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#übersicht)
- [Struktur](#struktur)
- [Unterordner](#unterordner)

---

## Übersicht

Ziel: Bessere Übersicht durch thematische Gruppierung der Entwicklungsdokumente.

Neue/empfohlene Unterordner:

- `changefeed/` — alle Changefeed/CDC‑bezogenen Snippets, Testspezifikationen, SSE‑Beispiele und CMake‑Helfer.
- `security/` — Sicherheits‑ und Kryptographiebezogene Design‑Dokumente (z. B. Content ZSTD, HKDF Cache).
- `overviews/` — konsolidierte Übersichten und Verifikations‑Zusammenfassungen (`consolidated_development_overview.md`, `verification_by_area.md`, `feature_status_*`).

Aktueller Status:
- Die relevanten Dateien wurden kopiert in die neuen Unterordner unter `docs/development/`.
- Die Originaldateien befinden sich weiterhin im Ordner `docs/development/` (zur momentanen Sicherheit/Review).

Empfohlenes Vorgehen:
- Review der neuen Ordnerinhalte.
- Nach Bestätigung: Löschen der Duplikate im Top‑Level `docs/development/` (ich kann das automatisiert durchführen).

Wenn du möchtest, lösche ich die Originaldateien jetzt. Antworte mit "Lösche Duplikate" oder "Behalte Kopien".

```
