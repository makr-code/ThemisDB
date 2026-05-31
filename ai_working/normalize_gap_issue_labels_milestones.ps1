$repo = 'makr-code/ThemisDB'

$all = gh issue list --repo $repo --state all --limit 1000 --json number,title,state,labels | ConvertFrom-Json
$targets = $all | Where-Object {
  (($_.labels | ForEach-Object { $_.name }) -contains 'gap-remediation') -or
  ($_.title -match '^\[Wave1\]') -or
  ($_.title -match '^\[PHASE 1-5\] Gap Scanner Analysis') -or
  ($_.title -match '^\[P0-CRITICAL\].*Module') -or
  ($_.title -match '^\[(SECURITY|MEMORY|RELIABILITY|CONCURRENCY|RAII|CONTAINER|PLATFORM|PERFORMANCE|TYPE CONVERSION|INPUT VALIDATION|EXCEPTION SAFETY|UNINITIALIZED|OOP DESIGN)\]')
} | Sort-Object number

$existingLabelRows = gh label list --repo $repo --limit 500
$labelSet = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
foreach ($row in $existingLabelRows) {
  $name = ($row -split '\s{2,}')[0].Trim()
  if ($name) { [void]$labelSet.Add($name) }
}

$ok = 0
$fail = 0

foreach ($i in $targets) {
  $existing = @($i.labels | ForEach-Object { $_.name })
  $toAdd = New-Object System.Collections.Generic.List[string]

  if ($existing -notcontains 'gap-remediation') { $toAdd.Add('gap-remediation') }

  $title = $i.title
  if ($title -match '^\[P0-CRITICAL\]\s+(.+?)\s+Module') {
    $module = $Matches[1].Trim().ToLowerInvariant()
    foreach ($lbl in @('type:bug', 'priority:P0')) {
      if ($existing -notcontains $lbl) { $toAdd.Add($lbl) }
    }

    $areaMap = @{
      llm = 'area:llm'; server = 'area:server'; sharding = 'area:sharding';
      index = 'area:index'; query = 'area:query'; storage = 'area:storage';
      analytics = 'area:analytics'; rag = 'area:rag'; security = 'area:security';
      content = 'area:content'
    }

    if ($areaMap.ContainsKey($module)) {
      $a = $areaMap[$module]
      if (($existing -notcontains $a) -and $labelSet.Contains($a)) { $toAdd.Add($a) }
    }
  }
  elseif ($title -match '^\[PHASE 1-5\] Gap Scanner Analysis') {
    foreach ($lbl in @('type:enhancement', 'strategic', 'high-priority')) {
      if ($existing -notcontains $lbl) { $toAdd.Add($lbl) }
    }
  }
  elseif ($title -match '^\[(SECURITY|MEMORY|RELIABILITY|CONCURRENCY|RAII|CONTAINER|PLATFORM|PERFORMANCE|TYPE CONVERSION|INPUT VALIDATION|EXCEPTION SAFETY|UNINITIALIZED|OOP DESIGN)\]') {
    if ($existing -notcontains 'type:bug') { $toAdd.Add('type:bug') }

    $hasPriority =
      ($existing -contains 'priority:P0') -or
      ($existing -contains 'priority:P1') -or
      ($existing -contains 'priority:P2') -or
      ($existing -contains 'priority:P3')

    if (-not $hasPriority) { $toAdd.Add('priority:P1') }
  }

  if ($title -match '^\[Wave1\]') {
    if ($title -match '\[W1-[SL]0[1-2]\]' -and $existing -notcontains 'priority:P0') { $toAdd.Add('priority:P0') }
    elseif ($title -match '\[W1-[SL]0[3-6]\]' -and $existing -notcontains 'priority:P1') { $toAdd.Add('priority:P1') }
    elseif ($title -match '\[W1-[SL]0[7]\]' -and $existing -notcontains 'priority:P2') { $toAdd.Add('priority:P2') }
  }

  $milestone = if ($i.state -eq 'OPEN') { 'v2.0.0' } else { 'v1.9.0' }

  $args = @('issue', 'edit', "$($i.number)", '--repo', $repo, '--milestone', $milestone)
  foreach ($lbl in ($toAdd | Select-Object -Unique)) {
    if ($labelSet.Contains($lbl)) {
      $args += @('--add-label', $lbl)
    }
  }

  & gh @args | Out-Null
  if ($LASTEXITCODE -eq 0) {
    $ok++
  }
  else {
    $fail++
    Write-Output "fail #$($i.number)"
  }
}

Write-Output "SUMMARY ok=$ok fail=$fail total=$($targets.Count)"
