<#
Find all directories under projects/ and tools/ that contain a .csproj
and run `git subtree split --prefix=<path> -b split-<sanitized>` for each.
This creates local branches only; it does NOT push to any remote.

Usage examples:
  .\scripts\batch_subtree_splits.ps1
  .\scripts\batch_subtree_splits.ps1 -TimeoutMinutes 20 -MaxSplits 5
  .\scripts\batch_subtree_splits.ps1 -DryRun
#>
param(
    [int]$TimeoutMinutes = 30,
    [int]$MaxSplits = 0,
    [switch]$DryRun,
    [string]$LogDir = "scripts/logs/subtree-splits"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

Write-Host "Scanning for C# projects under 'projects/' and 'tools/'..."

$roots = @("projects", "tools")
$found = New-Object System.Collections.Generic.List[string]
$seen = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
$repoRoot = (git rev-parse --show-toplevel).Trim()
if (-not $repoRoot) {
    throw "Could not detect git repository root."
}

if (-not (Test-Path $LogDir)) {
    New-Item -ItemType Directory -Path $LogDir -Force | Out-Null
}

foreach ($r in $roots) {
    if (Test-Path $r) {
        $csprojFiles = Get-ChildItem -Path $r -Recurse -Filter *.csproj -File -ErrorAction SilentlyContinue
        foreach ($f in $csprojFiles) {
            $full = $f.Directory.FullName
            if ($full.StartsWith($repoRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
                $rel = $full.Substring($repoRoot.Length + 1) -replace "\\", "/"
            } else {
                $rel = $full -replace "\\", "/"
            }
            if ($seen.Add($rel)) {
                [void]$found.Add($rel)
            }
        }
    }
}

if ($found.Count -eq 0) {
    Write-Host "No C# projects found under projects/ or tools/. Nothing to do."
    exit 0
}

Write-Host "Found $($found.Count) project directories."

$visited = 0
$scheduled = 0
$ok = 0
$skipped = 0
$failed = 0
$timeouts = 0

foreach ($p in $found) {
    $visited += 1

    $san = $p -replace "[^a-zA-Z0-9]", "-"
    $branch = "split-$san"
    $stamp = Get-Date -Format "yyyyMMdd-HHmmss"
    $logBase = Join-Path $LogDir "$stamp-$san"
    $stdoutLog = "$logBase.stdout.log"
    $stderrLog = "$logBase.stderr.log"

    Write-Host "Processing '$p' -> '$branch'"

    $exists = git branch --list $branch
    if ($exists) {
        Write-Host "  SKIP: branch exists"
        $skipped += 1
        continue
    }

    if (-not (Test-Path $p)) {
        Write-Host "  SKIP: path does not exist"
        $skipped += 1
        continue
    }

    if ($MaxSplits -gt 0 -and $scheduled -ge $MaxSplits) {
        Write-Host "Reached MaxSplits=$MaxSplits new split candidate(s). Stopping."
        break
    }

    $scheduled += 1

    if ($DryRun) {
        Write-Host "  dry-run: git subtree split --prefix=$p -b $branch"
        continue
    }

    Write-Host "  logs: $stdoutLog"
    $args = @("subtree", "split", "--prefix=$p", "-b", $branch)
    $proc = Start-Process -FilePath "git" -ArgumentList $args -WorkingDirectory $repoRoot -PassThru -NoNewWindow -RedirectStandardOutput $stdoutLog -RedirectStandardError $stderrLog
    $exited = $proc.WaitForExit($TimeoutMinutes * 60 * 1000)

    if (-not $exited) {
        try { Stop-Process -Id $proc.Id -Force -ErrorAction Stop } catch {}
        Write-Host "  TIMEOUT after $TimeoutMinutes minute(s)"
        $timeouts += 1
        continue
    }

    if ($proc.ExitCode -ne 0) {
        Write-Host "  FAILED exit=$($proc.ExitCode)"
        $failed += 1
        continue
    }

    Write-Host "  OK"
    $ok += 1
}

Write-Host "All done. No pushes were performed."
Write-Host "Summary: visited=$visited scheduled=$scheduled ok=$ok skipped=$skipped failed=$failed timeouts=$timeouts"
