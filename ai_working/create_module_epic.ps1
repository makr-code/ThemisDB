$ErrorActionPreference = [System.Management.Automation.ActionPreference]::Stop
$today = Get-Date -Format 'yyyy-MM-dd'
$repo = 'makr-code/ThemisDB'

function New-ModuleBody {
  param(
    [string]$Module,
    [string]$RoadmapPath,
    [string]$FuturePath
  )
@"
## Module Identity

- Module: $Module
- Area Label: area:$Module
- Roadmap Path: $RoadmapPath
- Future Path: $FuturePath

## Current Status

- Status: [ ] open [~] in progress [x] done [?] blocked
- Last validated: $today
- Roadmap progress: <short summary>
- Implementation coverage: <short summary>

## Implementation Phases Snapshot

- [ ] Phase 1: Design / API Contract
- [ ] Phase 2: Core Implementation
- [ ] Phase 3: Error Handling and Edge Cases
- [ ] Phase 4: Tests
- [ ] Phase 5: Performance and Hardening
- [ ] Phase 6: Documentation and Acceptance

## Evidence

- Build preset: windows-release
- Build target(s): <target names>
- Test target(s): <target names>
- Latest run/result: <command + pass/fail summary + date>

## Open Work

- [ ] <task 1> (Target: <milestone/quarter>)
- [ ] <task 2> (Target: <milestone/quarter>)

## Risks / Blockers

- <risk or blocker, if any>

## Next Milestone

- <next concrete step>

## Closure Criteria

- [ ] All module acceptance criteria updated and traceable.
- [ ] Evidence updated (build/tests) or explicit justified gap.
- [ ] Parent epic task entry checked.
- [ ] Status labels updated before close (status:open removed/updated).
- [ ] Close reason documented (completed or not planned).
"@
}

$modules = @(
  @{ Name='api'; Roadmap='src/api/ROADMAP.md'; Future='src/api/FUTURE_ENHANCEMENTS.md' },
  @{ Name='storage'; Roadmap='src/storage/ROADMAP.md'; Future='src/storage/FUTURE_ENHANCEMENTS.md' },
  @{ Name='sharding'; Roadmap='src/sharding/ROADMAP.md'; Future='src/sharding/FUTURE_ENHANCEMENTS.md' },
  @{ Name='llm'; Roadmap='src/llm/ROADMAP.md'; Future='src/llm/FUTURE_ENHANCEMENTS.md' },
  @{ Name='server'; Roadmap='src/server/ROADMAP.md'; Future='src/server/FUTURE_ENHANCEMENTS.md' },
  @{ Name='replication'; Roadmap='src/replication/ROADMAP.md'; Future='src/replication/FUTURE_ENHANCEMENTS.md' }
)

$subIssues = @()
foreach ($m in $modules) {
  $title = "[module:$($m.Name)] Development Status $today"
  $body = New-ModuleBody -Module $m.Name -RoadmapPath $m.Roadmap -FuturePath $m.Future
  $url = gh issue create --repo $repo --title $title --body $body --label "type:enhancement" --label "status:open" --label "priority:medium" --label "area:$($m.Name)"
  $num = [regex]::Match($url, '/issues/(\d+)$').Groups[1].Value
  if (-not $num) { throw "Could not parse issue number from URL: $url" }
  $subIssues += [PSCustomObject]@{ Module=$m.Name; Number=$num; Url=$url; Roadmap=$m.Roadmap; Future=$m.Future }
}

$matrixRows = ($subIssues | ForEach-Object {
  "| $($_.Module) | ``$($_.Roadmap)`` + ``$($_.Future)`` | #$($_.Number) | [ ] | $today |"
}) -join "`n"

$taskRows = ($subIssues | ForEach-Object {
  "- [ ] [module:$($_.Module)] #$($_.Number)"
}) -join "`n"

$epicBody = @"
## Epic Summary

Document and track the current development status for each ThemisDB module.

## Goal

- Establish one source of tracking for module-level progress.
- Link one sub-issue per module.
- Keep roadmap, implementation status, and verification evidence aligned.

## Epic Metadata

- Tracking Date: $today
- Release Lane: develop
- Epic Branch (optional): <epic/...>
- Owner: <team or handle>

## Scope

- Branch: develop (unless explicitly edition-specific).
- Source files per module: ROADMAP.md, FUTURE_ENHANCEMENTS.md, relevant tests/build targets.

## Definition of Done (Epic)

- [ ] Every module has exactly one linked sub-issue.
- [ ] Each sub-issue documents status, blockers, milestones, and evidence.
- [ ] Status labels and checklist state are synchronized before closing.
- [ ] Epic task list is fully linked and up to date.

## Module Status Matrix

| Module | Canonical Paths | Sub-Issue | Status | Last Validation |
|---|---|---|---|---|
$matrixRows

## Module Sub-Issue Tasks

$taskRows

## Epic Checklist (Operational)

- [ ] Sub-issue created for each module row.
- [ ] Sub-issues linked in matrix with issue numbers.
- [ ] Area labels applied consistently.
- [ ] Priority labels normalized across sub-issues.
- [ ] Closure checks run before epic closure.

## Notes

- Keep updates concise and factual.
- If blocked, capture dependency and owner.
- Avoid duplicate/superseded epics; prefer one active parent and cross-links.
"@

$epicTitle = "[EPIC][STATUS][MODULES] ThemisDB Development Status $today"
$epicUrl = gh issue create --repo $repo --title $epicTitle --body $epicBody --label "epic" --label "status:open" --label "priority:medium"
$epicNum = [regex]::Match($epicUrl, '/issues/(\d+)$').Groups[1].Value
if (-not $epicNum) { throw "Could not parse epic number from URL: $epicUrl" }

Write-Output "EPIC: #$epicNum $epicUrl"
$subIssues | ForEach-Object { Write-Output "SUB: $($_.Module) #$($_.Number) $($_.Url)" }
