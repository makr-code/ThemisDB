@echo off
set VSINSTALLDIR=
set DevEnvDir=
set VisualStudioVersion=
set VCToolsInstallDir=
set VCToolsVersion=
set VS150COMNTOOLS=
set VS160COMNTOOLS=
set VS170COMNTOOLS=
set __VSCMD_PREINIT_VCToolsVersion=
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b %errorlevel%
cmake --preset windows-release -B build-msvc-windows-release-clean --fresh
if errorlevel 1 exit /b %errorlevel%
cmake --build build-msvc-windows-release-clean --clean-first --parallel 16
exit /b %errorlevel%
