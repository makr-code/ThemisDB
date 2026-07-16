$timeout = 3600
$start = Get-Date
Write-Output "Waiting for build processes to finish (timeout ${timeout}s)"
while (@(Get-Process -Name ninja,link,cl,cmake -ErrorAction SilentlyContinue).Count -gt 0) {
    if (((Get-Date) - $start).TotalSeconds -gt $timeout) {
        Write-Output 'timeout-waiting-build'
        exit 2
    }
    Start-Sleep -Seconds 5
}
Write-Output 'build-complete'
ctest --preset windows-release --stop-on-failure -V --output-on-failure -j 1 --timeout 60
