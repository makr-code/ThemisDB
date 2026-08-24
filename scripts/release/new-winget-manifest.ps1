param(
    [Parameter(Mandatory = $true)]
    [string]$Version,

    [Parameter(Mandatory = $true)]
    [string]$InstallerUrl,

    [Parameter(Mandatory = $true)]
    [string]$InstallerSha256,

    [ValidateSet("zip", "msi")]
    [string]$InstallerType = "zip",

    [string]$PackageIdentifier = "ThemisDB.ThemisDB",
    [string]$PackageName = "ThemisDB",
    [string]$Publisher = "ThemisDB Team",
    [string]$PublisherUrl = "https://github.com/makr-code/ThemisDB",
    [string]$PublisherSupportUrl = "https://github.com/makr-code/ThemisDB/issues",
    [string]$PackageUrl = "https://github.com/makr-code/ThemisDB",
    [string]$Moniker = "themisdb",
    [string]$License = "MIT",
    [string]$ShortDescription = "Multi-model database system with ACID transactions",
    [string]$PortableRelativeFilePath = "bin\themis_server.exe",
    [string]$PortableCommandAlias = "themis_server",
    [string[]]$PackageDependencies = @(),
    [string]$ReleaseNotes = "",
    [switch]$IncludeGermanLocale,
    [string]$GermanShortDescription = "Multi-Modell-Datenbanksystem mit ACID-Transaktionen",
    [string]$GermanReleaseNotes = "",
    [string]$OutputRoot = "packaging/winget/manifests"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

function Assert-VersionFormat {
    param([string]$Value)

    if ($Value -notmatch '^\d+\.\d+\.\d+([-.][0-9A-Za-z.-]+)?$') {
        throw "Version '$Value' has invalid format. Expected MAJOR.MINOR.PATCH[-suffix]."
    }
}

function Assert-Sha256Format {
    param([string]$Value)

    if ($Value -notmatch '^[A-Fa-f0-9]{64}$') {
        throw "InstallerSha256 must be a 64 character hexadecimal SHA256 string."
    }
}

function Get-ZipRootFolderName {
    param([string]$Url)

    try {
        $uri = [System.Uri]::new($Url)
    } catch {
        throw "InstallerUrl '$Url' is not a valid absolute URL."
    }

    $fileName = [System.IO.Path]::GetFileName($uri.AbsolutePath)
    if ([string]::IsNullOrWhiteSpace($fileName)) {
        throw "InstallerUrl '$Url' does not contain a ZIP file name."
    }

    $rootName = [System.IO.Path]::GetFileNameWithoutExtension($fileName)
    if ([string]::IsNullOrWhiteSpace($rootName)) {
        throw "InstallerUrl '$Url' does not yield a valid ZIP root folder name."
    }

    return $rootName
}

function ConvertTo-YamlScalar {
    param([string]$Value)

    if ($null -eq $Value) {
        return "''"
    }

    $escaped = $Value.Replace("'", "''")
    return "'$escaped'"
}

function ConvertTo-YamlBlock {
    param([string]$Value)

    if ([string]::IsNullOrWhiteSpace($Value)) {
        return "''"
    }

    $normalized = ($Value -replace "`r`n", "`n") -replace "`r", "`n"
    $lines = $normalized -split "`n"
    $indented = $lines | ForEach-Object { "  $_" }
    return "|-`n" + ($indented -join "`n")
}

Assert-VersionFormat -Value $Version
Assert-Sha256Format -Value $InstallerSha256

# Pre-release versions are represented via the package version string (e.g. 2.4.0-alpha).
# Winget does not accept an IsPreRelease field in the installer manifest schema.
$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot "..\.."))
$manifestRoot = if ([System.IO.Path]::IsPathRooted($OutputRoot)) {
    [System.IO.Path]::GetFullPath($OutputRoot)
} else {
    [System.IO.Path]::GetFullPath((Join-Path $repoRoot $OutputRoot))
}
$packagePath = $PackageIdentifier -split '\.'
$manifestDir = Join-Path $manifestRoot ((@($PackageIdentifier.Substring(0, 1).ToLowerInvariant()) + $packagePath + @($Version)) -join '\')

New-Item -ItemType Directory -Force -Path $manifestDir | Out-Null

$releaseTag = if ($Version.StartsWith("v")) { $Version } else { "v$Version" }
$licenseUrl = "https://github.com/makr-code/ThemisDB/blob/$releaseTag/LICENSE"
$releaseNotesUrl = "https://github.com/makr-code/ThemisDB/releases/tag/$releaseTag"
$author = $Publisher
$copyright = "Copyright (c) 2026 ThemisDB Team"

$versionManifestPath = Join-Path $manifestDir "$PackageIdentifier.yaml"
$installerManifestPath = Join-Path $manifestDir "$PackageIdentifier.installer.yaml"
$localeManifestPath = Join-Path $manifestDir "$PackageIdentifier.locale.en-US.yaml"
$germanLocaleManifestPath = Join-Path $manifestDir "$PackageIdentifier.locale.de-DE.yaml"

$versionManifest = @"
PackageIdentifier: $PackageIdentifier
PackageVersion: $Version
DefaultLocale: en-US
ManifestType: version
ManifestVersion: 1.6.0
"@

$installerLines = @(
    "PackageIdentifier: $PackageIdentifier",
    "PackageVersion: $Version",
    "Installers:",
    "- Architecture: x64",
    "  InstallerType: $InstallerType",
    "  InstallerUrl: $InstallerUrl",
    "  InstallerSha256: $($InstallerSha256.ToUpperInvariant())"
)

if ($InstallerType -eq "zip") {
    if ($PackageDependencies.Count -eq 0) {
        $PackageDependencies = @("Microsoft.VCRedist.2015+.x64")
    }

    $portableRelativePath = $PortableRelativeFilePath
    $zipRootFolder = Get-ZipRootFolderName -Url $InstallerUrl
    if (-not $portableRelativePath.StartsWith($zipRootFolder, [System.StringComparison]::OrdinalIgnoreCase)) {
        $portableRelativePath = Join-Path $zipRootFolder $portableRelativePath
    }

    $installerLines += @(
        "  NestedInstallerType: portable",
        "  NestedInstallerFiles:",
        "  - RelativeFilePath: $portableRelativePath",
        "    PortableCommandAlias: $PortableCommandAlias"
    )

    if ($PackageDependencies.Count -gt 0) {
        $installerLines += @(
            "  Dependencies:",
            "    PackageDependencies:"
        )

        foreach ($dependency in $PackageDependencies) {
            $installerLines += "    - PackageIdentifier: $dependency"
        }
    }
}

$installerLines += @(
    "ManifestType: installer",
    "ManifestVersion: 1.6.0"
)

$installerManifest = $installerLines -join "`n"

$releaseNotesBlock = ConvertTo-YamlBlock -Value $ReleaseNotes
$germanReleaseNotesText = if ([string]::IsNullOrWhiteSpace($GermanReleaseNotes)) {
        $ReleaseNotes
} else {
        $GermanReleaseNotes
}
$germanReleaseNotesBlock = ConvertTo-YamlBlock -Value $germanReleaseNotesText

$localeManifest = @"
PackageIdentifier: $PackageIdentifier
PackageVersion: $Version
PackageLocale: en-US
Publisher: $(ConvertTo-YamlScalar -Value $Publisher)
PublisherUrl: $(ConvertTo-YamlScalar -Value $PublisherUrl)
PublisherSupportUrl: $(ConvertTo-YamlScalar -Value $PublisherSupportUrl)
Author: $(ConvertTo-YamlScalar -Value $author)
PackageName: $(ConvertTo-YamlScalar -Value $PackageName)
PackageUrl: $(ConvertTo-YamlScalar -Value $PackageUrl)
License: $(ConvertTo-YamlScalar -Value $License)
LicenseUrl: $(ConvertTo-YamlScalar -Value $licenseUrl)
Copyright: $(ConvertTo-YamlScalar -Value $copyright)
ShortDescription: $(ConvertTo-YamlScalar -Value $ShortDescription)
Description: |-
  ThemisDB is a high-performance multi-model database system that supports:
  
  - Relational data with secondary indexes and hybrid query execution
  - Graph traversals across transactional storage
  - Vector search for semantic workloads
  - Time-series analytics and document storage
  - ACID transactions with MVCC and snapshot isolation
  - Operational observability with metrics and tracing
Moniker: $(ConvertTo-YamlScalar -Value $Moniker)
Tags:
- database
- multi-model
- graph
- vector
- timeseries
- nosql
- acid
- mvcc
- aql
ReleaseNotes: $releaseNotesBlock
ReleaseNotesUrl: $(ConvertTo-YamlScalar -Value $releaseNotesUrl)
ManifestType: defaultLocale
ManifestVersion: 1.6.0
"@

$germanLocaleManifest = @"
PackageIdentifier: $PackageIdentifier
PackageVersion: $Version
PackageLocale: de-DE
Publisher: $(ConvertTo-YamlScalar -Value $Publisher)
PublisherUrl: $(ConvertTo-YamlScalar -Value $PublisherUrl)
PublisherSupportUrl: $(ConvertTo-YamlScalar -Value $PublisherSupportUrl)
Author: $(ConvertTo-YamlScalar -Value $author)
PackageName: $(ConvertTo-YamlScalar -Value $PackageName)
PackageUrl: $(ConvertTo-YamlScalar -Value $PackageUrl)
License: $(ConvertTo-YamlScalar -Value $License)
LicenseUrl: $(ConvertTo-YamlScalar -Value $licenseUrl)
Copyright: $(ConvertTo-YamlScalar -Value $copyright)
ShortDescription: $(ConvertTo-YamlScalar -Value $GermanShortDescription)
Description: |-
  ThemisDB ist ein hochperformantes Multi-Modell-Datenbanksystem mit:
  
  - Relationalen Daten und hybrider Abfrageausfuehrung
  - Graph-Traversalen auf transaktionalem Speicher
  - Vektorsuche fuer semantische Workloads
  - Zeitreihenanalyse und Dokumentenspeicherung
  - ACID-Transaktionen mit MVCC und Snapshot Isolation
  - Betriebsmetriken und Tracing fuer Observability
ReleaseNotes: $germanReleaseNotesBlock
ReleaseNotesUrl: $(ConvertTo-YamlScalar -Value $releaseNotesUrl)
ManifestType: locale
ManifestVersion: 1.6.0
"@

Set-Content -Path $versionManifestPath -Value $versionManifest -Encoding utf8NoBOM
Set-Content -Path $installerManifestPath -Value $installerManifest -Encoding utf8NoBOM
Set-Content -Path $localeManifestPath -Value $localeManifest -Encoding utf8NoBOM
if ($IncludeGermanLocale) {
    Set-Content -Path $germanLocaleManifestPath -Value $germanLocaleManifest -Encoding utf8NoBOM
}

Write-Host "Generated WinGet manifests:" -ForegroundColor Green
Write-Host "  $versionManifestPath"
Write-Host "  $installerManifestPath"
Write-Host "  $localeManifestPath"
if ($IncludeGermanLocale) {
    Write-Host "  $germanLocaleManifestPath"
}