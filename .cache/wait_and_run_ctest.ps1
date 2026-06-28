$maxWait=3600
$elapsed=0
while ($true) {
    $p = Get-Process -Name ninja,link,cl,cmake -ErrorAction SilentlyContinue
    if (-not $p) {
        Write-Output 'No build processes detected, proceeding to tests'
        break
    } else {
        $names = ($p | Select-Object -ExpandProperty Name -Unique) -join ','
        Write-Output ("Build still running - processes: $names (elapsed ${elapsed}s)")
        Start-Sleep -Seconds 10
        $elapsed += 10
        if ($elapsed -ge $maxWait) {
            Write-Output 'Timeout waiting for build'
            exit 2
        }
    }
}
# Run ctest and stop on first failure
ctest --stop-on-failure -V --output-on-failure -j 1 --timeout 60
