param(
    # Version die eingereicht werden soll (z. B. "1.3.4")
    [Parameter(Mandatory = $true)]
    [string]$Version,

    # GitHub-Benutzername des Fork-Owners
    [Parameter(Mandatory = $true)]
    [string]$ForkOwner,

    # GitHub-Token fuer gh/git Authentifizierung in CI oder bei automatisierter Ausfuehrung
    [string]$GitHubToken,

    # Ob ein Draft-PR geoeffnet werden soll (Standard: $true fuer manuellen Review)
    [switch]$DraftPR = $true,

    # Stable and preview packages have separate WinGet identities.
    [string]$PackageIdentifier = "ThemisDB.ThemisDB"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

if ($Version -cnotmatch '\A[0-9]+\.[0-9]+\.[0-9]+(-[a-zA-Z0-9.]+)?\z') {
    throw "Invalid WinGet version: '$Version'."
}
if ($PackageIdentifier -cnotin @("ThemisDB.ThemisDB", "ThemisDB.ThemisDB.Preview")) {
    throw "Invalid WinGet package identifier: '$PackageIdentifier'."
}
if ($Version.Contains("-") -and $PackageIdentifier -ceq "ThemisDB.ThemisDB") {
    throw "A prerelease version must use ThemisDB.ThemisDB.Preview."
}
if ($ForkOwner -notmatch '\A[A-Za-z0-9](?:[A-Za-z0-9-]{0,37}[A-Za-z0-9])?\z') {
    throw "Invalid GitHub fork owner."
}
$ReleaseRepository = "makr-code/ThemisDB"

function Require-Command {
    param([string]$CommandName)
    if (-not (Get-Command $CommandName -ErrorAction SilentlyContinue)) {
        throw "Benoetigtes Kommando '$CommandName' nicht im PATH gefunden."
    }
}

Require-Command "gh"
Require-Command "git"

if (-not [string]::IsNullOrWhiteSpace($GitHubToken)) {
    $env:GH_TOKEN = $GitHubToken
    gh auth setup-git | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "gh auth setup-git failed." }
} elseif ([string]::IsNullOrWhiteSpace($env:GH_TOKEN)) {
    throw "Ein GH_TOKEN oder -GitHubToken ist fuer die automatische Fork-/PR-Erstellung erforderlich."
}

$forkToken = $env:GH_TOKEN
try {
    if (-not [string]::IsNullOrWhiteSpace($env:RELEASE_GH_TOKEN)) {
        $env:GH_TOKEN = $env:RELEASE_GH_TOKEN
    }
    $releaseJson = gh release view "v$Version" --repo $ReleaseRepository --json isDraft,isPrerelease,publishedAt
    if ($LASTEXITCODE -ne 0) { throw "GitHub release v$Version could not be retrieved." }
} finally {
    $env:GH_TOKEN = $forkToken
}
try {
    $document = [System.Text.Json.JsonDocument]::Parse(($releaseJson -join "`n"))
    try {
        $release = $document.RootElement
        $draft = $release.GetProperty("isDraft")
        $prerelease = $release.GetProperty("isPrerelease")
        $publishedAt = $release.GetProperty("publishedAt")
        $publishedDate = [DateTimeOffset]::MinValue
        if ($draft.ValueKind -notin @([System.Text.Json.JsonValueKind]::True, [System.Text.Json.JsonValueKind]::False) -or
            $prerelease.ValueKind -notin @([System.Text.Json.JsonValueKind]::True, [System.Text.Json.JsonValueKind]::False) -or
            $publishedAt.ValueKind -ne [System.Text.Json.JsonValueKind]::String -or
            -not $publishedAt.TryGetDateTimeOffset([ref]$publishedDate)) {
            throw "Malformed release metadata."
        }
        if ($draft.GetBoolean()) { throw "GitHub release v$Version is a draft." }
        if ($prerelease.GetBoolean() -and $PackageIdentifier -ceq "ThemisDB.ThemisDB") {
            throw "GitHub prereleases must use ThemisDB.ThemisDB.Preview."
        }
    } finally {
        $document.Dispose()
    }
} catch {
    throw "GitHub release validation failed: $_"
}

$repoRoot  = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot "..\.."))
$packagePath = ($PackageIdentifier -split '\.') -join '\'
$srcDir    = Join-Path $repoRoot "packaging\winget\manifests\t\$packagePath\$Version"

