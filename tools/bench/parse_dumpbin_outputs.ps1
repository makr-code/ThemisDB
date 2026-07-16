param(
    [string]$ArtifactDir = ''
)
if (-not $ArtifactDir) { $ArtifactDir = (Get-ChildItem -Path artifacts\bench_release -Directory | Sort-Object LastWriteTime -Descending | Select-Object -First 1).FullName }
if (-not $ArtifactDir) { Write-Error 'No artifact dir'; exit 2 }
$hdrFile = Join-Path $ArtifactDir 'dumpbin_headers.txt'
$dirFile = Join-Path $ArtifactDir 'dumpbin_directives.txt'
if (-not (Test-Path $hdrFile)) { Write-Error "Headers file missing: $hdrFile"; exit 3 }
Write-Host "Parsing: $hdrFile"
$hdr = Get-Content -Path $hdrFile -Encoding Unicode -Raw -ErrorAction SilentlyContinue
if (-not $hdr) { Write-Host 'Headers empty after Unicode read; trying ASCII'; $hdr = Get-Content -Path $hdrFile -Encoding ASCII -Raw -ErrorAction SilentlyContinue }
if ($hdr) {
    if ($hdr -match 'Image base:\s*0x?([0-9A-Fa-f]+)') { Write-Host ('ImageBase=0x{0}' -f $Matches[1]) }
    elseif ($hdr -match 'preferred load address is 0x?([0-9A-Fa-f]+)') { Write-Host ('ImageBase=0x{0}' -f $Matches[1]) }
    else { Write-Host 'No image base found in headers.' }
} else { Write-Host 'Failed to read headers file.' }

if (Test-Path $dirFile) {
    Write-Host "Parsing directives: $dirFile"
    $dir = Get-Content -Path $dirFile -Encoding Unicode -Raw -ErrorAction SilentlyContinue
    if (-not $dir) { $dir = Get-Content -Path $dirFile -Encoding ASCII -Raw -ErrorAction SilentlyContinue }
    if ($dir -match 'PdbFile:\s*(.+)') { Write-Host ('PdbFile=' + $Matches[1].Trim()) } else { Write-Host 'No PdbFile line found in directives.' }
} else { Write-Host 'Directives file missing.' }

Write-Host 'Done.'
