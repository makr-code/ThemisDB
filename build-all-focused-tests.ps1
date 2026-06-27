#!/usr/bin/env pwsh
# Build all focused test targets in parallel
param(
    [string]$Preset = "windows-release",
    [int]$Parallel = 8,
    [string]$BuildDir = "c:\Projects\ThemisDB\build-msvc-windows-release"
)

Set-Location $BuildDir
Write-Host "Extracting focused test targets from build.ninja..." -ForegroundColor Cyan

$focusedTargets = Select-String "^build .*module_.*_focused\.exe:" build.ninja `
    | ForEach-Object { 
        $_ -replace "^build (.+?)\.exe:.*", '$1' -replace '\\', '/'
    } `
    | Sort-Object -Unique

$count = ($focusedTargets | Measure-Object).Count
Write-Host "Found $count unique focused test targets" -ForegroundColor Green

# Extract into file and build
$focusedTargets | Out-File -FilePath "focused_targets.txt" -Encoding UTF8

Write-Host "Starting build of all focused targets..." -ForegroundColor Cyan
Write-Host "This will take a while. Monitor progress in VS Code build output." -ForegroundColor Yellow

# Split into smaller batches to avoid command line length issues
$batchSize = 100
$batches = @()
for ($i = 0; $i -lt $count; $i += $batchSize) {
    $batch = $focusedTargets[$i..([Math]::Min($i + $batchSize - 1, $count - 1))]
    $batches += , @($batch)
}

Write-Host "Divided into $($batches.Count) batches of ~$batchSize targets each" -ForegroundColor Cyan

$builtCount = 0
foreach ($batchIndex in 0..($batches.Count - 1)) {
    $batch = $batches[$batchIndex]
    $batchNum = $batchIndex + 1
    
    Write-Host "Batch $batchNum/$($batches.Count) - Building $($batch.Count) targets..." -ForegroundColor Yellow
    
    $targetArgs = @()
    foreach ($target in $batch) {
        $targetArgs += "--target"
        $targetArgs += $target
    }
    
    $output = cmake --build . --preset $Preset --parallel $Parallel @targetArgs 2>&1
    
    # Count successes
    $successes = $output | Where-Object { $_ -match "100%" } | Measure-Object
    if ($successes.Count -gt 0 -or $LASTEXITCODE -eq 0) {
        $builtCount += $batch.Count
        Write-Host "  ✓ Batch $batchNum completed" -ForegroundColor Green
    } else {
        Write-Host "  ⚠ Batch $batchNum - Check output for errors" -ForegroundColor Yellow
    }
}

Write-Host ""
Write-Host "Build Complete! Total focused test targets: $count" -ForegroundColor Cyan
Write-Host "Built approximately: $builtCount targets" -ForegroundColor Green
Write-Host ""
Write-Host "Run focused tests with:" -ForegroundColor Green
Write-Host "  ctest --output-on-failure -j 1 --timeout 180 -L focus" -ForegroundColor Cyan
