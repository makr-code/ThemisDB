> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Blog/Wiki-System - Content-Management mit ThemisDB

![Status](https://img.shields.io/badge/status-ready-brightgreen)
![Difficulty](https://img.shields.io/badge/difficulty-easy-green)
![Duration](https://img.shields.io/badge/duration-30--40%20min-blue)

## 📝 Übersicht

Das Blog/Wiki-System demonstriert ein flexibles Content-Management-System mit ThemisDB. Sie lernen:
- Artikel mit Markdown-Support zu erstellen und verwalten
- Volltext-Suche über Inhalte
- Versionierung und Artikel-Historie
- Kategorien und Tags für Organisation
- Kommentarsystem mit verschachtelten Strukturen

Die Anwendung bietet eine Tkinter-GUI für das Erstellen, Bearbeiten und Durchsuchen von Artikeln.

## ✨ Features

- ✅ **Artikel-Verwaltung** - Erstellen, Bearbeiten, Löschen von Artikeln
- ✅ **Markdown-Support** - Rich-Text-Formatierung mit Markdown
- ✅ **Volltext-Suche** - Schnelle Suche über Titel und Inhalt
- ✅ **Kategorien & Tags** - Flexible Organisation von Inhalten
- ✅ **Versionierung** - Vollständige Artikel-Historie mit Rollback
- ✅ **Kommentarsystem** - Nested Comments mit Threading
- ✅ **Favoriten** - Artikel als Favoriten markieren
- ✅ **Export/Import** - Backup und Wiederherstellung

## 🖼️ Screenshots

*UI-Screenshots werden nach Implementierung hinzugefügt*

## 📋 Voraussetzungen

### ThemisDB Server

```bash
docker run -d \
  --name themisdb \
  -p 8080:8080 \
  -p 18765:18765 \
  themisdb/themisdb:latest

# Server-Status prüfen
curl http://localhost:8080/health
```

### Python und Dependencies

- Python 3.8 oder höher
- Tkinter (normalerweise mit Python vorinstalliert)

## 🚀 Installation

1. **Navigieren Sie zum Beispiel-Verzeichnis**:
   ```bash
   cd examples/11_blog_wiki
   ```

2. **Installieren Sie die Abhängigkeiten**:
   ```bash
   pip install -r requirements.txt
   ```

## 🎮 Verwendung

### Anwendung starten

```bash
python main.py
```

### Grundlegende Operationen

1. **Artikel erstellen**:
   - Klicken Sie auf "New Article"
   - Geben Sie Titel, Kategorie und Inhalt ein
   - Fügen Sie Tags hinzu (komma-getrennt)
   - Klicken Sie auf "Save"

2. **Artikel bearbeiten**:
   - Wählen Sie einen Artikel aus der Liste
   - Klicken Sie auf "Edit"
   - Nehmen Sie Änderungen vor
   - Speichern Sie - eine neue Version wird erstellt

3. **Artikel suchen**:
   - Geben Sie Suchbegriffe ein
   - Volltext-Suche über Titel und Inhalt
   - Filtern nach Kategorie oder Tag

4. **Kommentare hinzufügen**:
   - Öffnen Sie einen Artikel
   - Scrollen Sie zum Kommentarbereich
   - Verfassen und posten Sie Kommentare
   - Antworten Sie auf bestehende Kommentare

5. **Versionshistorie anzeigen**:
   - Öffnen Sie einen Artikel
   - Klicken Sie auf "History"
   - Vergleichen Sie Versionen
   - Stellen Sie frühere Versionen wieder her

Siehe [HOW_TO.md](HOW_TO.md) für detaillierte Anleitungen.

## 📊 Datenmodell

### Article (Artikel)

```json
{
  "id": "article_uuid",
  "title": "Mein erster Blog-Artikel",
  "slug": "mein-erster-blog-artikel",
  "content": "# Überschrift\n\nDas ist der Inhalt...",
  "category": "Tutorial",
  "tags": ["python", "database", "themisdb"],
  "author": "max.mustermann",
  "status": "published",
  "version": 3,
  "created_at": "2025-12-22T10:00:00Z",
  "updated_at": "2025-12-22T15:30:00Z",
  "published_at": "2025-12-22T10:05:00Z",
  "favorites_count": 42,
  "comments_count": 8
}
```

### Comment (Kommentar)

```json
{
  "id": "comment_uuid",
  "article_id": "article_uuid",
  "parent_comment_id": null,
  "author": "jane.doe",
  "content": "Toller Artikel! Vielen Dank.",
  "created_at": "2025-12-22T11:15:00Z",
  "replies": []
}
```

### ArticleVersion (Artikel-Version)

```json
{
  "id": "version_uuid",
  "article_id": "article_uuid",
  "version_number": 2,
  "title": "Mein erster Blog-Artikel (v2)",
  "content": "Vorheriger Inhalt...",
  "changed_by": "max.mustermann",
  "changed_at": "2025-12-22T14:00:00Z",
  "change_note": "Rechtschreibkorrektur"
}
```

### Category (Kategorie)

```json
{
  "id": "category_uuid",
  "name": "Tutorial",
  "slug": "tutorial",
  "description": "Anleitungen und How-Tos",
  "article_count": 15
}
```

## 🛠️ ThemisDB Features

### Document Model
- Flexible Dokumentstruktur für Artikel
- Nested Documents für Kommentare
- Arrays für Tags

### Full-Text Search
- Schnelle Suche über alle Artikel
- Index auf Titel und Inhalt
- Relevanz-Scoring

### Time-Series
- Versionierung als Time-Series
- Historie aller Änderungen
- Temporal Queries

### Secondary Indexes
- Index auf Tags
- Index auf Kategorie
- Index auf Status

## 📚 Was Sie lernen

1. **Document Store Pattern**:
   - Flexible Schema-Definition
   - JSON-Dokumente mit ThemisDB
   - Nested Structures

2. **Full-Text Search**:
   - Index-Erstellung
   - Query-Syntax
   - Relevanz-Tuning

3. **Versionierung**:
   - Time-Series für Historie
   - Version Tracking
   - Rollback-Mechanismen

4. **Content Management**:
   - CRUD für Artikel
   - Metadaten-Management
   - Kategorisierung

## 🔗 Navigation

- ⬅️ Vorheriges Beispiel: [10 - Drone Image Analysis](../10_drone_image_analysis/)
- ➡️ Nächstes Beispiel: [12 - Expense Tracker](../12_expense_tracker/)
- 🏠 [Zurück zur Übersicht](../README.md)

## 📄 Lizenz

Dieses Beispiel steht unter der MIT-Lizenz und kann frei verwendet werden.

---

**Status**: Ready für Implementation | **Letzte Aktualisierung**: 2025-12-22
