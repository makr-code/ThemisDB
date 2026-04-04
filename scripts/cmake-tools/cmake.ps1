# PowerShell CMake wrapper that properly handles arguments with spaces
# Initialize VS2022 environment
$VsDevCmd = 'C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat'
cmd /c "`"$VsDevCmd`" -arch=x64 -no_logo"

# Call cmake with all arguments preserved
& cmake.exe @args
exit $LASTEXITCODE
