# ThemisDB Formula Renderer

Ein WordPress-Plugin zum Rendern mathematischer Formeln in LaTeX-Notation mit KaTeX.

## Beschreibung

Das ThemisDB Formula Renderer Plugin ermöglicht es, mathematische Formeln in LaTeX-Notation (z.B. `$$D = (t_n – t_{n-1}) – (t_{n-1} – t_{n-2})$$`) automatisch in schön dargestellte, lesbare Formeln umzuwandeln. Das Plugin nutzt [KaTeX](https://katex.org/), eine schnelle und leichtgewichtige JavaScript-Bibliothek für mathematisches Rendering.

## Features

- ✅ **Conditional Loading** - KaTeX nur bei Bedarf laden für optimale Performance
- ✅ Automatisches Rendering von LaTeX-Formeln in Beiträgen, Seiten und Kommentaren
- ✅ Unterstützung für Inline-Formeln (`$...$`) und Block-Formeln (`$$...$$`)
- ✅ **Copy-to-Clipboard** - LaTeX-Code einfach kopieren
- ✅ **Formula Library** - Bibliothek häufiger Formeln im Admin-Bereich
- ✅ Schnelles Rendering mit KaTeX
- ✅ **Themis Brand Colors** - Professionelles Design mit Themis-Branding
- ✅ Responsive Design - funktioniert auf allen Geräten
- ✅ **MathML Support** - Barrierefreiheit für Screen Reader
- ✅ Dark Mode Unterstützung
- ✅ Einfache Integration per Shortcode
- ✅ Keine Server-seitige Verarbeitung erforderlich
- ✅ Anpassbare Delimiters (Trennzeichen)
- ✅ Vollständige LaTeX-Mathematik-Unterstützung
- ✅ Fehlertolerante Rendering-Engine
- ✅ WordPress Gutenberg und Classic Editor kompatibel

## Installation

### Methode 1: Manueller Upload

1. Laden Sie das Plugin-Verzeichnis `themisdb-formula-renderer` herunter
2. Laden Sie es in Ihr WordPress-Verzeichnis hoch: `/wp-content/plugins/`
3. Aktivieren Sie das Plugin im WordPress Admin unter "Plugins"
4. Konfigurieren Sie die Einstellungen unter "Einstellungen → Formula Renderer"

### Methode 2: ZIP-Upload

1. Erstellen Sie eine ZIP-Datei des Plugin-Verzeichnisses:
   ```bash
   cd wordpress-plugin
   zip -r themisdb-formula-renderer.zip themisdb-formula-renderer/
   ```
2. Gehen Sie zu WordPress Admin → Plugins → Installieren
3. Klicken Sie auf "Plugin hochladen"
4. Wählen Sie die ZIP-Datei aus und klicken Sie auf "Jetzt installieren"
5. Aktivieren Sie das Plugin nach der Installation

## Verwendung

### Automatisches Rendering

Das Plugin rendert automatisch alle Formeln in Ihrem Content. Fügen Sie einfach Formeln in Ihre Beiträge oder Seiten ein:

**Inline-Formel:**
```
Die Energie-Masse-Äquivalenz wird durch die Formel $E = mc^2$ beschrieben.
```

**Block-Formel:**
```
Die Differenz der zweiten Ordnung wird berechnet als:

$$D = (t_n - t_{n-1}) - (t_{n-1} - t_{n-2})$$
```

### Shortcode-Verwendung

Sie können auch Shortcodes verwenden für mehr Kontrolle:

**Block-Formel mit Shortcode:**
```
[themisdb_formula]
D = (t_n - t_{n-1}) - (t_{n-1} - t_{n-2})
[/themisdb_formula]
```

**Inline-Formel mit Shortcode:**
```
[themisdb_formula display="inline"]E = mc^2[/themisdb_formula]
```

**Alternative Shortcodes:**
- `[formula]...[/formula]`
- `[latex]...[/latex]`
- `[math]...[/math]`

### Beispiele

#### Grundlegende Formeln

```
$$E = mc^2$$

$$a^2 + b^2 = c^2$$

$$F = ma$$
```

#### Brüche und Wurzeln

```
$$\frac{a}{b} = c$$

$$\sqrt{x^2 + y^2}$$

$$\frac{\partial f}{\partial x}$$
```

#### Summen und Integrale

```
$$\sum_{i=1}^{n} x_i = x_1 + x_2 + \cdots + x_n$$

$$\int_{0}^{\infty} e^{-x^2} dx = \frac{\sqrt{\pi}}{2}$$
```

#### Matrizen

```
$$\begin{pmatrix}
a & b \\
c & d
\end{pmatrix}$$
```

#### Griechische Buchstaben

```
$$\alpha, \beta, \gamma, \delta, \epsilon, \theta, \lambda, \mu, \pi, \sigma, \omega$$
```

#### Physikalische Formeln

```
$$\nabla \times \vec{E} = -\frac{\partial \vec{B}}{\partial t}$$

$$H = -\sum_{i} p_i \log p_i$$

$$\Delta x \cdot \Delta p \geq \frac{\hbar}{2}$$
```

## Konfiguration

### Einstellungen

Gehen Sie zu **Einstellungen → Formula Renderer** um das Plugin zu konfigurieren:

- **Auto-Render Formulas**: Aktivieren/Deaktivieren Sie automatisches Rendering
- **Inline Delimiter**: Trennzeichen für Inline-Formeln (Standard: `$`)
- **Block Delimiter**: Trennzeichen für Block-Formeln (Standard: `$$`)

### Erweiterte Anpassung

#### CSS-Anpassung

Sie können das Aussehen der Formeln über CSS anpassen:

```css
/* Eigene Styles in Ihrem Theme */
.themisdb-formula-block {
    background-color: #f0f0f0;
    border-left: 4px solid #0073aa;
    padding: 1.5em;
}

.themisdb-formula-inline {
    color: #333;
}
```

#### JavaScript-Hooks

Für dynamisch geladenen Content:

```javascript
// Formeln nach AJAX-Load neu rendern
jQuery(document).on('contentLoaded', function() {
    if (typeof themisdbRenderFormulas !== 'undefined') {
        themisdbRenderFormulas();
    }
});
```

## LaTeX-Syntax

KaTeX unterstützt eine große Auswahl an LaTeX-Befehlen. Hier sind einige häufig verwendete:

### Mathematische Operatoren

- `+`, `-`, `*`, `/`, `=`
- `\times` (×), `\div` (÷)
- `\pm` (±), `\mp` (∓)
- `\approx` (≈), `\equiv` (≡)
- `\leq` (≤), `\geq` (≥)
- `\neq` (≠), `\sim` (∼)

### Klammern

- `()`, `[]`, `\{\}` für Klammern
- `\left(` und `\right)` für automatische Größenanpassung
- `\langle` und `\rangle` für spitze Klammern

### Hoch- und Tiefgestellt

- `x^2` für hochgestellt
- `x_i` für tiefgestellt
- `x^{2n}` für mehrbuchstabige Exponenten

### Brüche und Wurzeln

- `\frac{a}{b}` für Brüche
- `\sqrt{x}` für Quadratwurzel
- `\sqrt[n]{x}` für n-te Wurzel

### Summen, Produkte, Integrale

- `\sum_{i=1}^{n}` für Summen
- `\prod_{i=1}^{n}` für Produkte
- `\int_{a}^{b}` für Integrale
- `\lim_{x \to 0}` für Grenzwerte

### Vektoren und Matrizen

- `\vec{v}` für Vektoren
- `\begin{pmatrix} ... \end{pmatrix}` für Matrizen
- `\begin{bmatrix} ... \end{bmatrix}` für Matrizen mit eckigen Klammern

Vollständige Liste: [KaTeX Supported Functions](https://katex.org/docs/supported.html)

## Kompatibilität

- **WordPress**: 5.0+
- **PHP**: 7.2+
- **Browser**: Alle modernen Browser (Chrome, Firefox, Safari, Edge)
- **Editor**: Gutenberg und Classic Editor
- **Themes**: Funktioniert mit allen WordPress-Themes

## Performance

- **KaTeX** ist extrem schnell - rendert Formeln in Millisekunden
- **CDN-geliefert** - KaTeX wird von jsdelivr CDN geladen für optimale Performance
- **Keine Server-Last** - Alle Berechnungen erfolgen im Browser
- **Cache-freundlich** - Funktioniert perfekt mit WordPress-Caching-Plugins

## Fehlerbehebung

### Formeln werden nicht angezeigt

1. Überprüfen Sie, dass das Plugin aktiviert ist
2. Stellen Sie sicher, dass JavaScript im Browser aktiviert ist
3. Prüfen Sie die Browser-Konsole auf Fehler
4. Löschen Sie den WordPress-Cache
5. Überprüfen Sie, dass die Delimiters korrekt sind (`$$...$$`)

### Formeln sehen seltsam aus

1. Überprüfen Sie Ihre LaTeX-Syntax auf Fehler
2. Verwenden Sie die [KaTeX-Dokumentation](https://katex.org/docs/supported.html) als Referenz
3. Testen Sie die Formel auf [katex.org](https://katex.org/)

### Konflikte mit anderen Plugins

1. Deaktivieren Sie andere Mathematik-Plugins
2. Überprüfen Sie Theme-CSS auf Konflikte
3. Prüfen Sie auf JavaScript-Konflikte in der Browser-Konsole

### CDN-Probleme

Falls Sie CDN-Probleme haben, können Sie KaTeX lokal hosten:

1. Laden Sie KaTeX herunter: https://github.com/KaTeX/KaTeX/releases
2. Extrahieren Sie es nach `/wp-content/plugins/themisdb-formula-renderer/assets/katex/`
3. Modifizieren Sie die Plugin-Datei, um lokale Dateien zu verwenden

## Sicherheit

- ✅ Keine Remote-Code-Ausführung
- ✅ Alle Eingaben werden escaped
- ✅ XSS-geschützt
- ✅ Verwendet WordPress Security Best Practices
- ✅ KaTeX trust mode ist standardmäßig deaktiviert

## Support

- **Dokumentation**: Siehe [KaTeX Documentation](https://katex.org/docs/support_table.html)
- **GitHub Issues**: https://github.com/makr-code/wordpressPlugins/issues
- **LaTeX Hilfe**: https://en.wikibooks.org/wiki/LaTeX/Mathematics

## Lizenz

MIT License - siehe [LICENSE](LICENSE) Datei

## Credits

- **KaTeX**: https://katex.org/
- **ThemisDB Team**: https://github.com/makr-code/wordpressPlugins

## Changelog

### Version 1.1.0 (2026-02-11)

**⚡ Performance Enhancements:**
- ✅ **Conditional Loading** - KaTeX wird nur noch auf Seiten mit Formeln geladen (~70% schnellere Ladezeit)
- ✅ Preload-Hints für KaTeX-Ressourcen hinzugefügt

**🎨 Design & Branding:**
- ✅ **Themis Brand Colors** - Neue Farbpalette mit Themis-Branding
- ✅ Verbesserte mobile Darstellung mit horizontalem Scroll
- ✅ Dark Mode Optimierungen
- ✅ Print-Styles für bessere Druckausgabe
- ✅ High Contrast Mode Support

**✨ Neue Features:**
- ✅ **Copy-to-Clipboard Button** - LaTeX-Code direkt aus Formeln kopieren
- ✅ **Formula Library** - Admin-Bereich mit häufig verwendeten Formeln (Algebra, Calculus, Statistics, Physics, Geometry)
- ✅ **MathML Export** - Verbesserte Barrierefreiheit für Screen Reader

**♿ Accessibility:**
- ✅ MathML-Alternative für Screen Reader
- ✅ Verbesserte ARIA-Labels
- ✅ Reduced Motion Support

**📱 Mobile:**
- ✅ Optimierte Darstellung auf kleinen Bildschirmen
- ✅ Touch-optimierte Copy-Buttons
- ✅ Besseres Overflow-Handling für lange Formeln

### Version 1.0.0 (Initial Release)
- ✅ Grundlegende LaTeX-Formel-Rendering
- ✅ Shortcode-Support
- ✅ Auto-Render mit konfigurierbaren Delimiters
- ✅ Admin-Einstellungen

Siehe [CHANGELOG.md](CHANGELOG.md) für die vollständige Versionshistorie.

## Formula Library

Ab Version 1.1.0 enthält das Plugin eine **Formula Library** mit häufig verwendeten mathematischen Formeln:

**Zugriff:** WordPress Admin → Einstellungen → Formula Library

**Kategorien:**
- **Algebra** - Quadratische Formel, Binomialtheorem, Logarithmen
- **Calculus** - Ableitungsregeln, Integration, Fundamentalsatz
- **Statistics** - Mittelwert, Standardabweichung, Normalverteilung
- **Physics** - E=mc², Newtonsche Gesetze, kinetische Energie
- **Geometry** - Pythagoras, Kreisfläche, Kugelvolumen

Jede Formel kann mit einem Klick als Shortcode oder LaTeX-Code kopiert werden.

## Weiterentwicklung

Geplante Features:

- [ ] Visual Formula Editor
- [ ] Formel-Bibliothek mit häufigen Formeln
- [ ] Export nach MathML
- [ ] Chemische Formeln mit mhchem
- [ ] Physikalische Einheiten mit physics package
- [ ] Musik-Notation
- [ ] Diagramme mit tikz

## Beiträge

Beiträge sind willkommen! Siehe [CONTRIBUTING.md](../../CONTRIBUTING.md) für Guidelines.

---

**Entwickelt mit ❤️ vom ThemisDB Team**
