param(
    [string]$ExePath = ''
)
if (-not $ExePath) {
    $ExePath = 'build-msvc-windows-release\bin\test_self_rag_alce_focused.exe'
}
if (-not (Test-Path $ExePath)) { Write-Error "EXE not found: $ExePath"; exit 1 }
Write-Host "Reading PE: $ExePath`n"
$bytes = [System.IO.File]::ReadAllBytes($ExePath)

# search for ASCII marker 'RSDS'
$sig = [System.Text.Encoding]::ASCII.GetBytes('RSDS')
$pos = -1
for ($i=0; $i -lt $bytes.Length - $sig.Length; $i++) {
    $match = $true
    for ($j=0; $j -lt $sig.Length; $j++) { if ($bytes[$i+$j] -ne $sig[$j]) { $match = $false; break } }
    if ($match) { $pos = $i; break }
}
if ($pos -lt 0) { Write-Host 'No RSDS signature found in PE.'; exit 0 }

$offset = $pos + 4
# read GUID (16 bytes)
$guidBytes = $bytes[$offset..($offset+15)]
# GUID in RSDS is little-endian for first parts; construct accordingly
$guid = [System.Guid]::New([System.ReadOnlySpan[byte]]$guidBytes)
$offset += 16
# age (4 bytes little-endian)
$age = [BitConverter]::ToUInt32($bytes, $offset)
$offset += 4
# read null-terminated UTF8 (or ANSI) path
$sb = New-Object System.Text.StringBuilder
while ($offset -lt $bytes.Length) {
    $b = $bytes[$offset]; $offset++;
    if ($b -eq 0) { break }
    $sb.Append([char]$b) |> Out-Null
}
$pdbPath = $sb.ToString()

Write-Host "Found CodeView RSDS:`n GUID: $guid`n Age: $age`n PDB path (embedded): $pdbPath`n"

Write-Host 'Searching for PDB files with matching filename in workspace (first 50 results):'
$pdbName = [System.IO.Path]::GetFileName($pdbPath)
Get-ChildItem -Path . -Recurse -Include $pdbName -File -ErrorAction SilentlyContinue | Select-Object FullName,Length | Select-Object -First 50 | Format-Table -AutoSize

Write-Host "\nIf you have a matching PDB, I can generate WinDbg/symchk commands to load it and resolve addresses." 
