param(
    [Parameter(Mandatory = $true)]
    [string]$ServerExe,

    [Parameter(Mandatory = $true)]
    [string]$CtlExe,

    [Parameter(Mandatory = $false)]
    [int]$Iterations = 30,

    [Parameter(Mandatory = $false)]
    [int]$DelayMs = 200
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
if (Get-Variable -Name PSNativeCommandUseErrorActionPreference -ErrorAction SilentlyContinue) {
    $PSNativeCommandUseErrorActionPreference = $false
}

if (-not (Test-Path -LiteralPath $ServerExe)) {
    throw "Server binary not found: $ServerExe"
}
if (-not (Test-Path -LiteralPath $CtlExe)) {
    throw "themisctl binary not found: $CtlExe"
}

function Add-ThemisRuntimePath {
    $serverDir = Split-Path -Parent $ServerExe
    $projectRoot = (Resolve-Path (Join-Path $serverDir "..\..")).Path
    $runtimeDirs = @(
        $serverDir,
        (Join-Path $projectRoot "vcpkg_installed\x64-windows\debug\bin"),
        (Join-Path $projectRoot "vcpkg_installed\x64-windows\bin")
    )

    $filtered = @()
    foreach ($dir in $runtimeDirs) {
        if ($dir -and (Test-Path -LiteralPath $dir)) {
            $filtered += $dir
        }
    }

    if ($filtered.Count -gt 0) {
        $pathList = @()
        foreach ($dir in $filtered) {
            $pathList += $dir
        }
        $unique = @()
        foreach ($dir in $pathList) {
            if (-not ($unique -contains $dir)) {
                $unique += $dir
            }
        }
        $env:PATH = ($unique + ($env:PATH -split ';' | Where-Object { $_ -and $_.Trim() })) -join ';'
    }
}

Add-ThemisRuntimePath

function Get-FreeTcpPort {
    $listener = [System.Net.Sockets.TcpListener]::new([System.Net.IPAddress]::Loopback, 0)
    try {
        $listener.Start()
        return $listener.LocalEndpoint.Port
    }
    finally {
        $listener.Stop()
    }
}

function Invoke-ThemisCtlProbe {
    param(
        [Parameter(Mandatory = $true)]
        [string[]]$Args
    )

    $output = @()
    $exitCode = 0
    $previousPreference = $ErrorActionPreference
    $ErrorActionPreference = "Continue"

    try {
        $output = & $CtlExe @Args 2>&1
        if ($null -ne $LASTEXITCODE) {
            $exitCode = [int]$LASTEXITCODE
        }
    }
    catch {
        $output = @($_.Exception.Message)
        $exitCode = 1
    }
    finally {
        $ErrorActionPreference = $previousPreference
    }

    return [pscustomobject]@{
        ExitCode = $exitCode
        Output   = @($output)
    }
}

if ($Iterations -lt 1) {
    throw "Iterations must be >= 1"
}
if ($DelayMs -lt 0) {
    throw "DelayMs must be >= 0"
}

$port = Get-FreeTcpPort
$tempRoot = Join-Path $env:TEMP ("themis_schema_soak_" + [guid]::NewGuid().ToString("N"))
$null = New-Item -ItemType Directory -Path $tempRoot -Force

$serverOut = Join-Path $tempRoot "server.out.log"
$serverErr = Join-Path $tempRoot "server.err.log"

$serverProc = $null
try {
    $serverProc = Start-Process -FilePath $ServerExe `
        -ArgumentList @("--host", "127.0.0.1", "--port", "$port") `
        -WorkingDirectory $tempRoot `
        -RedirectStandardOutput $serverOut `
        -RedirectStandardError $serverErr `
        -PassThru

    $deadline = (Get-Date).AddSeconds(30)
    $ready = $false
    while ((Get-Date) -lt $deadline) {
        if ($serverProc.HasExited) {
            throw "Server exited early with code $($serverProc.ExitCode)."
        }

        $probe = Invoke-ThemisCtlProbe -Args @('--host', '127.0.0.1', '--port', [string]$port, 'version')
        if ($probe.ExitCode -eq 0) {
            $ready = $true
            break
        }

        Start-Sleep -Milliseconds 300
    }

    if (-not $ready) {
        throw "Server did not become ready within timeout."
    }

    for ($i = 1; $i -le $Iterations; $i++) {
        $healthProbe = Invoke-ThemisCtlProbe -Args @('--host', '127.0.0.1', '--port', [string]$port, 'health')
        if ($healthProbe.ExitCode -ne 0) {
            throw "health command failed in iteration $i with exit code $($healthProbe.ExitCode): $($healthProbe.Output -join ' ')"
        }

        $schemaProbe = Invoke-ThemisCtlProbe -Args @('--host', '127.0.0.1', '--port', [string]$port, 'schema')
        if ($schemaProbe.ExitCode -ne 0) {
            throw "schema command failed in iteration $i with exit code $($schemaProbe.ExitCode): $($schemaProbe.Output -join ' ')"
        }

        if ($serverProc.HasExited) {
            throw "Server exited during soak at iteration $i (exit $($serverProc.ExitCode))."
        }

        if ($DelayMs -gt 0) {
            Start-Sleep -Milliseconds $DelayMs
        }
    }
}
finally {
    if ($serverProc -and -not $serverProc.HasExited) {
        Stop-Process -Id $serverProc.Id -Force -ErrorAction SilentlyContinue
        Wait-Process -Id $serverProc.Id -Timeout 5 -ErrorAction SilentlyContinue
    }

    Remove-Item -LiteralPath $tempRoot -Recurse -Force -ErrorAction SilentlyContinue
}
