# Build ThemisDB Documentation with MkDocs
# Generates static site and PDF export

param(
    [switch]$SkipPdf
)

$ErrorActionPreference = 'Stop'

Write-Host "=== Build ThemisDB Documentation ===" -ForegroundColor Cyan

# Check if Python is available
if (-not (Get-Command python -ErrorAction SilentlyContinue)) {
    if (-not (Get-Command python3 -ErrorAction SilentlyContinue)) {
        Write-Error "Python 3 is required but not found in PATH"
        exit 1
    }
    $pythonCmd = "python3"
} else {
    $pythonCmd = "python"
}

# Install/update dependencies
Write-Host "Installing documentation dependencies..." -ForegroundColor Green
& $pythonCmd -m pip install -r requirements-docs.txt --upgrade
if ($LASTEXITCODE -ne 0) {
    Write-Error "Failed to install dependencies"
    exit 1
}

# Clean previous build
Write-Host "Cleaning previous build..." -ForegroundColor Yellow
if (Test-Path "site") {
    Remove-Item -Path "site" -Recurse -Force
}

# Build MkDocs site
Write-Host "Building MkDocs site..." -ForegroundColor Green
& $pythonCmd -m mkdocs build --clean
if ($LASTEXITCODE -ne 0) {
    Write-Error "MkDocs build failed"
    exit 1
}

# Generate PDF if not skipped
if (-not $SkipPdf) {
    # Path consistent with export_pdf_wkhtml.ps1
    $wkhtmlPath = "C:\\Program Files\\wkhtmltopdf\\bin\\wkhtmltopdf.exe"
    
    if (Test-Path $wkhtmlPath) {
        Write-Host "Generating PDF..." -ForegroundColor Green
        & .\scripts\export_pdf_wkhtml.ps1
    } elseif (Get-Command wkhtmltopdf -ErrorAction SilentlyContinue) {
        Write-Host "Generating PDF..." -ForegroundColor Green
        & .\scripts\export_pdf_wkhtml.ps1
    } else {
        Write-Host "⚠️  wkhtmltopdf not found. Skipping PDF generation." -ForegroundColor Yellow
        Write-Host "   Install from: https://wkhtmltopdf.org/downloads.html" -ForegroundColor Gray
    }
}

Write-Host "✅ Documentation build complete!" -ForegroundColor Green
Write-Host "   Static site: ./site/" -ForegroundColor Gray
if (Test-Path "docs/ThemisDB-Documentation.pdf") {
    $pdfSize = (Get-Item "docs/ThemisDB-Documentation.pdf").Length / 1MB
    Write-Host "   PDF: ./docs/ThemisDB-Documentation.pdf ($([math]::Round($pdfSize, 2)) MB)" -ForegroundColor Gray
}
