> **Aktueller Build-Flow:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Blog/Wiki-System - Anleitung

Diese Anleitung führt Sie Schritt für Schritt durch die Verwendung des Blog/Wiki-Systems.

## 🚀 Schnellstart

### 1. ThemisDB Server starten

```bash
docker run -d \
  --name themisdb \
  -p 8080:8080 \
  -p 18765:18765 \
  themisdb/themisdb:latest
```

### 2. Anwendung starten

```bash
cd examples/11_blog_wiki
pip install -r requirements.txt
python main.py
```

## 📖 Hauptfunktionen

### Artikel erstellen

1. Klicken Sie auf den **"New Article"** Button in der Toolbar
2. Ein Formular erscheint mit folgenden Feldern:
   - **Titel**: Überschrift des Artikels
   - **Kategorie**: Dropdown zur Kategorieauswahl
   - **Tags**: Komma-getrennte Liste (z.B. "python, tutorial, database")
   - **Inhalt**: Markdown-Editor für den Artikel-Text
3. Markdown-Formatierung:
   ```markdown
   # Große Überschrift
   ## Mittlere Überschrift
   
   **Fett** und *kursiv*
   
   - Liste
   - Punkt 2
   
   [Link](https://example.com)
   ```
4. Klicken Sie auf **"Save"**, um den Artikel zu veröffentlichen
5. Status wählen:
   - **Draft**: Entwurf (nicht öffentlich)
   - **Published**: Veröffentlicht

### Artikel bearbeiten

1. Wählen Sie einen Artikel aus der **Artikel-Liste** (linke Seite)
2. Klicken Sie auf den **"Edit"** Button
3. Nehmen Sie Ihre Änderungen vor
4. **Optional**: Fügen Sie eine Change-Note hinzu (beschreibt die Änderung)
5. Klicken Sie auf **"Save"**
6. Eine neue Version wird automatisch erstellt

### Artikel suchen

#### Volltext-Suche
1. Geben Sie Suchbegriffe in das **Suchfeld** ein
2. Die Suche läuft über:
   - Titel
   - Inhalt
   - Tags
3. Ergebnisse werden nach Relevanz sortiert

#### Nach Kategorie filtern
1. Wählen Sie eine Kategorie aus dem **Filter-Dropdown**
2. Nur Artikel dieser Kategorie werden angezeigt

#### Nach Tags filtern
1. Klicken Sie auf einen **Tag** in der Tag-Cloud
2. Alle Artikel mit diesem Tag werden angezeigt

### Kommentare verwalten

#### Kommentar hinzufügen
1. Öffnen Sie einen Artikel
2. Scrollen Sie zum **Kommentarbereich** unten
3. Geben Sie Ihren Kommentar ein
4. Klicken Sie auf **"Post Comment"**

#### Auf Kommentar antworten
1. Klicken Sie auf **"Reply"** unter einem Kommentar
2. Geben Sie Ihre Antwort ein
3. Die Antwort wird als verschachtelter Kommentar angezeigt

#### Kommentar löschen
1. Klicken Sie auf **"Delete"** neben Ihrem Kommentar
2. Bestätigen Sie die Löschung

### Versionshistorie

#### Historie anzeigen
1. Öffnen Sie einen Artikel
2. Klicken Sie auf **"History"** Button
3. Eine Liste aller Versionen erscheint mit:
   - Version-Nummer
   - Änderungsdatum
   - Autor
   - Change-Note

#### Versionen vergleichen
1. Wählen Sie zwei Versionen aus
2. Klicken Sie auf **"Compare"**
3. Ein Diff wird angezeigt (Hinzugefügt/Gelöscht/Geändert)

#### Version wiederherstellen
1. Wählen Sie eine frühere Version aus
2. Klicken Sie auf **"Restore"**
3. Bestätigen Sie die Wiederherstellung
4. Eine neue Version wird mit dem alten Inhalt erstellt

### Kategorien verwalten

#### Neue Kategorie erstellen
1. Gehen Sie zu **"Settings"** → **"Categories"**
2. Klicken Sie auf **"New Category"**
3. Geben Sie ein:
   - Name
   - Beschreibung
4. Klicken Sie auf **"Create"**

#### Kategorie bearbeiten
1. Wählen Sie eine Kategorie aus
2. Ändern Sie Name oder Beschreibung
3. Klicken Sie auf **"Save"**

#### Kategorie löschen
1. Wählen Sie eine Kategorie
2. Klicken Sie auf **"Delete"**
3. **Hinweis**: Artikel der Kategorie werden auf "Uncategorized" gesetzt

### Favoriten

#### Artikel als Favorit markieren
1. Öffnen Sie einen Artikel
2. Klicken Sie auf das **Stern-Symbol** ⭐
3. Der Artikel wird zu Ihren Favoriten hinzugefügt

#### Favoriten anzeigen
1. Klicken Sie auf **"Favorites"** in der Sidebar
2. Alle favorisierten Artikel werden angezeigt

### Export und Import

#### Artikel exportieren
1. Gehen Sie zu **"File"** → **"Export"**
2. Wählen Sie:
   - **Single Article**: Exportiert einen Artikel als JSON
   - **All Articles**: Exportiert alle Artikel als JSON
   - **Category**: Exportiert eine Kategorie mit allen Artikeln
