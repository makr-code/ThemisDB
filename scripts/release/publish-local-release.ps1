param(
    [Parameter(Mandatory = $true)]
    [string]$Tag,

    [string]$ConfigurePreset = "windows-release",
    [string]$BuildPreset = "windows-release",
    [string]$TestPreset = "windows-release",
    [string]$Edition = "COMMUNITY",
    [string]$Triplet = "x64-windows",
    [string]$BinaryDir = "build-msvc-windows-release",

    [switch]$AllowPreRelease,
    [switch]$IncludeDevelopmentInZip,
    [switch]$SkipTests,

    [switch]$GenerateWingetManifest,
    [ValidateSet("zip", "msi")]
    [string]$WingetInstallerType = "zip",
    [switch]$IncludeGermanWingetLocale,
    [string[]]$WingetPackageDependencies = @("Microsoft.VCRedist.2015+.x64"),

    [switch]$PublishGitHub,
    [switch]$PublishDocker,
    [string]$GitHubRepo = "",
    [string]$DockerImage = "themisdb/themisdb"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

function Invoke-Step {
    param(
        [string]$Name,
        [scriptblock]$Action
    )

    Write-Host "[STEP] $Name" -ForegroundColor Cyan
    & $Action
    Write-Host "[ OK ] $Name" -ForegroundColor Green
}

function Require-Command {
    param([string]$CommandName)

    if (-not (Get-Command $CommandName -ErrorAction SilentlyContinue)) {
        throw "Required command '$CommandName' not found in PATH."
    }
}

function Resolve-GitHubRepoFromOrigin {
    $origin = git remote get-url origin
    if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($origin)) {
        throw "Could not resolve origin URL. Pass -GitHubRepo owner/repo explicitly."
    }

    if ($origin -match "github\.com[:/](.+?)(\.git)?$") {
        return $matches[1]
    }

    throw "Origin '$origin' is not a GitHub URL. Pass -GitHubRepo owner/repo explicitly."
}

function New-Sha256File {
    param([string]$InputFile)

    $hash = Get-FileHash -Algorithm SHA256 -Path $InputFile
    $line = "{0}  {1}" -f $hash.Hash.ToLowerInvariant(), [System.IO.Path]::GetFileName($InputFile)
    $outFile = "$InputFile.sha256"
    Set-Content -Path $outFile -Value $line -NoNewline -Encoding ASCII
    return $outFile
}

function New-Sbom {
    param([string]$InputFile)

    Require-Command "syft"
    $outFile = "$InputFile.sbom.cdx.json"
    & syft "file:$InputFile" -o cyclonedx-json | Set-Content -Path $outFile -Encoding UTF8
    if ($LASTEXITCODE -ne 0) {
        throw "SBOM generation failed for '$InputFile'."
    }
    return $outFile
}

function New-Signature {
    param([string]$InputFile)

    Require-Command "gpg"
    $outFile = "$InputFile.asc"
    & gpg --batch --yes --armor --detach-sign --output $outFile $InputFile
    if ($LASTEXITCODE -ne 0) {
        throw "GPG signing failed for '$InputFile'."
    }
    return $outFile
}

if (-not ($Tag -match '^v\d+\.\d+\.\d+(-[0-9A-Za-z.-]+)?$')) {
    throw "Tag '$Tag' has invalid format. Expected vMAJOR.MINOR.PATCH[-suffix]."
}

$isPreRelease = $Tag -match '-'
if ($isPreRelease -and -not $AllowPreRelease) {
    throw "Pre-release tags are blocked by default. Use -AllowPreRelease to publish pre-releases."
}

$versionFromTag = $Tag.TrimStart('v')
$versionFile = Join-Path $PSScriptRoot "..\..\VERSION"
$versionFile = [System.IO.Path]::GetFullPath($versionFile)
$versionValue = (Get-Content -Path $versionFile -Raw).Trim()
if ($versionValue -ne $versionFromTag) {
    throw "VERSION mismatch. VERSION='$versionValue', Tag='$Tag'. Update VERSION before release."
}

Require-Command "cmake"
Require-Command "git"

$vcpkgExe = Join-Path $PSScriptRoot "..\..\vcpkg\vcpkg.exe"
$vcpkgExe = [System.IO.Path]::GetFullPath($vcpkgExe)
if (-not (Test-Path $vcpkgExe)) {
    throw "vcpkg executable not found at '$vcpkgExe'."
}

Invoke-Step "vcpkg install to vcpkg_installed/$Triplet" {
    & $vcpkgExe install --triplet $Triplet
    if ($LASTEXITCODE -ne 0) {
        throw "vcpkg install failed."
    }
}

Invoke-Step "Configure preset $ConfigurePreset" {
    & cmake --preset $ConfigurePreset "-DTHEMIS_EDITION=$Edition" "-DTHEMIS_ZIP_INCLUDE_DEVELOPMENT=$($IncludeDevelopmentInZip.IsPresent)"
    if ($LASTEXITCODE -ne 0) {
        throw "cmake configure failed."
    }
}

Invoke-Step "Build preset $BuildPreset" {
    & cmake --build --preset $BuildPreset --parallel
    if ($LASTEXITCODE -ne 0) {
        throw "cmake build failed."
    }
}

if (-not $SkipTests) {
    Invoke-Step "Test preset $TestPreset" {
        & ctest --preset $TestPreset --output-on-failure
        if ($LASTEXITCODE -ne 0) {
            throw "ctest failed."
        }
    }
}

