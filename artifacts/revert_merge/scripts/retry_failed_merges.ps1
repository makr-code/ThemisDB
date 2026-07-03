# Retry GH API merge for PRs that previously failed in force_merge_v2_report.json
$reportPath = Join-Path $PSScriptRoot '..\force_merge_v2_report.json'
if (-not (Test-Path $reportPath)) { Write-Error "report not found: $reportPath"; exit 1 }
$r = Get-Content $reportPath -Raw | ConvertFrom-Json
$entries = $r.value
foreach ($e in $entries) {
    if ($e.merged) { continue }
    $n = $e.number
    Write-Output "Retrying PR $n"
    try {
        $apiOut = gh api -X PUT "/repos/makr-code/ThemisDB/pulls/$n/merge" -f merge_method=merge 2>&1
        Write-Output $apiOut
    } catch {
        Write-Error ("API merge failed for {0}: {1}" -f $n, $_)
    }
    Start-Sleep -Seconds 1
}
Write-Output "Done."
