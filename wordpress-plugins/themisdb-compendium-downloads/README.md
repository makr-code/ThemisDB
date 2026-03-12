# ThemisDB Compendium Downloads

Ein WordPress-Plugin zum Anbieten von ThemisDB Kompendium PDF-Versionen als Downloads auf der Website, analog zu Docker und GitHub Releases.

## Beschreibung

Das ThemisDB Compendium Downloads Plugin ermöglicht es, die neuesten Kompendium-PDFs (Print- und Professional-Versionen) von GitHub Releases automatisch auf Ihrer WordPress-Website anzuzeigen und zum Download anzubieten.

## Features

- ✅ Automatisches Abrufen der neuesten Releases von GitHub
- ✅ Anzeige von Print- und Professional-PDF-Versionen
- ✅ Caching-Mechanismus für optimale Performance
- ✅ Flexible Anzeige per Shortcode
- ✅ Widget für Sidebar-Integration
- ✅ Download-Tracking und Statistiken
- ✅ Responsive Design - funktioniert auf allen Geräten
- ✅ Dark Mode Unterstützung
- ✅ Mehrere Stil-Optionen (Modern, Klassisch, Minimal)
- ✅ Verschiedene Layout-Optionen (Cards, List, Compact)
- ✅ Anpassbare Einstellungen im Admin-Bereich
- ✅ Mehrsprachig (German/English)

## Installation

### Methode 1: Manueller Upload

1. Laden Sie das Plugin-Verzeichnis `themisdb-compendium-downloads` herunter
2. Laden Sie es in Ihr WordPress-Verzeichnis hoch: `/wp-content/plugins/`
3. Aktivieren Sie das Plugin im WordPress Admin unter "Plugins"
4. Konfigurieren Sie die Einstellungen unter "Einstellungen → Kompendium Downloads"

### Methode 2: ZIP-Upload

1. Erstellen Sie eine ZIP-Datei des Plugin-Verzeichnisses:
   ```bash
   cd wordpress-plugin
   ./themisdb-compendium-downloads/package.sh
   ```
2. Gehen Sie zu WordPress Admin → Plugins → Installieren
3. Klicken Sie auf "Plugin hochladen"
4. Wählen Sie die ZIP-Datei aus und klicken Sie auf "Jetzt installieren"
5. Aktivieren Sie das Plugin nach der Installation

## Verwendung

### Shortcode

Fügen Sie den folgenden Shortcode in Ihre Seiten oder Beiträge ein:

```
[themisdb_compendium_downloads]
```

#### Shortcode-Optionen

```
[themisdb_compendium_downloads 
    style="modern" 
    show_version="yes" 
    show_date="yes" 
    show_size="yes" 
    layout="cards"]
```

**Verfügbare Parameter:**

- `style` - Stil: `modern`, `classic`, `minimal` (Standard: modern)
- `show_version` - Version anzeigen: `yes`, `no` (Standard: yes)
- `show_date` - Datum anzeigen: `yes`, `no` (Standard: yes)
- `show_size` - Dateigröße anzeigen: `yes`, `no` (Standard: yes)
- `layout` - Layout: `cards`, `list`, `compact` (Standard: cards)

#### Beispiele

**Standard-Anzeige:**
```
[themisdb_compendium_downloads]
```

**Minimale Anzeige ohne Version:**
```
[themisdb_compendium_downloads style="minimal" show_version="no"]
```

**Kompaktes Layout für Sidebar:**
```
[themisdb_compendium_downloads layout="compact" show_date="no"]
```

**Klassischer Stil mit Liste:**
```
[themisdb_compendium_downloads style="classic" layout="list"]
```

### Widget

1. Gehen Sie zu "Design → Widgets"
2. Ziehen Sie das Widget "ThemisDB Kompendium Downloads" in Ihre gewünschte Widget-Area
3. Konfigurieren Sie die Widget-Einstellungen
4. Speichern Sie die Änderungen

### Alternative Shortcode-Namen

Das Plugin unterstützt auch folgende Alias-Shortcodes:

- `[themisdb_compendium]`

## Konfiguration

### Admin-Einstellungen

Gehen Sie zu **Einstellungen → Kompendium Downloads** um das Plugin zu konfigurieren:

#### Haupteinstellungen

- **GitHub Repository**: Das Repository, von dem Releases abgerufen werden (Standard: `makr-code/wordpressPlugins`)
- **Dateigrößen anzeigen**: Aktivieren/Deaktivieren der Anzeige von Dateigrößen
- **Cache-Dauer**: Wie lange Release-Daten zwischengespeichert werden (Standard: 3600 Sekunden = 1 Stunde)
- **Button-Stil**: Standard-Stil für Download-Buttons (Modern, Klassisch, Minimal)

#### Cache-Verwaltung

- **Cache leeren**: Löscht den Cache, um die neuesten Release-Daten von GitHub abzurufen

#### Download-Statistiken

Das Plugin erfasst Download-Klicks und zeigt eine Statistik-Übersicht im Admin-Bereich an.

## Design-Stile

### Modern (Standard)

Modernes Design mit Farbverläufen, Schatten und Hover-Effekten.

