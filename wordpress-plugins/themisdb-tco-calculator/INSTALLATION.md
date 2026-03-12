# ThemisDB TCO Calculator - WordPress Installation Guide

Detaillierte Schritt-für-Schritt-Anleitung zur Installation und Konfiguration des ThemisDB TCO Calculator WordPress-Plugins.

## 📋 Voraussetzungen

Bevor Sie beginnen, stellen Sie sicher, dass Sie folgendes haben:

- ✅ WordPress 5.0 oder höher
- ✅ PHP 7.4 oder höher
- ✅ Admin-Zugriff auf Ihre WordPress-Installation
- ✅ FTP/SFTP-Zugang oder Zugriff auf das WordPress-Admin-Panel

## 🚀 Installationsmethoden

### Methode 1: Über WordPress Admin-Panel (Empfohlen)

#### Schritt 1: ZIP-Datei vorbereiten

1. Laden Sie das Plugin aus dem GitHub-Repository herunter
2. Oder erstellen Sie ein ZIP-Archiv:

```bash
cd /path/to/ThemisDB/tools/tco-calculator-wordpress
zip -r themisdb-tco-calculator.zip . -x "*.git*" -x "*.DS_Store"
```

#### Schritt 2: Plugin hochladen

1. Melden Sie sich im WordPress Admin-Panel an
2. Navigieren Sie zu: **Plugins → Installieren**
3. Klicken Sie auf **"Plugin hochladen"**
4. Wählen Sie die ZIP-Datei `themisdb-tco-calculator.zip`
5. Klicken Sie auf **"Jetzt installieren"**
6. Warten Sie, bis der Upload abgeschlossen ist

#### Schritt 3: Plugin aktivieren

1. Nach erfolgreicher Installation erscheint ein Erfolgs-Banner
2. Klicken Sie auf **"Plugin aktivieren"**
3. Sie werden zur Plugin-Liste weitergeleitet

#### Schritt 4: Einstellungen konfigurieren

1. Navigieren Sie zu: **Einstellungen → TCO Calculator**
2. Passen Sie die Standardwerte nach Bedarf an:
   - AI Features aktivieren (Standard: Ja)
   - Standard Anfragen/Tag (Standard: 1.000.000)
   - Standard Datengröße (Standard: 500 GB)
   - Standard Spitzenlast-Faktor (Standard: 3)
   - Standard Verfügbarkeit (Standard: 99.999%)
3. Klicken Sie auf **"Einstellungen speichern"**

### Methode 2: Via FTP/SFTP

#### Schritt 1: Dateien hochladen

1. Verbinden Sie sich per FTP/SFTP mit Ihrem Server
2. Navigieren Sie zum Verzeichnis: `/wp-content/plugins/`
3. Laden Sie den kompletten Ordner `tco-calculator-wordpress` hoch
4. Benennen Sie ihn um in: `themisdb-tco-calculator`

```
/wp-content/plugins/themisdb-tco-calculator/
├── themisdb-tco-calculator.php
├── README.md
├── LICENSE
├── assets/
│   ├── css/
│   │   └── tco-calculator.css
│   └── js/
│       └── tco-calculator.js
└── templates/
    ├── calculator.php
    └── admin-settings.php
```

#### Schritt 2: Dateiberechtigungen setzen

```bash
# Via SSH
cd /path/to/wordpress/wp-content/plugins/themisdb-tco-calculator
find . -type f -exec chmod 644 {} \;
find . -type d -exec chmod 755 {} \;
```

#### Schritt 3: Plugin aktivieren

1. Melden Sie sich im WordPress Admin-Panel an
2. Navigieren Sie zu: **Plugins → Installierte Plugins**
3. Suchen Sie nach "ThemisDB TCO Calculator"
4. Klicken Sie auf **"Aktivieren"**

### Methode 3: Via WP-CLI (Für Entwickler)

```bash
# In WordPress-Installationsverzeichnis
cd /path/to/wordpress

# Plugin-Verzeichnis erstellen
mkdir -p wp-content/plugins/themisdb-tco-calculator

# Dateien kopieren
cp -r /path/to/ThemisDB/tools/tco-calculator-wordpress/* \
      wp-content/plugins/themisdb-tco-calculator/

# Plugin aktivieren
wp plugin activate themisdb-tco-calculator

# Bestätigung
wp plugin list | grep themisdb
```

## 📝 Erste Schritte nach der Installation

### 1. Testseite erstellen

1. **Neue Seite erstellen**
   - WordPress Admin → **Seiten → Erstellen**
   - Titel: "TCO-Rechner" oder "Kostenrechner"

2. **Shortcode einfügen**
   ```
   [themisdb_tco_calculator]
   ```

