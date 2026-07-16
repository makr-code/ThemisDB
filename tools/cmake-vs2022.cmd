@echo off
:: ============================================================
:: cmake-vs2022.cmd
:: Thin cmake wrapper that ensures a clean VS 2022 Professional
:: developer environment before invoking cmake.
::
:: Purpose: Prevents VS 18 Insiders environment contamination
::          from breaking CMake Tools in VS Code.
:: Usage:   Set cmake.cmakePath to the path of this file.
:: ============================================================

:: --- Clear VS-version-specific variables that can leak from
::     a contaminated shell (e.g. VS 18 Insiders loaded earlier)
set "VSINSTALLDIR="
set "VCINSTALLDIR="
set "VCToolsInstallDir="
set "VCToolsVersion="
set "VCToolsRedistDir="
set "VisualStudioVersion="
set "DevEnvDir="
set "VSCMD_ARG_TGT_ARCH="
set "VSCMD_ARG_HOST_ARCH="
set "VSCMD_ARG_APP_PLAT="
set "VSCMD_VER="
set "__VSCMD_PREINIT_PATH="
set "__VSCMD_script_err_count="

:: --- Load VS 2022 Professional developer environment
::     Suppress all VsDevCmd.bat output so CMake Tools only sees cmake output.
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 -no_logo >nul 2>&1

:: --- Forward all arguments to the real cmake binary
"C:\Program Files\CMake\bin\cmake.exe" %*
exit /b %ERRORLEVEL%
