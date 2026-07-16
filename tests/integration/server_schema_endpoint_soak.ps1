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
if ($Iterations -lt 1) {
    throw "Iterations must be >= 1"
}
if ($DelayMs -lt 0) {
    throw "DelayMs must be >= 0"
}

$port = 19000 + (Get-Random -Minimum 0 -Maximum 400)
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

        & $CtlExe --host 127.0.0.1 --port $port version *> $null
        if ($LASTEXITCODE -eq 0) {
            $ready = $true
            break
        }

        Start-Sleep -Milliseconds 300
    }

    if (-not $ready) {
        throw "Server did not become ready within timeout."
    }

    for ($i = 1; $i -le $Iterations; $i++) {
        & $CtlExe --host 127.0.0.1 --port $port health *> $null
        if ($LASTEXITCODE -ne 0) {
            throw "health command failed in iteration $i with exit code $LASTEXITCODE"
        }

        & $CtlExe --host 127.0.0.1 --port $port schema *> $null
        if ($LASTEXITCODE -ne 0) {
            throw "schema command failed in iteration $i with exit code $LASTEXITCODE"
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
