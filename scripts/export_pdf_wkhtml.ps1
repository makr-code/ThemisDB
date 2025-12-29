#!/usr/bin/env pwsh
# Export MkDocs site to PDF using wkhtmltopdf

param(
    [string]$OutputPath = "docs/ThemisDB-Documentation.pdf"
)

$ErrorActionPreference = 'Stop'

Write-Host "=== Export MkDocs to PDF (wkhtmltopdf) ===" -ForegroundColor Cyan

# Resolve wkhtmltopdf path
$wkhtml = "C:\\Program Files\\wkhtmltopdf\\bin\\wkhtmltopdf.exe"
if (-not (Test-Path $wkhtml)) {
    $cmd = Get-Command wkhtmltopdf -ErrorAction SilentlyContinue
    if ($cmd) { $wkhtml = $cmd.Source }
}
if (-not $wkhtml) { Write-Error "wkhtmltopdf not found. Install from https://wkhtmltopdf.org/downloads.html"; exit 1 }

# Determine source HTML (print-site plugin preferred)
$siteRoot = Join-Path $PWD "site"
if (-not (Test-Path $siteRoot)) { Write-Error "MkDocs site not found at ./site. Run mkdocs build first."; exit 1 }

$candidates = @(
    (Join-Path $siteRoot "print/index.html"),
    (Join-Path $siteRoot "print.html"),
    (Join-Path $siteRoot "index.html")
)

$sourceHtml = $null
foreach ($c in $candidates) { if (Test-Path $c) { $sourceHtml = $c; break } }
if (-not $sourceHtml) { Write-Error "No suitable HTML found in site/."; exit 1 }

# Ensure output directory exists
$outDir = Split-Path -Parent $OutputPath
if (-not (Test-Path $outDir)) { New-Item -ItemType Directory -Path $outDir | Out-Null }

Write-Host "Source: $sourceHtml" -ForegroundColor Gray
Write-Host "Output: $OutputPath" -ForegroundColor Gray

# Run wkhtmltopdf
& "$wkhtml" --enable-local-file-access --print-media-type --outline --dpi 150 --margin-top 15mm --margin-bottom 15mm --footer-center "[page]/[toPage]" "$sourceHtml" "$OutputPath"
if ($LASTEXITCODE -ne 0) { Write-Error "wkhtmltopdf failed"; exit $LASTEXITCODE }

$pdfSize = (Get-Item $OutputPath).Length / 1MB
Write-Host "✅ PDF generated: $([math]::Round($pdfSize,2)) MB" -ForegroundColor Green
