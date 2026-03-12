# ThemisDB TCO Calculator - WordPress Plugin

Ein WordPress-Plugin für den interaktiven Total Cost of Ownership (TCO) Rechner für ThemisDB. Vergleichen Sie die Gesamtbetriebskosten verschiedener Datenbanklösungen direkt auf Ihrer WordPress-Website.

## 📋 Übersicht

Dieses Plugin ist eine WordPress-Adaptation des [JavaScript TCO-Rechners](../tco-calculator/) und bietet alle Features des Original-Rechners in einer nahtlosen WordPress-Integration:

- **Shortcode-basierte Einbindung**: `[themisdb_tco_calculator]`
- **Admin-Einstellungsseite**: Anpassung von Standardwerten
- **Vollständige Funktionalität**: Alle Features des Original-Rechners
- **WordPress-optimiert**: Nutzt WordPress-Best-Practices

## ✨ Features

### Umfassende TCO-Analyse
- 💰 **Infrastrukturkosten**: Server, Storage, Netzwerk, Backups
- 👥 **Personalkosten**: DBAs, Entwickler mit Overhead-Berechnung
- 📜 **Lizenzkosten**: ThemisDB Editions und Enterprise Support
- 🔧 **Betriebskosten**: Schulungen, Wartung, Support
- 🤖 **AI/LLM-Kosten**: Native Integration vs. externe APIs

### WordPress-Integration
- 📝 **Shortcode**: Einfache Einbindung via `[themisdb_tco_calculator]`
- ⚙️ **Admin-Panel**: Einstellungsseite unter Einstellungen → TCO Calculator
- 🔄 **GitHub Auto-Updates**: Automatische Plugin-Updates von GitHub
- 🔗 **Plugin-Action-Links**: Direkter Zugriff auf Einstellungen von der Plugins-Seite
- 🧹 **Saubere Deinstallation**: Automatische Bereinigung bei Plugin-Löschung
- 🎨 **Theme-kompatibel**: Funktioniert mit jedem WordPress-Theme
- 📱 **Responsive**: Optimiert für alle Bildschirmgrößen

### Interaktive Features
- ⚡ **Echtzeit-Berechnung**: Werte aktualisieren sich automatisch beim Bewegen der Schieberegler (500ms Debouncing)
- 📊 **Visualisierungen**: Dynamische Charts mit Chart.js
- 📈 **Jahresvergleich**: Detaillierte Aufschlüsselung über 3 Jahre
- 💡 **Intelligente Insights**: Automatische Analyse und Empfehlungen
- 📥 **Export-Funktionen**: PDF, CSV, Drucken

## 🚀 Installation

### Methode 1: Manuelle Installation

1. **Plugin herunterladen**
   ```bash
   cd /path/to/wordpress/wp-content/plugins/
   git clone https://github.com/makr-code/wordpressPlugins.git
   cp -r ThemisDB/tools/tco-calculator-wordpress ./themisdb-tco-calculator
   ```

2. **Plugin aktivieren**
   - Gehen Sie zu WordPress Admin → Plugins
   - Suchen Sie nach "ThemisDB TCO Calculator"
   - Klicken Sie auf "Aktivieren"

3. **Konfiguration (Optional)**
   - Gehen Sie zu Einstellungen → TCO Calculator
   - Passen Sie die Standardwerte nach Bedarf an

4. **Shortcode verwenden**
   - Erstellen Sie eine neue Seite oder Beitrag
   - Fügen Sie `[themisdb_tco_calculator]` ein
   - Veröffentlichen

### Methode 2: ZIP-Upload

1. **ZIP-Archiv erstellen**
   ```bash
   cd tools/tco-calculator-wordpress
   zip -r themisdb-tco-calculator.zip .
   ```

2. **In WordPress hochladen**
   - WordPress Admin → Plugins → Installieren
   - "Plugin hochladen" → ZIP-Datei auswählen
   - "Jetzt installieren" → "Plugin aktivieren"

### Methode 3: FTP/SFTP Upload

1. **Dateien hochladen**
   - Laden Sie den Ordner `tco-calculator-wordpress` hoch nach:
     `/wp-content/plugins/themisdb-tco-calculator/`

