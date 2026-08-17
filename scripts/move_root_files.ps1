param(
    [switch]$WhatIf
)

# Moves common root clutter into appropriate folders.
# Usage: .\move_root_files.ps1 [-WhatIf]

$root = Resolve-Path (Join-Path $PSScriptRoot '..')
Set-Location $root

function Ensure-Dir($p) {
    if (-not (Test-Path $p)) { New-Item -ItemType Directory -Path $p | Out-Null }
}

# mapping: destination -> glob patterns
$map = @{
    'logs/build' = @(
        'build*.log', 'build_*.log', 'build_output*.txt', 'build*.txt', 'buildlog*', 'build-*.log', 'build_*.*', 'build_output*'
    )
    'logs' = @(
        'cmake*.log', 'ctest*.log', 'ctest*.txt', 'vsdevcmd*.log', 'tmp_*.txt', 'tmp*.txt', 'ctest*'
    )
    'ai_working' = @(
        'CLAUDE.md'
    )
}

# explicit excludes (filenames in project root that must not be moved)
$excludeNames = @(
    'CMakeLists.txt',
    'CMakePresets.json',
    'CMakeUserPresets.json',
    'CMakeUserPresets.json.example',
    'CTEST.md',
    'CMAKE_HARDENING_PLAN.md'
)

$moved = @()

foreach ($dest in $map.Keys) {
    $destPath = Join-Path $root $dest
    Ensure-Dir $destPath
    foreach ($pattern in $map[$dest]) {
        Get-ChildItem -Path $root -Filter $pattern -File -ErrorAction SilentlyContinue | ForEach-Object {
            $src = $_.FullName
            if ($excludeNames -contains $_.Name) {
                Write-Output "Skipping excluded file: $($_.Name)"
                return
            }
            $target = Join-Path $destPath $_.Name
            if ($WhatIf) {
                Write-Output "Would move: $($_.Name) -> $dest/"
            } else {
                # If file is tracked by git, use git mv to preserve history
                $isTracked = $false
                try {
                    git ls-files --error-unmatch -- "$_" > $null 2>&1
                    if ($LASTEXITCODE -eq 0) { $isTracked = $true }
                } catch {
                    $isTracked = $false
                }

                if ($isTracked) {
                    git mv -f -- "$src" "$target" 2>$null
                    if ($LASTEXITCODE -ne 0) {
                        # fallback to move
                        Move-Item -Force -Path $src -Destination $target
                    }
                } else {
                    Move-Item -Force -Path $src -Destination $target
                }
                $moved += @{ src = $src; dest = $target }
                Write-Output "Moved: $($_.Name) -> $dest/"
            }
        }
    }
}

Write-Output "\nSummary: $($moved.Count) files moved."
if ($moved.Count -gt 0) {
    $moved | ForEach-Object { Write-Output " - $($_.src) -> $($_.dest)" }
}
