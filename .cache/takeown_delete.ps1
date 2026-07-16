$path='C:\Projects\ThemisDB\build-msvc-windows-release\bin_out\test_content_security_focused.exe'
if (-not (Test-Path $path)) { Write-Output 'File not found'; exit 0 }
Write-Output 'Attempting takeown'
cmd /c "takeown /F `"$path`" /A"
Write-Output 'Attempting icacls'
cmd /c "icacls `"$path`" /grant `"$env:USERNAME`:F`" /C"
try {
    Remove-Item -LiteralPath $path -Force -ErrorAction Stop
    Write-Output 'Deleted'
} catch {
    Write-Output 'Delete failed after icacls'
    Write-Output $_
}
