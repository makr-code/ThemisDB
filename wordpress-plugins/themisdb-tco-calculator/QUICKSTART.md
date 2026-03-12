# ThemisDB TCO Calculator - WordPress Plugin Quick Start

## 🎯 Schnellübersicht

Das WordPress-Plugin für den ThemisDB TCO-Rechner ermöglicht die einfache Integration des interaktiven Kostenrechners in jede WordPress-Website via Shortcode.

### ✨ Hauptmerkmale

- **Einfache Installation**: Als WordPress-Plugin direkt über Admin-Panel
- **Shortcode-Integration**: `[themisdb_tco_calculator]`
- **Admin-Einstellungen**: Konfiguration über WordPress-Admin
- **Vollständige Funktionalität**: Alle Features der Original-Version
- **Theme-kompatibel**: Funktioniert mit jedem WordPress-Theme

## ⚡ Installation in 3 Schritten

### 1. Plugin hochladen
```
WordPress Admin → Plugins → Installieren → Plugin hochladen
→ ZIP-Datei auswählen → Jetzt installieren
```

### 2. Plugin aktivieren
```
Nach Installation: "Plugin aktivieren" klicken
```

### 3. Shortcode verwenden
```
Neue Seite erstellen → Shortcode einfügen:
[themisdb_tco_calculator]
```

## 📝 Grundlegende Verwendung

### Shortcode-Syntax

**Basis:**
```
[themisdb_tco_calculator]
```

**Ohne Einführung:**
```
[themisdb_tco_calculator show_intro="no"]
```

**Mit eigenem Titel:**
```
[themisdb_tco_calculator title="Kostenrechner"]
```

## ⚙️ Einstellungen

**Admin-Panel öffnen:**
```
WordPress Admin → Einstellungen → TCO Calculator
```

**Verfügbare Optionen:**
- AI Features aktivieren
- Standard Anfragen/Tag
- Standard Datengröße (GB)
- Standard Spitzenlast-Faktor
- Standard Verfügbarkeit (%)

## 🎨 Anpassung

### CSS anpassen

```css
/* In Design → Customizer → Zusätzliches CSS */
.themisdb-tco-calculator-wrapper .btn-primary {
    background: #your-color !important;
}
```

### PHP-Hooks

```php
/* In functions.php */
add_filter('themisdb_tco_default_values', function($defaults) {
    $defaults['requestsPerDay'] = 2000000;
    return $defaults;
});
```

## 🔧 Systemanforderungen

- WordPress 5.0+
- PHP 7.4+
- Modernes Browser (Chrome 90+, Firefox 88+, Safari 14+)

## 📚 Dokumentation

- **README.md** - Vollständige Dokumentation
- **INSTALLATION.md** - Detaillierte Installationsanleitung
- **COMPARISON.md** - Vergleich Original vs. WordPress

## 🆚 Original vs. WordPress

| Feature | Original | WordPress |
|---------|----------|-----------|
| Deployment | Standalone | Plugin |
| Konfiguration | Code | Admin-UI |
| Integration | iframe | Shortcode |
| Theme-Design | Separat | Automatisch |

**Empfehlung:**
- Nutzen Sie das WordPress-Plugin wenn Sie bereits WordPress verwenden
- Nutzen Sie die Original-Version für standalone-Deployment

## 💡 Tipps

### Best Practices

1. **Performance**: Nutzen Sie ein Caching-Plugin (WP Rocket, W3 Total Cache)
2. **SEO**: Optimieren Sie die Seite mit Yoast SEO oder Rank Math
3. **Analytics**: Tracken Sie Nutzung mit Google Analytics
4. **Design**: Passen Sie Farben an Ihr Corporate Design an

### Häufige Use Cases

**Landing Page:**
```
Erstellen Sie eine dedizierte Seite "TCO-Rechner"
→ Fügen Sie Shortcode ein
→ Verlinken Sie in Hauptnavigation
```

**Blog-Integration:**
```
Erstellen Sie Beitrag über TCO
→ Betten Sie Rechner via Shortcode ein
→ Kombinieren Sie mit Text/Erklärungen
```

**Page Builder:**
```
Elementor/Gutenberg: Shortcode-Block nutzen
→ Shortcode einfügen
→ Layout anpassen
```

## 🐛 Häufige Probleme

### Plugin erscheint nicht
```
Lösung: Prüfen Sie Verzeichnisstruktur
/wp-content/plugins/themisdb-tco-calculator/
```

### Styles werden nicht geladen
```
Lösung: Cache leeren (WordPress + Browser)
Strg+Shift+R (Windows) oder Cmd+Shift+R (Mac)
```

### JavaScript funktioniert nicht
```
Lösung: Browser-Konsole prüfen (F12)
→ Andere Plugins temporär deaktivieren
```

## 📞 Support

- **GitHub**: [makr-code/wordpressPlugins](https://github.com/makr-code/wordpressPlugins)
- **Issues**: [GitHub Issues](https://github.com/makr-code/wordpressPlugins/issues)
- **Dokumentation**: Siehe README.md und INSTALLATION.md

## 📦 Lieferumfang

```
themisdb-tco-calculator/
├── themisdb-tco-calculator.php    # Haupt-Plugin-Datei
├── README.md                      # Vollständige Dokumentation
├── INSTALLATION.md                # Installations-Guide
├── COMPARISON.md                  # Versions-Vergleich
├── LICENSE                        # MIT Lizenz
├── assets/
│   ├── css/tco-calculator.css    # Plugin-Styles
│   └── js/tco-calculator.js      # Plugin-JavaScript
└── templates/
    ├── calculator.php             # HTML-Template
    └── admin-settings.php         # Admin-Seite
```

## ✅ Checkliste

Nach Installation prüfen:

- [ ] Plugin aktiviert
- [ ] Testseite mit Shortcode erstellt
- [ ] Rechner wird angezeigt
- [ ] Berechnung funktioniert
- [ ] Chart wird dargestellt
- [ ] Export-Funktionen testen
- [ ] Mobile Ansicht prüfen
- [ ] Einstellungen konfiguriert

## 🚀 Nächste Schritte

1. **Installation abschließen** - Plugin aktivieren und testen
2. **Seite erstellen** - Dedicated TCO-Rechner-Seite
3. **Einstellungen anpassen** - Standardwerte konfigurieren
4. **Design anpassen** - An Corporate Design angleichen
5. **SEO optimieren** - Meta-Tags und Keywords
6. **Navigation integrieren** - In Hauptmenü einbinden
7. **Testen** - Auf verschiedenen Geräten
8. **Veröffentlichen** - Live schalten

---

**Version**: 1.0.0  
**Lizenz**: MIT  
**Autor**: ThemisDB Team

**Viel Erfolg mit Ihrem TCO-Rechner!** 🎉
