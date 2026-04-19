> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Todo-App - Aufgabenverwaltung mit ThemisDB

![Status](https://img.shields.io/badge/status-ready-brightgreen)
![Difficulty](https://img.shields.io/badge/difficulty-easy-green)
![Duration](https://img.shields.io/badge/duration-15--20%20min-blue)

## 📝 Übersicht

Die Todo-App demonstriert, wie man eine einfache Aufgabenverwaltung mit ThemisDB erstellt. Sie zeigt Listenoperationen, Filterung nach Status und persistente Datenspeicherung.

## ✨ Features

- ✅ **Aufgaben erstellen** - Neue Tasks mit Titel und Beschreibung
- ✅ **Status-Verwaltung** - Offen, In Arbeit, Erledigt
- ✅ **Prioritäten** - Niedrig, Normal, Hoch
- ✅ **Filterung** - Nach Status und Priorität filtern
- ✅ **Suche** - Volltextsuche in Titel und Beschreibung
- ✅ **Listen-UI** - Übersichtliche Darstellung aller Tasks
- ✅ **Persistierung** - Alle Daten in ThemisDB gespeichert

## 🖼️ Screenshots

*Screenshots werden nach Implementierung hinzugefügt*

## 📋 Voraussetzungen

- ThemisDB Server (siehe [Hauptdokumentation](../README.md))
- Python 3.8+
- Tkinter

## 🚀 Installation

```bash
cd examples/02_todo_app
pip install -r requirements.txt
python main.py
```

## 📊 Datenmodell

```python
{
    "id": "task_uuid",
    "title": "Aufgabe erledigen",
    "description": "Beschreibung der Aufgabe",
    "status": "open",  # open, in_progress, done
    "priority": "normal",  # low, normal, high
    "created_at": "2025-12-22T10:00:00Z",
    "updated_at": "2025-12-22T10:00:00Z",
    "due_date": "2025-12-25T00:00:00Z"
}
```

## 🔧 Verwendung

Siehe [HOW_TO.md](HOW_TO.md) für detaillierte Anleitungen.

## 📚 Was Sie lernen

- **Listen-Operationen** - Mehrere Entities verwalten
- **Filterung** - Daten nach Kriterien filtern
- **Indizes** - Secondary Indexes für schnelle Queries
- **AQL Queries** - Fortgeschrittene Datenbankabfragen
- **Tkinter Listbox** - Listen-UI-Komponenten

## 📄 Dateien

- `README.md` - Diese Datei
- `HOW_TO.md` - Bedienungsanleitung
- `main.py` - Hauptanwendung
- `models.py` - Datenmodelle
- `requirements.txt` - Dependencies

## 🔗 Weiterführend

- Vorheriges Beispiel: [Hello World](../01_hello_world/)
- Nächstes Beispiel: [Kontaktmanager](../03_contact_manager/)

---

**Status**: ✅ Implementiert | Voll funktionsfähig
