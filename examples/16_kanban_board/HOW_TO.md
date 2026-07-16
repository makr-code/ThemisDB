> **Aktueller Build-Flow:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Kanban Board - Anleitung

> **Historischer Stand:** 2025-12-22 — Inhalte nicht gegen aktuelle Quellen geprüft.

## 🚀 Schnellstart

```bash
cd examples/16_kanban_board
pip install -r requirements.txt
pip install themisdb-client
python main.py
```
<!-- TODO: verify against current source -->

## 📖 Hauptfunktionen

### Tasks verwalten
- Drag & Drop zwischen Spalten
- Prioritäten setzen (High, Medium, Low)
- Assignees zuweisen
- Story Points vergeben

### Sprint-Planung
- Sprint erstellen mit Start/End-Datum
- Tasks dem Sprint zuweisen
- Velocity tracking

### Burndown-Chart
- Automatische Generierung
- Zeigt Sprint-Fortschritt
- Ideal vs. Actual Line

---

**Letzte Aktualisierung**: 2025-12-22
