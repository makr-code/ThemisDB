$ErrorActionPreference = 'Stop'

$root = 'C:/Projects/ThemisDB/tests'
$sources = @('wire','ws','websocket','quic','udp','socket','transport')
$dest = Join-Path $root 'network'

$moved = 0
$renamed = 0
$dedup = 0

foreach ($d in $sources) {
    $dir = Join-Path $root $d
    if (-not (Test-Path $dir)) {
        continue
    }

    Get-ChildItem $dir -File -Filter 'test_*.cpp' | ForEach-Object {
        $src = $_.FullName
        $dst = Join-Path $dest $_.Name

        if (Test-Path $dst) {
            $srcHash = (Get-FileHash -Algorithm SHA256 $src).Hash
            $dstHash = (Get-FileHash -Algorithm SHA256 $dst).Hash

            if ($srcHash -eq $dstHash) {
                Remove-Item $src -Force
                $dedup++
                return
            }

            $base = [System.IO.Path]::GetFileNameWithoutExtension($_.Name)
            $ext = $_.Extension
            $candidate = Join-Path $dest ($base + '_' + $d + $ext)
            $i = 1
            while (Test-Path $candidate) {
                $candidate = Join-Path $dest ($base + '_' + $d + '_' + $i + $ext)
                $i++
            }

            Move-Item $src $candidate
            $renamed++
            $moved++
            return
        }

        Move-Item $src $dst
        $moved++
    }
}

Write-Output ('MOVED=' + $moved)
Write-Output ('RENAMED=' + $renamed)
Write-Output ('DEDUP=' + $dedup)
Write-Output ('NETWORK_COUNT=' + ((Get-ChildItem $dest -File -Filter 'test_*.cpp' | Measure-Object).Count))
