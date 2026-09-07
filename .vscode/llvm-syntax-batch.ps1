$ErrorActionPreference = [System.Management.Automation.ActionPreference]::Stop
$PSNativeCommandUseErrorActionPreference = $false
$workspaceRoot = (Get-Location).Path
$excludePathPattern = "\\(vcpkg|vcpkg_installed|external|llama_cpp)\\"

# Detect OS for cross-compiler compatibility
$isWinPlatform = ($env:OS -eq "Windows_NT") -or ($PSVersionTable.Platform -eq "Win32NT") -or ($PSVersionTable.OS -match "Windows")
$isLinuxPlatform = (-not $isWinPlatform) -and ($PSVersionTable.OS -match "Linux")
$isMacPlatform = (-not $isWinPlatform) -and ($PSVersionTable.OS -match "Darwin")

$vcpkgArch = if ($isWinPlatform) { "x64-windows" } elseif ($isLinuxPlatform) { "x64-linux" } elseif ($isMacPlatform) { "arm64-osx" } else { "x64-linux" }

# Determine appropriate compiler candidates based on OS
$llvmBin = if ($isWinPlatform) { "C:\llvm\bin" } else { "/usr/bin:/usr/local/bin" }
$candidates = if ($isWinPlatform) {
    @(
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
} else {
    @(
        "clang++",
        "clang",
        "g++",
        "gcc"
    )
}

$clang = $null
foreach ($cand in $candidates) {
    if ((Test-Path $cand) -or (Get-Command $cand -ErrorAction SilentlyContinue)) {
        $clang = $cand
        break
    }
}

if (-not $clang) {
    $msg = if ($isWinPlatform) { "No clang/LLVM compiler found. Checked C:\llvm\bin and PATH." } else { "No clang/GCC compiler found in PATH." }
    Write-Error $msg
    exit 1
}

Write-Host "--- LLVM Syntax Check (Cross-Compiler Aware) ---"
Write-Host "Platform: $(if ($isWinPlatform) { 'Windows' } elseif ($isLinuxPlatform) { 'Linux' } elseif ($isMacPlatform) { 'macOS' } else { 'Unknown' })"
Write-Host "Compiler: $clang"
Write-Host "vcpkg Architecture: $vcpkgArch"
Write-Host ""

$files = Get-ChildItem -Path "src" -Recurse -File -Include *.c,*.cpp,*.cc,*.cxx |
    Where-Object { $_.FullName -notmatch $excludePathPattern }
if (-not $files) {
    Write-Host "No C++ source files found under src."
    exit 0
}


$vcpkgInclude = "vcpkg_installed/$vcpkgArch/include"
$upbInclude = "vcpkg_installed/$vcpkgArch/include/upb/reflection/stage0"

# Detect compiler capabilities and choose appropriate flags
$clangFileName = [System.IO.Path]::GetFileName("$clang").ToLowerInvariant()
$isClangCl = $isWinPlatform -and $clangFileName.StartsWith("clang-cl")
$isGcc = $clangFileName.StartsWith("g")

if ($isClangCl) {
    # Windows MSVC-compatible mode
    $common = @(
        "/std:c++20",
        "/clang:-fsyntax-only",
        "/EHsc",
        "/W4",
        "/Iinclude",
        "/Isrc",
        "/I$vcpkgInclude",
        "/I$upbInclude"
    )
    # Platform-specific defines only for Windows
    if ($isWinPlatform) {
        $common += @(
            "/DWIN32",
            "/D_WINDOWS",
            "/DWIN32_LEAN_AND_MEAN",
            "/DNOMINMAX",
            "/D_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH"
        )
    }
} else {
    # GCC/Clang-GNU compatible mode (portable)
    $common = @(
        "-std=c++20",
        "-fsyntax-only",
        "-Wall",
        "-Wextra",
        "-Iinclude",
        "-Isrc",
        "-I$vcpkgInclude",
        "-I$upbInclude"
    )
    # Platform-specific defines
    if ($isWinPlatform) {
        $common += @("-DWIN32", "-D_WINDOWS", "-DWIN32_LEAN_AND_MEAN", "-DNOMINMAX")
    } elseif ($isLinuxPlatform) {
        $common += @("-DLINUX", "-D_GNU_SOURCE")
    } elseif ($isMacPlatform) {
        $common += @("-DMACOS", "-D_DARWIN_C_SOURCE")
    }
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
    Write-Host ""
    Write-Error ("LLVM syntax check failed for $failed of " + $files.Count + " file(s).")
    exit 1
}

Write-Host ""
Write-Host ("LLVM syntax check completed successfully - {0} file(s) verified." -f $files.Count)
exit 0
