param(
    [switch]$WhatIf
)

$root = Resolve-Path (Join-Path $PSScriptRoot '..')
Set-Location $root

$dest = Join-Path $root 'ai_working'
if (-not (Test-Path $dest)) { New-Item -ItemType Directory -Path $dest | Out-Null }

$excludeNames = @(
    'CMakeLists.txt', 'CMakePresets.json', 'CMakeUserPresets.json', 'CMakeUserPresets.json.example', 'CTEST.md', 'CMAKE_HARDENING_PLAN.md'
)

$filenamePattern = '(?i)issue|report|phase|gap|query|analysis|analytics|voice|tensor|llm|llama|ai|model|embedding|vector|rag|prompt'
$contentPattern = '(?i)\b(ai|llm|llama|model|ml|analytics|rag|prompt|embedding|vector)\b'

$candidates = @()
Get-ChildItem -Path $root -Filter '*.md' -File | ForEach-Object {
    if ($excludeNames -contains $_.Name) { return }
    $name = $_.Name
    $text = Get-Content -Raw -ErrorAction SilentlyContinue -Path $_.FullName
    $byName = $name -match $filenamePattern
    $byContent = $false
    if ($null -ne $text) { $byContent = ($text -match $contentPattern) }
    if ($byName -or $byContent) {
        $candidates += $_.FullName
    }
}

if ($WhatIf) {
    Write-Output "Dry-run: Found $($candidates.Count) candidate files to move to ai_working/"
    $candidates | ForEach-Object { Write-Output $_ }
    $candidates | Out-File -FilePath scripts/ai_move_candidates.txt -Encoding utf8
} else {
    foreach ($src in $candidates) {
        $fileName = Split-Path $src -Leaf
        $target = Join-Path $dest $fileName
        # use git mv if tracked
        $isTracked = $false
        try { git ls-files --error-unmatch -- "$src" > $null 2>&1; if ($LASTEXITCODE -eq 0) { $isTracked = $true } } catch {}
        if ($isTracked) { git mv -f -- "$src" "$target" 2>$null; if ($LASTEXITCODE -ne 0) { Move-Item -Force -Path $src -Destination $target } }
        else { Move-Item -Force -Path $src -Destination $target }
        Write-Output "Moved: $fileName -> ai_working/"
    }
    Write-Output "Moved $($candidates.Count) files to ai_working/"
}
