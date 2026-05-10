$ErrorActionPreference = 'Stop'
Set-CimInstance -Query "SELECT * FROM Win32_ComputerSystem" -Property @{AutomaticManagedPagefile=$false}
$pf = Get-CimInstance Win32_PageFileSetting -Filter "Name='C:\\pagefile.sys'"
if ($pf) {
	Set-CimInstance -InputObject $pf -Property @{InitialSize=32768; MaximumSize=131072} | Out-Null
} else {
	New-CimInstance -ClassName Win32_PageFileSetting -Property @{Name='C:\\pagefile.sys'; InitialSize=32768; MaximumSize=131072} | Out-Null
}
Get-CimInstance Win32_ComputerSystem | Select-Object AutomaticManagedPagefile | Format-List
Get-CimInstance Win32_PageFileSetting | Select-Object Name,InitialSize,MaximumSize | Format-Table -AutoSize