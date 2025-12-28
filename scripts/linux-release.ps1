param(
    [string]$Version = "1.3.4",
    [switch]$SkipBuild = $false
)

$ErrorActionPreference = "Stop"
$RootDir = "/mnt/c/VCC/themis"
$ReleaseDir = "/mnt/c/VCC/themis/release/v$Version"

Write-Host "================================================" -ForegroundColor Cyan
Write-Host " ThemisDB v$Version Linux Build (WSL)" -ForegroundColor Cyan
Write-Host "================================================" -ForegroundColor Cyan
Write-Host ""

# Step 1: Configure and build in WSL
if (-not $SkipBuild) {
    Write-Host "[1/6] Configuring CMake (Linux)..." -ForegroundColor Green
    
    $configCmd = "cd $RootDir && export VCPKG_ROOT=$RootDir/vcpkg && cmake -S . -B build-wsl -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$RootDir/vcpkg/scripts/buildsystems/vcpkg.cmake -DTHEMIS_ENABLE_LLM=OFF"
    wsl bash -c $configCmd
    
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configuration failed"
    }
    Write-Host "  OK CMake configured" -ForegroundColor Gray
    
    Write-Host "`n[2/6] Building Release (Linux)..." -ForegroundColor Green
    
    $buildCmd = "cd $RootDir && export VCPKG_ROOT=$RootDir/vcpkg && cmake --build build-wsl --target themis_core -j8"
    wsl bash -c $buildCmd
    
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed"
    }
    Write-Host "  OK Build completed" -ForegroundColor Gray
} else {
    Write-Host "[1-2/6] Using existing Linux build..." -ForegroundColor Yellow
}

# Step 3: Create release directory structure
Write-Host "`n[3/6] Creating Linux release structure..." -ForegroundColor Green

$mkdirCmd = @"
mkdir -p $ReleaseDir/linux-x64/bin && \
mkdir -p $ReleaseDir/linux-x64/docs && \
mkdir -p $ReleaseDir/linux-x64/config
"@

wsl bash -c $mkdirCmd
Write-Host "  OK Directory structure created" -ForegroundColor Gray

# Step 4: Copy binaries
Write-Host "`n[4/6] Copying Linux binaries..." -ForegroundColor Green

$copyCmd = @"
cd $RootDir && \
cp build-wsl/themis_server $ReleaseDir/linux-x64/bin/ 2>/dev/null || true && \
cp build-wsl/themis_cli $ReleaseDir/linux-x64/bin/ 2>/dev/null || true && \
cp build-wsl/themis_demo $ReleaseDir/linux-x64/bin/ 2>/dev/null || true && \
chmod +x $ReleaseDir/linux-x64/bin/* && \
ls -lh $ReleaseDir/linux-x64/bin/
"@

wsl bash -c $copyCmd
Write-Host "  OK Binaries copied" -ForegroundColor Gray

# Step 5: Copy documentation and create startup scripts
Write-Host "`n[5/6] Creating Linux documentation and scripts..." -ForegroundColor Green

$docCmd = @"
cd $RootDir && \
cp README.md $ReleaseDir/linux-x64/docs/ && \
cp CHANGELOG.md $ReleaseDir/linux-x64/docs/ && \
cp LICENSE $ReleaseDir/linux-x64/docs/ && \
cp RELEASE_NOTES_v$Version.md $ReleaseDir/linux-x64/docs/RELEASE_NOTES.md
"@

wsl bash -c $docCmd

# Create startup script
$startScript = @'
#!/bin/bash
echo "ThemisDB v{VERSION}"
echo ""
cd "$(dirname "$0")"
./bin/themis_server --help
'@ -replace '\{VERSION\}', $Version

$startScript | Out-File "C:\VCC\themis\release\v$Version\linux-x64\start.sh" -Encoding UTF8 -NoNewline
wsl bash -c "chmod +x $ReleaseDir/linux-x64/start.sh"

# Create README
$readmeLines = @(
    "ThemisDB v$Version - Linux x64",
    "",
    "Quick Start:",
    "1. Run ./start.sh to see server options",
    "2. Start server: ./bin/themis_server",
    "",
    "What's New:",
    "- 23-77x faster bulk inserts",
    "- Metadata caching",
    "",
    "Documentation: docs/RELEASE_NOTES.md",
    "GitHub: https://github.com/makr-code/ThemisDB"
)
$readmeLines | Out-File "C:\VCC\themis\release\v$Version\linux-x64\README.txt" -Encoding UTF8

Write-Host "  OK Documentation and scripts created" -ForegroundColor Gray

# Step 6: Create tar.gz archive
Write-Host "`n[6/6] Creating Linux archive..." -ForegroundColor Green

$archiveName = "themis-v$Version-linux-x64"
$tarCmd = @"
cd $ReleaseDir && \
tar -czf $archiveName.tar.gz linux-x64/ && \
ls -lh $archiveName.tar.gz
"@

wsl bash -c $tarCmd

# Create checksum
$sha256Cmd = "cd $ReleaseDir && sha256sum $archiveName.tar.gz > $archiveName.tar.gz.sha256 && cat $archiveName.tar.gz.sha256"
$checksum = wsl bash -c $sha256Cmd

Write-Host "  + TAR.GZ created" -ForegroundColor Gray
Write-Host "  + SHA256: $($checksum.Split()[0].Substring(0,16))..." -ForegroundColor Gray

# Summary
Write-Host ""
Write-Host "================================================" -ForegroundColor Green
Write-Host " Linux Release v$Version Created!" -ForegroundColor Green
Write-Host "================================================" -ForegroundColor Green
Write-Host ""
Write-Host "Release artifacts:" -ForegroundColor Cyan
Write-Host "  Archive: C:\VCC\themis\release\v$Version\$archiveName.tar.gz"
Write-Host "  Checksum: C:\VCC\themis\release\v$Version\$archiveName.tar.gz.sha256"
Write-Host ""
Write-Host "Next: Upload both Windows and Linux archives to GitHub Release"
Write-Host ""
