$tempContext = ".docker-build-minimal"
$sourceDir = "."
$includeDownloadsCache = $true

if (Test-Path $tempContext) {
    Write-Host "Cleaning old context..."
    Remove-Item $tempContext -Recurse -Force
}

mkdir $tempContext | Out-Null
Write-Host "Created: $tempContext"

# Files/dirs to copy (no vcpkg, no builds)
$items = @(
    "Dockerfile",
    "CMakeLists.txt",
    "VERSION",
    "RELEASE_TYPE",
    "vcpkg.json",
    "vcpkg-configuration.json",
    "cmake",
    "include",
    "src",
    "proto",
    "internal",
    "adapters",
    "aql",
    "docker",
    "llama.cpp",
    "ports",
    "tests"
)

foreach ($item in $items) {
    $path = Join-Path $sourceDir $item
    if (Test-Path $path) {
        Copy-Item $path "$tempContext\" -Recurse -Force
        Write-Host "✓ $item"
    }
}

# Copy vcpkg downloads cache (if available)
if ($includeDownloadsCache) {
    $vcpkgDownloads = "vcpkg\downloads"
    $targetDownloads = "$tempContext\vcpkg-downloads"
    
    # Always create target directory (even if empty)
    mkdir $targetDownloads -Force | Out-Null
    
    if (Test-Path $vcpkgDownloads) {
        Write-Host "`nCopying vcpkg downloads cache..."
        
        # Copy only source archives (skip tools and temp files)
        $archives = Get-ChildItem $vcpkgDownloads -File | Where-Object { 
            $_.Extension -in @('.tar.gz', '.tar.bz2', '.tar.xz', '.zip', '.7z', '.tgz') -or
            $_.Name -like '*.tar.gz' -or $_.Name -like '*LICENSE*' -or $_.Name -like '*COPYING*'
        }
        
        if ($archives.Count -gt 0) {
            $archives | ForEach-Object { Copy-Item $_.FullName $targetDownloads -Force }
            $cacheSize = ($archives | Measure-Object -Property Length -Sum).Sum / 1MB
            Write-Host "✓ vcpkg-downloads: $($archives.Count) archives, $([Math]::Round($cacheSize, 2)) MB"
        } else {
            Write-Host "⚠ No archives found in vcpkg downloads (empty cache will be used)"
        }
    } else {
        Write-Host "⚠ vcpkg downloads not found (empty cache, will download from internet)"
    }
}

# Show size
$count = (Get-ChildItem $tempContext -Recurse | Measure-Object).Count
$size = (Get-ChildItem $tempContext -Recurse -File | Measure-Object -Property Length -Sum).Sum / 1MB

Write-Host "`n✓ Context ready: $count items, $([Math]::Round($size, 2)) MB"
