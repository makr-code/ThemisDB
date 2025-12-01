# Build MkDocs Documentation Locally
# Erstellt die MkDocs HTML-Dokumentation lokal im site/ Verzeichnis

Write-Host "=== ThemisDB Documentation Build ===" -ForegroundColor Cyan

# Prüfe ob Python und pip verfügbar sind
$pythonCmd = Get-Command python -ErrorAction SilentlyContinue
if (-not $pythonCmd) {
    Write-Error "Python nicht gefunden! Bitte Python 3.x installieren."
    exit 1
}

Write-Host "Python Version: $(python --version)" -ForegroundColor Gray

# Prüfe/Installiere MkDocs Dependencies
Write-Host "`nInstalliere/Aktualisiere MkDocs Dependencies..." -ForegroundColor Green
if (Test-Path "requirements-docs.txt") {
    pip install -r requirements-docs.txt --upgrade --quiet
} else {
    Write-Host "requirements-docs.txt nicht gefunden, installiere Basis-Pakete..." -ForegroundColor Yellow
    pip install mkdocs mkdocs-material mkdocs-print-site-plugin --upgrade --quiet
}

# Build Documentation
Write-Host "`nBaue Dokumentation..." -ForegroundColor Green

# Aktiviert PDF-Export-Plugin für strukturierten PDF-Index/Bookmarks
$env:MKDOCS_PDF_EXPORT = "1"
mkdocs build --clean

Write-Host "`nErzeuge strukturiertes PDF (mit Index/Bookmarks)..." -ForegroundColor Green
$pdfPath = Join-Path (Get-Location) "docs/ThemisDB-Documentation.pdf"

# Versuche PDF über Plugin, andernfalls wkhtmltopdf-Fallback
if (Test-Path $pdfPath) {
    Write-Host "✅ PDF erstellt (Plugin): $pdfPath" -ForegroundColor Green
} else {
    Write-Host "Plugin-PDF nicht gefunden, nutze wkhtmltopdf-Fallback..." -ForegroundColor Yellow
    $wkhtml = "C:\Program Files\wkhtmltopdf\bin\wkhtmltopdf.exe"
    if (Test-Path $wkhtml) {
        $src = Join-Path (Get-Location) "site/print_page/index.html"
        if (-not (Test-Path $src)) {
            Write-Host "Druckseite nicht gefunden: $src. Stelle sicher, dass der print-site Plugin aktiv ist." -ForegroundColor Yellow
        } else {
            & $wkhtml --outline --enable-local-file-access --zoom 1.0 "$src" "$pdfPath"
            if ($LASTEXITCODE -eq 0 -and (Test-Path $pdfPath)) {
                Write-Host "✅ PDF erstellt (wkhtmltopdf): $pdfPath" -ForegroundColor Green
            } else {
                Write-Host "❌ wkhtmltopdf fehlgeschlagen." -ForegroundColor Red
            }
        }
    } else {
        Write-Host "wkhtmltopdf nicht installiert. Installiere von https://wkhtmltopdf.org/downloads.html" -ForegroundColor Yellow
    }
}

if ($LASTEXITCODE -eq 0) {
    Write-Host "`n✅ Dokumentation erfolgreich gebaut!" -ForegroundColor Green
    Write-Host "   PDF: docs\\ThemisDB-Documentation.pdf" -ForegroundColor White
    
    # Lösche HTML-Ausgabe nach PDF-Generierung
    Write-Host "`nLösche HTML-Ausgabe (site/) zur Speicherplatzersparnis..." -ForegroundColor Yellow
    if (Test-Path "site") {
        Remove-Item -Recurse -Force "site"
        Write-Host "✅ site/ gelöscht" -ForegroundColor Green
    }
} else {
    Write-Error "Build fehlgeschlagen!"
    exit 1
}
