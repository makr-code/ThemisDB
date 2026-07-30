<#
Find all directories under projects/ and tools/ that contain a .csproj
and run `git subtree split --prefix=<path> -b split-<sanitized>` for each.
This creates local branches only; it does NOT push to any remote.

Usage: .\scripts\batch_subtree_splits.ps1
#>
Write-Host "Scanning for C# projects under 'projects/' and 'tools/'..."

$roots = @('projects','tools')
$found = @()
$repoRoot = (Get-Location).Path.TrimEnd('\')
foreach($r in $roots){
    if(Test-Path $r){
        $csprojFiles = Get-ChildItem -Path $r -Recurse -Filter *.csproj -File -ErrorAction SilentlyContinue
        foreach($f in $csprojFiles){
            $full = $f.Directory.FullName
            if($full.StartsWith($repoRoot)){
                $rel = $full.Substring($repoRoot.Length + 1) -replace '\\','/'
            } else {
                $rel = $full -replace '\\','/'
            }
            if(-not ($found -contains $rel)){
                $found += $rel
            }
        }
    }
}

if($found.Count -eq 0){
    Write-Host "No C# projects found under projects/ or tools/. Nothing to do."
    exit 0
}

Write-Host "Found the following project directories:`n"
$found | ForEach-Object { Write-Host " - $_" }

foreach($p in $found){
    # sanitize branch name
    $san = $p -replace '[^a-zA-Z0-9]','-'
    $branch = "split-$san"
    Write-Host "\nProcessing '$p' -> branch '$branch'"

    # skip if branch exists
    $exists = git branch --list $branch
    if($exists){
        Write-Host "Branch '$branch' already exists; skipping."
        continue
    }

    if(-not (Test-Path $p)){
        Write-Host "Path '$p' does not exist relative to repository root; skipping."
        continue
    }

    Write-Host "Running: git subtree split --prefix=$p -b $branch"
    git subtree split --prefix=$p -b $branch
    if($LASTEXITCODE -ne 0){
        Write-Host "subtree split failed for $p (exit $LASTEXITCODE). Continuing."
        continue
    } else {
        Write-Host "Created branch '$branch'."
    }
}

Write-Host "All done. No pushes were performed."
