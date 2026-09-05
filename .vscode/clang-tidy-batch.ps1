$ErrorActionPreference = [System.Management.Automation.ActionPreference]::Stop
$PSNativeCommandUseErrorActionPreference = $false
$workspaceRoot = (Get-Location).Path
$excludePathPattern = "\\(vcpkg|vcpkg_installed|external|llama_cpp)\\"

$clangTidy = $null
if (Get-Command clang-tidy -ErrorAction SilentlyContinue) {
    $clangTidy = "clang-tidy"
} else {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $vsInstall = & $vswhere -latest -products * -property installationPath
        if ($vsInstall) {
            $candidates = @(
                (Join-Path $vsInstall "VC\Tools\Llvm\x64\bin\clang-tidy.exe"),
                (Join-Path $vsInstall "VC\Tools\Llvm\bin\clang-tidy.exe"),
                (Join-Path $vsInstall "VC\Tools\Llvm\ARM64\bin\clang-tidy.exe")
            )
            foreach ($cand in $candidates) {
                if (Test-Path $cand) {
                    $clangTidy = $cand
                    break
                }
            }
        }
    }
}

$db = Join-Path $PWD "build-msvc-windows-debug/compile_commands.json"
if (-not (Test-Path $db)) {
    Write-Error "compile_commands.json missing. Run CMake configure with export compile commands first."
    exit 1
}

if (-not $clangTidy) {
    Write-Error "clang-tidy not found (neither in PATH nor Visual Studio LLVM bundle)."
    exit 1
}

$files = Get-ChildItem -Path "src" -Recurse -File -Include *.cpp,*.cc,*.cxx |
    Where-Object { $_.FullName -notmatch $excludePathPattern }
if (-not $files) {
    Write-Host "No C++ source files found under src."
    exit 0
}

foreach ($f in $files) {
    Write-Host ("[tidy] " + $f.FullName)
    $previousEap = $ErrorActionPreference
    $ErrorActionPreference = [System.Management.Automation.ActionPreference]::Continue
    $output = & $clangTidy $f.FullName -p build-msvc-windows-debug --header-filter="^(src|include)/" --quiet 2>&1
    $ErrorActionPreference = $previousEap
    $projectDiagnostics = @($output | ForEach-Object { "$_" } | Where-Object {
        $_ -match [regex]::Escape($workspaceRoot) -and $_ -notmatch $excludePathPattern
    })
    if ($projectDiagnostics.Count -gt 0) {
        $projectDiagnostics | ForEach-Object { Write-Host $_ }
    }

    $hasProjectErrors = ($projectDiagnostics | Where-Object { $_ -match "\berror:\b" }).Count -gt 0
    if ($LASTEXITCODE -ne 0 -and $hasProjectErrors) {
        Write-Host ("[tidy-nonzero] " + $f.FullName)
    }
}

Write-Host "clang-tidy batch completed."
exit 0
