# Installation Guide

Schritt-für-Schritt-Anleitung zur Installation des ThemisDB Compendium Downloads Plugins.

## Voraussetzungen

- WordPress 5.0 oder höher
- PHP 7.2 oder höher
- Aktive Internetverbindung (für GitHub API-Zugriff)

## Installationsmethoden

### Methode 1: Automatische Installation (WordPress.org)

Sobald das Plugin auf WordPress.org verfügbar ist:

1. Melden Sie sich in Ihrem WordPress-Admin-Bereich an
2. Gehen Sie zu **Plugins → Installieren**
3. Suchen Sie nach "ThemisDB Compendium Downloads"
4. Klicken Sie auf **Jetzt installieren**
5. Klicken Sie auf **Aktivieren**

### Methode 2: Manuelle Installation via Upload

1. Laden Sie die neueste Version des Plugins herunter
2. Melden Sie sich in Ihrem WordPress-Admin-Bereich an
3. Gehen Sie zu **Plugins → Installieren**
4. Klicken Sie auf **Plugin hochladen**
5. Wählen Sie die ZIP-Datei aus und klicken Sie auf **Jetzt installieren**
6. Klicken Sie auf **Plugin aktivieren**

### Methode 3: Manuelle Installation via FTP

1. Laden Sie die neueste Version des Plugins herunter
2. Entpacken Sie die ZIP-Datei
3. Verbinden Sie sich per FTP mit Ihrem Webserver
4. Laden Sie den Ordner `themisdb-compendium-downloads` in das Verzeichnis `/wp-content/plugins/` hoch
5. Gehen Sie in Ihrem WordPress-Admin zu **Plugins**
6. Suchen Sie "ThemisDB Compendium Downloads" und klicken Sie auf **Aktivieren**

### Methode 4: Installation aus dem Repository (Entwickler)

Für Entwickler, die direkt aus dem GitHub-Repository installieren möchten:

```bash
# In Ihr WordPress-Plugin-Verzeichnis wechseln
cd /path/to/wordpress/wp-content/plugins/

# Repository klonen
git clone https://github.com/makr-code/wordpressPlugins.git themisdb-temp

# Plugin-Verzeichnis kopieren
cp -r themisdb-temp/wordpress-plugin/themisdb-compendium-downloads ./

# Temporäres Verzeichnis entfernen
rm -rf themisdb-temp

# WordPress-Admin aufrufen und Plugin aktivieren
```

## Erstinstallation

Nach der Aktivierung des Plugins:

### 1. Grundeinstellungen konfigurieren

1. Gehen Sie zu **Einstellungen → Kompendium Downloads**
2. Überprüfen Sie das GitHub-Repository (Standard: `makr-code/wordpressPlugins`)
3. Passen Sie die Cache-Dauer an, falls gewünscht (Standard: 3600 Sekunden)
4. Wählen Sie Ihren bevorzugten Button-Stil (Modern, Klassisch, Minimal)
5. Aktivieren/Deaktivieren Sie die Anzeige von Dateigrößen
6. Klicken Sie auf **Einstellungen speichern**

### 2. Shortcode einbinden

Fügen Sie den Shortcode auf einer Seite oder in einem Beitrag ein:

```
[themisdb_compendium_downloads]
```

**Hinweis für Classic Editor:**
Fügen Sie den Shortcode direkt in den Text-Editor ein.

**Hinweis für Gutenberg:**
1. Fügen Sie einen "Shortcode"-Block hinzu
2. Geben Sie den Shortcode ein: `[themisdb_compendium_downloads]`

### 3. Widget einrichten (Optional)

Falls Sie das Widget in einer Sidebar verwenden möchten:

1. Gehen Sie zu **Design → Widgets**
2. Suchen Sie das Widget "ThemisDB Kompendium Downloads"
3. Ziehen Sie es in die gewünschte Widget-Area (z.B. Sidebar)
4. Konfigurieren Sie die Widget-Einstellungen:
   - Titel
   - Stil (Modern, Klassisch, Minimal)
   - Version anzeigen (Ja/Nein)
   - Dateigröße anzeigen (Ja/Nein)
5. Klicken Sie auf **Speichern**

## Erste Schritte nach der Installation

### Testen Sie die Installation

