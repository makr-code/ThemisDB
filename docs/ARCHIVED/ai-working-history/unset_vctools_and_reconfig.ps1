$old = [Environment]::GetEnvironmentVariable('VCToolsVersion','User')
if ($old -ne $null -and $old -ne '') {
  $old | Out-File -FilePath '.\ai_working\vctools_user_env_backup.txt' -Encoding utf8
}
[Environment]::SetEnvironmentVariable('VCToolsVersion',$null,'User')
& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat' x64 > '.\ai_working\vcvars_after_unset.txt' 2>&1
cmake -S . -B build-msvc-relwithdebinfo-clean -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_TOOLCHAIN_FILE=C:/Projects/ThemisDB/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_INSTALLED_DIR=C:/Projects/ThemisDB/vcpkg_installed -DVCPKG_TARGET_TRIPLET=x64-windows > '.\ai_working\cmake_config_after_unset.txt' 2>&1
Write-Output 'DONE'
