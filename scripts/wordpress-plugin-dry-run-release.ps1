param(
    [Parameter(Mandatory = $true)]
    [string]$PluginSlug,

    [Parameter(Mandatory = $false)]
    [string]$Version,

    [Parameter(Mandatory = $false)]
    [string]$TargetRef = "main"
)

$ErrorActionPreference = 'Stop'

function Get-PluginMainFile {
    param(
        [string]$PluginDir,
        [string]$Slug
    )

    $defaultMain = Join-Path $PluginDir "$Slug.php"
    if (Test-Path $defaultMain) {
        return $defaultMain
    }

    if ($Slug -eq 'persistent-podcast-player') {
        $fallbackMain = Join-Path $PluginDir 'persistent-podcast-player.php'
        if (Test-Path $fallbackMain) {
            return $fallbackMain
        }
    }

    return $null
}

function Assert-PathExists {
    param(
        [string]$Path,
        [string]$Label
    )

    if (-not (Test-Path $Path)) {
        throw "$Label fehlt: $Path"
    }
}

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$nestedPluginDir = Join-Path $repoRoot "wordpress-plugins/$PluginSlug"
$flatPluginDir = Join-Path $repoRoot $PluginSlug

if (Test-Path $flatPluginDir) {
    $pluginDir = $flatPluginDir
    $sharedUpdater = Join-Path $repoRoot 'includes/class-themisdb-plugin-updater.php'
    $pluginRelative = $PluginSlug
}
elseif (Test-Path $nestedPluginDir) {
    $pluginDir = $nestedPluginDir
    $sharedUpdater = Join-Path $repoRoot 'wordpress-plugins/includes/class-themisdb-plugin-updater.php'
    $pluginRelative = "wordpress-plugins/$PluginSlug"
}
else {
    throw "Plugin-Verzeichnis fehlt im Flat- oder Nested-Layout: $PluginSlug"
}

$metadataPath = Join-Path $pluginDir 'update-info.json'

Assert-PathExists -Path $pluginDir -Label 'Plugin-Verzeichnis'
Assert-PathExists -Path $metadataPath -Label 'update-info.json'
Assert-PathExists -Path $sharedUpdater -Label 'Gemeinsamer Updater'

$mainFile = Get-PluginMainFile -PluginDir $pluginDir -Slug $PluginSlug
if (-not $mainFile) {
    throw "Main-Plugin-Datei nicht gefunden fuer '$PluginSlug'."
}

$metadata = Get-Content $metadataPath -Raw | ConvertFrom-Json
$metadataVersion = [string]$metadata.version
if ([string]::IsNullOrWhiteSpace($metadataVersion)) {
    throw "Version in update-info.json ist leer."
}

if ([string]::IsNullOrWhiteSpace($Version)) {
    $Version = $metadataVersion
}

if ($metadataVersion -ne $Version) {
    throw "Versionskonflikt: update-info.json=$metadataVersion, dry-run version=$Version"
}

$tagName = "$PluginSlug/v$Version"

$distDir = Join-Path $repoRoot 'dist'
if (-not (Test-Path $distDir)) {
    New-Item -ItemType Directory -Path $distDir | Out-Null
}

$zipPath = Join-Path $distDir "$PluginSlug-dryrun.zip"
if (Test-Path $zipPath) {
    Remove-Item $zipPath -Force
}

$tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("themis-wp-dryrun-" + [System.Guid]::NewGuid().ToString('N'))
$tempPluginRoot = Join-Path $tempRoot $PluginSlug
New-Item -ItemType Directory -Path $tempPluginRoot -Force | Out-Null

Copy-Item -Path (Join-Path $pluginDir '*') -Destination $tempPluginRoot -Recurse -Force

$tempUpdaterDir = Join-Path $tempPluginRoot 'includes'
if (-not (Test-Path $tempUpdaterDir)) {
    New-Item -ItemType Directory -Path $tempUpdaterDir -Force | Out-Null
}
Copy-Item -Path $sharedUpdater -Destination (Join-Path $tempUpdaterDir 'class-themisdb-plugin-updater.php') -Force

Compress-Archive -Path $tempPluginRoot -DestinationPath $zipPath -Force

Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = [System.IO.Compression.ZipFile]::OpenRead($zipPath)
try {
    $entryNames = $zip.Entries | ForEach-Object { $_.FullName }
    $updaterEntry = "$PluginSlug/includes/class-themisdb-plugin-updater.php"
    $metadataEntry = "$PluginSlug/update-info.json"

    $hasUpdater = $entryNames -contains $updaterEntry
    $hasMetadata = $entryNames -contains $metadataEntry

    if (-not $hasUpdater) {
        throw "ZIP-Validierung fehlgeschlagen: Updater fehlt ($updaterEntry)."
    }

    if (-not $hasMetadata) {
        throw "ZIP-Validierung fehlgeschlagen: update-info.json fehlt ($metadataEntry)."
    }

    $releaseCmd = "gh release create `"$tagName`" --title `"$PluginSlug v$Version`" --notes-file `"$pluginRelative/CHANGELOG.md`""

    Write-Output "DRY_RUN_OK"
    Write-Output "plugin_slug=$PluginSlug"
    Write-Output "version=$Version"
    Write-Output "target_ref=$TargetRef"
    Write-Output "main_file=$mainFile"
    Write-Output "zip_file=$zipPath"
    Write-Output "zip_contains_updater=$hasUpdater"
    Write-Output "zip_contains_update_info=$hasMetadata"
    Write-Output "would_create_tag=$tagName"
    Write-Output "would_run=$releaseCmd"
}
finally {
    $zip.Dispose()
    if (Test-Path $tempRoot) {
        Remove-Item $tempRoot -Recurse -Force
    }
}
