param()

$root = Get-Location
$out = 'scripts/root_move_candidates.txt'

$preserve = @(
    '.github', '.gitignore', '.gitattributes', '.vscode', '.venv',
    'CMakeLists.txt', 'CMakePresets.json', 'CMakeUserPresets.json', 'CMakeUserPresets.json.example',
    'cmake', 'cmake/', 'build.ps1', 'build', 'build/', 'README.md', 'CHANGELOG.md', 'LICENSE', 'LICENSE.txt',
    'Dockerfile', 'docker-compose.yml', 'docker-compose.user-storage.yml', 'docker-compose.qnap.yml',
    'docs', 'docs/', 'src', 'include', 'scripts', 'tools', 'ci', 'helm',
    'mkdocs.yml', 'mkdocs-nopdf.yml', 'README.md', 'ROADMAP.md', 'RELEASE_STRATEGY.md', 'BRANCHING_STRATEGY.md',
    'VERSIONING.md', 'CTEST.md', 'Doxyfile', 'Doxyfile.local', 'Doxyfile.html.local'
)

Remove-Item -LiteralPath $out -ErrorAction SilentlyContinue

$files = Get-ChildItem -Path $root -Force | Where-Object { -not $_.PSIsContainer }

foreach ($f in $files) {
    $name = $f.Name
    $skip = $false
    foreach ($p in $preserve) {
        if ($name -ieq $p) { $skip = $true; break }
        # allow pattern prefixes
        if ($p.EndsWith('/') -or $p.EndsWith('\')) { continue }
    }
    # skip dotfiles and git internals
    if ($name -like '.git*' -or $name -like '.g*') { $skip = $true }
    if (-not $skip) {
        # also skip common binary artifacts
        if ($name -like '*.sln' -or $name -like '*.vcxproj*' -or $name -like '*.obj' -or $name -like '*.log') { continue }
        $f.FullName | Out-File -FilePath $out -Append
    }
}

$cnt = (Get-Content $out | Measure-Object).Count
Write-Output "Root move candidates: $cnt (written to $out)"
if ($cnt -gt 0) { Get-Content $out | Select-Object -First 50 | ForEach-Object { Write-Output $_ } }
