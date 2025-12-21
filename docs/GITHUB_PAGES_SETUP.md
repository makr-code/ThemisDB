# GitHub Pages Setup Guide

This guide explains how to enable and configure GitHub Pages for the ThemisDB documentation.

## Prerequisites

- Repository admin access to `makr-code/ThemisDB`
- GitHub Actions enabled in the repository

## Setup Steps

### 1. Enable GitHub Pages

1. Go to the repository on GitHub: https://github.com/makr-code/ThemisDB
2. Click on **Settings** tab
3. In the left sidebar, click **Pages**
4. Under "Build and deployment":
   - **Source**: Select "GitHub Actions"
   - This allows the documentation workflow to deploy directly

### 2. Workflow Configuration

The documentation workflow (`.github/workflows/docs.yml`) is already configured to:
- Build the documentation on pushes to `main` or `develop` branches
- Deploy to GitHub Pages automatically
- Generate and include a PDF export

No additional configuration is needed - the workflow will run automatically once GitHub Pages is enabled.

### 3. Verify Deployment

After the first workflow run:
1. Go to **Actions** tab
2. Check the "Documentation Build & Deploy" workflow
3. Once complete, your documentation will be available at:
   - **Main branch**: `https://makr-code.github.io/ThemisDB/`
   - **Develop branch**: `https://makr-code.github.io/ThemisDB/` (latest from develop)

### 4. Custom Domain (Optional)

To use a custom domain:
1. Go to **Settings** → **Pages**
2. Under "Custom domain", enter your domain (e.g., `docs.themisdb.org`)
3. Add a CNAME file to the repository root pointing to your custom domain
4. Configure DNS settings:
   ```
   CNAME docs.themisdb.org. -> makr-code.github.io.
   ```

## GitHub Wiki Integration

The repository also includes a wiki sync script (`scripts/sync-wiki.ps1`) that can sync documentation to the GitHub Wiki:

### Enable GitHub Wiki

1. Go to repository **Settings**
2. Scroll to "Features" section
3. Enable **Wikis** checkbox

### Sync Documentation to Wiki

Run the sync script (requires Windows/PowerShell):
```powershell
.\scripts\sync-wiki.ps1
```

This will:
- Clone the wiki repository
- Copy documentation files
- Include the PDF export
- Push changes to the wiki

**Note:** The wiki is separate from GitHub Pages. Choose one or both based on your needs:
- **GitHub Pages**: Better formatting, navigation, and search
- **GitHub Wiki**: Easier editing directly on GitHub, simpler structure

## Automated Builds

The documentation workflow automatically triggers on:
- Pushes to `main` or `develop` that modify:
  - `docs/**` (any documentation files)
  - `mkdocs.yml` (configuration)
  - `requirements-docs.txt` (dependencies)
  - `.github/workflows/docs.yml` (workflow itself)
- Pull requests to `main` or `develop` with documentation changes
- Manual workflow dispatch

## Troubleshooting

### Workflow Fails

1. Check the workflow run logs in the **Actions** tab
2. Common issues:
   - Pages not enabled (see step 1)
   - Insufficient permissions (check workflow permissions)
   - Build errors (check MkDocs configuration)

### Pages Not Updating

1. Check workflow completed successfully
2. GitHub Pages can take a few minutes to update
3. Clear browser cache
4. Check the **Pages** settings shows the correct source

### PDF Not Generated

1. The workflow will still deploy even if PDF fails
2. Check workflow logs for wkhtmltopdf errors
3. External resource errors (fonts, badges) can be ignored - PDF is still generated

## Verification Checklist

After setup, verify:
- [ ] GitHub Pages is enabled with "GitHub Actions" as source
- [ ] Workflow runs successfully on documentation changes
- [ ] Documentation site is accessible at the GitHub Pages URL
- [ ] PDF is generated and available in artifacts
- [ ] Navigation and search work correctly
- [ ] Images and assets load properly

## Additional Resources

- [GitHub Pages Documentation](https://docs.github.com/en/pages)
- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [MkDocs Documentation](https://www.mkdocs.org/)
- [Material for MkDocs](https://squidfunk.github.io/mkdocs-material/)
