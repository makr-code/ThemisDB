param(
    [string]$CorruptPath,
    [string]$RepairedPath,
    [string]$ReportDir,
    [string[]]$ExcludeDirs = @(
        '.git', '.vs', '.vscode', '.idea',
        'build', 'build-*', 'out', 'tmp',
        'node_modules', 'artifacts', 'logs', 'symbols'
    ),
    [switch]$Apply,
    [switch]$MirrorDeletes
)

$ErrorActionPreference = 'Stop'
$script:SkippedFiles = New-Object System.Collections.Generic.List[object]

function Resolve-DefaultRepairedPath {
    return (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
}

function Resolve-DefaultCorruptPath {
    param([string]$baseDir)

    $candidates = Get-ChildItem -Path $baseDir -Directory -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -like 'themis_corrupt_backup*' } |
        Sort-Object LastWriteTime -Descending

    if ($candidates -and $candidates.Count -gt 0) {
        return $candidates[0].FullName
    }

    $fallback = Join-Path $baseDir 'themis'
    if (Test-Path $fallback) {
        return $fallback
    }

    throw 'Kein korruptes/quellseitiges Verzeichnis gefunden. Bitte -CorruptPath explizit setzen.'
}

function Is-ExcludedPath {
    param(
        [string]$RelativePath,
        [string[]]$Patterns
    )

    $normalized = $RelativePath.Replace('\\', '/').Replace('\', '/')
    foreach ($pattern in $Patterns) {
        $p = $pattern.Replace('\\', '/').Replace('\', '/')
        if ($normalized -like "$p") { return $true }
        if ($normalized -like "$p/*") { return $true }
        if ($normalized -like "*/$p/*") { return $true }
        if ($normalized -like "*/$p") { return $true }
    }
    return $false
}

function Get-RelativePathSafe {
    param(
        [string]$BasePath,
        [string]$TargetPath
    )

    $baseResolved = (Resolve-Path -LiteralPath $BasePath).Path
    $targetResolvedInfo = Resolve-Path -LiteralPath $TargetPath -ErrorAction SilentlyContinue
    if (-not $targetResolvedInfo) {
        return $null
    }
    $targetResolved = $targetResolvedInfo.Path

    if (-not $baseResolved.EndsWith('\')) {
        $baseResolved = "$baseResolved\"
    }

    if ($targetResolved.StartsWith($baseResolved, [System.StringComparison]::OrdinalIgnoreCase)) {
        return $targetResolved.Substring($baseResolved.Length)
    }

    return $targetResolved
}

function Build-FileMap {
    param(
        [string]$Root,
        [string[]]$ExcludePatterns
    )

    $map = @{}
    $rootResolved = (Resolve-Path -LiteralPath $Root).Path
    $files = Get-ChildItem -Path $rootResolved -File -Recurse -Force

    foreach ($file in $files) {
        $relative = Get-RelativePathSafe -BasePath $rootResolved -TargetPath $file.FullName
        if (-not $relative) {
            continue
        }

        if (Is-ExcludedPath -RelativePath $relative -Patterns $ExcludePatterns) {
            continue
        }

        try {
            $hash = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256 -ErrorAction Stop).Hash
        } catch {
            $script:SkippedFiles.Add([PSCustomObject]@{
                Path   = $file.FullName
                Reason = $_.Exception.Message
            })
            continue
        }
        $map[$relative.Replace('\\', '/').Replace('\', '/')] = [PSCustomObject]@{
            RelativePath = $relative.Replace('\\', '/').Replace('\', '/')
            FullPath     = $file.FullName
            Length       = $file.Length
            Hash         = $hash
        }
    }

    return $map
}

function Ensure-Directory {
    param([string]$Path)
    if (-not (Test-Path $Path)) {
        New-Item -ItemType Directory -Path $Path -Force | Out-Null
    }
}

if (-not $RepairedPath) {
    $RepairedPath = Resolve-DefaultRepairedPath
}

$base = Split-Path $RepairedPath -Parent
if (-not $CorruptPath) {
    $CorruptPath = Resolve-DefaultCorruptPath -baseDir $base
}

if (-not $ReportDir) {
    $ReportDir = Join-Path $RepairedPath 'artifacts\recovery'
}

$CorruptPath = (Resolve-Path $CorruptPath).Path
$RepairedPath = (Resolve-Path $RepairedPath).Path
Ensure-Directory -Path $ReportDir

Write-Host "CorruptPath : $CorruptPath"
Write-Host "RepairedPath: $RepairedPath"
Write-Host "ReportDir   : $ReportDir"

$sourceMap = Build-FileMap -Root $CorruptPath -ExcludePatterns $ExcludeDirs
$targetMap = Build-FileMap -Root $RepairedPath -ExcludePatterns $ExcludeDirs

$sourceKeys = [System.Collections.Generic.HashSet[string]]::new($sourceMap.Keys)
$targetKeys = [System.Collections.Generic.HashSet[string]]::new($targetMap.Keys)

$missingInRepaired = New-Object System.Collections.Generic.List[object]
$extraInRepaired = New-Object System.Collections.Generic.List[object]
$contentDiffs = New-Object System.Collections.Generic.List[object]

foreach ($path in $sourceKeys) {
    if (-not $targetMap.ContainsKey($path)) {
        $missingInRepaired.Add([PSCustomObject]@{
            RelativePath = $path
            SourceLength = $sourceMap[$path].Length
            SourceHash   = $sourceMap[$path].Hash
        })
        continue
    }

    if ($sourceMap[$path].Hash -ne $targetMap[$path].Hash) {
        $contentDiffs.Add([PSCustomObject]@{
            RelativePath = $path
            SourceLength = $sourceMap[$path].Length
            TargetLength = $targetMap[$path].Length
            SourceHash   = $sourceMap[$path].Hash
            TargetHash   = $targetMap[$path].Hash
        })
    }
}

foreach ($path in $targetKeys) {
    if (-not $sourceMap.ContainsKey($path)) {
        $extraInRepaired.Add([PSCustomObject]@{
            RelativePath = $path
            TargetLength = $targetMap[$path].Length
            TargetHash   = $targetMap[$path].Hash
        })
    }
}

$timestamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$missingFile = Join-Path $ReportDir "missing_in_repaired_$timestamp.csv"
$extraFile = Join-Path $ReportDir "extra_in_repaired_$timestamp.csv"
$diffFile = Join-Path $ReportDir "content_diffs_$timestamp.csv"

$missingInRepaired | Export-Csv -Path $missingFile -NoTypeInformation -Encoding UTF8
$extraInRepaired | Export-Csv -Path $extraFile -NoTypeInformation -Encoding UTF8
$contentDiffs | Export-Csv -Path $diffFile -NoTypeInformation -Encoding UTF8

Write-Host ''
Write-Host '==== Vergleichsergebnis ===='
Write-Host "Quelldateien              : $($sourceMap.Count)"
Write-Host "Zieldateien               : $($targetMap.Count)"
Write-Host "Fehlend in Repaired       : $($missingInRepaired.Count)"
Write-Host "Extra in Repaired         : $($extraInRepaired.Count)"
Write-Host "Inhaltlich unterschiedlich: $($contentDiffs.Count)"
Write-Host "Übersprungene Dateien     : $($script:SkippedFiles.Count)"
Write-Host "Reports:"
Write-Host "  $missingFile"
Write-Host "  $extraFile"
Write-Host "  $diffFile"

if ($script:SkippedFiles.Count -gt 0) {
    $skipFile = Join-Path $ReportDir "skipped_files_$timestamp.csv"
    $script:SkippedFiles | Export-Csv -Path $skipFile -NoTypeInformation -Encoding UTF8
    Write-Host "  $skipFile"
}

if ($Apply) {
    Write-Host ''
    Write-Host '==== APPLY aktiv: Synchronisation startet ===='

    foreach ($item in $missingInRepaired) {
        $src = Join-Path $CorruptPath $item.RelativePath
        $dst = Join-Path $RepairedPath $item.RelativePath
        Ensure-Directory -Path (Split-Path $dst -Parent)
        Copy-Item -Path $src -Destination $dst -Force
    }

    foreach ($item in $contentDiffs) {
        $src = Join-Path $CorruptPath $item.RelativePath
        $dst = Join-Path $RepairedPath $item.RelativePath
        Ensure-Directory -Path (Split-Path $dst -Parent)
        Copy-Item -Path $src -Destination $dst -Force
    }

    if ($MirrorDeletes) {
        foreach ($item in $extraInRepaired) {
            $dst = Join-Path $RepairedPath $item.RelativePath
            if (Test-Path $dst) {
                Remove-Item -Path $dst -Force
            }
        }
    }

    Write-Host 'Synchronisation abgeschlossen. Führe Re-Scan durch...'

    $postSource = Build-FileMap -Root $CorruptPath -ExcludePatterns $ExcludeDirs
    $postTarget = Build-FileMap -Root $RepairedPath -ExcludePatterns $ExcludeDirs
    $postMissing = 0
    $postDiff = 0

    foreach ($path in $postSource.Keys) {
        if (-not $postTarget.ContainsKey($path)) {
            $postMissing++
            continue
        }
        if ($postSource[$path].Hash -ne $postTarget[$path].Hash) {
            $postDiff++
        }
    }

    $postExtra = 0
    foreach ($path in $postTarget.Keys) {
        if (-not $postSource.ContainsKey($path)) {
            $postExtra++
        }
    }

    Write-Host ''
    Write-Host '==== Ergebnis nach APPLY ===='
    Write-Host "Fehlend in Repaired       : $postMissing"
    Write-Host "Extra in Repaired         : $postExtra"
    Write-Host "Inhaltlich unterschiedlich: $postDiff"

    if ($postMissing -eq 0 -and $postDiff -eq 0 -and (($MirrorDeletes -and $postExtra -eq 0) -or (-not $MirrorDeletes))) {
        Write-Host 'Status: Konsistent gemäss gewählter Optionen.'
    } else {
        Write-Warning 'Status: Noch nicht vollständig konsistent. Bitte Reports prüfen.'
        exit 2
    }
}