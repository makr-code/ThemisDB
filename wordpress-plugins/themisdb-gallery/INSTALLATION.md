# Installation Guide - ThemisDB Gallery Plugin

## Schnellstart

### 1. Plugin installieren

**Option A: ZIP-Upload (empfohlen)**

```bash
cd /path/to/ThemisDB/wordpress-plugin
zip -r themisdb-gallery.zip themisdb-gallery/
```

Dann in WordPress:
1. **Plugins → Installieren → Plugin hochladen**
2. ZIP-Datei auswählen und installieren
3. Plugin aktivieren

**Option B: Manueller Upload**

1. Laden Sie den Ordner `themisdb-gallery` nach `/wp-content/plugins/` hoch
2. In WordPress: **Plugins → ThemisDB Gallery → Aktivieren**

### 2. API-Schlüssel konfigurieren

Gehen Sie zu **Einstellungen → ThemisDB Gallery**

#### Unsplash einrichten (empfohlen)

1. Besuchen Sie https://unsplash.com/developers
2. Registrieren Sie sich oder melden Sie sich an
3. Klicken Sie auf "New Application"
4. Akzeptieren Sie die Nutzungsbedingungen
5. Geben Sie App-Details ein:
   - **Application name**: "Ihre Website Name"
   - **Description**: "WordPress Bildsuche"
6. Kopieren Sie den **Access Key**
7. Fügen Sie ihn in WordPress ein unter **Einstellungen → ThemisDB Gallery → Unsplash Access Key**

#### Pexels einrichten (empfohlen)

1. Besuchen Sie https://www.pexels.com/api/
2. Klicken Sie auf "Get Started"
3. Registrieren Sie sich oder melden Sie sich an
4. Kopieren Sie Ihren **API Key**
5. Fügen Sie ihn in WordPress ein unter **Einstellungen → ThemisDB Gallery → Pexels API Key**

#### Pixabay einrichten (optional)

1. Besuchen Sie https://pixabay.com/api/docs/
2. Registrieren Sie sich oder melden Sie sich an
3. Kopieren Sie Ihren **API Key**
4. Fügen Sie ihn in WordPress ein unter **Einstellungen → ThemisDB Gallery → Pixabay API Key**

#### OpenAI einrichten (optional, für AI-Generierung)

**Hinweis**: Dies ist ein kostenpflichtiger Service!

1. Besuchen Sie https://platform.openai.com/api-keys
2. Melden Sie sich an oder registrieren Sie sich
3. Fügen Sie Zahlungsinformationen hinzu (erforderlich)
4. Erstellen Sie einen neuen API-Schlüssel
5. Kopieren Sie den Schlüssel
6. Fügen Sie ihn in WordPress ein unter **Einstellungen → ThemisDB Gallery → OpenAI API Key**

### 3. Erste Verwendung

1. Öffnen Sie einen Post oder eine Page zum Bearbeiten
2. Suchen Sie in der rechten Seitenleiste nach **"ThemisDB Gallery - Bildsuche"**
3. Geben Sie einen Suchbegriff ein (z.B. "Natur")
4. Klicken Sie auf **Suchen**
5. Wählen Sie ein Bild aus und klicken Sie auf **Bild einfügen**
6. Das Bild wird in Ihren Post eingefügt mit automatischer Quellenangabe

## Detaillierte Konfiguration

### Cache-Einstellungen

**Empfohlene Werte basierend auf Website-Größe:**

| Website-Größe | Besucher/Tag | Cache-Dauer |
|---------------|--------------|-------------|
| Klein | < 1.000 | 3600 (1 Std) |
| Mittel | 1.000 - 10.000 | 7200 (2 Std) |
| Groß | > 10.000 | 14400 (4 Std) |

Ändern Sie dies unter **Einstellungen → ThemisDB Gallery → Cache-Dauer (Sekunden)**

### Weitere Einstellungen

- **Standard-Anbieter**: Wählen Sie Ihren bevorzugten Bildanbieter
- **Bilder pro Seite**: 20 ist ein guter Standard (5-50 möglich)
- **Automatische Quellenangabe**: Empfohlen: Aktiviert (für korrekte Attribution)

## API-Limits und Best Practices

### Kostenlose Limits

| Anbieter | Anfragen/Stunde | Anfragen/Monat |
|----------|-----------------|----------------|
| Unsplash | 50 | ~36.000 |
| Pexels | 200 | ~146.000 |
| Pixabay | 5.000 | ~3.600.000 |

