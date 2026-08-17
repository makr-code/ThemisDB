# Root wrapper for backward compatibility. Actual script: scripts/root/setup-build-env.ps1
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
& (Join-Path $ScriptDir "scripts/root/setup-build-env.ps1") @args
exit $LASTEXITCODE