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
        $unique = @()
        foreach ($dir in $filtered) {
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

function Invoke-ThemisCtl {
    param(
        [string[]]$Args
    )

    $output = @()
    $exitCode = 0

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

    return [pscustomobject]@{
        ExitCode = $exitCode
        Output   = @($output)
    }
}

function Test-ThemisServerReady {
    param(
        [int]$Port,
        [int]$TimeoutSeconds = 30
    )

    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        try {
            $response = Invoke-WebRequest -Uri "http://127.0.0.1:$Port/version" -Method Get -TimeoutSec 5 -UseBasicParsing
            if ($response.StatusCode -ge 200 -and $response.StatusCode -lt 300) {
                $body = [string]$response.Content
                if ($body -match '"version"|"api_version"') {
                    return $true
                }
            }
        }
        catch {
            # Server is still starting up; keep polling until the deadline is reached.
        }

        Start-Sleep -Milliseconds 300
    }

    return $false
}

function New-EntityPutBody {
    param(
        [string]$EntityId,
        [string]$JsonBody
    )

    $payload = [pscustomobject]@{
        key  = $EntityId
        blob = $JsonBody
    }

    return ($payload | ConvertTo-Json -Compress -Depth 10)
}

function Invoke-ThemisHttp {
    param(
        [ValidateSet('GET', 'PUT', 'DELETE')]
        [string]$Method,
        [int]$Port,
        [string]$EntityId,
        [string]$Body = $null
    )

    $uri = "http://127.0.0.1:$Port/entities/$EntityId"
    $headers = @{ 'Content-Type' = 'application/json' }

    $invokeArgs = @{
        Uri = $uri
        Method = $Method
        Headers = $headers
        TimeoutSec = 10
        UseBasicParsing = $true
    }

    if (-not [string]::IsNullOrEmpty($Body) -and ($Method -eq 'PUT' -or $Method -eq 'POST' -or $Method -eq 'PATCH')) {
        $invokeArgs.Body = $Body
    }

    try {
        $response = Invoke-WebRequest @invokeArgs
        return [pscustomobject]@{
            StatusCode = [int]$response.StatusCode
            Content    = [string]$response.Content
        }
    }
    catch {
        $ex = $_.Exception
        $response = $null
        if ($null -ne $ex -and $ex.PSObject.Properties.Name -contains 'Response') {
            $response = $ex.Response
        }

        $content = ""
        if ($null -ne $response) {
            try {
                $reader = New-Object System.IO.StreamReader($response.GetResponseStream())
                $content = $reader.ReadToEnd()
                $reader.Dispose()
            }
            catch {
                $content = ""
            }
        }
        if ([string]::IsNullOrWhiteSpace($content) -and $null -ne $ex -and -not [string]::IsNullOrWhiteSpace($ex.Message)) {
            $content = $ex.Message
        }

        return [pscustomobject]@{
            StatusCode = if ($null -ne $response) { [int]$response.StatusCode } else { 500 }
            Content    = $content
        }
    }
}

function Assert-HttpSuccess {
    param(
        [string]$Label,
        [object]$Response
    )

    if ($Response.StatusCode -lt 200 -or $Response.StatusCode -ge 300) {
        throw "$Label failed. HTTP status $($Response.StatusCode). Body: $($Response.Content)"
    }
}

$port = Get-FreeTcpPort
$tempRoot = Join-Path $env:TEMP ("themis_crud_lifecycle_" + [guid]::NewGuid().ToString("N"))
$null = New-Item -ItemType Directory -Path $tempRoot -Force

$serverOut = Join-Path $tempRoot "server.out.log"
$serverErr = Join-Path $tempRoot "server.err.log"

$serverProc = $null
$entityId = "server:crud_lifecycle_" + [guid]::NewGuid().ToString("N")
$createPayload = '{"name":"Alice","status":"created","count":1}'
$updatePayload = '{"name":"Alice","status":"updated","count":2,"role":"admin"}'

