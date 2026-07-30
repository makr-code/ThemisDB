param(
    [int]$TimeoutMinutes = 30,
    [int]$MaxSplits = 0,
    [switch]$DryRun,
    [string]$LogDir = "scripts/logs/subtree-splits"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# Run individual subtree splits for a curated list of admin tool folders.
$paths = @(
    "projects/Themis.AdminTools.Shared",
    "tools/admin_tools_dotnet/Themis.AdminTools.Shared",
    "tools/admin_tools_dotnet/Themis.USBAdminTool",
    "tools/admin_tools_dotnet/Themis.SAGAVerifier",
    "tools/Themis.USBAdminTool",
    "tools/admin_tools_dotnet/Themis.RetentionManager",
    "tools/Themis.SAGAVerifier",
    "tools/admin_tools_dotnet/Themis.PIIManager",
    "tools/Themis.RetentionManager",
    "tools/admin_tools_dotnet/Themis.KeyRotationDashboard",
    "tools/Themis.PIIManager",
    "tools/admin_tools_dotnet/Themis.IngestionTool",
    "tools/Themis.KeyRotationDashboard",
    "tools/Themis.IngestionTool",
    "tools/admin_tools_dotnet/Themis.ImpactAnalysisViewer",
    "tools/admin_tools_dotnet/Themis.GISViewer.ControlPanel",
    "tools/Themis.ImpactAnalysisViewer",
    "tools/admin_tools_dotnet/Themis.ComplianceReports",
    "tools/Themis.GISViewer.ControlPanel",
    "tools/Themis.ComplianceReports",
    "tools/admin_tools_dotnet/Themis.ClassificationDashboard",
    "tools/Themis.ClassificationDashboard",
    "tools/admin_tools_dotnet/Themis.AuditLogViewer",
    "tools/Themis.AuditLogViewer",
    "tools/admin_tools_dotnet/Themis.AqlQueryBuilder",
    "tools/Themis.AqlQueryBuilder",
    "tools/Themis.AdminTools.Shared"
)

$repoRoot = (git rev-parse --show-toplevel).Trim()
if (-not $repoRoot) {
    throw "Could not detect git repository root."
}

if (-not (Test-Path $LogDir)) {
    New-Item -ItemType Directory -Path $LogDir -Force | Out-Null
}

$dedup = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
$todo = New-Object System.Collections.Generic.List[string]
foreach ($p in $paths) {
    if ($dedup.Add($p)) {
        [void]$todo.Add($p)
    }
}

$visited = 0
$scheduled = 0
$ok = 0
$skipped = 0
$failed = 0
$timeouts = 0

Write-Host "Starting subtree splits. Timeout per path: $TimeoutMinutes minute(s)."
Write-Host "Log directory: $LogDir"

foreach ($p in $todo) {
    $visited += 1
    $san = $p -replace "[^a-zA-Z0-9]", "-"
    $branch = "split-$san"
    $stamp = Get-Date -Format "yyyyMMdd-HHmmss"
    $logBase = Join-Path $LogDir "$stamp-$san"
    $stdoutLog = "$logBase.stdout.log"
    $stderrLog = "$logBase.stderr.log"

    $branchExists = @((git branch --list -- "$branch") | Where-Object { $_ -and $_.Trim() }).Count -gt 0
    if ($branchExists) {
        Write-Host "SKIP (branch exists): $branch"
        $skipped += 1
        continue
    }

    # Validate path against HEAD tree (works even if local working tree path was deleted).
    $pathInHead = @((git ls-tree -d --name-only HEAD -- "$p") | Where-Object { $_ -and $_.Trim() }).Count -gt 0
    if (-not $pathInHead) {
        Write-Host "SKIP (not in HEAD): $p"
        $skipped += 1
        continue
    }

    if ($MaxSplits -gt 0 -and $scheduled -ge $MaxSplits) {
        Write-Host "Reached MaxSplits=$MaxSplits new split candidate(s). Stopping."
        break
    }

    $scheduled += 1

    Write-Host "SPLIT: $p -> $branch"
    Write-Host "  logs: $stdoutLog"

    if ($DryRun) {
        Write-Host "  dry-run: git subtree split --prefix=$p -b $branch"
        continue
    }

    $args = @("subtree", "split", "--prefix=$p", "-b", $branch)
    $proc = Start-Process -FilePath "git" -ArgumentList $args -WorkingDirectory $repoRoot -PassThru -NoNewWindow -RedirectStandardOutput $stdoutLog -RedirectStandardError $stderrLog
    $exited = $proc.WaitForExit($TimeoutMinutes * 60 * 1000)

    if (-not $exited) {
        try { Stop-Process -Id $proc.Id -Force -ErrorAction Stop } catch {}
        Write-Host "TIMEOUT: $p exceeded $TimeoutMinutes minute(s)."
        $timeouts += 1
        continue
    }

    if ($proc.ExitCode -ne 0) {
        Write-Host "FAILED: $p (exit $($proc.ExitCode))"
        $failed += 1
        continue
    }

    Write-Host "OK: $branch"
    $ok += 1
}

Write-Host "Done individual splits."
Write-Host "Summary: visited=$visited scheduled=$scheduled ok=$ok skipped=$skipped failed=$failed timeouts=$timeouts"
