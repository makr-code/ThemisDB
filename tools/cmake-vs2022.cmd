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

:: --- If VsDevCmd did not fully populate the linker environment, seed it here.
if not defined VCToolsInstallDir (
	for /d %%D in ("C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\*") do set "VCToolsInstallDir=%%~fD"
)
if not defined WindowsSdkDir (
	if exist "C:\Program Files (x86)\Windows Kits\10" set "WindowsSdkDir=C:\Program Files (x86)\Windows Kits\10"
)
if not defined WindowsSDKVersion (
	for /f "delims=" %%V in ('dir /b /ad "C:\Program Files (x86)\Windows Kits\10\Include" 2^>nul ^| sort /r') do (
		if exist "C:\Program Files (x86)\Windows Kits\10\Include\%%V\um" set "WindowsSDKVersion=%%V"
		if defined WindowsSDKVersion goto :seed_lib_paths
	)
)

:seed_lib_paths
set "_THEMIS_LIB_PATHS="
if defined VCToolsInstallDir if exist "%VCToolsInstallDir%\lib\x64" set "_THEMIS_LIB_PATHS=%VCToolsInstallDir%\lib\x64"
if defined WindowsSdkDir if defined WindowsSDKVersion (
	if exist "%WindowsSdkDir%\Lib\%WindowsSDKVersion%\ucrt\x64" (
		if defined _THEMIS_LIB_PATHS (
			set "_THEMIS_LIB_PATHS=%_THEMIS_LIB_PATHS%;%WindowsSdkDir%\Lib\%WindowsSDKVersion%\ucrt\x64"
		) else (
			set "_THEMIS_LIB_PATHS=%WindowsSdkDir%\Lib\%WindowsSDKVersion%\ucrt\x64"
		)
	)
	if exist "%WindowsSdkDir%\Lib\%WindowsSDKVersion%\um\x64" (
		if defined _THEMIS_LIB_PATHS (
			set "_THEMIS_LIB_PATHS=%_THEMIS_LIB_PATHS%;%WindowsSdkDir%\Lib\%WindowsSDKVersion%\um\x64"
		) else (
			set "_THEMIS_LIB_PATHS=%WindowsSdkDir%\Lib\%WindowsSDKVersion%\um\x64"
		)
	)
)
if defined _THEMIS_LIB_PATHS (
	set "LIB=%_THEMIS_LIB_PATHS%;%LIB%"
	set "LIBPATH=%_THEMIS_LIB_PATHS%;%LIBPATH%"
)

:: --- Forward all arguments to the real cmake binary
"C:\Program Files\CMake\bin\cmake.exe" %*
exit /b %ERRORLEVEL%
