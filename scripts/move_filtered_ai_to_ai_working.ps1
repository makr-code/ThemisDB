param()

$list = 'scripts/ai_move_filtered.txt'
$targetDir = 'ai_working'
$results = 'scripts/move_ai_md_results.txt'

if (-not (Test-Path $list)) {
    Write-Error "List file not found: $list"
    exit 1
}

if (-not (Test-Path $targetDir)) {
    New-Item -ItemType Directory -Path $targetDir | Out-Null
}

Remove-Item -LiteralPath $results -ErrorAction SilentlyContinue

$movedGit = 0
$movedFs = 0
$failed = 0

foreach ($abs in Get-Content $list) {
    if (-not $abs) { continue }
    try {
        $absPath = (Resolve-Path -LiteralPath $abs).Path
    } catch {
        "MISSING: $abs" | Out-File -FilePath $results -Append
        $failed++;
        continue
    }
    $cwd = (Get-Location).Path
    if ($absPath.StartsWith($cwd, [System.StringComparison]::OrdinalIgnoreCase)) {
        $rel = $absPath.Substring($cwd.Length+1)
    } else {
        $rel = $absPath
    }
    $basename = Split-Path -Leaf $rel
    $dest = Join-Path $targetDir $basename
    if (Test-Path $dest) {
        $nameOnly = [System.IO.Path]::GetFileNameWithoutExtension($basename)
        $ext = [System.IO.Path]::GetExtension($basename)
        $dest = Join-Path $targetDir ("{0}_{1}{2}" -f $nameOnly, (Get-Random -Maximum 10000), $ext)
    }

    # check if tracked by git
    git ls-files --error-unmatch -- "${rel}" 2>$null
    if ($LASTEXITCODE -eq 0) {
        # tracked -> use git mv
        git mv -- "${rel}" "${dest}"
        if ($LASTEXITCODE -eq 0) {
            "GITMV: $rel -> $dest" | Out-File -FilePath $results -Append
            $movedGit++
        } else {
            "GITMV_FAIL: $rel -> $dest" | Out-File -FilePath $results -Append
            $failed++
        }
    } else {
        try {
            Move-Item -LiteralPath $rel -Destination $dest -Force
            "FSMV: $rel -> $dest" | Out-File -FilePath $results -Append
            $movedFs++
        } catch {
            "FSMV_FAIL: $rel -> $dest : $($_.Exception.Message)" | Out-File -FilePath $results -Append
            $failed++
        }
    }
}

"SUMMARY: moved_git=$movedGit moved_fs=$movedFs failed=$failed" | Out-File -FilePath $results -Append
Write-Output "Done. Results written to $results"
