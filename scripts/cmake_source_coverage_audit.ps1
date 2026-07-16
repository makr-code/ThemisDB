param(
    [string]$RepoRoot = ".",
    [string]$ReportPath = "",
    [string]$BaselinePath = "",
    [switch]$CheckCMakeModules,
    [switch]$FailOnNewA,
    [switch]$UpdateBaseline
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Normalize-PathString {
    param([string]$Path)
    return ($Path -replace "\\", "/")
}

function Get-RelSources {
    param(
        [string]$Root,
        [string]$SubDir
    )

    $dir = Join-Path $Root $SubDir
    if (-not (Test-Path -LiteralPath $dir)) {
        return @()
    }

    $prefix = (Normalize-PathString (Resolve-Path -LiteralPath $Root).Path).TrimEnd('/') + "/"
    $files = Get-ChildItem -LiteralPath $dir -Recurse -File -Include *.c, *.cc, *.cpp, *.cxx

    return $files |
        ForEach-Object {
            $full = Normalize-PathString $_.FullName
            if ($full.StartsWith($prefix)) {
                $full.Substring($prefix.Length)
            }
        } |
        Where-Object { $_ } |
        Sort-Object -Unique
}

function Get-CMakeText {
    param([string]$Root)

    $excludeDirNames = @(
        ".git", "build", "build-msvc-windows-debug", "build-msvc-windows-release",
        "vcpkg", "vcpkg_installed", "ffmpeg"
    )

    $allCmakeFiles = Get-ChildItem -LiteralPath $Root -Recurse -File |
        Where-Object { $_.Name -eq "CMakeLists.txt" -or $_.Extension -eq ".cmake" }

    $filtered = @()
    foreach ($f in $allCmakeFiles) {
        $p = "/" + (Normalize-PathString $f.FullName).ToLowerInvariant() + "/"
        $skip = $false
        foreach ($d in $excludeDirNames) {
            if ($p.Contains("/$($d.ToLowerInvariant())/")) {
                $skip = $true
                break
            }
        }
        if (-not $skip) {
            $filtered += $f
        }
    }

    return ($filtered | ForEach-Object { Get-Content -Raw -LiteralPath $_.FullName }) -join "`n"
}

function Get-RelCMakeModules {
    param([string]$Root)

    $cmakeDir = Join-Path $Root "cmake"
    if (-not (Test-Path -LiteralPath $cmakeDir)) {
        return @()
    }

    $prefix = (Normalize-PathString (Resolve-Path -LiteralPath $Root).Path).TrimEnd('/') + "/"
    $files = Get-ChildItem -LiteralPath $cmakeDir -Recurse -File |
        Where-Object { $_.Name -eq "CMakeLists.txt" -or $_.Extension -eq ".cmake" }

    return $files |
        ForEach-Object {
            $full = Normalize-PathString $_.FullName
            if ($full.StartsWith($prefix)) {
                $full.Substring($prefix.Length)
            }
        } |
        Where-Object { $_ -and $_ -ne "cmake/CMakeLists.txt" } |
        Sort-Object -Unique
}

function Get-UnreferencedCMakeModules {
    param(
        [string[]]$Modules,
        [string]$CMakeText
    )

    function Test-IsDynamicallyReferencedModule {
        param(
            [string]$ModulePath,
            [string]$AllCMakeText
        )

        # Edition files are selected dynamically via THEMIS_EDITION
        if ($ModulePath -like "cmake/editions/*.cmake") {
            if ($AllCMakeText -match [regex]::Escape('include(${CMAKE_CURRENT_LIST_DIR}/${THEMIS_EDITION}.cmake)')) {
                return $true
            }
        }

        # Toolchain files are selected externally via -DCMAKE_TOOLCHAIN_FILE
        if ($ModulePath -like "cmake/platforms/Toolchains/*.cmake") {
            if ($AllCMakeText -match [regex]::Escape('CMAKE_TOOLCHAIN_FILE')) {
                return $true
            }
        }

        return $false
    }

    $orphans = New-Object System.Collections.Generic.List[string]
    foreach ($m in $Modules) {
        $name = [System.IO.Path]::GetFileName($m)
        $pathRef = $CMakeText -match [regex]::Escape($m)
        $nameRef = $CMakeText -match [regex]::Escape($name)
        $dynamicRef = Test-IsDynamicallyReferencedModule -ModulePath $m -AllCMakeText $CMakeText

        if (-not $pathRef -and -not $nameRef -and -not $dynamicRef) {
            $orphans.Add($m)
        }
    }

    return @($orphans | Sort-Object -Unique)
}

function Get-PathCandidates {
    param(
        [string]$RelPath,
        [ValidateSet("src", "tests", "benchmarks")]
        [string]$Bucket
    )

    $candidates = New-Object System.Collections.Generic.HashSet[string]([System.StringComparer]::OrdinalIgnoreCase)
    [void]$candidates.Add((Normalize-PathString $RelPath))

    $parts = $RelPath.Split('/', 2)
    if ($parts.Count -eq 2) {
        $tail = $parts[1]
        [void]$candidates.Add($tail)

        switch ($Bucket) {
            "src" {
                [void]$candidates.Add("../src/$tail")
                [void]$candidates.Add("src/$tail")
                [void]$candidates.Add('${CMAKE_SOURCE_DIR}/src/' + $tail)
                [void]$candidates.Add('${PROJECT_SOURCE_DIR}/src/' + $tail)
            }
            "tests" {
                [void]$candidates.Add("tests/$tail")
                [void]$candidates.Add('${CMAKE_SOURCE_DIR}/tests/' + $tail)
                [void]$candidates.Add('${CMAKE_CURRENT_SOURCE_DIR}/' + $tail)
            }
            "benchmarks" {
                [void]$candidates.Add("benchmarks/$tail")
                [void]$candidates.Add('${CMAKE_SOURCE_DIR}/benchmarks/' + $tail)
                [void]$candidates.Add('${CMAKE_CURRENT_SOURCE_DIR}/' + $tail)
            }
        }
    }

    return @($candidates)
}

function Classify-Files {
    param(
        [string[]]$Files,
        [string]$CMakeText,
        [ValidateSet("src", "tests", "benchmarks")]
        [string]$Bucket
    )

    $result = [ordered]@{
        A = New-Object System.Collections.Generic.List[string]
        B = New-Object System.Collections.Generic.List[string]
        C = New-Object System.Collections.Generic.List[string]
    }

    foreach ($f in $Files) {
        $name = [System.IO.Path]::GetFileName($f)
        $nameRef = $CMakeText -match [regex]::Escape($name)

        $pathRef = $false
        foreach ($cand in (Get-PathCandidates -RelPath $f -Bucket $Bucket)) {
            if ($CMakeText -match [regex]::Escape($cand)) {
                $pathRef = $true
                break
            }
        }

        if ($pathRef) {
            $result.C.Add($f)
        }
        elseif ($nameRef) {
            $result.B.Add($f)
        }
        else {
            $result.A.Add($f)
        }
    }

    return $result
}

function Load-Baseline {
    param([string]$Path)
    if (-not $Path -or -not (Test-Path -LiteralPath $Path)) {
        return @()
    }
    return (Get-Content -LiteralPath $Path | Where-Object { $_ -and -not $_.StartsWith("#") } | Sort-Object -Unique)
}

function Save-Baseline {
    param(
        [string]$Path,
        [string[]]$Entries
    )
    if (-not $Path) {
        throw "BaselinePath muss gesetzt sein, wenn -UpdateBaseline verwendet wird."
    }
    $dir = Split-Path -Parent $Path
    if ($dir -and -not (Test-Path -LiteralPath $dir)) {
        New-Item -ItemType Directory -Path $dir -Force | Out-Null
    }

    $content = @(
        "# CMake Source Coverage Baseline"
        "# Kategorie A (weder Pfad noch Dateiname in CMake gefunden)"
        "# Aktualisiert: $(Get-Date -Format s)"
        ""
    ) + ($Entries | Sort-Object -Unique)

    $content | Set-Content -LiteralPath $Path -Encoding UTF8
}

$root = (Resolve-Path -LiteralPath $RepoRoot).Path
$cmakeText = Get-CMakeText -Root $root

$srcFiles = Get-RelSources -Root $root -SubDir "src"
$testFiles = Get-RelSources -Root $root -SubDir "tests"
$benchFiles = Get-RelSources -Root $root -SubDir "benchmarks"

$srcClass = Classify-Files -Files $srcFiles -CMakeText $cmakeText -Bucket "src"
$testClass = Classify-Files -Files $testFiles -CMakeText $cmakeText -Bucket "tests"
$benchClass = Classify-Files -Files $benchFiles -CMakeText $cmakeText -Bucket "benchmarks"

$allA = @($srcClass.A + $testClass.A + $benchClass.A | Sort-Object -Unique)

$cmakeModules = @()
$cmakeOrphans = @()
if ($CheckCMakeModules) {
    $cmakeModules = @(Get-RelCMakeModules -Root $root)
    $cmakeOrphans = @(Get-UnreferencedCMakeModules -Modules $cmakeModules -CMakeText $cmakeText)
}

if ($UpdateBaseline) {
    Save-Baseline -Path $BaselinePath -Entries $allA
}

$baseline = Load-Baseline -Path $BaselinePath
$newA = @($allA | Where-Object { $baseline -notcontains $_ })

$report = New-Object System.Collections.Generic.List[string]
$report.Add("CMake Source Coverage Audit")
$report.Add("Repository: $root")
$report.Add("")
$report.Add("src total: $($srcFiles.Count) | A: $($srcClass.A.Count) | B: $($srcClass.B.Count) | C: $($srcClass.C.Count)")
$report.Add("tests total: $($testFiles.Count) | A: $($testClass.A.Count) | B: $($testClass.B.Count) | C: $($testClass.C.Count)")
$report.Add("benchmarks total: $($benchFiles.Count) | A: $($benchClass.A.Count) | B: $($benchClass.B.Count) | C: $($benchClass.C.Count)")
if ($CheckCMakeModules) {
    $report.Add("cmake modules total: $($cmakeModules.Count) | orphan candidates: $($cmakeOrphans.Count)")
}
$report.Add("")
$report.Add("Neue Kategorie-A-Dateien ggü. Baseline: $($newA.Count)")
$report.Add("")

$report.Add("--- Kategorie A (Top 120) ---")
$allA | Select-Object -First 120 | ForEach-Object { $report.Add($_) }

$report.Add("")
$report.Add("--- Kategorie B (Top 120) ---")
@($srcClass.B + $testClass.B + $benchClass.B | Sort-Object -Unique) | Select-Object -First 120 | ForEach-Object { $report.Add($_) }

if ($CheckCMakeModules) {
    $report.Add("")
    $report.Add("--- CMake Orphan-Kandidaten (Top 120) ---")
    $cmakeOrphans | Select-Object -First 120 | ForEach-Object { $report.Add($_) }
}

$reportText = $report -join "`n"
Write-Output $reportText

if ($ReportPath) {
    $reportDir = Split-Path -Parent $ReportPath
    if ($reportDir -and -not (Test-Path -LiteralPath $reportDir)) {
        New-Item -ItemType Directory -Path $reportDir -Force | Out-Null
    }
    $reportText | Set-Content -LiteralPath $ReportPath -Encoding UTF8
}

if ($FailOnNewA -and $newA.Count -gt 0) {
    Write-Error "CMake Source Coverage Audit fehlgeschlagen: $($newA.Count) neue Kategorie-A-Dateien gefunden."
    exit 2
}

exit 0
