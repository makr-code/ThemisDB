# Root wrapper for backward compatibility. Actual script: scripts/root/build.ps1
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
& (Join-Path $ScriptDir "scripts/root/build.ps1") @args
exit $LASTEXITCODE