2. **Plugin aktivieren**
   - WordPress Admin → Plugins → Plugin aktivieren

## 📖 Verwendung

### Grundlegende Einbindung

Fügen Sie einfach den Shortcode in eine beliebige Seite oder einen Beitrag ein:

```
[themisdb_tco_calculator]
```

### Shortcode-Parameter

Der Shortcode unterstützt folgende optionale Parameter:

```
[themisdb_tco_calculator show_intro="no"]
```

**Verfügbare Parameter:**
- `show_intro` - Zeigt/verbirgt die Einführungssektion (Standard: "yes")
  - `"yes"` - Zeigt Einführung an
  - `"no"` - Verbirgt Einführung
- `title` - Angepasster Titel (Standard: "ThemisDB TCO-Rechner")

**Beispiele:**

```
// Nur der Rechner ohne Einführung
[themisdb_tco_calculator show_intro="no"]

// Mit eigenem Titel
[themisdb_tco_calculator title="Kostenrechner"]

// Kombiniert
[themisdb_tco_calculator show_intro="no" title="TCO Analyse"]
```

### Admin-Einstellungen

1. **Einstellungsseite öffnen**
   - WordPress Admin → Einstellungen → TCO Calculator
   - Oder direkt über den "Einstellungen"-Link auf der Plugins-Seite

2. **Verfügbare Einstellungen:**
   - AI Features aktivieren
   - Standard Anfragen/Tag
   - Standard Datengröße (GB)
   - Standard Spitzenlast-Faktor
   - Standard Verfügbarkeit (%)

3. **Einstellungen speichern**
   - Änderungen werden automatisch auf alle Instanzen des Rechners angewendet

4. **Update-Status prüfen**
   - Die Einstellungsseite zeigt den aktuellen Update-Status an
   - Neue Versionen werden automatisch erkannt

### GitHub Auto-Updates

Das Plugin unterstützt automatische Updates direkt von GitHub:

1. **Automatische Erkennung**
   - Das Plugin prüft regelmäßig auf neue Versionen
   - Updates werden unter Dashboard → Aktualisierungen angezeigt

2. **Update installieren**
   - Klicken Sie auf "Jetzt aktualisieren" wie bei jedem anderen Plugin
   - Das Plugin wird automatisch von GitHub heruntergeladen

3. **Update-Benachrichtigungen**
   - Sie werden benachrichtigt, wenn eine neue Version verfügbar ist
   - Der Update-Status wird auch auf der Plugin-Einstellungsseite angezeigt

4. **Release-Notes**
   - Changelog wird direkt von GitHub geladen
   - Klicken Sie auf "Details anzeigen" für vollständige Release-Notes

5. **Für Entwickler: Releases erstellen**
   - Für optimale Update-Funktionalität erstellen Sie GitHub Releases mit pre-packaged Plugin-ZIP-Dateien
   - Das Plugin sucht automatisch nach ZIP-Assets in Releases (z.B. `themisdb-tco-calculator-v1.0.0.zip`)
   - Falls keine Assets vorhanden sind, wird das Repository-Archiv verwendet

## 🎨 Anpassung

### CSS-Anpassungen

Das Plugin verwendet eigene CSS-Klassen, die Sie in Ihrem Theme überschreiben können:

```css
/* In Ihrem Theme's style.css oder Custom CSS */

.themisdb-tco-calculator-wrapper {
    /* Haupt-Container anpassen */
}

.themisdb-tco-calculator-wrapper .btn-primary {
    /* Primäre Buttons anpassen */
    background: #your-color;
}
```

### PHP-Hooks

Das Plugin bietet WordPress-Hooks für erweiterte Anpassungen:

```php
// Standardwerte überschreiben
add_filter('themisdb_tco_default_values', function($defaults) {
    $defaults['requestsPerDay'] = 2000000;
    return $defaults;
});

// Shortcode-Ausgabe anpassen
add_filter('themisdb_tco_calculator_output', function($output) {
    // Modifiziere $output
    return $output;
});
```

## 📁 Dateistruktur

