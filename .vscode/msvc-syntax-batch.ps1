$ErrorActionPreference = [System.Management.Automation.ActionPreference]::Stop

if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    Write-Error "cl.exe not found. Open a Developer PowerShell for VS 2022."
    exit 1
}

$files = Get-ChildItem -Path "src" -Recurse -File -Include *.cpp,*.cc,*.cxx
if (-not $files) {
    Write-Host "No C++ source files found under src."
    exit 0
}

$common = @(
    "/nologo", "/std:c++20", "/permissive-", "/EHsc", "/W4", "/utf-8",
    "/Zs", "/c", "/DWIN32", "/D_WINDOWS", "/DWIN32_LEAN_AND_MEAN", "/DNOMINMAX"
)
$includes = @("/Iinclude", "/Isrc")

$failed = 0
foreach ($f in $files) {
    Write-Host ("[syntax] " + $f.FullName)
    & cl.exe @common @includes $f.FullName 2>&1
    if ($LASTEXITCODE -ne 0) {
        $failed++
    }
}

if ($failed -gt 0) {
    Write-Error ("Syntax-only failed for $failed file(s).")
    exit 1
}

Write-Host "Syntax-only batch completed successfully."
exit 0