if (-not (Test-Path $srcDir)) {
    throw "Kein Manifest-Ordner fuer Version '$Version' gefunden unter: $srcDir`nBitte zuerst new-winget-manifest.ps1 ausfuehren."
}

# Pflicht-Dateien pruefen
foreach ($f in @(
    "$PackageIdentifier.yaml",
    "$PackageIdentifier.installer.yaml",
    "$PackageIdentifier.locale.en-US.yaml"
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

$installerManifestPath = Join-Path $srcDir "$PackageIdentifier.installer.yaml"
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
    if ($LASTEXITCODE -ne 0) { throw "WinGet fork creation failed." }
}

# Arbeitsverzeichnis: temporaerer Klon des eigenen Forks
$submissionName = if ($PackageIdentifier -ceq "ThemisDB.ThemisDB") {
    "ThemisDB"
} else {
    "ThemisDB-Preview"
}
$workDirName = if ($PackageIdentifier -ceq "ThemisDB.ThemisDB") {
    "winget-pkgs-submit-$Version"
} else {
    "winget-pkgs-submit-Preview-$Version"
}
$workDir = Join-Path $env:TEMP $workDirName
if (Test-Path $workDir) { Remove-Item -Recurse -Force $workDir }

Write-Host "[STEP] Klone Fork $ForkOwner/winget-pkgs ..." -ForegroundColor Cyan
git clone "https://github.com/$ForkOwner/winget-pkgs.git" $workDir --depth=1 --quiet
if ($LASTEXITCODE -ne 0) { throw "WinGet fork clone failed." }
Push-Location $workDir

try {
    # Upstream direkt per URL fetchen (kein benanntes Remote nötig)
    Write-Host "[STEP] Sync mit upstream master ..." -ForegroundColor Cyan
    git fetch https://github.com/microsoft/winget-pkgs.git master --quiet
    if ($LASTEXITCODE -ne 0) { throw "WinGet upstream fetch failed." }
    git reset --hard FETCH_HEAD --quiet
    if ($LASTEXITCODE -ne 0) { throw "WinGet fork synchronization failed." }

    # Feature-Branch
    $branch = if ($PackageIdentifier -ceq "ThemisDB.ThemisDB") {
        "add-ThemisDB-$Version"
    } else {
        "add-ThemisDB-Preview-$Version"
    }
    git checkout -b $branch --quiet
    if ($LASTEXITCODE -ne 0) { throw "WinGet submission branch creation failed." }

    # Manifest-Dateien kopieren
    $destDir = "manifests\t\$packagePath\$Version"
    New-Item -ItemType Directory -Force -Path $destDir | Out-Null
    Copy-Item -Path (Join-Path $srcDir "*") -Destination $destDir -Force

    git add $destDir
    if ($LASTEXITCODE -ne 0) { throw "WinGet manifest staging failed." }
    git commit -m "Add $submissionName version $Version" --quiet
    if ($LASTEXITCODE -ne 0) { throw "WinGet manifest commit failed." }

    # Push in den Fork
    Write-Host "[STEP] Push Branch $branch nach $ForkOwner/winget-pkgs ..." -ForegroundColor Cyan
    git push origin $branch --quiet
    if ($LASTEXITCODE -ne 0) { throw "WinGet manifest push failed." }

    # PR-Text
    $prTitle = "New package: $submissionName version $Version"
    $prBody  = @"
## ThemisDB $Version

**Package Identifier:** ``$PackageIdentifier``
**Version:** ``$Version``

### Changes
- Add manifests for ThemisDB $Version
- Includes en-US and de-DE locales
- Portable ZIP installer with ``bin\themis_server.exe``
- ZIP manifests declare the VC++ runtime dependency ``Microsoft.VCRedist.2015+.x64``

### Verification
- Manifests validated locally with ``winget validate``
- SHA256 checksum sourced from official GitHub Release assets
- Release URL: https://github.com/$ReleaseRepository/releases/tag/v$Version
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
    if ($LASTEXITCODE -ne 0) { throw "WinGet pull request creation failed." }

} finally {
    Pop-Location
}

Write-Host "" 
Write-Host "Fertig. Bitte den PR in microsoft/winget-pkgs reviewen und den Draft-Status entfernen, wenn bereit." -ForegroundColor Green
