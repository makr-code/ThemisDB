$ErrorActionPreference = [System.Management.Automation.ActionPreference]::Stop
$PSNativeCommandUseErrorActionPreference = $false
$workspaceRoot = (Get-Location).Path
$excludePathPattern = "\\(vcpkg|vcpkg_installed|external|llama_cpp)\\"

$llvmBin = "C:\llvm\bin"
$candidates = @(
    (Join-Path $llvmBin "clang-cl.exe"),
    (Join-Path $llvmBin "clang++.exe"),
    (Join-Path $llvmBin "clang.exe"),
    "clang-cl.exe",
    "clang-cl",
    "clang++.exe",
    "clang++",
    "clang.exe",
    "clang"
)

$clang = $null
foreach ($cand in $candidates) {
    if ((Test-Path $cand) -or (Get-Command $cand -ErrorAction SilentlyContinue)) {
        $clang = $cand
        break
    }
}

if (-not $clang) {
    Write-Error "No clang compiler found. Checked C:\\llvm\\bin and PATH (clang++, clang-cl, clang)."
    exit 1
}

$files = Get-ChildItem -Path "src" -Recurse -File -Include *.cpp,*.cc,*.cxx |
    Where-Object { $_.FullName -notmatch $excludePathPattern }
if (-not $files) {
    Write-Host "No C++ source files found under src."
    exit 0
}

$vcpkgInclude = "vcpkg_installed/x64-windows/include"
$upbInclude = "vcpkg_installed/x64-windows/include/upb/reflection/stage0"
$zlibInclude = "vcpkg/packages/zlib_x64-windows/include"

$clangFileName = [System.IO.Path]::GetFileName("$clang").ToLowerInvariant()
$isClangCl = $clangFileName.StartsWith("clang-cl")
if ($isClangCl) {
    $common = @(
        "/std:c++20",
        "/clang:-fsyntax-only",
        "/EHsc",
        "/W4",
        "/Iinclude",
        "/Isrc",
        "/I$vcpkgInclude",
        "/I$upbInclude",
        "/I$zlibInclude",
        "/DWIN32",
        "/D_WINDOWS",
        "/DWIN32_LEAN_AND_MEAN",
        "/DNOMINMAX",
        "/D_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH"
    )
} else {
    $common = @(
        "-std=c++20",
        "-fsyntax-only",
        "-Wall",
        "-Wextra",
        "-Iinclude",
        "-Isrc",
        "-I$vcpkgInclude",
        "-I$upbInclude",
        "-I$zlibInclude",
        "-DWIN32",
        "-D_WINDOWS",
        "-DWIN32_LEAN_AND_MEAN",
        "-DNOMINMAX",
        "-D_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH"
    )
}

$failed = 0
foreach ($f in $files) {
    Write-Host ("[llvm-syntax] " + $f.FullName)
    $previousEap = $ErrorActionPreference
    $ErrorActionPreference = [System.Management.Automation.ActionPreference]::Continue
    $output = & $clang @common $f.FullName 2>&1
    $ErrorActionPreference = $previousEap
    $projectDiagnostics = @($output | ForEach-Object { "$_" } | Where-Object {
        $_ -match [regex]::Escape($workspaceRoot) -and $_ -notmatch $excludePathPattern
    })
    if ($projectDiagnostics.Count -gt 0) {
        $projectDiagnostics | ForEach-Object { Write-Host $_ }
    }

    $hasProjectErrors = ($projectDiagnostics | Where-Object { $_ -match "\bfatal error:\b|\berror:\b" }).Count -gt 0
    if ($LASTEXITCODE -ne 0 -and $hasProjectErrors) {
        $failed++
    }
}

if ($failed -gt 0) {
    Write-Error ("LLVM syntax-only failed for $failed file(s).")
    exit 1
}

Write-Host "LLVM syntax-only batch completed successfully."
exit 0
