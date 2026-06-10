$ErrorActionPreference = 'Stop'

$root = 'C:/Projects/ThemisDB/tests'
$sources = @('http','http2','http3','cdn')

foreach ($d in $sources) {
    $count = (Get-ChildItem (Join-Path $root $d) -File -Filter 'test_*.cpp' -ErrorAction SilentlyContinue | Measure-Object).Count
    Write-Output ($d + '=' + $count)
}

$networkCount = (Get-ChildItem (Join-Path $root 'network') -File -Filter 'test_*.cpp' | Measure-Object).Count
Write-Output ('network=' + $networkCount)
