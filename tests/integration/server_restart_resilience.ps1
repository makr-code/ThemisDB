param(
    [Parameter(Mandatory = $true)]
    [string]$ServerExe,

    [Parameter(Mandatory = $true)]
    [string]$CtlExe
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

function Invoke-ExternalProbe {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Exe,

        [Parameter(Mandatory = $true)]
        [string[]]$Args
    )

    $output = @()
    $exitCode = 0
    $previousPreference = $ErrorActionPreference
    $ErrorActionPreference = "Continue"

    try {
        $output = & $Exe @Args 2>&1
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

function Wait-ServerReady {
    param(
        [string]$Exe,
        [int]$Port,
        [datetime]$Deadline
    )

    while ((Get-Date) -lt $Deadline) {
        $probe = Invoke-ExternalProbe -Exe $Exe -Args @('--host', '127.0.0.1', '--port', [string]$Port, 'version')
        if ($probe.ExitCode -eq 0) {
            return $true
        }
        Start-Sleep -Milliseconds 250
    }

    return $false
}

function Assert-Endpoints {
    param(
        [string]$Exe,
        [int]$Port
    )

    $health = Invoke-ExternalProbe -Exe $Exe -Args @('--host', '127.0.0.1', '--port', [string]$Port, 'health')
    if ($health.ExitCode -ne 0) {
        throw "health failed on port $Port. $($health.Output -join ' ')"
    }

    $schema = Invoke-ExternalProbe -Exe $Exe -Args @('--host', '127.0.0.1', '--port', [string]$Port, 'schema')
    if ($schema.ExitCode -ne 0) {
        throw "schema failed on port $Port. $($schema.Output -join ' ')"
    }

    $version = Invoke-ExternalProbe -Exe $Exe -Args @('--host', '127.0.0.1', '--port', [string]$Port, 'version')
    if ($version.ExitCode -ne 0) {
        throw "version failed on port $Port. $($version.Output -join ' ')"
    }
}

$port = Get-FreeTcpPort
$tempRoot = Join-Path $env:TEMP ("themis_restart_resilience_" + [guid]::NewGuid().ToString("N"))
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

    if (-not (Wait-ServerReady -Exe $CtlExe -Port $port -Deadline (Get-Date).AddSeconds(30))) {
        throw "Server did not become ready in first boot cycle."
    }

    Assert-Endpoints -Exe $CtlExe -Port $port

    if (-not $serverProc.HasExited) {
        Stop-Process -Id $serverProc.Id -Force -ErrorAction SilentlyContinue
        Wait-Process -Id $serverProc.Id -Timeout 10 -ErrorAction SilentlyContinue
    }

    $serverProc = Start-Process -FilePath $ServerExe `
        -ArgumentList @("--host", "127.0.0.1", "--port", "$port") `
        -WorkingDirectory $tempRoot `
        -RedirectStandardOutput $serverOut `
        -RedirectStandardError $serverErr `
        -PassThru

    if (-not (Wait-ServerReady -Exe $CtlExe -Port $port -Deadline (Get-Date).AddSeconds(30))) {
        throw "Server did not become ready after restart."
    }

    Assert-Endpoints -Exe $CtlExe -Port $port

    if ($serverProc.HasExited) {
        throw "Server exited unexpectedly after restart (exit $($serverProc.ExitCode))."
    }
}
finally {
    if ($serverProc -and -not $serverProc.HasExited) {
        Stop-Process -Id $serverProc.Id -Force -ErrorAction SilentlyContinue
        Wait-Process -Id $serverProc.Id -Timeout 5 -ErrorAction SilentlyContinue
    }

    Remove-Item -LiteralPath $tempRoot -Recurse -Force -ErrorAction SilentlyContinue
}
