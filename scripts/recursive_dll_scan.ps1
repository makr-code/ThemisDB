param(
    [string[]]$targets = @("C:\Projects\ThemisDB\build-msvc-windows-release\bin\test_phase1_flash_attention.exe"),
    [string]$bin = 'C:\Projects\ThemisDB\build-msvc-windows-release\bin',
    [string]$dumpbin = 'C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\dumpbin.exe',
    [string]$out = 'C:\Projects\ThemisDB\tmp_recursive_deps.txt'
)
if(Test-Path $out){ Remove-Item $out -Force }
Add-Content -Path $out -Value "Recursive DLL dependency scan started: $(Get-Date)`n"
$visited = @{}
$missing = @{}
$found = @{}
function Get-Dependents($path){
    $lines = & $dumpbin /dependents "$path" 2>$null
    if(-not $lines){ return @() }
    $deps = @()
    foreach($l in $lines){
        if($l -match "([A-Za-z0-9_\-\.]+\.dll)" ){
            $deps += $Matches[1]
        }
    }
    return $deps | Select-Object -Unique
}
function Resolve-Name($name){
    # check in bin
    $binpath = Join-Path $bin $name
    if(Test-Path $binpath){ return @{status='in_bin'; path=$binpath} }
    # check where (on PATH)
    try{ $w = & where.exe $name 2>$null } catch { $w = $null }
    if($w){ return @{status='on_path'; path=$w[0]} }
    # check system32
    $sys = Join-Path $env:SystemRoot "System32\$name"
    if(Test-Path $sys){ return @{status='system'; path=$sys} }
    # check SysWOW64
    $syswow = Join-Path $env:SystemRoot "SysWOW64\$name"
    if(Test-Path $syswow){ return @{status='system'; path=$syswow} }
    return @{status='missing'; path=$null}
}
function Recurse($parentPath, $parentName){
    $deps = Get-Dependents $parentPath
    Add-Content -Path $out -Value "Parent: $parentName -> deps: $($deps -join ', ')"
    foreach($d in $deps){
        if($visited.ContainsKey($d)){ continue }
        $visited[$d] = $true
        $res = Resolve-Name $d
        if($res.status -eq 'missing'){
            $missing[$d] = @()  # init list
            Add-Content -Path $out -Value "MISSING: $d required by $parentName"
        } else {
            $found[$d] = $res.path
            Add-Content -Path $out -Value "FOUND: $d -> $($res.path) (status=$($res.status))"
            # if found in bin, recurse on that physical file
            if($res.status -eq 'in_bin' -or $res.status -eq 'on_path'){
                try{ Recurse $res.path $d } catch { Add-Content -Path $out -Value ("Error recursing {0}: {1}" -f $d, $_) }
            }
        }
    }
}
foreach($t in $targets){
    if(-not (Test-Path $t)){
        Add-Content -Path $out -Value "Target not found: $t"
        continue
    }
    Add-Content -Path $out -Value "\nStarting from target: $t"
    Recurse $t ([System.IO.Path]::GetFileName($t))
}
Add-Content -Path $out -Value "`nSummary:`nFound:"
foreach($k in $found.Keys){ Add-Content -Path $out -Value ("$k -> $($found[$k])") }
Add-Content -Path $out -Value "`nMissing:`n"
if($missing.Keys.Count -eq 0){ Add-Content -Path $out -Value "(none)" } else { foreach($m in $missing.Keys){ Add-Content -Path $out -Value $m } }
Write-Host "Wrote $out"