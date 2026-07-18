param(
    [Parameter(Mandatory = $true)]
    [string[]]$Modules,
    [string]$Repo = "makr-code/ThemisDB",
    [string]$Root = "C:\Projects\ThemisDB\src",
    [string]$DateTag = "2026-07-18"
)

$ErrorActionPreference = "Stop"

$issues = gh issue list --repo $Repo --state open --search "[module:" --limit 500 --json number,title | ConvertFrom-Json
$moduleToIssue = @{}
foreach ($i in $issues) {
    if ($i.title -match '^\[module:([^\]]+)\]') {
        $moduleToIssue[$matches[1]] = [int]$i.number
    }
}

$updated = New-Object System.Collections.Generic.List[string]

foreach ($m in $Modules) {
    if (-not $moduleToIssue.ContainsKey($m)) {
        continue
    }

    $n = $moduleToIssue[$m]
    $road = Join-Path $Root "$m\ROADMAP.md"
    $future = Join-Path $Root "$m\FUTURE_ENHANCEMENTS.md"

    if (-not (Test-Path $road) -or -not (Test-Path $future)) {
        continue
    }

    $roadLines = Get-Content -Path $road
    $futureLines = Get-Content -Path $future

    $roadOpen = @($roadLines | Where-Object { $_ -match '^\s*- \[( |~|I|\?|!)\]' } | Select-Object -First 8)
    $roadDone = @($roadLines | Where-Object { $_ -match '^\s*- \[x\]' } | Select-Object -First 4)
    $futureFocus = @($futureLines | Where-Object { $_ -match '^\s*- ' } | Select-Object -First 8)

    $eBuild = "not yet captured in this issue update"
    $eTest = "not yet captured in this issue update"
    $eResult = "Evidence gap - canonical content sync completed; focused verification pending."

    if ($m -eq "api") {
        $eBuild = "module_api_test_api_contracts_focused"
        $eTest = "module_api_test_api_contracts_focused"
        $eResult = "PASS (focused test run previously validated)."
    } elseif ($m -eq "replication") {
        $eBuild = "module_replication_test_replication_conflict_focused"
        $eTest = "module_replication_test_replication_conflict_focused"
        $eResult = "PASS (focused test run previously validated)."
    } elseif ($m -eq "server") {
        $eBuild = "module_server_test_server_activation_profile_focused"
        $eTest = "module_server_test_server_activation_profile_focused"
        $eResult = "FAIL - ServerActivationProfile.StandardProfileRequiresCoreProductionFlags (SEH 0xC0000005 observed in focused run)."
    }

    $lines = @()
    $lines += "## Module Identity"
    $lines += ""
    $lines += "- Module: $m"
    $lines += "- Issue: #$n"
    $lines += "- Parent Epic: #5624"
    $lines += "- Area Label: area:$m"
    $lines += "- Roadmap Path: src/$m/ROADMAP.md"
    $lines += "- Future Path: src/$m/FUTURE_ENHANCEMENTS.md"
    $lines += ""
    $lines += "## Current Status"
    $lines += ""
    $lines += "- Status: [ ] open"
    $lines += "- Last validated: $DateTag"
    $lines += "- Canonical synchronization: first content pass completed from ROADMAP and FUTURE_ENHANCEMENTS."
    $lines += "- Implementation coverage: partial, evidence expansion still required."
    $lines += ""
    $lines += "## Roadmap Open Priorities (Synced Snapshot)"
    $lines += ""
    if ($roadOpen.Count -gt 0) {
        $lines += $roadOpen
    } else {
        $lines += "- No open roadmap checkbox entries extracted in this pass."
    }
    $lines += ""
    $lines += "## Roadmap Completed Highlights"
    $lines += ""
    if ($roadDone.Count -gt 0) {
        $lines += $roadDone
    } else {
        $lines += "- No completed roadmap items extracted in this pass."
    }
    $lines += ""
    $lines += "## Future Enhancements Focus (Synced Snapshot)"
    $lines += ""
    if ($futureFocus.Count -gt 0) {
        $lines += $futureFocus
    } else {
        $lines += "- No future enhancement bullets extracted in this pass."
    }
    $lines += ""
    $lines += "## Evidence"
    $lines += ""
    $lines += "- Build preset: windows-release"
    $lines += "- Build target(s): $eBuild"
    $lines += "- Test target(s): $eTest"
    $lines += "- Latest run/result: $eResult"
    $lines += ""
    $lines += "## Open Work"
    $lines += ""
    $lines += "- [ ] Validate and refine extracted roadmap priorities against full module docs in src/$m/ROADMAP.md"
    $lines += "- [ ] Validate and refine extracted future focus points against full module docs in src/$m/FUTURE_ENHANCEMENTS.md"
    $lines += "- [ ] Add/refresh focused build and test evidence for this module."
    $lines += "- [ ] Mark completed synced items and risks with explicit status transitions."
    $lines += ""
    $lines += "## Closure Criteria"
    $lines += ""
    $lines += "- [ ] All module acceptance criteria updated and traceable."
    $lines += "- [ ] Evidence updated (build/tests) or explicit justified gap."
    $lines += "- [ ] Parent epic task entry checked."
    $lines += "- [ ] Status labels updated before close."
    $lines += "- [ ] Close reason documented (completed or not planned)."

    $body = $lines -join "`n"
    gh issue edit $n --repo $Repo --body $body | Out-Null
    $updated.Add("${m}:#$n") | Out-Null
}

"UPDATED_COUNT=$($updated.Count)"
if ($updated.Count -gt 0) {
    "UPDATED=$($updated -join ',')"
}