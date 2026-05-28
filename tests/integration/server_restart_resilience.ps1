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

function Wait-ServerReady {
    param(
        [string]$Exe,
        [int]$Port,
        [datetime]$Deadline
    )

    while ((Get-Date) -lt $Deadline) {
        & $Exe --host 127.0.0.1 --port $Port version *> $null
        if ($LASTEXITCODE -eq 0) {
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

    & $Exe --host 127.0.0.1 --port $Port health *> $null
    if ($LASTEXITCODE -ne 0) {
        throw "health failed on port $Port"
    }

    & $Exe --host 127.0.0.1 --port $Port schema *> $null
    if ($LASTEXITCODE -ne 0) {
        throw "schema failed on port $Port"
    }

    & $Exe --host 127.0.0.1 --port $Port version *> $null
    if ($LASTEXITCODE -ne 0) {
        throw "version failed on port $Port"
    }
}

$port = 19800 + (Get-Random -Minimum 0 -Maximum 200)
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
