Param()

Set-StrictMode -Version Latest

$relationsPath = "artifacts/epic_relations.json"
if (-not (Test-Path $relationsPath)) {
    Write-Output "Relations file not found: $relationsPath — attempting to reconstruct from GitHub issues (this may take a while)."
    $allIssuesJson = gh issue list --state all --limit 1000 --json number,title,body,url
    if (-not $allIssuesJson) { Write-Error "Failed to list issues via gh. Please create $relationsPath first."; exit 1 }
    $allIssues = $allIssuesJson | ConvertFrom-Json
    $reln = @()
    foreach ($iss in $allIssues) {
        [regex]$rx = '#(\d{2,6})'
        if ($iss.title) {
            $mt = $rx.Matches($iss.title)
            foreach ($m in $mt) {
                $parent = [int]$m.Groups[1].Value
                $reln += [PSCustomObject]@{ parent = $parent; child = $iss.number; child_url = $iss.url }
            }
        }
        if ($iss.body) {
            $bodyText = [string]$iss.body
            $mb = $rx.Matches($bodyText)
            foreach ($m in $mb) {
                $parent = [int]$m.Groups[1].Value
                $reln += [PSCustomObject]@{ parent = $parent; child = $iss.number; child_url = $iss.url }
            }
        }
    }
    if ($reln.Count -eq 0) { Write-Output "No relations found via issue scan."; $reln | ConvertTo-Json | Out-File $relationsPath -Encoding utf8; }
    else { $reln | ConvertTo-Json -Depth 4 | Out-File $relationsPath -Encoding utf8; Write-Output "Reconstructed relations: $($reln.Count) -> $relationsPath" }
}
$relations = Get-Content $relationsPath | ConvertFrom-Json
$parents = $relations | Select-Object -ExpandProperty parent | Sort-Object -Unique

$openPRsJson = gh pr list --state open --limit 1000 --json number,title,body,url,headRefName,baseRefName
if (-not $openPRsJson) { Write-Output "No open PRs found or gh failed."; exit 0 }
$openPRs = $openPRsJson | ConvertFrom-Json

$report = @()
$retargetable = 0
$noBranch = 0

foreach ($pr in $openPRs) {
    $matched = @()
    foreach ($p in $parents) {
        if ($null -ne $pr.title -and $pr.title -match "#${p}") { $matched += $p; continue }
        if ($null -ne $pr.body -and $pr.body -match "#${p}") { $matched += $p; continue }
    }
    if ($matched.Count -eq 0) { continue }

    foreach ($parent in $matched) {
        $entry = [PSCustomObject]@{
            pr_number = $pr.number
            pr_url = $pr.url
            head = $pr.headRefName
            base = $pr.baseRefName
            parent = $parent
            candidate_branch = $null
            suggestion = $null
        }

        $branches = git branch -r --list "origin/*${parent}*" | ForEach-Object { $_.Trim() }
        if ($branches -and $branches.Count -gt 0) {
            $candidate = ($branches[0] -replace '^origin/','')
            $entry.candidate_branch = $candidate
            $entry.suggestion = "would retarget to $candidate"
            $retargetable++
        } else {
            $entry.suggestion = 'no matching epic branch found'
            $noBranch++
        }

        $report += $entry
    }
}

$outPath = 'artifacts/epic_retarget_dryrun_report.json'
$report | ConvertTo-Json -Depth 6 | Out-File $outPath -Encoding utf8
Write-Output "Wrote dry-run report to $outPath. Entries: $($report.Count) Retargetable: $retargetable NoBranch: $noBranch"
