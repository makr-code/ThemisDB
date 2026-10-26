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

$port = Get-FreeTcpPort
$tempRoot = Join-Path $env:TEMP ("themis_schema_smoke_" + [guid]::NewGuid().ToString("N"))
$null = New-Item -ItemType Directory -Path $tempRoot -Force

$serverOut = Join-Path $tempRoot "server.out.log"
$serverErr = Join-Path $tempRoot "server.err.log"

$serverProc = $null
try {
    $serverArgs = @("--host", "127.0.0.1", "--port", "$port")
    $serverProc = Start-Process -FilePath $ServerExe `
        -ArgumentList $serverArgs `
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

        try {
            $versionResponse = Invoke-WebRequest -Uri "http://127.0.0.1:$port/version" -Method Get -TimeoutSec 5 -UseBasicParsing
            if ($versionResponse.StatusCode -ge 200 -and $versionResponse.StatusCode -lt 300) {
                $ready = $true
                break
            }
        }
        catch {
            # Server is still starting or not yet serving the API.
        }

        Start-Sleep -Milliseconds 300
    }

    if (-not $ready) {
        throw "Server did not become ready within timeout."
    }

    try {
        $schemaResponse = Invoke-WebRequest -Uri "http://127.0.0.1:$port/api/v1/schema" -Method Get -TimeoutSec 10 -UseBasicParsing
        if ($schemaResponse.StatusCode -lt 200 -or $schemaResponse.StatusCode -ge 300) {
            throw "Schema request failed with HTTP status $($schemaResponse.StatusCode)."
        }
    }
    catch {
        $message = $_.Exception.Message
        if ($null -ne $_.Exception.Response) {
            $message = "HTTP status $($_.Exception.Response.StatusCode). $message"
        }
        throw "Schema endpoint probe failed: $message"
    }

    if ($serverProc.HasExited) {
        throw "Server exited unexpectedly after schema endpoint call (exit $($serverProc.ExitCode))."
    }
}
finally {
    if ($serverProc -and -not $serverProc.HasExited) {
        Stop-Process -Id $serverProc.Id -Force -ErrorAction SilentlyContinue
        Wait-Process -Id $serverProc.Id -Timeout 5 -ErrorAction SilentlyContinue
    }

    Remove-Item -LiteralPath $tempRoot -Recurse -Force -ErrorAction SilentlyContinue
}