```
themisdb-tco-calculator/
├── themisdb-tco-calculator.php    # Haupt-Plugin-Datei
├── README.md                      # Diese Datei
├── LICENSE                        # MIT Lizenz
├── assets/
│   ├── css/
│   │   └── tco-calculator.css    # Plugin-Styles
│   └── js/
│       └── tco-calculator.js     # Plugin-JavaScript
└── templates/
    ├── calculator.php             # HTML-Template für Rechner
    └── admin-settings.php         # Admin-Einstellungsseite
```

## 🔧 Technische Details

### Systemanforderungen

- **WordPress**: 5.0 oder höher
- **PHP**: 7.4 oder höher
- **Browser**: Chrome 90+, Firefox 88+, Safari 14+, Edge 90+

### Verwendete Technologien

- **Backend**: PHP (WordPress-Plugin-API)
- **Frontend**: HTML5, CSS3, JavaScript (ES6+)
- **Charts**: Chart.js 4.4.0 (via CDN)
- **Architektur**: Objektorientiertes PHP und JavaScript

### WordPress-Hooks verwendet

**Actions:**
- `init` - Plugin-Initialisierung
- `wp_enqueue_scripts` - Assets laden
- `admin_menu` - Admin-Menü hinzufügen
- `admin_init` - Einstellungen registrieren

**Filters:**
- Keine Standard-Filters (Custom Filters verfügbar)

### JavaScript-API

Das Plugin exponiert eine JavaScript-API für erweiterte Nutzung:

```javascript
// Zugriff auf Calculator-Instanz
if (window.tcoCalculator) {
    // Berechnung manuell auslösen
    window.tcoCalculator.calculate();
    
    // Ergebnisse abrufen
    console.log(window.tcoCalculator.results);
}
```

## 🔒 Sicherheit

Das Plugin implementiert WordPress-Sicherheits-Best-Practices:

- ✅ **Nonce-Verification**: Alle AJAX-Requests sind gesichert
- ✅ **Capability Checks**: Admin-Funktionen nur für berechtigte Benutzer
- ✅ **Data Sanitization**: Alle Eingaben werden bereinigt
- ✅ **Output Escaping**: Alle Ausgaben werden escaped
- ✅ **No Direct Access**: Direkter Zugriff auf PHP-Dateien verhindert

## 🐛 Fehlerbehebung

### Plugin wird nicht angezeigt

1. **Cache leeren**: WordPress-Cache und Browser-Cache leeren
2. **Shortcode prüfen**: Sicherstellen, dass `[themisdb_tco_calculator]` korrekt ist
3. **Plugin-Aktivierung**: Plugin muss aktiviert sein

### Styles werden nicht geladen

1. **Theme-Kompatibilität**: Prüfen, ob Theme wp_head() und wp_footer() aufruft
2. **Conflict-Check**: Andere Plugins temporär deaktivieren
3. **Browser-Konsole**: JavaScript-Fehler prüfen (F12)

### Chart wird nicht angezeigt

1. **Chart.js laden**: Prüfen, ob Chart.js geladen wird (Netzwerk-Tab in DevTools)
2. **JavaScript-Fehler**: Browser-Konsole auf Fehler prüfen
3. **Content Security Policy**: CSP-Einstellungen prüfen (CDN muss erlaubt sein)

### Berechnung funktioniert nicht

1. **JavaScript aktiviert**: Sicherstellen, dass JavaScript im Browser aktiv ist
2. **Eingabewerte**: Alle Pflichtfelder müssen ausgefüllt sein
3. **Browser-Konsole**: Fehlermeldungen prüfen

## 📊 Vergleich: Original vs. WordPress-Plugin

| Feature | Original (HTML/JS) | WordPress-Plugin |
|---------|-------------------|------------------|
| Installation | Manuell | WordPress-Plugin-System |
| Konfiguration | Direkt im Code | Admin-Oberfläche |
| Einbindung | iframe/direkter Link | Shortcode |
| Updates | Manuell | WordPress-Updates |
| Theme-Integration | Extern | Nahtlos integriert |
| Mehrsprachigkeit | Manuell | WordPress i18n |

## 🌐 Empfohlene WordPress-Plugins

Für beste Ergebnisse kombinieren Sie den TCO-Calculator mit:

