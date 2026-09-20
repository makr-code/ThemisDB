param(
    [string]$RepoRoot = ".",
    [string]$DateStamp = "",
    [switch]$DryRun
)

$ErrorActionPreference = [System.Management.Automation.ActionPreference]::Stop

$resolvedRoot = (Resolve-Path -Path $RepoRoot).Path
if ([string]::IsNullOrWhiteSpace($DateStamp)) {
    $DateStamp = Get-Date -Format "yyyy-MM-dd"
}

$destination = Join-Path $resolvedRoot ("logs/local-snapshots/{0}/root" -f $DateStamp)
$rootLogs = Get-ChildItem -Path $resolvedRoot -File -Filter *.log

Write-Host ("Repo root: {0}" -f $resolvedRoot)
Write-Host ("Destination: {0}" -f $destination)
Write-Host ("Found root logs: {0}" -f $rootLogs.Count)

if ($rootLogs.Count -eq 0) {
    Write-Host "Nothing to move."
    exit 0
}

if ($DryRun) {
    Write-Host "DryRun enabled. Files that would be moved:"
    foreach ($logFile in $rootLogs) {
        Write-Host (" - {0}" -f $logFile.Name)
    }
    exit 0
}

New-Item -ItemType Directory -Path $destination -Force | Out-Null

foreach ($logFile in $rootLogs) {
    $targetPath = Join-Path $destination $logFile.Name
    Move-Item -Path $logFile.FullName -Destination $targetPath -Force
    Write-Host ("Moved: {0}" -f $logFile.Name)
}

$remaining = (Get-ChildItem -Path $resolvedRoot -File -Filter *.log | Measure-Object).Count
Write-Host ("Done. Remaining root logs: {0}" -f $remaining)
