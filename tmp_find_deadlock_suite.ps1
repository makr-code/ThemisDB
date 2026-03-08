Set-Location "c:\VCC\themis\build-msvc-ninja-release\cmake\tests"

$all = @()
$inList = .\themis_tests.exe --gtest_list_tests 2>&1
foreach ($line in $inList) {
    if ($line -match '^[A-Za-z0-9_].*\.$') {
        $all += $line.Trim().TrimEnd('.')
    }
}

$start = [Array]::IndexOf($all, 'IntegrationTest')
if ($start -lt 0) {
    Write-Host "START_SUITE_NOT_FOUND"
    exit 1
}

$max = [Math]::Min($all.Count - 1, $start + 80)
for ($i = $start; $i -le $max; $i++) {
    $suite = $all[$i]
    Write-Host "RUNNING_SUITE $suite"
    .\themis_tests.exe --gtest_filter="$suite.*" --gtest_brief=1 > "$env:TEMP\suite_run.out" 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host "FAILED_SUITE $suite EXIT=$LASTEXITCODE"
        Get-Content "$env:TEMP\suite_run.out" -Tail 120
        exit $LASTEXITCODE
    }
}

Write-Host "NO_FAILURE_IN_WINDOW"
