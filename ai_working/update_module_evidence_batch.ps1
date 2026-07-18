param(
    [Parameter(Mandatory = $true)]
    [string]$EvidenceJsonPath,
    [string]$Repo = "makr-code/ThemisDB",
    [string]$DateTag = "2026-07-18"
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path $EvidenceJsonPath)) {
    throw "EvidenceJsonPath not found: $EvidenceJsonPath"
}

$evidence = Get-Content -Raw -Path $EvidenceJsonPath | ConvertFrom-Json

$issues = gh issue list --repo $Repo --state open --search "[module:" --limit 500 --json number,title | ConvertFrom-Json
$moduleToIssue = @{}
foreach ($i in $issues) {
    if ($i.title -match '^\[module:([^\]]+)\]') {
        $moduleToIssue[$matches[1]] = [int]$i.number
    }
}

$updated = New-Object System.Collections.Generic.List[string]

foreach ($item in $evidence) {
    $m = [string]$item.module
    if (-not $moduleToIssue.ContainsKey($m)) {
        continue
    }

    $n = $moduleToIssue[$m]
    $body = gh issue view $n --repo $Repo --json body --jq .body

    $newEvidence = @(
        "## Evidence",
        "",
        "- Build preset: windows-release",
        "- Last validated: $DateTag",
        "- Build target(s): $($item.build)",
        "- Test target(s): $($item.test)",
        "- Latest run/result: $($item.result)"
    ) -join "`n"

    $newBody = [regex]::Replace($body, '## Evidence[\s\S]*?## Open Work', $newEvidence + "`n`n## Open Work")
    gh issue edit $n --repo $Repo --body $newBody | Out-Null
    $updated.Add("${m}:#$n") | Out-Null
}

"UPDATED_COUNT=$($updated.Count)"
if ($updated.Count -gt 0) {
    "UPDATED=$($updated -join ',')"
}