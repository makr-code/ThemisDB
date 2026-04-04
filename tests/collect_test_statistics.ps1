param(
    [string]$CTestPreset = "msvc-ninja-release",
    [string]$GraphPreset = "graph-tests-release",
    [string]$BuildDir = "build-msvc-ninja-release"
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
Push-Location $repoRoot

try {
    $sourcePattern = '^\s*(TEST|TEST_F|TEST_P|TYPED_TEST|TYPED_TEST_P)\s*\('
    if (Get-Command rg -ErrorAction SilentlyContinue) {
        $sourceDecls = (rg -n -g "tests/**" -e $sourcePattern | Measure-Object).Count
    } else {
        $sourceDecls = (Get-ChildItem tests -Recurse -Include *.cpp,*.cc,*.cxx,*.h,*.hpp |
            Select-String -Pattern $sourcePattern | Measure-Object).Count
    }

    $ctestList = ctest --preset $CTestPreset -N 2>&1
    $ctestTotal = ($ctestList | Select-String 'Total Tests:\s+(\d+)' |
        ForEach-Object { [int]$_.Matches[0].Groups[1].Value } | Select-Object -First 1)
    $ctestMissingExecutables = ($ctestList | Select-String '^Could not find executable ' | Measure-Object).Count
    $ctestRunnable = $ctestTotal - $ctestMissingExecutables

    $graphList = ctest --preset $GraphPreset -N 2>&1
    $graphTotal = ($graphList | Select-String 'Total Tests:\s+(\d+)' |
        ForEach-Object { [int]$_.Matches[0].Groups[1].Value } | Select-Object -First 1)
    $graphMissingExecutables = ($graphList | Select-String '^Could not find executable ' | Measure-Object).Count

    $exeDir = Join-Path $BuildDir 'cmake/tests'
    $exeFiles = Get-ChildItem $exeDir -Filter *.exe -File -ErrorAction SilentlyContinue
    $builtExecutables = $exeFiles.Count

    $gtestExecutables = 0
    $gtestSuites = 0
    $gtestTests = 0

    foreach ($file in $exeFiles) {
        try {
            $output = & $file.FullName --gtest_list_tests 2>$null
            if (-not $output) {
                continue
            }

            $localSuites = 0
            $localTests = 0

            foreach ($line in $output) {
                if ($line -match '^([A-Za-z0-9_./:-]+)\.$') {
                    $localSuites++
                    continue
                }
                if ($line -match '^\s{2,}#') {
                    continue
                }
                if ($line -match '^\s{2,}[^\s].*') {
                    $localTests++
                }
            }

            if ($localSuites -gt 0 -or $localTests -gt 0) {
                $gtestExecutables++
                $gtestSuites += $localSuites
                $gtestTests += $localTests
            }
        } catch {
            continue
        }
    }

    [pscustomobject]@{
        TimestampUtc = (Get-Date).ToUniversalTime().ToString('yyyy-MM-dd HH:mm:ss')
        SourceGoogleTestDeclarations = $sourceDecls
        CTestPreset = $CTestPreset
        CTestTotal = $ctestTotal
        CTestMissingExecutables = $ctestMissingExecutables
        CTestRunnable = $ctestRunnable
        GraphPreset = $GraphPreset
        GraphCTestTotal = $graphTotal
        GraphCTestMissingExecutables = $graphMissingExecutables
        BuiltExecutables = $builtExecutables
        GTestExecutables = $gtestExecutables
        GTestSuites = $gtestSuites
        GTestTests = $gtestTests
    } | Format-List
}
finally {
    Pop-Location
}