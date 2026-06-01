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

function Invoke-CtlJson {
    param(
        [string]$Exe,
        [int]$Port,
        [string[]]$CommandArgs
    )

    $output = & $Exe --json --host 127.0.0.1 --port $Port @CommandArgs 2>&1 | Out-String
    $exitCode = $LASTEXITCODE
    if ($exitCode -ne 0) {
        throw "themisctl $($CommandArgs -join ' ') failed with exit code $exitCode. Output: $output"
    }

    try {
        return ($output | ConvertFrom-Json)
    }
    catch {
        throw "Invalid JSON from themisctl $($CommandArgs -join ' '): $output"
    }
}

$port = 19400 + (Get-Random -Minimum 0 -Maximum 400)
$tempRoot = Join-Path $env:TEMP ("themis_core_contract_" + [guid]::NewGuid().ToString("N"))
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

        $healthRaw = & $CtlExe --json --host 127.0.0.1 --port $port health 2>&1 | Out-String
        if ($LASTEXITCODE -eq 0) {
            $ready = $true
            break
        }

        Start-Sleep -Milliseconds 250
    }

    if (-not $ready) {
        throw "Server did not become ready within timeout."
    }

    $health = Invoke-CtlJson -Exe $CtlExe -Port $port -CommandArgs @("health")
    if (($health.live.status -ne 200) -or ($health.ready.status -ne 200)) {
        throw "Health contract failed: expected live/ready status 200."
    }

    $version = Invoke-CtlJson -Exe $CtlExe -Port $port -CommandArgs @("version")
    if (-not ($version.PSObject.Properties.Name -contains "version")) {
        throw "Version contract failed: missing 'version' field."
    }

    $schema = Invoke-CtlJson -Exe $CtlExe -Port $port -CommandArgs @("schema")
    if (-not ($schema.PSObject.Properties.Name -contains "tables")) {
        throw "Schema contract failed: missing 'tables' field."
    }

    if ($serverProc.HasExited) {
        throw "Server exited unexpectedly during core endpoint contract checks (exit $($serverProc.ExitCode))."
    }
}
finally {
    if ($serverProc -and -not $serverProc.HasExited) {
        Stop-Process -Id $serverProc.Id -Force -ErrorAction SilentlyContinue
        Wait-Process -Id $serverProc.Id -Timeout 5 -ErrorAction SilentlyContinue
    }

    Remove-Item -LiteralPath $tempRoot -Recurse -Force -ErrorAction SilentlyContinue
}