1. Besuchen Sie die Seite, auf der Sie den Shortcode eingefügt haben
2. Überprüfen Sie, dass die Download-Buttons angezeigt werden
3. Klicken Sie auf einen Download-Button, um zu testen, ob der Download funktioniert
4. Überprüfen Sie die Download-Statistiken im Admin-Bereich

### Cache leeren

Wenn keine Downloads angezeigt werden:

1. Gehen Sie zu **Einstellungen → Kompendium Downloads**
2. Klicken Sie auf **Cache leeren**
3. Aktualisieren Sie die Seite im Frontend

### Anpassen des Designs

Wenn Sie das Design anpassen möchten:

1. Verwenden Sie die Shortcode-Parameter für schnelle Änderungen:
   ```
   [themisdb_compendium_downloads style="minimal" layout="list"]
   ```

2. Oder fügen Sie Custom CSS in Ihrem Theme hinzu:
   - Gehen Sie zu **Design → Customizer → Zusätzliches CSS**
   - Fügen Sie Ihre CSS-Regeln hinzu

## Fehlerbehebung bei der Installation

### Problem: Plugin lässt sich nicht aktivieren

**Lösung:**
- Überprüfen Sie, dass Sie PHP 7.2 oder höher verwenden
- Überprüfen Sie, dass Sie WordPress 5.0 oder höher verwenden
- Prüfen Sie die Datei-Berechtigungen (sollten 755 für Verzeichnisse und 644 für Dateien sein)

### Problem: "Keine Kompendium-Downloads verfügbar"

**Lösung:**
- Überprüfen Sie Ihre Internetverbindung
- Leeren Sie den Cache im Admin-Bereich
- Überprüfen Sie, dass das GitHub-Repository korrekt konfiguriert ist
- Prüfen Sie die Browser-Konsole auf JavaScript-Fehler

### Problem: Styles werden nicht geladen

**Lösung:**
- Leeren Sie den WordPress-Cache
- Leeren Sie den Browser-Cache
- Überprüfen Sie, dass die CSS-Dateien hochgeladen wurden
- Prüfen Sie in den Browser-Entwicklertools, ob CSS-Dateien geladen werden

### Problem: Widget wird nicht angezeigt

**Lösung:**
- Stellen Sie sicher, dass das Plugin aktiviert ist
- Überprüfen Sie, dass die Widget-Area in Ihrem Theme aktiv ist
- Prüfen Sie, ob das Widget in der richtigen Widget-Area platziert ist

## Deinstallation

Falls Sie das Plugin deinstallieren möchten:

### Soft-Deinstallation (Einstellungen bleiben erhalten)

1. Gehen Sie zu **Plugins**
2. Suchen Sie "ThemisDB Compendium Downloads"
3. Klicken Sie auf **Deaktivieren**

Die Plugin-Einstellungen bleiben in der Datenbank erhalten.

### Komplette Deinstallation (Einstellungen werden gelöscht)

1. Gehen Sie zu **Plugins**
2. Suchen Sie "ThemisDB Compendium Downloads"
3. Klicken Sie auf **Deaktivieren**
4. Klicken Sie auf **Löschen**

Dies entfernt:
- Plugin-Dateien
- Plugin-Einstellungen aus der Datenbank
- Gecachte Daten
- Download-Statistiken

## Upgrade

### Automatisches Update (über WordPress.org)

Wenn das Plugin über WordPress.org installiert wurde:

1. Gehen Sie zu **Dashboard → Aktualisierungen**
2. Wählen Sie "ThemisDB Compendium Downloads" aus
3. Klicken Sie auf **Plugins aktualisieren**

### Manuelles Update

1. Deaktivieren Sie das alte Plugin (nicht löschen)
2. Laden Sie die neue Version herunter
3. Entfernen Sie das alte Plugin-Verzeichnis via FTP
4. Laden Sie das neue Plugin-Verzeichnis hoch
5. Aktivieren Sie das Plugin erneut

**Wichtig:** Ihre Einstellungen bleiben erhalten, solange Sie das Plugin nicht komplett löschen.

## Support

Bei Problemen oder Fragen:

- **Dokumentation**: Siehe [README.md](README.md)
- **GitHub Issues**: https://github.com/makr-code/wordpressPlugins/issues
- **WordPress Support Forum**: (falls auf wordpress.org verfügbar)

---

**Viel Erfolg mit dem ThemisDB Compendium Downloads Plugin!**