Invoke-Step "Build deployable ZIP package" {
    & cmake --build $BinaryDir --target package-zip --parallel
    if ($LASTEXITCODE -ne 0) {
        throw "package-zip failed."
    }
}

$releaseDir = Join-Path $PSScriptRoot "..\..\releases"
$releaseDir = [System.IO.Path]::GetFullPath($releaseDir)
$zipPattern = "ThemisDB-$Edition-$versionFromTag-windows-x64.zip"
$zipFile = Join-Path $releaseDir $zipPattern
if (-not (Test-Path $zipFile)) {
    throw "Expected ZIP artifact not found: $zipFile"
}

$shaFile = $null
$sbomFile = $null
$sigFile = $null
$wingetManifestDir = $null
$wingetArtifactFile = $zipFile

Invoke-Step "Generate SHA256" {
    $shaFile = New-Sha256File -InputFile $zipFile
}

Invoke-Step "Generate CycloneDX SBOM" {
    $sbomFile = New-Sbom -InputFile $zipFile
}

Invoke-Step "Generate GPG signature" {
    $sigFile = New-Signature -InputFile $zipFile
}

if ($GenerateWingetManifest) {
    $wingetScript = Join-Path $PSScriptRoot "new-winget-manifest.ps1"
    if (-not (Test-Path $wingetScript)) {
        throw "WinGet manifest generator not found at '$wingetScript'."
    }

    if ($WingetInstallerType -eq "msi") {
        Invoke-Step "Build deployable MSI package" {
            & cmake --build $BinaryDir --target package-msi --parallel
            if ($LASTEXITCODE -ne 0) {
                throw "package-msi failed."
            }
        }

        $msiPattern = "ThemisDB-$Edition-$versionFromTag-windows-x64.msi"
        $wingetArtifactFile = Join-Path $releaseDir $msiPattern
        if (-not (Test-Path $wingetArtifactFile)) {
            throw "Expected MSI artifact not found: $wingetArtifactFile"
        }
    }

    $releaseUrl = "https://github.com/makr-code/ThemisDB/releases/download/$Tag/$([System.IO.Path]::GetFileName($wingetArtifactFile))"
    $installerSha256 = (Get-FileHash -Algorithm SHA256 -Path $wingetArtifactFile).Hash

    Invoke-Step "Generate WinGet manifests" {
        $wingetArgumentList = @(
            "-Version", $versionFromTag,
            "-InstallerUrl", $releaseUrl,
            "-InstallerSha256", $installerSha256,
            "-InstallerType", $WingetInstallerType,
            "-ReleaseNotes", "Local release build for $Tag."
        )
        if ($WingetInstallerType -eq "zip" -and $WingetPackageDependencies.Count -gt 0) {
            $wingetArgumentList += @("-PackageDependencies")
            $wingetArgumentList += $WingetPackageDependencies
        }
        if ($IncludeGermanWingetLocale) {
            $wingetArgumentList += "-IncludeGermanLocale"
        }

        & $wingetScript @wingetArgumentList
        if ($LASTEXITCODE -ne 0) {
            throw "WinGet manifest generation failed."
        }
    }

    $wingetManifestDir = Join-Path $PSScriptRoot "..\..\packaging\winget\manifests\t\ThemisDB\ThemisDB\$versionFromTag"
    $wingetManifestDir = [System.IO.Path]::GetFullPath($wingetManifestDir)
}

if ($PublishGitHub) {
    Require-Command "gh"
    if ([string]::IsNullOrWhiteSpace($GitHubRepo)) {
        $GitHubRepo = Resolve-GitHubRepoFromOrigin
    }

    $notes = "Local best-practice release package (no CI/CD)."

    Invoke-Step "Publish GitHub release" {
        $ghArgs = @(
            "release", "create", $Tag,
            "--repo", $GitHubRepo,
            "--title", $Tag,
            "--notes", $notes,
            $zipFile,
            $shaFile,
            $sbomFile,
            $sigFile
        )

        if ($isPreRelease) {
            $ghArgs += "--prerelease"
        }

        & gh @ghArgs
        if ($LASTEXITCODE -ne 0) {
            throw "GitHub release publish failed."
        }
    }
}

if ($PublishDocker) {
    Require-Command "docker"

    $baseTag = $versionFromTag
    $minorTag = ($versionFromTag -split '\.')[0..1] -join "."

    $tags = @(
        "${DockerImage}:$baseTag",
        "${DockerImage}:$minorTag"
    )

    if (-not $isPreRelease) {
        $tags += "${DockerImage}:latest"
    }

    Invoke-Step "Build Docker image" {
        $buildArgs = @("buildx", "build", ".",
            "--file", "docker/Dockerfile.unified",
            "--platform", "linux/amd64",
            "--build-arg", "THEMIS_EDITION=COMMUNITY",
            "--load")
        foreach ($t in $tags) {
            $buildArgs += @("-t", $t)
        }

        & docker @buildArgs
        if ($LASTEXITCODE -ne 0) {
            throw "Docker buildx build failed."
        }
    }

    foreach ($t in $tags) {
        Invoke-Step "Push Docker tag $t" {
            & docker push $t
            if ($LASTEXITCODE -ne 0) {
                throw "Docker push failed for $t"
            }
        }
    }
}

Write-Host "" 
Write-Host "Release completed successfully." -ForegroundColor Green
Write-Host "ZIP: $zipFile"
Write-Host "SHA256: $shaFile"
Write-Host "SBOM: $sbomFile"
Write-Host "Signature: $sigFile"
if ($wingetManifestDir) {
    Write-Host "WinGet manifests: $wingetManifestDir"
}
