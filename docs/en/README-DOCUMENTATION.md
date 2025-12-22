# ThemisDB Documentation System

This directory contains the source documentation for ThemisDB, built with [MkDocs](https://www.mkdocs.org/) and the [Material for MkDocs](https://squidfunk.github.io/mkdocs-material/) theme.

## Overview

The documentation system provides:
- **Static documentation site** - Built with MkDocs and deployed to GitHub Pages
- **Single PDF export** - Complete documentation in one PDF file for distribution
- **Automated builds** - GitHub Actions workflow for continuous documentation deployment

## Quick Start

### Prerequisites

- Python 3.12 or higher
- pip (Python package manager)

### Local Development

1. **Install dependencies:**
   ```bash
   pip install -r requirements-docs.txt
   ```

2. **Serve documentation locally:**
   ```bash
   mkdocs serve
   ```
   
   The documentation will be available at http://localhost:8000

3. **Build static site:**
   ```bash
   mkdocs build
   ```
   
   The static site will be generated in the `site/` directory.

### PDF Generation

Generate a PDF from the documentation:

**Linux/macOS:**
```bash
# Install wkhtmltopdf first
sudo apt-get install wkhtmltopdf  # Debian/Ubuntu
# or
brew install wkhtmltopdf           # macOS

# Build docs and generate PDF
mkdocs build
./scripts/export_pdf_wkhtml.sh
```

**Windows:**
```powershell
# Install wkhtmltopdf from https://wkhtmltopdf.org/downloads.html

# Build docs and generate PDF
mkdocs build
.\scripts\export_pdf_wkhtml.ps1
```

The PDF will be generated at `docs/ThemisDB-Documentation.pdf`.

## Automated Deployment

The documentation is automatically built and deployed via GitHub Actions:

### Workflow: `.github/workflows/docs.yml`

**Triggers:**
- Push to `main` or `develop` branches (when docs/ or mkdocs.yml change)
- Pull requests affecting documentation
- Manual workflow dispatch

**Actions:**
1. Builds the MkDocs site with Material theme
2. Generates PDF from the print page
3. Uploads artifacts (site + PDF)
4. Deploys to GitHub Pages (on push to main/develop)

**Artifacts:**
- `documentation-site` - Complete static site
- `documentation-pdf` - Single PDF export

### GitHub Pages

The documentation is deployed to GitHub Pages at:
- **Main branch:** `https://makr-code.github.io/ThemisDB/`
- Automatically updated on every push to main/develop

## Documentation Structure

```
docs/
├── index.md              # Documentation home page
├── Home.md               # Welcome page
├── architecture/         # Architecture documentation
├── api/                  # API documentation
├── guides/               # User guides
├── security/             # Security documentation
├── development/          # Development documentation
└── ...                   # Other documentation sections
```

### Configuration

**mkdocs.yml** - Main configuration file:
- Site metadata (name, description, URL)
- Navigation structure
- Theme configuration (Material)
- Plugins:
  - `search` - Full-text search
  - `print-site` - Single-page print view for PDF generation
- Markdown extensions

**requirements-docs.txt** - Python dependencies:
- mkdocs
- mkdocs-material
- mkdocs-print-site-plugin
- mkdocs-with-pdf

## PDF Export Details

The PDF is generated using the `mkdocs-print-site-plugin`, which creates a single-page print view of all documentation pages. This page is then converted to PDF using wkhtmltopdf.

**Features:**
- Complete table of contents with bookmarks
- All documentation in a single PDF file
- A4 page size with appropriate margins
- Suitable for printing or digital distribution

**Note:** The PDF may show warnings about external resources (fonts, badges) that cannot be loaded in the build environment, but the PDF will still be generated successfully.

## Packaging Integration

The generated PDF (`docs/ThemisDB-Documentation.pdf`) is included in release packages. See `scripts/package_releases.ps1` for details on how the PDF is bundled with releases.

## Contributing

When adding or updating documentation:

1. Edit the appropriate `.md` files in the `docs/` directory
2. Update `mkdocs.yml` navigation if adding new pages
3. Test locally with `mkdocs serve`
4. Create a pull request
5. The documentation will be automatically built and verified

## Troubleshooting

### Build Errors

If you encounter build errors:
```bash
# Check for missing dependencies
pip install -r requirements-docs.txt

# Build with verbose output
mkdocs build --verbose

# Check for strict mode errors
mkdocs build --strict
```

### PDF Generation Issues

If PDF generation fails:
1. Ensure wkhtmltopdf is installed and in PATH
2. Check that `mkdocs build` completed successfully
3. Verify `site/print_page/index.html` exists
4. External resource errors (fonts, badges) can be ignored

### Local Preview Issues

If `mkdocs serve` doesn't work:
1. Check port 8000 is not in use
2. Try a different port: `mkdocs serve -a localhost:8080`
3. Clear browser cache if styles look wrong

## Additional Resources

- [MkDocs Documentation](https://www.mkdocs.org/)
- [Material for MkDocs](https://squidfunk.github.io/mkdocs-material/)
- [MkDocs Print Site Plugin](https://timvink.github.io/mkdocs-print-site-plugin/)
