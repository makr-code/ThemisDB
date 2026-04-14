$ErrorActionPreference = 'Continue'
$wfDir = '.github/workflows'
$files = Get-ChildItem $wfDir -File -Filter '04-release_*.yml' | Sort-Object Name
$results = @()
New-Item -ItemType Directory -Force -Path tmp | Out-Null

function Add-Result($wf,$event,$status,$code,$note,$err){
  $script:results += [pscustomobject]@{ Workflow=$wf; Event=$event; Status=$status; ExitCode=$code; Note=$note; ErrorLine=$err }
}

foreach($f in $files){
  $content = Get-Content $f.FullName -Raw
  $event = if($content -match 'workflow_dispatch:'){ 'workflow_dispatch' }
           elseif($content -match 'release:'){ 'release' }
           elseif($content -match 'push:'){ 'push' }
           elseif($content -match 'pull_request:'){ 'pull_request' }
           elseif($content -match 'workflow_call:'){ 'workflow_call' }
           else { '' }

  $args = @()
  $note = ''
  if($event -eq 'workflow_dispatch'){
    $evFile = 'tmp/act-event-generic.json'
    '{}' | Set-Content -Encoding utf8 $evFile
    if($f.Name -eq '04-release_build-binary-linux.yml' -or $f.Name -eq '04-release_build-binary-windows.yml'){
      $evFile = 'tmp/act-event-release-binary.json'
      '{"inputs":{"tag_name":"v1.9.0","dry_run":"true"}}' | Set-Content -Encoding utf8 $evFile
    }
    if($f.Name -eq '04-release_create-release-archive.yml'){
      $evFile = 'tmp/act-event-release-archive.json'
      '{"inputs":{"version":"1.9.0","prerelease":"false"}}' | Set-Content -Encoding utf8 $evFile
    }
    $args = @('workflow_dispatch','-W',$f.FullName,'-e',$evFile,'-n')
  } elseif($event -eq 'release'){
    $evFile = 'tmp/act-event-release.json'
    '{"release":{"tag_name":"v1.9.0","draft":false,"prerelease":false},"action":"published"}' | Set-Content -Encoding utf8 $evFile
    $args = @('release','-W',$f.FullName,'-e',$evFile,'-n')
  } elseif($event -eq 'push'){
    $evFile = 'tmp/act-event-push.json'
    '{"ref":"refs/tags/v1.9.0"}' | Set-Content -Encoding utf8 $evFile
    $args = @('push','-W',$f.FullName,'-e',$evFile,'-n')
  } elseif($event -eq 'pull_request'){
    $evFile = 'tmp/act-event-pr.json'
    '{"pull_request":{"head":{"ref":"feature/test"},"base":{"ref":"main"}},"action":"opened"}' | Set-Content -Encoding utf8 $evFile
    $args = @('pull_request','-W',$f.FullName,'-e',$evFile,'-n')
  } elseif($event -eq 'workflow_call'){
    Add-Result $f.Name $event 'SKIP' '' 'workflow_call-only: skipped' ''
    continue
  } else {
    Add-Result $f.Name '' 'SKIP' '' 'no recognizable trigger' ''
    continue
  }

  if($f.Name -eq '04-release_build-binary-windows.yml'){
    $args += @('-P','windows-latest=catthehacker/ubuntu:act-latest')
    $note = 'windows mapped to ubuntu image for dry-run compatibility'
  }

  $output = & act @args 2>&1
  $rc = $LASTEXITCODE
  $status = if($rc -eq 0){'PASS'} else {'FAIL'}
  $err = ($output | Select-String -Pattern 'Error:|authentication required|unable to|invalid|failed' | Select-Object -First 1).Line
  Add-Result $f.Name $event $status $rc $note $err
}

$results | ConvertTo-Json -Depth 4 | Set-Content -Encoding utf8 tmp/act-04-release-dryrun-matrix.json
$results | Sort-Object Workflow | Format-Table -AutoSize | Out-String | Set-Content -Encoding utf8 tmp/act-04-release-dryrun-matrix.txt
$results | Sort-Object Workflow | Format-Table -AutoSize
Write-Host 'Report: tmp/act-04-release-dryrun-matrix.json'
Write-Host 'Report: tmp/act-04-release-dryrun-matrix.txt'
