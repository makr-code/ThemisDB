# fix_test_fixture_conflicts.ps1
# Finds and reports TEST/TEST_F conflicts in test suites

$conflicts = @()

# Get all test files
$test_files = Get-ChildItem -Path 'C:\VCC\themis\tests' -Filter 'test_*.cpp' -Recurse | Select-Object -ExpandProperty FullName

foreach ($file in $test_files) {
    $content = Get-Content -Path $file
    
    # Find all TEST(...) and TEST_F(...) patterns
    $tests = @{}
    
    $content | ForEach-Object {
        if ($_ -match '^TEST\(([^,]+),') {
            $suite = $matches[1]
            if (-not $tests.ContainsKey($suite)) {
                $tests[$suite] = @{ 'TEST' = 0; 'TEST_F' = 0; 'file' = $file }
            }
            $tests[$suite]['TEST']++
        }
        elseif ($_ -match '^TEST_F\(([^,]+),') {
            $suite = $matches[1]
            if (-not $tests.ContainsKey($suite)) {
                $tests[$suite] = @{ 'TEST' = 0; 'TEST_F' = 0; 'file' = $file }
            }
            $tests[$suite]['TEST_F']++
        }
    }
    
    # Check for conflicts within this file
    foreach ($suite in $tests.Keys) {
        $test_count = $tests[$suite]['TEST']
        $test_f_count = $tests[$suite]['TEST_F']
        
        if ($test_count -gt 0 -and $test_f_count -gt 0) {
            $conflicts += [PSCustomObject]@{
                Suite = $suite
                File = $file
                TestCount = $test_count
                TestFCount = $test_f_count
                Conflict = "BOTH TEST and TEST_F"
            }
        }
    }
}

if ($conflicts.Count -gt 0) {
    Write-Host "`n=== Found $($conflicts.Count) Fixture Conflicts ===" -ForegroundColor Red
    $conflicts | Format-Table -AutoSize
} else {
    Write-Host "`nNo TEST/TEST_F fixture conflicts found within individual files." -ForegroundColor Green
}

# Also check for cross-file conflicts
Write-Host "`n=== Checking for Cross-File Suite Conflicts ===" -ForegroundColor Yellow
$cross_file_suites = @{}

foreach ($file in $test_files) {
    $content = Get-Content -Path $file
    
    $content | ForEach-Object {
        if ($_ -match '^TEST\(([^,]+),') {
            $suite = $matches[1]
            $type = 'TEST'
        }
        elseif ($_ -match '^TEST_F\(([^,]+),') {
            $suite = $matches[1]
            $type = 'TEST_F'
        }
        else {
            return
        }
        
        if (-not $cross_file_suites.ContainsKey($suite)) {
            $cross_file_suites[$suite] = @()
        }
        
        $cross_file_suites[$suite] += @{ 'type' = $type; 'file' = $file }
    }
}

$cross_conflicts = @()
foreach ($suite in $cross_file_suites.Keys) {
    $types = $cross_file_suites[$suite].type | Select-Object -Unique
    if ($types.Count -gt 1) {
        $cross_conflicts += [PSCustomObject]@{
            Suite = $suite
            Files = ($cross_file_suites[$suite].file | Select-Object -Unique) -join "`n"
            Types = $types -join '/'
        }
    }
}

if ($cross_conflicts.Count -gt 0) {
    Write-Host "`nFound $($cross_conflicts.Count) Cross-File Conflicts:" -ForegroundColor Red
    $cross_conflicts | ForEach-Object {
        Write-Host "`n  Suite: $($_.Suite)"
        Write-Host "  Types: $($_.Types)"
        Write-Host "  Files:`n$($_.Files | ForEach-Object { '    $_' })"
    }
} else {
    Write-Host "No cross-file fixture conflicts found." -ForegroundColor Green
}
