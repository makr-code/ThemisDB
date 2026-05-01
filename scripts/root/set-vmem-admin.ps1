# Root wrapper for backward compatibility. Actual script: scripts/root/set-vmem-admin.ps1
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
& (Join-Path $ScriptDir "scripts/root/set-vmem-admin.ps1") @args
exit $LASTEXITCODE