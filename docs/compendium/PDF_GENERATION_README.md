# ThemisDB Compendium - PDF Generation

Professional PDF generation with modern book layout and typography.

> **✨ NEU (v1.3.4):** Professionelles Buchlayout mit intelligenter Seitenverwaltung!  
> Siehe [PDF_LAYOUT_IMPROVEMENTS.md](PDF_LAYOUT_IMPROVEMENTS.md) für Details.

## 📚 Current Status

- **Word Count:** ~135,500 words
- **Pages:** ~271 pages (A4 format)
- **Target:** 1,000 pages comprehensive technical book
- **Version:** 1.3.4

## 🆕 Professional Book Layout (NEU v1.3.4)

### Schnellstart (step1-5 Pipeline)
```bash
cd compendium
python3 step1_generate_svgs.py
python3 step2_generate_html.py    # ← Mit professionellem Layout
python3 step3_generate_pdf.py
python3 step4_add_bookmarks.py
python3 step5_cleanup.py
```

### Neue Features
- ✅ **Durchgehende Seitennummerierung** - Alle Seiten korrekt nummeriert
- ✅ **Seitenzahlen im Inhaltsverzeichnis** - Automatisch mit CSS target-counter
- ✅ **Widow/Orphan Control** - Keine abgeschnittenen Absätze (min. 3 Zeilen)
- ✅ **Intelligente Seitenumbrüche** - Kapitel und Absätze werden nicht getrennt
- ✅ **Running Headers** - Buchtitel in Kopfzeile
- ✅ **Professionelle Typografie** - Serif für Text, Sans-Serif für Überschriften
- ✅ **Buchdruck-Ränder** - Unterschiedliche Ränder für linke/rechte Seiten
- ✅ **Automatische Silbentrennung** - Blocksatz wie in professionellen Büchern

**Orientiert an:** Microsoft Word Buchvorlagen und professionellen Verlagsstandards

Für detaillierte Informationen siehe: [PDF_LAYOUT_IMPROVEMENTS.md](PDF_LAYOUT_IMPROVEMENTS.md)

## 🎨 Features

### Modern Book Layout

- **Typography:**
  - Serif fonts (Georgia) for body text - optimal readability in print
  - Sans-serif fonts (Helvetica Neue) for headings - modern, clean look
  - Monospace fonts (Fira Code/Courier New) for code blocks

- **Professional Styling:**
  - Generous margins (25mm) for comfortable reading
  - Line spacing: 1.65 for body text, 1.3 for headings
  - Page headers with book title and version
  - Page footers with page numbers
  - Text justification with automatic hyphenation

