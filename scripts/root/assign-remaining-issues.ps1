# Root wrapper for backward compatibility. Actual script: scripts/root/assign-remaining-issues.ps1
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
& (Join-Path $ScriptDir "scripts/root/assign-remaining-issues.ps1") @args
exit $LASTEXITCODE