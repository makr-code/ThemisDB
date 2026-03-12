# ThemisDB Downloads WordPress Plugin - Installationsanleitung

## Schritt 1: Plugin herunterladen

Das Plugin befindet sich im ThemisDB Repository unter:
```
wordpress-plugin/themisdb-downloads/
```

## Schritt 2: Plugin-Verzeichnis vorbereiten

Erstellen Sie ein ZIP-Archiv des Plugin-Ordners:

```bash
cd wordpress-plugin
zip -r themisdb-downloads.zip themisdb-downloads/
```

Oder kopieren Sie den gesamten Ordner direkt in Ihre WordPress-Installation.

## Schritt 3: Installation in WordPress

### Option A: Upload über WordPress Admin (Empfohlen)

1. Melden Sie sich in Ihrem WordPress-Admin-Bereich an
2. Navigieren Sie zu **Plugins → Installieren**
3. Klicken Sie auf **Plugin hochladen**
4. Wählen Sie die `themisdb-downloads.zip` Datei aus
5. Klicken Sie auf **Jetzt installieren**
6. Klicken Sie auf **Plugin aktivieren**

### Option B: Manuelle Installation via FTP

1. Verbinden Sie sich via FTP mit Ihrem Webserver
2. Navigieren Sie zu `/wp-content/plugins/`
3. Laden Sie den Ordner `themisdb-downloads` hoch
4. Gehen Sie zu WordPress Admin → Plugins
5. Aktivieren Sie "ThemisDB Downloads"

### Option C: Installation via WP-CLI

```bash
# Plugin-Verzeichnis in WordPress kopieren
cp -r themisdb-downloads /var/www/html/wp-content/plugins/

# Plugin aktivieren
wp plugin activate themisdb-downloads
```

## Schritt 4: Plugin konfigurieren

1. Gehen Sie zu **Einstellungen → ThemisDB Downloads**
2. Überprüfen Sie die Standard-Einstellungen:
   - **GitHub Repository:** `makr-code/wordpressPlugins` (Standard)
   - **Cache Dauer:** `3600` Sekunden (1 Stunde)
   - **Anzahl Releases:** `10`
3. Optional: Fügen Sie einen GitHub Personal Access Token hinzu für höhere API-Limits

## Schritt 5: Seite erstellen

1. Erstellen Sie eine neue Seite: **Seiten → Erstellen**
2. Titel: "Downloads" (oder wie gewünscht)
3. Fügen Sie den Shortcode ein:
   ```
   [themisdb_downloads]
   ```
4. Veröffentlichen Sie die Seite

## Schritt 6: Testen

1. Öffnen Sie die erstellte Seite
2. Sie sollten die neuesten ThemisDB Releases sehen
3. Überprüfen Sie, dass die Download-Links funktionieren
4. Überprüfen Sie, dass die SHA256-Checksums angezeigt werden

## Erweiterte Konfiguration

### GitHub Personal Access Token erstellen

Für höhere API-Limits (5000 statt 60 Anfragen pro Stunde):

1. Gehen Sie zu: https://github.com/settings/tokens
2. Klicken Sie auf **Generate new token (classic)**
3. Token-Name: "WordPress ThemisDB Plugin"
4. Wählen Sie **keine** Berechtigungen (öffentliche Repos benötigen keine Scopes)
5. Klicken Sie auf **Generate token**
6. Kopieren Sie den Token
7. Fügen Sie ihn in WordPress unter **Einstellungen → ThemisDB Downloads** ein

### Cache-Optimierung

Für Websites mit vielen Besuchern:

1. Erhöhen Sie die Cache-Dauer auf 7200 (2 Stunden) oder mehr
2. Verwenden Sie ein WordPress-Cache-Plugin (z.B. WP Super Cache)
3. Aktivieren Sie Browser-Caching

### Mehrere Seiten erstellen

Sie können mehrere Seiten mit unterschiedlichen Filtern erstellen:

**Download-Seite (Windows):**
```
[themisdb_downloads platform="windows"]
```

**Download-Seite (Linux):**
```
[themisdb_downloads platform="linux"]
```

**Download-Seite (Alle Versionen):**
```
[themisdb_downloads show="all" style="table"]
```

## Problemlösung

### Problem: "Plugin konnte nicht aktiviert werden"

**Lösung:**
- Überprüfen Sie die PHP-Version (mindestens 7.2)
- Überprüfen Sie die WordPress-Version (mindestens 5.0)
- Überprüfen Sie die Dateiberechtigungen

### Problem: "API-Fehler: 404"

**Lösung:**
- Überprüfen Sie die Repository-Einstellung
- Format muss sein: `owner/repository`
- Beispiel: `makr-code/wordpressPlugins`

### Problem: "Keine Releases gefunden"

**Lösung:**
1. Leeren Sie den Cache in den Plugin-Einstellungen
2. Überprüfen Sie, ob das Repository öffentliche Releases hat
3. Überprüfen Sie die Netzwerkverbindung des Servers

## Deinstallation

1. Deaktivieren Sie das Plugin unter **Plugins**
2. Klicken Sie auf **Löschen**
3. Der Cache wird automatisch gelöscht

## Support

Bei Problemen:
- Erstellen Sie ein Issue auf GitHub: https://github.com/makr-code/wordpressPlugins/issues
- Überprüfen Sie die Dokumentation im README.md
- Kontaktieren Sie das ThemisDB-Team

## Lizenz

MIT License - Siehe LICENSE-Datei für Details
