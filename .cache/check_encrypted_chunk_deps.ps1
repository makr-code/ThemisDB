$root='C:\Projects\ThemisDB\build-msvc-windows-release'
$exes = Get-ChildItem -Path $root -Recurse -Filter '*encrypted*exe' -File -ErrorAction SilentlyContinue
if ($null -eq $exes -or $exes.Count -eq 0) { Write-Output 'no-exe-found'; exit 1 }
foreach ($e in $exes) { Write-Output "FOUND: $($e.FullName)" }
$dump = (try { where.exe dumpbin.exe 2>$null } catch { $null })
if ($dump) {
    Write-Output "dumpbin found: $dump"
    foreach ($e in $exes) {
        Write-Output "---- dependents for $($e.FullName) ----"
        & $dump /dependents "$($e.FullName)" | ForEach-Object { Write-Output $_ }
    }
} else {
    Write-Output 'dumpbin not found; attempting to run the first exe to capture exit code'
    $first = $exes[0].FullName
    Write-Output "Running: $first"
    try {
        $out = & "$first" 2>&1
        Write-Output 'STDOUT/ERR:'
        Write-Output $out
        Write-Output ("ExitCode: {0}" -f $LASTEXITCODE)
    } catch {
        Write-Output ('Run failed: {0}' -f $_)
    }
}