try {
    $serverProc = Start-Process -FilePath $ServerExe `
        -ArgumentList @("--host", "127.0.0.1", "--port", "$port") `
        -WorkingDirectory $tempRoot `
        -RedirectStandardOutput $serverOut `
        -RedirectStandardError $serverErr `
        -PassThru

    if ($serverProc.HasExited) {
        throw "Server exited early with code $($serverProc.ExitCode)."
    }

    $ready = Test-ThemisServerReady -Port $port -TimeoutSeconds 30
    if (-not $ready) {
        $logsOut = if (Test-Path -LiteralPath $serverOut) { Get-Content -LiteralPath $serverOut -ErrorAction SilentlyContinue | Out-String } else { "" }
        $logsErr = if (Test-Path -LiteralPath $serverErr) { Get-Content -LiteralPath $serverErr -ErrorAction SilentlyContinue | Out-String } else { "" }
        throw "Server did not become ready within timeout. Stdout: $logsOut Stderr: $logsErr"
    }

    $createRequestBody = New-EntityPutBody -EntityId $entityId -JsonBody $createPayload
    $createResult = Invoke-ThemisHttp -Method 'PUT' -Port $port -EntityId $entityId -Body $createRequestBody
    Assert-HttpSuccess -Label "Create entity" -Response $createResult

    $readAfterCreateResult = Invoke-ThemisHttp -Method 'GET' -Port $port -EntityId $entityId
    Assert-HttpSuccess -Label "Read created entity" -Response $readAfterCreateResult
    $readAfterCreateJson = $readAfterCreateResult.Content | ConvertFrom-Json
    $createdBlob = [string]$readAfterCreateJson.blob
    if ($createdBlob -notmatch '"status"\s*:\s*"created"') {
        throw "Create/read lifecycle validation failed. Response did not contain created state: $($readAfterCreateResult.Content)"
    }

    $updateRequestBody = New-EntityPutBody -EntityId $entityId -JsonBody $updatePayload
    $updateResult = Invoke-ThemisHttp -Method 'PUT' -Port $port -EntityId $entityId -Body $updateRequestBody
    Assert-HttpSuccess -Label "Update entity" -Response $updateResult

    $readAfterUpdateResult = Invoke-ThemisHttp -Method 'GET' -Port $port -EntityId $entityId
    Assert-HttpSuccess -Label "Read updated entity" -Response $readAfterUpdateResult
    $readAfterUpdateJson = $readAfterUpdateResult.Content | ConvertFrom-Json
    $updatedBlob = [string]$readAfterUpdateJson.blob
    if ($updatedBlob -notmatch '"status"\s*:\s*"updated"') {
        throw "Update/read lifecycle validation failed. Response did not contain updated state: $($readAfterUpdateResult.Content)"
    }

    $deleteResult = Invoke-ThemisHttp -Method 'DELETE' -Port $port -EntityId $entityId
    Assert-HttpSuccess -Label "Delete entity" -Response $deleteResult

    $readAfterDeleteResult = Invoke-ThemisHttp -Method 'GET' -Port $port -EntityId $entityId
    if ($readAfterDeleteResult.StatusCode -ge 200 -and $readAfterDeleteResult.StatusCode -lt 300) {
        throw "Delete lifecycle validation failed. Entity still exists after delete: $($readAfterDeleteResult.Content)"
    }

    if ($serverProc.HasExited) {
        throw "Server exited unexpectedly after CRUD lifecycle checks (exit $($serverProc.ExitCode))."
    }
}
finally {
    if ($serverProc -and -not $serverProc.HasExited) {
        Stop-Process -Id $serverProc.Id -Force -ErrorAction SilentlyContinue
        Wait-Process -Id $serverProc.Id -Timeout 5 -ErrorAction SilentlyContinue
    }

    Remove-Item -LiteralPath $tempRoot -Recurse -Force -ErrorAction SilentlyContinue
}

Write-Host "CRUD lifecycle validation passed for server: $entityId"