3. **Seite veröffentlichen**
   - Klicken Sie auf **"Veröffentlichen"**
   - Notieren Sie sich die URL der Seite

4. **Seite testen**
   - Öffnen Sie die Seite in einem neuen Browser-Tab
   - Überprüfen Sie, ob der Rechner korrekt angezeigt wird
   - Testen Sie eine Beispielberechnung

### 2. In Navigation einfügen

1. **Navigationsmenü öffnen**
   - WordPress Admin → **Design → Menüs**

2. **Seite zum Menü hinzufügen**
   - Wählen Sie Ihr Hauptmenü aus
   - Klicken Sie auf "Seiten" → Wählen Sie die TCO-Rechner-Seite
   - Klicken Sie auf **"Zum Menü hinzufügen"**
   - Position nach Bedarf anpassen
   - **"Menü speichern"** klicken

### 3. Einstellungen optimieren

1. **Standard-Werte anpassen**
   - Einstellungen → TCO Calculator
   - Passen Sie Werte an Ihre typischen Kundenanforderungen an
   - Speichern Sie die Änderungen

2. **SEO optimieren** (mit Yoast SEO oder ähnlichem)
   - Seite öffnen
   - Meta-Titel: "TCO-Rechner für ThemisDB | [Ihr Firmenname]"
   - Meta-Beschreibung: "Vergleichen Sie die Gesamtbetriebskosten von ThemisDB und Cloud-Hyperscalern. Kostenlose TCO-Analyse über 3 Jahre."
   - Focus-Keyword: "TCO-Rechner" oder "Datenbank-Kostenrechner"

## 🔧 Erweiterte Konfiguration

### Shortcode-Varianten

**Basis-Version:**
```
[themisdb_tco_calculator]
```

**Ohne Einführungstext:**
```
[themisdb_tco_calculator show_intro="no"]
```

**Mit benutzerdefiniertem Titel:**
```
[themisdb_tco_calculator title="Kostenvergleich"]
```

**Kombinierte Optionen:**
```
[themisdb_tco_calculator show_intro="no" title="TCO Analyse"]
```

### CSS-Anpassungen

Fügen Sie benutzerdefiniertes CSS über **Design → Customizer → Zusätzliches CSS** hinzu:

```css
/* Buttons anpassen */
.themisdb-tco-calculator-wrapper .btn-primary {
    background: #your-brand-color !important;
}

/* Container-Breite anpassen */
.themisdb-tco-calculator-wrapper {
    max-width: 1200px;
    margin: 0 auto;
}

/* Formular-Layout anpassen */
.themisdb-tco-calculator-wrapper .form-grid {
    grid-template-columns: repeat(2, 1fr) !important;
}
```

### PHP-Hooks verwenden

In Ihrer `functions.php` oder einem Custom-Plugin:

```php
// Standardwerte überschreiben
add_filter('themisdb_tco_default_values', function($defaults) {
    $defaults['requestsPerDay'] = 2000000;
    $defaults['dataSize'] = 1000;
    return $defaults;
});

// Chart.js Version überschreiben
add_filter('themisdb_tco_chartjs_version', function($version) {
    return '4.5.0'; // Neuere Version verwenden
});
```

## 🎨 Theme-Integration

### Mit Page Buildern

#### Elementor
1. Seite mit Elementor bearbeiten
2. "Shortcode"-Widget hinzufügen
3. Shortcode einfügen: `[themisdb_tco_calculator]`
4. Aktualisieren und veröffentlichen

#### Gutenberg (Block Editor)
1. Seite bearbeiten
2. Block hinzufügen: "Shortcode"
3. Shortcode einfügen: `[themisdb_tco_calculator]`
4. Veröffentlichen

#### Classic Editor
1. Seite im Text-Modus öffnen
2. Shortcode direkt einfügen
3. Veröffentlichen

### Vollbreiten-Layout

Wenn Ihr Theme Container/Wrapper um den Content legt:

```php
// In functions.php
add_filter('themisdb_tco_fullwidth_pages', function($pages) {
    $pages[] = 123; // Page ID der TCO-Rechner-Seite
    return $pages;
});
```

## 🐛 Problemlösungen

### Problem: Plugin erscheint nicht in der Plugin-Liste

**Lösung:**
```bash
# Verzeichnisstruktur prüfen
ls -la /path/to/wordpress/wp-content/plugins/themisdb-tco-calculator/

# Haupt-Plugin-Datei muss vorhanden sein
# themisdb-tco-calculator.php
```

### Problem: "Die Seite hat einen kritischen Fehler"

**Lösung:**
1. **Debug-Modus aktivieren** (in `wp-config.php`):
   ```php
   define('WP_DEBUG', true);
   define('WP_DEBUG_LOG', true);
   ```

