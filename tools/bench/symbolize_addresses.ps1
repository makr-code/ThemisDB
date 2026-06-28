param(
    [string]$ArtifactDir = ''
)
if (-not $ArtifactDir) {
    $ArtifactDir = (Get-ChildItem -Path artifacts\bench_release -Directory | Sort-Object LastWriteTime -Descending | Select-Object -First 1).FullName
}
if (-not $ArtifactDir) { Write-Error "No artifact directory found under artifacts/bench_release"; exit 2 }
$topFile = Join-Path $ArtifactDir 'backtraces_top10.txt'
if (-not (Test-Path $topFile)) { Write-Error "Top backtraces file not found: $topFile"; exit 3 }

Write-Host "Reading addresses from: $topFile`n"
$text = Get-Content -Path $topFile -Raw -ErrorAction Stop
$addrPattern = '0x[0-9A-Fa-f]+'
$addrs = ([regex]::Matches($text,$addrPattern)) | ForEach-Object { $_.Value } | Select-Object -Unique
if ($addrs.Count -eq 0) { Write-Host "No addresses found in top backtraces."; exit 0 }
Write-Host "Found addresses:`n" ($addrs -join "`n") `n
# locate dumpbin.exe in common VS paths if not in PATH
$dumpbinCmd = (Get-Command dumpbin.exe -ErrorAction SilentlyContinue).Source
if (-not $dumpbinCmd) {
    $candidates = @(
        "$env:ProgramFiles\Microsoft Visual Studio*\*\VC\Tools\MSVC\*\bin\Hostx64\x64\dumpbin.exe",
        "$env:ProgramFiles(x86)\Microsoft Visual Studio*\*\VC\Tools\MSVC\*\bin\Hostx64\x64\dumpbin.exe"
    )
    foreach ($pat in $candidates) {
        $found = Get-ChildItem -Path $pat -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($found) { $dumpbinCmd = $found.FullName; break }
    }
}

if (-not $dumpbinCmd) { Write-Warning "dumpbin.exe not found in PATH or common VS locations. Install/enable Visual Studio build tools or run inside Developer Command Prompt."; exit 0 }
Write-Host "Using dumpbin: $dumpbinCmd`n"

# find focused exe (heuristic)
$exe = Get-ChildItem -Path . -Recurse -Filter 'test_self_rag_alce_focused.exe' -File -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $exe) { $exe = Get-ChildItem -Path . -Recurse -Include '*self_rag*focused*.exe' -File -ErrorAction SilentlyContinue | Select-Object -First 1 }
if (-not $exe) { Write-Warning 'Focused EXE not found under workspace. Provide path to EXE.'; exit 0 }
Write-Host "Found EXE: $($exe.FullName)`n"

# run dumpbin /headers and /directives
$hdrOut = Join-Path $ArtifactDir 'dumpbin_headers.txt'
& $dumpbinCmd /headers $exe.FullName > $hdrOut
& $dumpbinCmd /directives $exe.FullName > (Join-Path $ArtifactDir 'dumpbin_directives.txt')
Write-Host "Wrote dumpbin outputs to: $hdrOut`n"

$hdr = Get-Content -Path $hdrOut -Raw -ErrorAction SilentlyContinue
# try to extract image base
$m = [regex]::Match($hdr, 'Image base:\s*0x?([0-9A-Fa-f]+)')
if (-not $m.Success) { $m = [regex]::Match($hdr, 'preferred load address is 0x?([0-9A-Fa-f]+)') }
if ($m.Success) { $imageBase = [convert]::ToInt64($m.Groups[1].Value,16); Write-Host ('ImageBase=0x{0:X}' -f $imageBase) } else { Write-Warning 'ImageBase not found in dumpbin headers.' }

# try to extract referenced PDB path from directives/dump
$dirText = Get-Content -Path (Join-Path $ArtifactDir 'dumpbin_directives.txt') -Raw -ErrorAction SilentlyContinue
$pdbMatch = [regex]::Match($dirText, 'PdbFile:\s*(.+)')
if ($pdbMatch.Success) { Write-Host "PDB reference: $($pdbMatch.Groups[1].Value.Trim())" }

foreach ($a in $addrs) {
    try {
        $addrVal = [convert]::ToInt64($a,16)
    } catch { Write-Warning "Invalid address: $a"; continue }
    if ($imageBase) {
        $rva = $addrVal - $imageBase
        if ($rva -lt 0) { Write-Host "$a -> RVA negative (module base mismatch)"; continue }
        Write-Host "$a -> RVA=0x{0:X}" -f $rva
    } else {
        Write-Host "$a -> ImageBase unknown; cannot compute RVA." 
    }
}

Write-Host "\nNote: Full symbolization requires matching PDBs and a symbol resolver (WinDbg/cdb with .sympath or symchk). If you want, I can try to locate matching PDB files and suggest exact WinDbg commands to resolve addresses." 
