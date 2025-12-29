param(
    [string]$Version = "1.3.4",
    [switch]$SkipBuild = $true
)

$ErrorActionPreference = "Stop"
$RootDir = "C:\VCC\themis"
$BuildDir = Join-Path $RootDir "build-msvc\Release"
$ReleaseDir = Join-Path $RootDir "release\v$Version"

Set-Location $RootDir

Write-Host "================================================" -ForegroundColor Cyan
Write-Host " ThemisDB v$Version Release Pipeline" -ForegroundColor Cyan
Write-Host "================================================" -ForegroundColor Cyan
Write-Host ""

# Step 1: Build check
if (-not $SkipBuild) {
    Write-Host "[1/6] Building Release..." -ForegroundColor Green
    Push-Location "build-msvc"
    cmake --build . --config Release --parallel 8
    if ($LASTEXITCODE -ne 0) {
        Pop-Location
        throw "Build failed"
    }
    Pop-Location
    Write-Host "  OK Build completed" -ForegroundColor Gray
} else {
    Write-Host "[1/6] Using existing build..." -ForegroundColor Yellow
}

# Step 2: Create directories
Write-Host "`n[2/6] Creating release structure..." -ForegroundColor Green

$binDir = Join-Path $ReleaseDir "windows-x64\bin"
$docsDir = Join-Path $ReleaseDir "windows-x64\docs"
$configDir = Join-Path $ReleaseDir "windows-x64\config"

foreach ($dir in @($binDir, $docsDir, $configDir)) {
    if (Test-Path $dir) {
        Remove-Item $dir -Recurse -Force
    }
    New-Item -ItemType Directory -Path $dir -Force | Out-Null
}

Write-Host "  OK Directory structure created" -ForegroundColor Gray

# Step 3: Copy binaries
Write-Host "`n[3/6] Copying binaries..." -ForegroundColor Green

$binaries = @(
    "themis_server.exe",
    "themis_cli.exe",
    "themis_demo.exe"
)

$copiedCount = 0
foreach ($bin in $binaries) {
    $src = Join-Path $BuildDir $bin
    if (Test-Path $src) {
        Copy-Item $src $binDir -Force
        Write-Host "  + $bin" -ForegroundColor Gray
        $copiedCount++
    }
}

# Copy DLLs
$dlls = Get-ChildItem $BuildDir -Filter "*.dll" -ErrorAction SilentlyContinue
if ($dlls) {
    $dlls | ForEach-Object {
        Copy-Item $_.FullName $binDir -Force
    }
    Write-Host "  + $($dlls.Count) DLL files" -ForegroundColor Gray
    $copiedCount += $dlls.Count
}

Write-Host "  OK Copied $copiedCount files" -ForegroundColor Gray

# Step 4: Copy documentation
Write-Host "`n[4/6] Copying documentation..." -ForegroundColor Green

$docs = @(
    @{Src="README.md"; Dst="README.md"},
    @{Src="CHANGELOG.md"; Dst="CHANGELOG.md"},
    @{Src="LICENSE"; Dst="LICENSE"},
    @{Src="RELEASE_NOTES_v$Version.md"; Dst="RELEASE_NOTES.md"}
)

$docCount = 0
foreach ($doc in $docs) {
    if (Test-Path $doc.Src) {
        Copy-Item $doc.Src (Join-Path $docsDir $doc.Dst) -Force
        Write-Host "  + $($doc.Dst)" -ForegroundColor Gray
        $docCount++
    }
}

Write-Host "  OK Copied $docCount documents" -ForegroundColor Gray

# Step 5: Create startup scripts
Write-Host "`n[5/6] Creating startup scripts..." -ForegroundColor Green

$batLines = @(
    "@echo off",
    "echo ThemisDB v$Version",
    "echo.",
    "cd /d `"%~dp0`"",
    "bin\themis_server.exe --help"
)
$batLines | Out-File (Join-Path $ReleaseDir "windows-x64\start.bat") -Encoding ASCII

$readmeLines = @(
    "ThemisDB v$Version - Windows x64",
    "",
    "Quick Start:",
    "1. Run start.bat to see server options",
    "2. Start server: bin\themis_server.exe",
    "",
    "What's New:",
    "- 23-77x faster bulk inserts",
    "- Metadata caching",
    "",
    "Documentation: docs\RELEASE_NOTES.md",
    "GitHub: https://github.com/makr-code/ThemisDB"
)
$readmeLines | Out-File (Join-Path $ReleaseDir "windows-x64\README.txt") -Encoding UTF8

Write-Host "  OK Scripts created" -ForegroundColor Gray

# Step 6: Create archive
Write-Host "`n[6/6] Creating archive..." -ForegroundColor Green

$archiveName = "themis-v$Version-windows-x64"
$zipPath = Join-Path $ReleaseDir "$archiveName.zip"

if (Test-Path $zipPath) {
    Remove-Item $zipPath -Force
}

Compress-Archive -Path (Join-Path $ReleaseDir "windows-x64\*") -DestinationPath $zipPath -CompressionLevel Optimal

$zipSize = (Get-Item $zipPath).Length / 1MB
Write-Host "  + ZIP created: $([math]::Round($zipSize, 2)) MB" -ForegroundColor Gray

# Create checksum
$hash = Get-FileHash $zipPath -Algorithm SHA256
$checksumPath = "$zipPath.sha256"
"$($hash.Hash.ToLower())  $archiveName.zip" | Out-File $checksumPath -Encoding ASCII

Write-Host "  + SHA256: $($hash.Hash.Substring(0,16))..." -ForegroundColor Gray

# Summary
Write-Host ""
Write-Host "================================================" -ForegroundColor Green
Write-Host " Release v$Version Created Successfully!" -ForegroundColor Green
Write-Host "================================================" -ForegroundColor Green
Write-Host ""
Write-Host "Release artifacts:" -ForegroundColor Cyan
Write-Host "  Archive: $zipPath" -ForegroundColor White
Write-Host "  Checksum: $checksumPath" -ForegroundColor White
Write-Host "  Size: $([math]::Round($zipSize, 2)) MB" -ForegroundColor White
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "1. Go to: https://github.com/makr-code/ThemisDB/releases/new"
Write-Host "2. Tag: v$Version (already pushed)"
Write-Host "3. Upload: $archiveName.zip + .sha256"
Write-Host ""
