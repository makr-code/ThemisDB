# Exportiert die MkDocs Druckseite zu PDF mit Inhaltsverzeichnis/Bookmarks via wkhtmltopdf
param(
    [string]$SourceHtml = ".\site\print_page\index.html",
    [string]$OutputPdf = ".\docs\ThemisDB-Documentation.pdf",
    [string]$WkhtmlPath = "C:\\Program Files\\wkhtmltopdf\\bin\\wkhtmltopdf.exe"
)

Write-Host "=== Export PDF (wkhtmltopdf) ===" -ForegroundColor Cyan

if (-not (Test-Path $SourceHtml)) {
    Write-Error "Quelle nicht gefunden: $SourceHtml. Bitte zuerst mkdocs build mit print-site ausfuehren."
    exit 1
}

if (-not (Test-Path $WkhtmlPath)) {
    Write-Error "wkhtmltopdf nicht gefunden unter: $WkhtmlPath"
    Write-Host "Installiere von: https://wkhtmltopdf.org/downloads.html" -ForegroundColor Yellow
    exit 1
}

# Erzeuge PDF mit Outline (Bookmarks) und lokalem Dateizugriff
& $WkhtmlPath --outline --enable-local-file-access --zoom 1.0 "$SourceHtml" "$OutputPdf"

if ($LASTEXITCODE -eq 0 -and (Test-Path $OutputPdf)) {
    Write-Host "✅ PDF erstellt: $OutputPdf" -ForegroundColor Green
} else {
    Write-Error "PDF-Erstellung fehlgeschlagen."
    exit 1
}