2. **Log-Datei prüfen**:
   ```bash
   tail -f /path/to/wordpress/wp-content/debug.log
   ```

3. **PHP-Version prüfen**:
   ```bash
   php -v  # Muss >= 7.4 sein
   ```

### Problem: Styles werden nicht geladen

**Lösung:**
1. **Cache leeren**:
   - WordPress-Cache-Plugin
   - Browser-Cache (Strg+Shift+R / Cmd+Shift+R)
   - Server-Cache (falls vorhanden)

2. **Theme-Check**:
   ```php
   // In header.php sollte vorhanden sein:
   <?php wp_head(); ?>
   
   // In footer.php sollte vorhanden sein:
   <?php wp_footer(); ?>
   ```

3. **Dateiberechtigungen prüfen**:
   ```bash
   chmod 644 /path/to/plugins/themisdb-tco-calculator/assets/css/tco-calculator.css
   ```

### Problem: JavaScript funktioniert nicht

**Lösung:**
1. **Browser-Konsole öffnen** (F12)
2. **Fehler identifizieren**
3. **Häufige Ursachen**:
   - jQuery nicht geladen → Plugin-Konflikt
   - Chart.js nicht geladen → CDN blockiert
   - JavaScript-Fehler → Andere Plugins deaktivieren

### Problem: Chart.js wird nicht geladen

**Lösung:**
1. **CDN-Verfügbarkeit prüfen**:
   ```bash
   curl -I https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.js
   ```

2. **Alternative: Lokale Chart.js Kopie**:
   ```php
   // In functions.php
   add_filter('themisdb_tco_use_local_chartjs', '__return_true');
   ```

## ✅ Installations-Checkliste

Nach der Installation sollten Sie folgende Punkte überprüfen:

- [ ] Plugin ist aktiviert
- [ ] Testseite mit Shortcode erstellt
- [ ] Rechner wird korrekt angezeigt
- [ ] Formular-Eingaben funktionieren
- [ ] Berechnung liefert Ergebnisse
- [ ] Chart wird angezeigt
- [ ] Export-Funktionen (PDF/CSV) funktionieren
- [ ] Responsive Design funktioniert (Mobilgeräte testen)
- [ ] Seite in Navigation eingefügt
- [ ] SEO-Einstellungen konfiguriert
- [ ] Cache-Plugin konfiguriert (falls verwendet)

## 📊 Performance-Optimierung

### Caching konfigurieren

**Mit WP Rocket:**
```
1. WP Rocket → Einstellungen
2. JavaScript → Exclude-Liste:
   /wp-content/plugins/themisdb-tco-calculator/assets/js/
3. CSS → Exclude-Liste:
   /wp-content/plugins/themisdb-tco-calculator/assets/css/
```

**Mit W3 Total Cache:**
```
1. Performance → Minify → JS
2. Exclude-Liste: themisdb-tco-calculator
3. Performance → Minify → CSS
4. Exclude-Liste: themisdb-tco-calculator
```

### CDN-Integration

Wenn Sie ein CDN verwenden:

```php
// In functions.php
add_filter('themisdb_tco_assets_url', function($url) {
    return str_replace(
        site_url(),
        'https://your-cdn.com',
        $url
    );
});
```

## 🔒 Sicherheits-Checkliste

- [ ] WordPress ist aktuell
- [ ] PHP ist aktuell (>= 7.4)
- [ ] SSL-Zertifikat installiert (HTTPS)
- [ ] Sichere Passwörter für Admin-Konten
- [ ] WordPress-Login geschützt (z.B. mit Limit Login Attempts)
- [ ] Regelmäßige Backups konfiguriert
- [ ] Security-Plugin installiert (z.B. Wordfence)

## 📞 Support

Bei Problemen während der Installation:

1. **Dokumentation prüfen**: README.md und diese Datei
2. **GitHub Issues**: [ThemisDB Issues](https://github.com/makr-code/wordpressPlugins/issues)
3. **Community**: WordPress.org Support-Forum

## 🔄 Updates

Das Plugin wird regelmäßig aktualisiert. So bleiben Sie auf dem neuesten Stand:

### Manuelle Updates

1. **Neue Version herunterladen**
2. **Plugin deaktivieren** (Einstellungen bleiben erhalten)
3. **Alte Dateien löschen**
4. **Neue Dateien hochladen**
5. **Plugin wieder aktivieren**

### Automatische Updates (zukünftig)

```php
// In functions.php - GitHub-Updates aktivieren
add_filter('auto_update_plugin', function($update, $item) {
    if ($item->slug === 'themisdb-tco-calculator') {
        return true;
    }
    return $update;
}, 10, 2);
```

---

**Viel Erfolg mit Ihrer Installation!** 🚀

Bei Fragen oder Problemen erstellen Sie bitte ein Issue auf GitHub.
