# Lists focused test executable and PDB files under common build dirs
param(
    [string]$Root = '.'
)
Write-Host "Searching in: $Root`n"
# find focused exe
$exe = Get-ChildItem -Path $Root -Recurse -Filter 'test_self_rag_alce_focused.exe' -File -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $exe) {
    $exe = Get-ChildItem -Path $Root -Recurse -Include '*self_rag*focused*.exe' -File -ErrorAction SilentlyContinue | Select-Object -First 1
}
if ($exe) { Write-Host "Found exe:`n" $exe.FullName } else { Write-Host "No focused exe found." }

Write-Host "`nSearching for PDBs under build* directories...`n"
$pdbs = Get-ChildItem -Path $Root -Recurse -Include *.pdb -File -ErrorAction SilentlyContinue | Where-Object { $_.FullName -match '\\build' -or $_.FullName -match '\\bin\\' -or $_.FullName -match 'build-' } | Select-Object FullName,Length
if ($pdbs.Count -gt 0) { $pdbs | Format-Table -AutoSize } else { Write-Host "No PDBs found under build dirs." }

Write-Host "`nDone.`n"