### SEO-Plugins
- **Yoast SEO** oder **Rank Math**: Für bessere Suchmaschinen-Optimierung
- Tipp: Verwenden Sie aussagekräftige Meta-Beschreibungen für Seiten mit dem Rechner

### Performance-Plugins
- **WP Rocket** oder **W3 Total Cache**: Für schnellere Ladezeiten
- Tipp: Excludieren Sie `/wp-content/plugins/themisdb-tco-calculator/` vom JS-Minifying

### Page Builder
- **Elementor**, **Beaver Builder**, oder **Divi**: 
  - Fügen Sie den Shortcode in ein Shortcode-Widget ein
  - Funktioniert out-of-the-box mit allen gängigen Page Buildern

### Analytics
- **MonsterInsights** (Google Analytics): Tracken Sie Rechner-Nutzung
- Tipp: Setzen Sie Events für "Calculate" Button-Klicks

## 🔄 Migration vom Original-Rechner

Wenn Sie bereits den Original-HTML-Rechner verwenden:

### Schritt 1: Plugin installieren
```bash
# WordPress-Plugin installieren (siehe Installation oben)
```

### Schritt 2: Seite erstellen
```
1. WordPress Admin → Seiten → Erstellen
2. Titel: "TCO-Rechner" (oder Ihren gewünschten Titel)
3. Inhalt: [themisdb_tco_calculator]
4. Veröffentlichen
```

### Schritt 3: Alte Seite ersetzen
```
- Entfernen Sie iframe/Links zum alten Rechner
- Verlinken Sie auf die neue WordPress-Seite
```

### Vorteile der Migration:
- ✅ Nahtlose Theme-Integration
- ✅ WordPress-SEO automatisch
- ✅ Einfachere Wartung
- ✅ Keine externen Abhängigkeiten

## 📝 Changelog

### Version 1.0.0 (Januar 2026)
- ✅ Erste Veröffentlichung
- ✅ Shortcode-Integration `[themisdb_tco_calculator]`
- ✅ Admin-Einstellungsseite mit Standardwerten
- ✅ **GitHub Auto-Update-Unterstützung**
- ✅ **Plugin-Action-Links** (direkter Zugriff auf Einstellungen)
- ✅ **Uninstall-Hook** für saubere Deinstallation
- ✅ **JavaScript-Button-Fix**: Event-Listener funktionieren jetzt korrekt
- ✅ Vollständige Feature-Parität mit Original-Rechner
- ✅ WordPress-optimierte Code-Struktur
- ✅ Chart.js Integration
- ✅ Export-Funktionen (PDF, CSV, Druck)
- ✅ Responsive Design
- ✅ Deutsche Lokalisierung
- ✅ WordPress.org readme.txt Format
- ✅ Multisite-Unterstützung

## 🤝 Beitragen

Verbesserungsvorschläge und Pull Requests sind willkommen!

### Entwicklungs-Setup

```bash
# Repository klonen
git clone https://github.com/makr-code/wordpressPlugins.git
cd ThemisDB/tools/tco-calculator-wordpress

# In lokale WordPress-Installation kopieren
cp -r . /path/to/wordpress/wp-content/plugins/themisdb-tco-calculator/

# Plugin aktivieren und testen
```

### Contribution Guidelines

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

## 📄 Lizenz

Dieses Plugin ist Teil von ThemisDB und unter der MIT-Lizenz lizenziert.

```
MIT License

Copyright (c) 2026 ThemisDB Team

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

## 📞 Support & Kontakt

- **GitHub Repository**: [makr-code/wordpressPlugins](https://github.com/makr-code/wordpressPlugins)
- **Issues**: [GitHub Issues](https://github.com/makr-code/wordpressPlugins/issues)
- **Dokumentation**: [ThemisDB Docs](https://github.com/makr-code/wordpressPlugins)

## ⚠️ Disclaimer

Dieser Rechner dient als Orientierungshilfe. Tatsächliche Kosten können je nach spezifischen Anforderungen, Verträgen und Nutzungsmustern variieren. Für genaue Kostenschätzungen kontaktieren Sie bitte die jeweiligen Anbieter.

---

**Version**: 1.0.0  
**Letzte Aktualisierung**: Januar 2026  
**Getestet bis**: WordPress 6.4  
**Autor**: ThemisDB Team
