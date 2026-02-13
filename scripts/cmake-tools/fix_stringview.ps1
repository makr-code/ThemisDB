$file = 'C:\VCC\themis\src\server\rpc\rpc_service_impl.cpp'
$content = Get-Content $file -Raw
$content = $content -creplace '(\bstd::string \w+) = (params\.value\([^)]+\))', '$1($2)'
Set-Content $file -Value $content -NoNewline
