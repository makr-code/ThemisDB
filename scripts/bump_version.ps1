param(
    [ValidateSet('major','minor','patch')]
    [string]$Part,
    [string]$Set,
    [string]$Pre
)

$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $MyInvocation.MyCommand.Path | Split-Path -Parent
Set-Location $repo

$versionFile = Join-Path $repo 'VERSION'
if (-not (Test-Path $versionFile)) { throw "VERSION file not found" }
$cur = (Get-Content $versionFile -Raw).Trim()

function Parse-Version([string]$v) {
    $m = [regex]::Match($v, '^(\d+)\.(\d+)\.(\d+)(-.+)?$')
    if (-not $m.Success) { throw "Invalid version: $v" }
    [pscustomobject]@{ major=[int]$m.Groups[1].Value; minor=[int]$m.Groups[2].Value; patch=[int]$m.Groups[3].Value; pre=$m.Groups[4].Value }
}

$v = Parse-Version $cur

if ($Set) {
    $new = $Set
} else {
    switch ($Part) {
        'major' { $new = "{0}.0.0" -f ($v.major + 1) }
        'minor' { $new = "{0}.{1}.0" -f $v.major, ($v.minor + 1) }
        'patch' { $new = "{0}.{1}.{2}" -f $v.major, $v.minor, ($v.patch + 1) }
        default { throw "Specify -Part (major|minor|patch) or -Set <x.y.z>" }
    }
}
if ($Pre) { $new = "$new-$Pre" }

$new | Out-File -FilePath $versionFile -Encoding ASCII -NoNewline
Write-Host "Version bumped: $cur -> $new" -ForegroundColor Green

# Optional: update docs badges etc. (extend as needed)
