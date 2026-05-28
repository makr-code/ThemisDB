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

$port = 19600 + (Get-Random -Minimum 0 -Maximum 300)
$tempRoot = Join-Path $env:TEMP ("themis_error_contract_" + [guid]::NewGuid().ToString("N"))
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

        Start-Sleep -Milliseconds 250
    }

    if (-not $ready) {
        throw "Server did not become ready within timeout."
    }

    $badPath = "/api/v1/does-not-exist"
    $previousErrorActionPreference = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        $errorOutput = & $CtlExe --host 127.0.0.1 --port $port api GET $badPath 2>&1 | Out-String
        $errorExit = $LASTEXITCODE
    }
    finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }

    if ($errorExit -eq 0) {
        throw "Expected non-zero exit for invalid endpoint '$badPath'."
    }
    if ($errorOutput -notmatch "HTTP\s+404") {
        throw "Expected HTTP 404 in error output. Actual output: $errorOutput"
    }

    & $CtlExe --host 127.0.0.1 --port $port health *> $null
    if ($LASTEXITCODE -ne 0) {
        throw "Server not healthy after negative endpoint contract check."
    }
}
finally {
    if ($serverProc -and -not $serverProc.HasExited) {
        Stop-Process -Id $serverProc.Id -Force -ErrorAction SilentlyContinue
        Wait-Process -Id $serverProc.Id -Timeout 5 -ErrorAction SilentlyContinue
    }

    Remove-Item -LiteralPath $tempRoot -Recurse -Force -ErrorAction SilentlyContinue
}
