param(
    # Version die eingereicht werden soll (z. B. "1.3.4")
    [Parameter(Mandatory = $true)]
    [string]$Version,

    # GitHub-Benutzername des Fork-Owners
    [Parameter(Mandatory = $true)]
    [string]$ForkOwner,

    # Ob ein Draft-PR geoeffnet werden soll (Standard: $true fuer manuellen Review)
    [switch]$DraftPR = $true
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

function Require-Command {
    param([string]$CommandName)
    if (-not (Get-Command $CommandName -ErrorAction SilentlyContinue)) {
        throw "Benoetigtes Kommando '$CommandName' nicht im PATH gefunden."
    }
}

Require-Command "gh"
Require-Command "git"

$repoRoot  = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot "..\.."))
$srcDir    = Join-Path $repoRoot "packaging\winget\manifests\t\ThemisDB\ThemisDB\$Version"

if (-not (Test-Path $srcDir)) {
    throw "Kein Manifest-Ordner fuer Version '$Version' gefunden unter: $srcDir`nBitte zuerst new-winget-manifest.ps1 ausfuehren."
}

# Pflicht-Dateien pruefen
foreach ($f in @(
    "ThemisDB.ThemisDB.yaml",
    "ThemisDB.ThemisDB.installer.yaml",
    "ThemisDB.ThemisDB.locale.en-US.yaml"
)) {
    if (-not (Test-Path (Join-Path $srcDir $f))) {
        throw "Pflichtdatei fehlt: $f"
    }
}

# winget validate als Sicherheits-Gate
Write-Host "[GATE] winget validate ..." -ForegroundColor Cyan
winget validate --manifest $srcDir
if ($LASTEXITCODE -ne 0) {
    throw "winget validate gescheitert. Keine Einreichung."
}
Write-Host "[ OK ] Manifest valide" -ForegroundColor Green

$installerManifestPath = Join-Path $srcDir "ThemisDB.ThemisDB.installer.yaml"
if (-not (Test-Path $installerManifestPath)) {
    throw "Installer manifest fehlt: $installerManifestPath"
}

$installerManifestContent = Get-Content -Path $installerManifestPath -Raw
if ($installerManifestContent -match '(?m)^\s*InstallerType:\s*zip\s*$' -and
    $installerManifestContent -notmatch '(?m)^\s*-\s*PackageIdentifier:\s*Microsoft\.VCRedist\.2015\+\.x64\s*$') {
    throw "ZIP-WinGet-Manifeste muessen die VC++ Runtime-Abhaengigkeit Microsoft.VCRedist.2015+.x64 deklarieren."
}

# Fork sicherstellen
Write-Host "[INFO] Fork von microsoft/winget-pkgs wird geprueft/erstellt ..." -ForegroundColor Cyan
$forkExists = gh repo view "$ForkOwner/winget-pkgs" 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "[INFO] Fork nicht gefunden, wird erstellt ..." -ForegroundColor Yellow
    gh repo fork microsoft/winget-pkgs --clone=false
}

# Arbeitsverzeichnis: temporaerer Klon des eigenen Forks
$workDir = Join-Path $env:TEMP "winget-pkgs-submit-$Version"
if (Test-Path $workDir) { Remove-Item -Recurse -Force $workDir }

Write-Host "[STEP] Klone Fork $ForkOwner/winget-pkgs ..." -ForegroundColor Cyan
git clone "https://github.com/$ForkOwner/winget-pkgs.git" $workDir --depth=1 --quiet
Push-Location $workDir

try {
    # Upstream direkt per URL fetchen (kein benanntes Remote nötig)
    Write-Host "[STEP] Sync mit upstream master ..." -ForegroundColor Cyan
    git fetch https://github.com/microsoft/winget-pkgs.git master --quiet
    git reset --hard FETCH_HEAD --quiet

    # Feature-Branch
    $branch = "add-ThemisDB-$Version"
    git checkout -b $branch --quiet

    # Manifest-Dateien kopieren
    $destDir = "manifests\t\ThemisDB\ThemisDB\$Version"
    New-Item -ItemType Directory -Force -Path $destDir | Out-Null
    Copy-Item -Path (Join-Path $srcDir "*") -Destination $destDir -Force

    git add $destDir
    git commit -m "Add ThemisDB version $Version" --quiet

    # Push in den Fork
    Write-Host "[STEP] Push Branch $branch nach $ForkOwner/winget-pkgs ..." -ForegroundColor Cyan
    git push origin $branch --quiet

    # PR-Text
    $prTitle = "New package: ThemisDB version $Version"
    $prBody  = @"
## ThemisDB $Version

**Package Identifier:** ``ThemisDB.ThemisDB``
**Version:** ``$Version``

### Changes
- Add manifests for ThemisDB $Version
- Includes en-US and de-DE locales
- Portable ZIP installer with ``bin\themis_server.exe``
- ZIP manifests declare the VC++ runtime dependency ``Microsoft.VCRedist.2015+.x64``

### Verification
- Manifests validated locally with ``winget validate``
- SHA256 checksum sourced from official GitHub Release assets
- Release URL: https://github.com/makr-code/ThemisDB/releases/tag/v$Version
- Installer manifest contains the portable ZIP root folder path and runtime dependency

### Package Information
ThemisDB is a high-performance multi-model database system with ACID transactions, graph traversals, vector search, time-series analytics, and a hybrid query language (AQL).

### Checklist
- [x] ``winget validate --manifest`` passed without errors
- [x] SHA256 hash matches the published release artifact
- [x] Installer URL is publicly accessible
- [x] Locale files (en-US, de-DE) are present
"@

    $prFlags = @("pr", "create",
        "--repo", "microsoft/winget-pkgs",
        "--head", "$ForkOwner`:$branch",
        "--base", "master",
        "--title", $prTitle,
        "--body", $prBody)
    if ($DraftPR) { $prFlags += "--draft" }

    Write-Host "[STEP] Erstelle Pull Request ..." -ForegroundColor Cyan
    gh @prFlags

} finally {
    Pop-Location
}

Write-Host "" 
Write-Host "Fertig. Bitte den PR in microsoft/winget-pkgs reviewen und den Draft-Status entfernen, wenn bereit." -ForegroundColor Green