3. Wählen Sie Speicherort
4. Klicken Sie auf **"Export"**

#### Artikel importieren
1. Gehen Sie zu **"File"** → **"Import"**
2. Wählen Sie JSON-Datei
3. Optionen:
   - **Overwrite existing**: Vorhandene Artikel überschreiben
   - **Create new**: Immer neue Artikel erstellen
4. Klicken Sie auf **"Import"**

## 💡 Tipps und Tricks

### Markdown Best Practices

1. **Überschriften hierarchisch verwenden**:
   ```markdown
   # Haupttitel (nur einer pro Artikel)
   ## Kapitel
   ### Unterkapitel
   ```

2. **Code-Blöcke mit Syntax-Highlighting**:
   ```markdown
   ```python
   def hello():
       print("Hello World")
   ```
   ```

3. **Tabellen erstellen**:
   ```markdown
   | Spalte 1 | Spalte 2 |
   |----------|----------|
   | Wert 1   | Wert 2   |
   ```

### Effektives Tagging

1. **Verwenden Sie konsistente Tags**:
   - Kleinschreibung: "python" statt "Python"
   - Singular: "tutorial" statt "tutorials"

2. **Nicht zu viele Tags**:
   - 3-5 Tags pro Artikel sind optimal
   - Zu viele Tags verwässern die Organisation

3. **Tag-Hierarchie nutzen**:
   - Allgemein → Spezifisch
   - z.B. "database, nosql, themisdb"

### Versionierung nutzen

1. **Aussagekräftige Change-Notes**:
   - ✅ "Rechtschreibfehler in Abschnitt 2 korrigiert"
   - ❌ "Update"

2. **Regelmäßig speichern**:
   - Bei größeren Änderungen mehrfach speichern
   - Kleinere Versionen sind besser als große

3. **Vor großen Änderungen Version erstellen**:
   - Manuell speichern vor Umstrukturierung
   - Ermöglicht einfaches Rollback

## ⌨️ Tastenkombinationen

### Allgemein
- `Ctrl + N`: Neuer Artikel
- `Ctrl + S`: Speichern
- `Ctrl + F`: Suchen
- `Ctrl + Q`: Beenden

### Editor
- `Ctrl + B`: Fett
- `Ctrl + I`: Kursiv
- `Ctrl + K`: Link einfügen
- `Ctrl + Shift + C`: Code-Block einfügen
- `Ctrl + Z`: Rückgängig
- `Ctrl + Y`: Wiederholen

### Navigation
- `Ctrl + →`: Nächster Artikel
- `Ctrl + ←`: Vorheriger Artikel
- `Ctrl + H`: Historie anzeigen
- `Ctrl + L`: Artikel-Liste fokussieren

## 🔧 Fehlerbehebung

### Verbindungsprobleme

**Problem**: "Cannot connect to ThemisDB"
- **Lösung**: 
  1. Prüfen Sie, ob ThemisDB läuft: `docker ps`
  2. Testen Sie die Verbindung: `curl http://localhost:8080/health`
  3. Prüfen Sie Firewall-Einstellungen

### Suche funktioniert nicht

**Problem**: Suche liefert keine Ergebnisse
- **Lösung**:
  1. Warten Sie auf Index-Aktualisierung (kann 1-2 Sekunden dauern)
  2. Prüfen Sie Suchbegriffe (mindestens 3 Zeichen)
  3. Versuchen Sie einfachere Begriffe

### Version kann nicht wiederhergestellt werden

**Problem**: "Failed to restore version"
- **Lösung**:
  1. Prüfen Sie, ob Artikel noch existiert
  2. Versuchen Sie, Artikel neu zu laden
  3. Prüfen Sie Berechtigungen

### Performance-Probleme

**Problem**: Langsame Suche/Laden
- **Lösung**:
  1. Reduzieren Sie Anzahl gleichzeitig geladener Artikel
  2. Nutzen Sie Pagination
  3. Archivieren Sie alte Artikel

## 🎯 Übungsaufgaben

### Anfänger
1. Erstellen Sie 3 Artikel in verschiedenen Kategorien
2. Fügen Sie jedem Artikel 2-3 Tags hinzu
3. Schreiben Sie Kommentare zu den Artikeln
4. Suchen Sie nach einem bestimmten Begriff

### Fortgeschritten
1. Erstellen Sie eine Artikel-Serie (Part 1, 2, 3) mit Verlinkung
2. Verwenden Sie komplexes Markdown (Tabellen, Code, Bilder)
3. Experimentieren Sie mit Versionierung (mehrere Edits)
4. Organisieren Sie Artikel in einer Kategorie-Hierarchie

### Experten
1. Exportieren und re-importieren Sie eine Kategorie
2. Vergleichen Sie zwei weit auseinanderliegende Versionen
3. Erstellen Sie eine Tag-Taxonomie für Ihr Wiki
4. Optimieren Sie die Suche für spezifische Use-Cases

## 📚 Weiterführende Ressourcen

- [Markdown Guide](https://www.markdownguide.org/)
- [ThemisDB Dokumentation](../../docs/)
- [Full-Text Search in ThemisDB](../../docs/full-text-search.md)
- [Document Model Guide](../../docs/document-model.md)

## 🆘 Support

Bei Fragen oder Problemen:
- [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)

---

**Letzte Aktualisierung**: 2025-12-22