### Klassisch

Klassisches Design mit geraderen Linien und dezenten Farben.

### Minimal

Minimalistisches Design mit reduzierter Optik und ohne Schatten.

## Layout-Optionen

### Cards (Standard)

Zeigt Downloads als Karten in einem Grid-Layout an.

### List

Zeigt Downloads als vertikale Liste an.

### Compact

Kompaktes Layout für schmale Bereiche wie Sidebars.

## Technische Details

### Anforderungen

- **WordPress**: 5.0+
- **PHP**: 7.2+
- **Browser**: Alle modernen Browser (Chrome, Firefox, Safari, Edge)

### GitHub API

Das Plugin nutzt die GitHub API, um Release-Informationen abzurufen:

- API Endpoint: `https://api.github.com/repos/{owner}/{repo}/releases/latest`
- Rate Limit: 60 Anfragen pro Stunde (ohne Authentication)
- Caching verhindert häufige API-Aufrufe

### Performance

- **Caching**: Release-Daten werden standardmäßig 1 Stunde lang zwischengespeichert
- **Lazy Loading**: Assets werden nur geladen, wenn der Shortcode verwendet wird
- **Optimierte Assets**: Minifizierte CSS/JS-Dateien (optional)

### Sicherheit

- ✅ Eingabevalidierung und Sanitization
- ✅ XSS-Schutz
- ✅ CSRF-Schutz mit Nonces
- ✅ Sichere API-Aufrufe
- ✅ WordPress Security Best Practices

## Anpassung

### CSS-Anpassung

Sie können das Aussehen über Custom CSS in Ihrem Theme anpassen:

```css
/* Eigene Farben */
.themisdb-compendium-downloads {
    background: #f5f5f5;
}

.themisdb-download-button {
    background: #ff6b6b;
}

.themisdb-download-button:hover {
    background: #ee5a6f;
}

/* Eigene Schriftarten */
.themisdb-compendium-title {
    font-family: 'Your Custom Font', sans-serif;
}
```

### PHP-Hooks

Für Entwickler stehen verschiedene Hooks zur Verfügung:

```php
// Filter für Release-Daten
add_filter('themisdb_compendium_release_data', function($data) {
    // Modify release data
    return $data;
});

// Action nach Download-Tracking
add_action('themisdb_compendium_download_tracked', function($asset_name, $download_count) {
    // Custom tracking logic
}, 10, 2);
```

## Fehlerbehebung

### Downloads werden nicht angezeigt

1. Überprüfen Sie, dass das Plugin aktiviert ist
2. Prüfen Sie die Admin-Einstellungen unter "Einstellungen → Kompendium Downloads"
3. Leeren Sie den Cache im Admin-Bereich
4. Überprüfen Sie, dass das GitHub-Repository korrekt ist
5. Prüfen Sie die Browser-Konsole auf Fehler

### API-Rate-Limit erreicht

GitHub erlaubt 60 API-Anfragen pro Stunde ohne Authentication. Wenn Sie das Limit erreichen:

1. Erhöhen Sie die Cache-Dauer in den Einstellungen
2. Erwägen Sie die Verwendung eines GitHub Personal Access Tokens (zukünftiges Feature)

### Cache-Probleme

Wenn veraltete Daten angezeigt werden:

1. Gehen Sie zu "Einstellungen → Kompendium Downloads"
2. Klicken Sie auf "Cache leeren"
3. Aktualisieren Sie die Seite

## Kompatibilität

- **Page Builder**: Kompatibel mit Elementor, WPBakery, Divi, Beaver Builder
- **Caching Plugins**: Kompatibel mit WP Rocket, W3 Total Cache, WP Super Cache
- **Themes**: Funktioniert mit allen WordPress-Themes
- **Multisite**: Ja, unterstützt WordPress Multisite

## Lizenz

MIT License - siehe [LICENSE](LICENSE) Datei

## Credits

- **ThemisDB Team**: https://github.com/makr-code/wordpressPlugins
- **GitHub API**: https://docs.github.com/en/rest

## Changelog

Siehe [CHANGELOG.md](CHANGELOG.md) für Versionshistorie.

## Support

- **Dokumentation**: Siehe dieses README
- **GitHub Issues**: https://github.com/makr-code/wordpressPlugins/issues
- **WordPress Support Forum**: (wenn auf wordpress.org verfügbar)

## Weiterentwicklung

Geplante Features:

- [ ] GitHub Personal Access Token Support
- [ ] Automatische Updates bei neuen Releases
- [ ] E-Mail-Benachrichtigungen bei neuen Versionen
- [ ] Mehrsprachige Übersetzungen (DE/EN/FR/ES)
- [ ] Integration mit anderen Dokumentationsquellen
- [ ] Erweiterte Statistiken und Analytics
- [ ] Export von Download-Statistiken
- [ ] Gutenberg-Block
- [ ] Elementor-Widget

## Beiträge

Beiträge sind willkommen! Siehe [CONTRIBUTING.md](../../CONTRIBUTING.md) für Guidelines.

---

**Entwickelt mit ❤️ vom ThemisDB Team**
