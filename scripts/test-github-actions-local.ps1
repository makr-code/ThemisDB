param(
    [ValidateSet('lint', 'dryrun', 'all')]
    [string]$Mode = 'all',

    [string[]]$Events = @('push', 'pull_request', 'workflow_dispatch', 'schedule'),

    [string]$Workflow = '',

    [string]$LogDir = 'tmp'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# Some native tools (e.g. act) emit informational output on STDERR.
# Keep those lines from being converted to terminating PowerShell errors.
if (Get-Variable -Name PSNativeCommandUseErrorActionPreference -ErrorAction SilentlyContinue) {
    $PSNativeCommandUseErrorActionPreference = $false
}

function Write-Section {
    param([string]$Title)
    Write-Host ""
    Write-Host "=== $Title ===" -ForegroundColor Cyan
}

function Assert-CommandAvailable {
    param(
        [Parameter(Mandatory = $true)]
        [string]$CommandName,
        [string]$Hint = ''
    )

    if (-not (Get-Command -Name $CommandName -ErrorAction SilentlyContinue)) {
        $message = "Erforderliches Tool '$CommandName' wurde nicht gefunden."
        if (-not [string]::IsNullOrWhiteSpace($Hint)) {
            $message = "$message $Hint"
        }
        throw $message
    }
}

function Invoke-CommandWithLog {
    param(
        [string]$Name,
        [scriptblock]$Command,
        [string]$LogFile
    )

    Write-Section $Name
    Write-Host "Log: $LogFile"

    $previousErrorAction = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'

    & $Command 2>&1 | Tee-Object -FilePath $LogFile | Out-Host

    $ErrorActionPreference = $previousErrorAction
    $exitCode = $LASTEXITCODE

    if ($null -eq $exitCode) {
        $exitCode = 0
    }

    Write-Host "Exit code: $exitCode"
    return $exitCode
}

if (-not (Test-Path -LiteralPath '.github/workflows')) {
    throw 'Bitte aus dem Repository-Root starten (Ordner .github/workflows nicht gefunden).'
}

if (-not (Test-Path -LiteralPath $LogDir)) {
    New-Item -ItemType Directory -Path $LogDir | Out-Null
}

if ($Mode -in @('lint', 'all')) {
    Assert-CommandAvailable -CommandName 'docker' -Hint "Installationshinweis: https://docs.docker.com/get-docker/"
}

if ($Mode -in @('dryrun', 'all')) {
    Assert-CommandAvailable -CommandName 'act' -Hint "Installationshinweis: https://github.com/nektos/act#installation"
}

$timestamp = Get-Date -Format 'yyyyMMdd_HHmmss'
$overallExit = 0

if ($Mode -in @('lint', 'all')) {
    $lintLog = Join-Path $LogDir "actionlint_$timestamp.log"
    $lintExit = Invoke-CommandWithLog -Name 'actionlint (Docker)' -LogFile $lintLog -Command {
        docker run --rm -v "${PWD}:/repo" -w /repo rhysd/actionlint:latest -color
    }

    if ($lintExit -ne 0) {
        $overallExit = 1
    }
}

if ($Mode -in @('dryrun', 'all')) {
    foreach ($eventName in $Events) {
        $safeEvent = $eventName -replace '[^a-zA-Z0-9_-]', '_'
        $actLog = Join-Path $LogDir "act_dryrun_${safeEvent}_$timestamp.log"

        $actArgs = @('-n', $eventName)
        if (-not [string]::IsNullOrWhiteSpace($Workflow)) {
            $actArgs += @('--workflows', $Workflow)
        }

        $actExit = Invoke-CommandWithLog -Name "act dry-run ($eventName)" -LogFile $actLog -Command {
            act @actArgs
        }

        if ($actExit -ne 0) {
            $actLogContent = ''
            if (Test-Path -LiteralPath $actLog) {
                $actLogContent = Get-Content -LiteralPath $actLog -Raw
            }

            if ($actLogContent -match 'Could not find any stages to run') {
                Write-Host "Hinweis: Keine passenden Jobs fuer Event '$eventName' gefunden (skip)." -ForegroundColor Yellow
            }
            else {
                $overallExit = 1
            }
        }
    }
}

Write-Section 'Zusammenfassung'
Write-Host "Mode       : $Mode"
Write-Host "Events     : $($Events -join ', ')"
Write-Host "Workflow   : $Workflow"
Write-Host "Log-Ordner : $LogDir"

if ($overallExit -ne 0) {
    Write-Host 'Mindestens ein Check war fehlerhaft.' -ForegroundColor Yellow
}
else {
    Write-Host 'Alle ausgefuehrten lokalen Checks erfolgreich.' -ForegroundColor Green
}

exit $overallExit
