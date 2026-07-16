$bin='C:\Projects\ThemisDB\build-msvc-windows-release\bin_out'
$files=Get-ChildItem -Path $bin -Filter *.exe -File -ErrorAction SilentlyContinue
foreach($f in $files) {
    Write-Output "Checking $($f.Name)"
    try {
        $out = & $f.FullName --gtest_filter=EncryptedChunkStoreTest.* 2>&1
    } catch {
        $out = $_.ToString()
    }
    # detect actual test runs (ignore the gtest filter echo)
    if ($out -match 'EncryptedChunkStoreTest\.' -or $out -match '\[\s*RUN\s*\].*EncryptedChunkStoreTest') {
        Write-Output 'MATCH:'
        Write-Output $f.FullName
        Write-Output $out
        break
    }
}
Write-Output 'done'
