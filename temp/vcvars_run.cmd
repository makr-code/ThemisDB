@echo off
set VSINSTALLDIR=
set VSCMD_DEBUG=3
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" amd64
echo __LIB__
echo %LIB%
echo __INCLUDE__
echo %INCLUDE%
where cl
where link
