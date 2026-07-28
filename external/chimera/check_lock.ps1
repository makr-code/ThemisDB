$path = Join-Path (Get-Location) 'build-msvc-windows-release\bin_out\module_tensor_test_tensor_core_bridge_focused.exe'
Write-Host "Path: $path"

Write-Host "\n1) File exists?"
if (Test-Path $path) { Get-Item $path | Format-List * } else { Write-Host "-> File does not exist" }

Write-Host "\n2) bin_out ACLs"
$bin = Join-Path (Get-Location) 'build-msvc-windows-release\bin_out'
if (Test-Path $bin) { icacls $bin } else { Write-Host "-> bin_out not found" }

Write-Host "\n3) where handle.exe"
try { & where handle.exe 2>$null; if ($LASTEXITCODE -ne 0) { Write-Host '-> handle.exe not found in PATH' } } catch { Write-Host '-> where failed' }

Write-Host "\n4) Try exclusive create/open"
try {
  $fs = [System.IO.File]::Open($path, [System.IO.FileMode]::Create, [System.IO.FileAccess]::ReadWrite, [System.IO.FileShare]::None)
  $fs.Close()
  Write-Host '-> Exclusive open OK (created or truncated)'
  Remove-Item -Force $path -ErrorAction SilentlyContinue
} catch {
  Write-Host '-> Exclusive open failed:'
  Write-Host $_.Exception.Message
}

Write-Host "\n5) Check for common suspect processes"
Get-Process | Where-Object { $_.ProcessName -match 'explorer|antimalware|MsMpEng|vcpkg|cmake|link|cl|clang|gcc' } | Select-Object Id,ProcessName | Format-Table

Write-Host "\nDone"
