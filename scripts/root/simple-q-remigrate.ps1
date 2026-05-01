# Root wrapper for backward compatibility. Actual script: scripts/root/simple-q-remigrate.ps1
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
& (Join-Path $ScriptDir "scripts/root/simple-q-remigrate.ps1") @args
exit $LASTEXITCODE