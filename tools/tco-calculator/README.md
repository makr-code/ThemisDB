> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# ThemisDB TCO-Rechner

Ein interaktiver Total Cost of Ownership (TCO) Rechner für ThemisDB - Vergleichen Sie die Gesamtbetriebskosten verschiedener Datenbanklösungen.

> **💡 WordPress-Version verfügbar:** Wenn Sie WordPress verwenden, schauen Sie sich das [WordPress-Plugin](../tco-calculator-wordpress/) an. Es bietet die gleiche Funktionalität mit nahtloser WordPress-Integration via Shortcode.

## 📋 Übersicht

Der TCO-Rechner hilft Ihnen, fundierte Entscheidungen über Ihre Datenbank-Infrastruktur zu treffen, indem er die Gesamtbetriebskosten über einen Zeitraum von 3 Jahren vergleicht:

- **ThemisDB** (Minimal, Community, Enterprise, Hyperscaler Editionen)
- **Cloud-Hyperscaler** (AWS DynamoDB, Azure Cosmos DB, Google Cloud Spanner)

## ✨ Features

### Umfassende Kostenanalyse
- 💰 **Infrastrukturkosten**: Server, Storage, Netzwerk, Backups
- 👥 **Personalkosten**: DBAs, Entwickler mit Overhead-Berechnung
- 📜 **Lizenzkosten**: ThemisDB Editions und Enterprise Support
- 🔧 **Betriebskosten**: Schulungen, Wartung, Support
- 🤖 **AI/LLM-Kosten**: Native Integration vs. externe APIs

### Interaktive Features
- 📊 **Visualisierungen**: Dynamische Charts mit Chart.js
- 📈 **Jahresvergleich**: Detaillierte Aufschlüsselung über 3 Jahre
- 💡 **Intelligente Insights**: Automatische Analyse und Empfehlungen
- 📥 **Export-Funktionen**: PDF, CSV, Drucken

### Best Practices
- ✅ **Responsive Design**: Optimiert für Desktop, Tablet und Mobile
- ♿ **Barrierefreiheit**: WCAG-konforme Implementierung
- 🎨 **Modernes UI**: Clean Design mit CSS-Grid und Flexbox
- 🚀 **Performance**: Optimiertes JavaScript (ES6+)

## 🚀 Verwendung

### Lokal öffnen
Öffnen Sie einfach die `index.html` Datei in Ihrem Browser:

```bash
cd tools/tco-calculator
# Öffnen mit Standard-Browser
open index.html  # macOS
xdg-open index.html  # Linux
start index.html  # Windows
```

### Mit lokalem Server
Für beste Ergebnisse verwenden Sie einen lokalen Webserver:

```bash
# Python 3
python -m http.server 8000

# Node.js (mit npx)
npx http-server -p 8000

# PHP
php -S localhost:8000
```

Dann öffnen Sie: `http://localhost:8000`

## 📊 Berechnungsmethodik

### ThemisDB-Kosten

**Infrastruktur:**
- Server-Anzahl basierend auf Datenmenge und Durchsatz
- Redundanz für High Availability
- GPU-Kosten für AI/LLM-Features
- Storage mit Backup und DR

**Personal:**
- DBAs und Entwickler mit 30% Overhead
- Skaliert mit Komplexität

**Lizenzen:**
- Community: €0 (MIT Lizenz)
- Enterprise: ~€50.000/Jahr (geschätzt)
- Hyperscaler: Custom Pricing

**Betrieb:**
- Schulungen und Zertifizierungen
- Enterprise Support
- Wartung

### Hyperscaler-Kosten

**Compute:**
- Pay-per-Request-Modell (~€0,00025/Request)
- Basierend auf DynamoDB/Cosmos DB Pricing

**Storage:**
- 1,5x Multiplikator für Replikation
- Monatliche Kosten pro GB

**Network:**
- Egress-Gebühren für ausgehenden Traffic
- Keine Ingress-Kosten

**AI APIs:**
- Externe API-Kosten (OpenAI, Anthropic, etc.)
- Vermeidbar durch ThemisDB's native Integration

## 🎯 Wann lohnt sich ThemisDB?

ThemisDB bietet signifikante Kostenvorteile bei:

1. **Hoher Durchsatz**: > 100.000 Anfragen/Tag
   - Pay-per-Request wird teuer
   - Vorhersagbare Kosten wichtig

2. **Große Datenmengen**: > 100 GB
   - Storage- und Backup-Kosten akkumulieren
   - Multi-Model vermeidet mehrere DBs

3. **AI/LLM-Workloads**: 
   - Native llama.cpp Integration
   - Keine API-Kosten
   - GPU-Hardware amortisiert sich

4. **Multi-Model-Anforderungen**:
   - Graph + Document + Vector + Timeseries
   - Ein System statt mehrere spezialisierte DBs

5. **Datensouveränität**:
   - On-Premise oder Private Cloud
   - Keine Vendor Lock-in

## 🔧 Technische Details

### Technologie-Stack
- **HTML5**: Semantisches Markup
- **CSS3**: Modern Layout (Grid, Flexbox), CSS Variables
- **JavaScript (ES6+)**: Klassen, Arrow Functions, Template Literals
- **Chart.js**: Datenvisualisierung
- **No Build Required**: Pure Vanilla Stack

### Browser-Kompatibilität
- ✅ Chrome/Edge (90+)
- ✅ Firefox (88+)
- ✅ Safari (14+)
- ✅ Mobile Browsers

### Code-Struktur

```
tco-calculator/
├── index.html          # Haupt-HTML-Struktur
├── styles.css          # Styling und Layout
├── app.js              # JavaScript Logik
└── README.md           # Diese Datei
```

## 📝 Annahmen

### Berechnungsparameter
- **Zeitraum**: 3 Jahre (typischer Planungshorizont)
- **Datenwachstum**: 20% pro Jahr
- **Personalkosten**: +30% Overhead für Benefits
- **Verfügbarkeit**: 99% bis 99.999%
- **Peak Load**: 1-10x Durchschnittslast

### Kostenmodelle
- ThemisDB: CapEx + OpEx (vorhersagbar)
- Hyperscaler: OpEx (variabel, skaliert linear)

## 🎨 Anpassung

### CSS-Variablen ändern
Passen Sie Farben in `styles.css` an:

```css
:root {
    --primary-color: #2c3e50;
    --secondary-color: #3498db;
    --success-color: #27ae60;
    /* ... weitere Variablen */
}
```

### Berechnungsparameter anpassen
Ändern Sie Konstanten in `app.js`:

```javascript
const CONFIG = {
    YEARS: 3,
    DATA_GROWTH_RATE: 0.20,
    THEMISDB_ENTERPRISE_LICENSE: 50000,
    // ... weitere Konstanten
};
```

## 📄 Lizenz

Dieses Tool ist Teil von ThemisDB und unter der MIT-Lizenz lizenziert.

## 🤝 Beitragen

Verbesserungsvorschläge und Pull Requests sind willkommen!

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

## 📞 Kontakt

- GitHub: [makr-code/ThemisDB](https://github.com/makr-code/ThemisDB)
- Issues: [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)

## ⚠️ Disclaimer

Dieser Rechner dient als Orientierungshilfe. Tatsächliche Kosten können je nach spezifischen Anforderungen, Verträgen und Nutzungsmustern variieren. Für genaue Kostenschätzungen kontaktieren Sie bitte die jeweiligen Anbieter.

---

**Version**: 1.0  
**Letzte Aktualisierung**: Januar 2026
