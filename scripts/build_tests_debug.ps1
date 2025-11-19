Param()
Push-Location "$PSScriptRoot\..\build-msvc"
$msbuild = "C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe"
if (-not (Test-Path $msbuild)) {
    Write-Error "MSBuild not found at $msbuild"
    Pop-Location
    exit 1
}
$proc = Start-Process -FilePath $msbuild -ArgumentList ".\themis_tests.vcxproj","/p:Configuration=Debug","/m","/nologo" -Wait -PassThru -NoNewWindow
$code = $proc.ExitCode
Pop-Location
exit $code
