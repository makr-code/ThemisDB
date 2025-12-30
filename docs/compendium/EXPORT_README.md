# ThemisDB Compendium Export Scripts

This directory contains scripts for exporting the ThemisDB Compendium documentation to various formats.

## Available Export Scripts

### 1. `export_compendium_pdf.sh`
Exports the compendium to a single PDF file using Pandoc.

**Requirements:**
- pandoc
- pdflatex (texlive-latex-base, texlive-latex-extra on Linux)
- Optional: ghostscript (for PDF optimization)

**Installation:**
```bash
# Ubuntu/Debian
sudo apt-get install pandoc texlive-latex-base texlive-latex-extra ghostscript

# macOS
brew install pandoc basictex ghostscript

# Windows
# Download pandoc from https://pandoc.org/installing.html
# Download MiKTeX from https://miktex.org/download
```

**Usage:**
```bash
./scripts/export_compendium_pdf.sh
```

**Output:** `ThemisDB-Compendium.pdf` in repository root

**Features:**
- Automatic table of contents with 3 levels
- Numbered sections
- Syntax-highlighted AQL code blocks
- A4 page size with 2.5cm margins
- PDF optimization with Ghostscript (if available)

### 2. `export_compendium_docx.sh`
Exports the compendium to Microsoft Word DOCX format using Pandoc.

**Requirements:**
- pandoc

**Installation:**
```bash
# Ubuntu/Debian
sudo apt-get install pandoc

# macOS
brew install pandoc

# Windows
# Download from https://pandoc.org/installing.html
```

**Usage:**
```bash
./scripts/export_compendium_docx.sh
```

**Output:** `ThemisDB-Compendium.docx` in repository root

**Features:**
- Automatic table of contents
- Numbered sections
- Syntax-highlighted code blocks
- Preserves tables, lists, and formatting
- Editable in Microsoft Word, LibreOffice, or Google Docs

### 3. `export_compendium_all.sh`
Convenience script that exports to both PDF and DOCX formats.

**Usage:**
```bash
./scripts/export_compendium_all.sh
```

**Output:** Both `ThemisDB-Compendium.pdf` and `ThemisDB-Compendium.docx` in repository root

## Chapter Order

The export scripts process chapters in the following order:

1. Preface
2. Index/Introduction
3. Chapter 01: Introduction
4. Chapter 02: Architecture
5. Chapter 02.5: MVCC Timeline
6. Chapter 03: Multi-Model
7. Chapter 04: Installation
8. Chapter 05: Relational
9. Chapter 06: Graph
10. Chapter 07: Document
11. Chapter 08: Vector
12. Chapter 09: Timeseries
13. Chapter 10: Enterprise
14. Chapter 11: Realtime
15. Chapter 12: Computer Vision
16. Chapter 13: Fulltext
17. Chapter 14: Geospatial
18. Chapter 15: Analytics
19. Chapter 16: Machine Learning
20. Chapter 18: Monitoring
21. Chapter 19: Backup
22. Chapter 20: Performance
23. Chapter 21: Clients

## Customization

### PDF Customization

Edit `export_compendium_pdf.sh` and modify the pandoc options:

```bash
--variable geometry:margin=2.5cm    # Page margins
--variable fontsize=11pt            # Font size
--variable papersize=a4             # Paper size (a4, letter, etc.)
--toc-depth=3                       # TOC depth (1-6)
```

### DOCX Customization

To use a custom Word template:

1. Create a reference document with your desired styles
2. Modify `export_compendium_docx.sh`:

```bash
pandoc "${EXISTING_FILES[@]}" \
    --reference-doc=path/to/reference.docx \
    ...
```

## Troubleshooting

### "pandoc: command not found"
Install pandoc following the instructions above.

### "pdflatex: command not found"
For PDF export, you need a LaTeX distribution:
- Linux: `sudo apt-get install texlive-latex-base texlive-latex-extra`
- macOS: `brew install basictex` then run `sudo tlmgr update --self && sudo tlmgr install collection-latexextra`
- Windows: Install MiKTeX from https://miktex.org/

### "Missing character" warnings during PDF generation
These warnings are normal and can be ignored. The PDF will be generated successfully.

### Large file sizes
- For PDF: Ensure ghostscript is installed for automatic optimization
- For DOCX: Word files are typically smaller than PDFs

### AQL syntax highlighting
The scripts use the `tango` highlighting style. To change it, modify `--highlight-style=`:
- Available styles: `pygments`, `tango`, `espresso`, `zenburn`, `kate`, `monochrome`, `breezedark`, `haddock`

## Integration with CI/CD

These scripts can be integrated into GitHub Actions or other CI/CD pipelines:

```yaml
- name: Install Pandoc
  run: |
    sudo apt-get update
    sudo apt-get install -y pandoc texlive-latex-base texlive-latex-extra ghostscript

- name: Export Documentation
  run: |
    ./scripts/export_compendium_all.sh

- name: Upload Artifacts
  uses: actions/upload-artifact@v3
  with:
    name: documentation-exports
    path: |
      ThemisDB-Compendium.pdf
      ThemisDB-Compendium.docx
```

## Notes

- The exports include all chapters that have been converted to AQL syntax
- Code blocks tagged with `aql` are properly syntax-highlighted
- Cross-references and internal links are preserved
- The generated files are placed in the repository root directory
- Existing files with the same name will be overwritten

## Support

For issues or questions about the export scripts, please open an issue on GitHub.