- **Code Blocks:**
  - Subtle gray background (#f8f8f8)
  - Purple left border for visual distinction
  - Syntax highlighting support
  - Optimized font size (9-9.5pt)
  - Page-break avoidance

- **Tables:**
  - Alternating row colors for readability
  - Purple header background (#7c4dff)
  - Subtle box shadows
  - Page-break avoidance

- **Sections:**
  - Chapter numbering with "Kapitel X:" prefix
  - H1: 28pt, purple bottom border, page-break before
  - H2: 20pt, gray bottom border
  - H3: 16pt, purple color
  - Hierarchical heading structure

## 🚀 PDF Generation Methods

### Method 1: Pandoc (LaTeX-based)

**Best for:** Maximum compatibility, traditional publishing workflow

```bash
./generate_pdf.sh
```

**Requirements:**
```bash
# Ubuntu/Debian
sudo apt-get install pandoc texlive-xetex texlive-fonts-recommended texlive-latex-extra

# macOS
brew install pandoc
brew install --cask basictex
```

**Features:**
- XeLaTeX engine for superior typography
- IEEE-style citations
- Advanced table support
- Mathematical formulas
- Professional footnotes

**Output:** `../../pdf_output/ThemisDB-Compendium-v1.3.4-YYYYMMDD.pdf`

### Method 2: WeasyPrint (CSS-based)

**Best for:** Modern CSS support, faster generation, smaller files

```bash
./generate_pdf_weasyprint.py
```

**Requirements:**
```bash
pip install markdown weasyprint pygments
```

**Features:**
- Full CSS3 support (flexbox, grid, shadows)
- Faster generation (no LaTeX compilation)
- Smaller file sizes
- Better image handling
- Modern web standards

**Output:** `../../pdf_output/ThemisDB-Compendium-v1.3.4-YYYYMMDD.pdf`

## 📖 Included Chapters

1. **Preface** - Introduction and overview
2. **Chapter 0: Genesis** - Development history and strategic context
3. **Chapter 1: Introduction** - Core concepts and Base Entity paradigm
4. **Chapter 2: Architecture** - Technical architecture deep-dive
5. **Chapter 3: Multi-Model** - Native vs Polyglot comparison
6. **Chapter 5: Relational** - Advanced AQL optimization techniques
7. **Chapter 6: Graph** - Temporal graph queries
8. **Chapter 8: Storage Layer** - RocksDB tuning and memory optimization
9. **Chapter 10: Enterprise** - Security stack and compliance
10. **Chapter 11: Realtime** - CDC and streaming architecture
11. **Chapter 15: Analytics** - Query optimization
12. **Chapter 16: Sharding** - Horizontal scaling
13. **Chapter 17: LLM Integration** - Hybrid search implementation
14. **Chapter 19: Monitoring** - Observability with Prometheus/Grafana
15. **Chapter 21: Performance** - Tuning and compression strategies
16. **Chapter 24: AI Ethics** - Governance and bias mitigation
17. **Appendix A: Literature** - IEEE-standard references
18. **Appendix D: Feature Status** - Implementation tracking matrix

## 🎨 Styling Files

### `styles_modern_book.scss`

Modern, professional book layout with:
- A4 page format (210mm x 297mm)
- 25mm margins (top/bottom/left/right)
- Page headers and footers
- Chapter numbering
- Color scheme: Purple accent (#7c4dff)
- Typography hierarchy
- Table and code block styling
- Print-optimized colors and shadows

### `styles_pdf_optimization.scss` (Legacy)

Minimal styling focused on file size reduction.

## 📊 Build Information

Each PDF generation creates a `build_info.txt` file with:
- Build date and time
- File size
- Chapter count
- Pandoc/WeasyPrint version
- Build log location

## 🔧 Customization

### Changing Fonts

Edit the SCSS file or script:

```scss
body {
    font-family: "Your Preferred Serif", Georgia, serif;
}

h1, h2, h3 {
    font-family: "Your Preferred Sans", Helvetica, sans-serif;
}

code {
    font-family: "Your Preferred Mono", "Courier New", monospace;
}
```

### Changing Colors

```scss
$primary-color: #7c4dff;  /* Purple accent */
$text-color: #2c3e50;     /* Dark gray text */
$code-bg: #f8f8f8;        /* Light gray background */
```

### Adjusting Margins

```scss
@page {
    margin: 30mm 25mm 30mm 25mm;  /* Top Right Bottom Left */
}
```

### Font Sizes

```scss
body { font-size: 11pt; }
h1 { font-size: 28pt; }
h2 { font-size: 20pt; }
h3 { font-size: 16pt; }
code { font-size: 9.5pt; }
```

## 🐛 Troubleshooting

### Pandoc Errors

**Error:** `pdflatex not found`
**Solution:** Install TeX Live: `sudo apt-get install texlive-xetex`

**Error:** `Package not found`
**Solution:** Install additional LaTeX packages: `sudo apt-get install texlive-latex-extra`

### WeasyPrint Errors

**Error:** `ModuleNotFoundError: No module named 'weasyprint'`
**Solution:** Install dependencies: `pip install weasyprint`

**Error:** `cairo library not found`
**Solution:** Install system libraries:
```bash
# Ubuntu/Debian
sudo apt-get install libcairo2 libpango-1.0-0 libpangocairo-1.0-0 libgdk-pixbuf2.0-0 libffi-dev shared-mime-info

# macOS
brew install cairo pango gdk-pixbuf libffi
```

### Common Issues

1. **PDF too large:** Use WeasyPrint method or optimize images
2. **Fonts not embedded:** Ensure fonts are system-installed
3. **Code blocks cut off:** Adjust `page-break-inside: avoid`
4. **Tables split awkwardly:** Add `page-break-inside: avoid` to table CSS

## 📈 Performance

### Pandoc Method
- Generation time: ~2-5 minutes (depends on chapter count)
- File size: 5-15 MB (with images)
- Memory usage: ~500MB-1GB

### WeasyPrint Method
- Generation time: ~30-90 seconds
- File size: 3-8 MB (with images)
- Memory usage: ~200-400MB

## 📝 Future Enhancements

- [ ] Table of Contents with hyperlinks
- [ ] Index generation
- [ ] Cross-references between chapters
- [ ] Footnote numbering
- [ ] Bibliography page
- [ ] Cover page design
- [ ] Chapter summary boxes
- [ ] Margin notes
- [ ] Color profiles for professional printing

## 🤝 Contributing

To add new chapters to the PDF:

1. Create the markdown file in `docs/compendium/`
2. Add the filename to the `CHAPTERS` list in `generate_pdf.sh`
3. Or add to `chapters` list in `generate_pdf_weasyprint.py`
4. Run the generator

## 📄 License

Same as ThemisDB: MIT License with Government Clause

## 🔗 Related Documentation

- [MkDocs Configuration](./mkdocs-compendium.yml)
- [Chapter Mapping](../../book/chapter_mapping.md)
- [Writing Guidelines](../../book/writing_guidelines.md)

---

**Last Updated:** 2025-12-30  
**Maintained by:** ThemisDB Documentation Team
