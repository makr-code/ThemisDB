param(
    [string]$OutputDir = "logs/hardware_baseline",
    [int]$CpuSeconds = 2,
    [int]$MemorySizeMB = 256,
    [int]$DiskFileSizeMB = 256,
    [int]$DiskBlockSizeMB = 4
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Get-HardwareInfo {
    $cpu = Get-CimInstance Win32_Processor | Select-Object -First 1
    $cs = Get-CimInstance Win32_ComputerSystem | Select-Object -First 1
    $os = Get-CimInstance Win32_OperatingSystem | Select-Object -First 1
    $gpu = Get-CimInstance Win32_VideoController | Select-Object Name, DriverVersion, AdapterRAM
    $disks = Get-CimInstance Win32_LogicalDisk -Filter "DriveType=3" |
        Select-Object DeviceID, VolumeName, FileSystem, Size, FreeSpace

    return [ordered]@{
        host_name = $env:COMPUTERNAME
        os = [ordered]@{
            caption = $os.Caption
            version = $os.Version
            build = $os.BuildNumber
        }
        cpu = [ordered]@{
            name = $cpu.Name
            cores = [int]$cpu.NumberOfCores
            logical_processors = [int]$cpu.NumberOfLogicalProcessors
            max_clock_mhz = [int]$cpu.MaxClockSpeed
        }
        memory = [ordered]@{
            total_gb = [math]::Round(([double]$cs.TotalPhysicalMemory / 1GB), 2)
        }
        gpus = @($gpu | ForEach-Object {
            [ordered]@{
                name = $_.Name
                driver = $_.DriverVersion
                vram_gb = if ($_.AdapterRAM) { [math]::Round(([double]$_.AdapterRAM / 1GB), 2) } else { $null }
            }
        })
        disks = @($disks | ForEach-Object {
            [ordered]@{
                device = $_.DeviceID
                volume = $_.VolumeName
                filesystem = $_.FileSystem
                size_gb = if ($_.Size) { [math]::Round(([double]$_.Size / 1GB), 2) } else { $null }
                free_gb = if ($_.FreeSpace) { [math]::Round(([double]$_.FreeSpace / 1GB), 2) } else { $null }
            }
        })
    }
}

function Measure-CpuIntegerOps {
    param([int]$Seconds)

    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    [long]$ops = 0
    [long]$x = 1
    while ($sw.Elapsed.TotalSeconds -lt $Seconds) {
        $x = ($x * 1664525 + 1013904223) -band 0x7FFFFFFF
        $ops++
    }
    $sw.Stop()

    return [ordered]@{
        elapsed_s = [math]::Round($sw.Elapsed.TotalSeconds, 3)
        operations = $ops
        ops_per_s = [math]::Round(($ops / $sw.Elapsed.TotalSeconds), 2)
        checksum = $x
    }
}

function Measure-CpuFloatOps {
    param([int]$Seconds)

    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    [long]$ops = 0
    [double]$x = 1.000001
    while ($sw.Elapsed.TotalSeconds -lt $Seconds) {
        $x = [math]::Sqrt($x * 1.000000119 + 0.0000003)
        $ops++
    }
    $sw.Stop()

    return [ordered]@{
        elapsed_s = [math]::Round($sw.Elapsed.TotalSeconds, 3)
        operations = $ops
        ops_per_s = [math]::Round(($ops / $sw.Elapsed.TotalSeconds), 2)
        checksum = [math]::Round($x, 9)
    }
}

function Measure-MemoryBandwidth {
    param([int]$SizeMB)

    $bytes = $SizeMB * 1MB
    $src = New-Object byte[] $bytes
    $dst = New-Object byte[] $bytes

    # Deterministic fill
    for ($i = 0; $i -lt $src.Length; $i += 4096) {
        $src[$i] = [byte]($i % 251)
    }

    $iterations = 8
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    for ($i = 0; $i -lt $iterations; $i++) {
        [System.Buffer]::BlockCopy($src, 0, $dst, 0, $src.Length)
    }
    $sw.Stop()

    $totalBytes = [double]$bytes * $iterations
    $mbPerSec = ($totalBytes / 1MB) / $sw.Elapsed.TotalSeconds

    return [ordered]@{
        buffer_mb = $SizeMB
        iterations = $iterations
        elapsed_s = [math]::Round($sw.Elapsed.TotalSeconds, 4)
        bandwidth_mb_s = [math]::Round($mbPerSec, 2)
        checksum = [int]$dst[0]
    }
}

function Measure-DiskSequential {
    param(
        [string]$FilePath,
        [int]$FileSizeMB,
        [int]$BlockSizeMB
    )

    $totalBytes = $FileSizeMB * 1MB
    $blockBytes = $BlockSizeMB * 1MB
    if ($blockBytes -le 0) { throw "DiskBlockSizeMB must be > 0" }

    $buffer = New-Object byte[] $blockBytes
    $rnd = [System.Random]::new(42)
    $rnd.NextBytes($buffer)

    # Write benchmark
    $writeSw = [System.Diagnostics.Stopwatch]::StartNew()
    $fs = [System.IO.File]::Open($FilePath, [System.IO.FileMode]::Create, [System.IO.FileAccess]::Write, [System.IO.FileShare]::None)
    try {
        [long]$written = 0
        while ($written -lt $totalBytes) {
            $toWrite = [int][math]::Min($blockBytes, $totalBytes - $written)
            $fs.Write($buffer, 0, $toWrite)
            $written += $toWrite
        }
        $fs.Flush($true)
    }
    finally {
        $fs.Dispose()
    }
    $writeSw.Stop()

    # Read benchmark
    $readSw = [System.Diagnostics.Stopwatch]::StartNew()
    $fr = [System.IO.File]::Open($FilePath, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, [System.IO.FileShare]::Read)
    try {
        [long]$read = 0
        while ($read -lt $totalBytes) {
            $toRead = [int][math]::Min($blockBytes, $totalBytes - $read)
            $n = $fr.Read($buffer, 0, $toRead)
            if ($n -le 0) { break }
            $read += $n
        }
    }
    finally {
        $fr.Dispose()
    }
    $readSw.Stop()

    $writeMBs = ($FileSizeMB / $writeSw.Elapsed.TotalSeconds)
    $readMBs = ($FileSizeMB / $readSw.Elapsed.TotalSeconds)

    return [ordered]@{
        file_size_mb = $FileSizeMB
        block_size_mb = $BlockSizeMB
        write_elapsed_s = [math]::Round($writeSw.Elapsed.TotalSeconds, 4)
        write_mb_s = [math]::Round($writeMBs, 2)
        read_elapsed_s = [math]::Round($readSw.Elapsed.TotalSeconds, 4)
        read_mb_s = [math]::Round($readMBs, 2)
    }
}

$resolvedOutputDir = Resolve-Path -LiteralPath "." | ForEach-Object { Join-Path $_.Path $OutputDir }
New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null

$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$tmpFile = Join-Path $resolvedOutputDir "disk_seq_${timestamp}.bin"
$outFile = Join-Path $resolvedOutputDir "hardware_baseline_${timestamp}.json"

try {
    $result = [ordered]@{
        generated_at = (Get-Date).ToString("o")
        benchmark_config = [ordered]@{
            cpu_seconds = $CpuSeconds
            memory_size_mb = $MemorySizeMB
            disk_file_size_mb = $DiskFileSizeMB
            disk_block_size_mb = $DiskBlockSizeMB
        }
        hardware = Get-HardwareInfo
        results = [ordered]@{
            cpu_integer = Measure-CpuIntegerOps -Seconds $CpuSeconds
            cpu_float = Measure-CpuFloatOps -Seconds $CpuSeconds
            memory_copy = Measure-MemoryBandwidth -SizeMB $MemorySizeMB
            disk_sequential = Measure-DiskSequential -FilePath $tmpFile -FileSizeMB $DiskFileSizeMB -BlockSizeMB $DiskBlockSizeMB
        }
    }

    $result | ConvertTo-Json -Depth 8 | Out-File -FilePath $outFile -Encoding utf8

    Write-Host "HARDWARE_BASELINE_JSON=$outFile"
    Write-Host "CPU_INT_OPS_PER_S=$($result.results.cpu_integer.ops_per_s)"
    Write-Host "CPU_FLOAT_OPS_PER_S=$($result.results.cpu_float.ops_per_s)"
    Write-Host "MEM_COPY_MB_S=$($result.results.memory_copy.bandwidth_mb_s)"
    Write-Host "DISK_WRITE_MB_S=$($result.results.disk_sequential.write_mb_s)"
    Write-Host "DISK_READ_MB_S=$($result.results.disk_sequential.read_mb_s)"
}
finally {
    if (Test-Path -LiteralPath $tmpFile) {
        Remove-Item -LiteralPath $tmpFile -Force -ErrorAction SilentlyContinue
    }
}