### Best Practices

1. **Mehrere API-Schlüssel konfigurieren**
   - Erhöht verfügbare Anfragen
   - Fallback wenn ein Service nicht verfügbar ist

2. **Cache aktiviert lassen**
   - Reduziert API-Anfragen erheblich
   - Verbessert Ladezeiten

3. **"Alle Anbieter" verwenden**
   - Verteilt Last auf mehrere APIs
   - Mehr Bildauswahl

## Fehlerbehandlung

### "API-Schlüssel nicht konfiguriert"

**Lösung**: Gehen Sie zu **Einstellungen → ThemisDB Gallery** und fügen Sie mindestens einen API-Schlüssel hinzu.

### "Rate Limit erreicht"

**Lösung**: 
- Warten Sie bis zur nächsten Stunde
- Konfigurieren Sie zusätzliche API-Schlüssel
- Erhöhen Sie die Cache-Dauer

### "Fehler beim Download"

**Mögliche Ursachen**:
1. **Upload-Ordner nicht beschreibbar**
   ```bash
   chmod 755 wp-content/uploads
   ```

2. **PHP-Upload-Limits zu niedrig**
   Erhöhen Sie in `php.ini`:
   ```ini
   upload_max_filesize = 64M
   post_max_size = 64M
   ```

3. **Netzwerk-Probleme**
   - Überprüfen Sie allow_url_fopen in PHP
   - Überprüfen Sie Firewall-Einstellungen

### Plugin funktioniert nicht

1. **PHP-Version prüfen**
   ```bash
   php -v  # Muss >= 7.2 sein
   ```

2. **WordPress-Version prüfen**
   - Muss >= 5.0 sein

3. **Fehlerprotokoll aktivieren**
   In `wp-config.php`:
   ```php
   define('WP_DEBUG', true);
   define('WP_DEBUG_LOG', true);
   ```
   Logs finden Sie in: `wp-content/debug.log`

## Sicherheit

### API-Schlüssel schützen

- Niemals API-Schlüssel in Posts/Pages veröffentlichen
- Nicht in versionierten Dateien speichern
- Verwenden Sie WordPress-Optionen (wie das Plugin es tut)

### Berechtigungen

Das Plugin benötigt folgende WordPress-Capabilities:
- `upload_files` - Zum Importieren von Bildern
- `manage_options` - Für Einstellungsseite (nur Admins)

## Performance-Optimierung

### 1. Object Cache verwenden

Installieren Sie einen Object Cache Plugin (Redis, Memcached):
```php
// wp-config.php
define('WP_CACHE', true);
```

### 2. CDN verwenden

Verwenden Sie ein CDN für schnellere Bildauslieferung:
- Cloudflare
- AWS CloudFront
- BunnyCDN

### 3. Lazy Loading aktivieren

WordPress 5.5+ hat eingebautes Lazy Loading für Bilder.

## Deinstallation

### Plugin deaktivieren

1. **Plugins → Installierte Plugins**
2. **ThemisDB Gallery → Deaktivieren**

**Hinweis**: Dies löscht den Cache, aber nicht die importierten Bilder.

### Plugin löschen

1. Plugin deaktivieren (siehe oben)
2. **Plugins → Installierte Plugins**
3. **ThemisDB Gallery → Löschen**

**Hinweis**: Importierte Bilder bleiben in der Mediathek erhalten.

### Vollständige Bereinigung

Wenn Sie alle Plugin-Daten entfernen möchten:

```sql
-- In phpMyAdmin oder via WP-CLI
DELETE FROM wp_options WHERE option_name LIKE 'themisdb_gallery_%';
DELETE FROM wp_postmeta WHERE meta_key LIKE '_themisdb_gallery_%';
```

## Support

Bei Problemen:
1. Lesen Sie die [README.md](README.md) FAQ-Sektion
2. Aktivieren Sie WP_DEBUG und überprüfen Sie die Logs
3. Erstellen Sie ein Issue auf GitHub: https://github.com/makr-code/wordpressPlugins/issues

## Weiterführende Links

- Plugin-Dokumentation: [README.md](README.md)
- Unsplash API Docs: https://unsplash.com/documentation
- Pexels API Docs: https://www.pexels.com/api/documentation/
- Pixabay API Docs: https://pixabay.com/api/docs/
- OpenAI API Docs: https://platform.openai.com/docs/
