param(
    [Parameter(Mandatory = $true)]
    [string[]]$Modules,
    [string]$BinDir = "C:\Projects\ThemisDB\build-msvc-windows-release\bin_out",
    [Parameter(Mandatory = $true)]
    [string]$OutJsonPath,
    [string]$DateTag = "2026-07-18"
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path $BinDir)) {
    throw "BinDir not found: $BinDir"
}

$results = @()

foreach ($m in $Modules) {
    $hits = Get-ChildItem $BinDir -Filter ("module_" + $m + "_test_*_focused.exe") -ErrorAction SilentlyContinue | Sort-Object Name

    if (-not $hits -or $hits.Count -eq 0) {
        $results += [PSCustomObject]@{
            module = $m
            build = "not found (module_${m}_test_*_focused.exe)"
            test = "not executed"
            result = "Evidence gap - no focused module binary found in build-msvc-windows-release/bin_out on $DateTag."
        }
        continue
    }

    $exe = $hits[0].FullName
    $exeName = $hits[0].Name
    $out = & $exe --gtest_brief=1 2>&1 | Out-String
    $code = $LASTEXITCODE

    $summary = ""
    if ($out -match '\[\s*FAILED\s*\]\s*\d+\s+tests?[,\.]') {
        $summary = $matches[0]
    } elseif ($out -match '\[\s*PASSED\s*\]\s*\d+\s+tests?\.') {
        $summary = $matches[0]
    } else {
        $interesting = $out -split "`r?`n" | Where-Object { $_ -match 'PASSED|FAILED|SEH|C000|Exception|error' } | Select-Object -First 3
        $summary = ($interesting -join " | ")
        if ([string]::IsNullOrWhiteSpace($summary)) {
            $summary = "No concise summary extracted; inspect raw test output if needed."
        }
    }

    $status = if ($code -eq 0) { "PASS" } else { "FAIL" }
    if ($code -ne 0 -and $summary -like '[  PASSED  ]*') {
        $summary = "Non-zero exit with no explicit FAILED summary extracted; inspect full output. " + $summary
    }

    $results += [PSCustomObject]@{
        module = $m
        build = $exeName
        test = "$exeName --gtest_brief=1"
        result = "$status (exit $code, $summary)"
    }
}

$json = $results | ConvertTo-Json -Depth 5
[System.IO.File]::WriteAllText($OutJsonPath, $json, [System.Text.UTF8Encoding]::new($false))

"WROTE=$OutJsonPath"
"COUNT=$($results.Count)"