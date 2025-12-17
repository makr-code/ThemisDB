# Copyright (c) Microsoft. All rights reserved.

<#
.SYNOPSIS
Downloads the given $Version of vsdbg for the given $RuntimeID and installs it to the given $InstallPath

.DESCRIPTION
The following script will download vsdbg and install vsdbg, the .NET Core Debugger

.PARAMETER Version
Specifies the version of vsdbg to install. Can be 'latest', 'vs2022', 'vs2019', 'vs2017u5', 'vs2017u1', or a specific version string i.e. 15.0.25930.0

.PARAMETER RuntimeID
Specifies the .NET Runtime ID of the vsdbg that will be downloaded. Example: linux-x64. Defaults to win7-x64.

.Parameter InstallPath
Specifies the path where vsdbg will be installed. Defaults to the directory containing this script.

.INPUTS
None. You cannot pipe inputs to GetVsDbg.

.EXAMPLE
C:\PS> .\GetVsDbg.ps1 -Version latest -RuntimeID linux-x64 -InstallPath .\vsdbg

.LINK
For more information about using this script with Visual Studio Code see: https://github.com/OmniSharp/omnisharp-vscode/wiki/Attaching-to-remote-processes

For more information about using this script with Visual Studio see: https://github.com/Microsoft/MIEngine/wiki/Offroad-Debugging-of-.NET-Core-on-Linux---OSX-from-Visual-Studio

To report issues, see: https://github.com/omnisharp/omnisharp-vscode/issues
#>

Param (
    [Parameter(Mandatory=$true, ParameterSetName="ByName")]
    [string]
    [ValidateSet("latest", "vs2022", "vs2019", "vs2017u1", "vs2017u5")]
    $Version,

    [Parameter(Mandatory=$true, ParameterSetName="ByNumber")]
    [string]
    [ValidatePattern("\d+\.\d+\.\d+.*")]
    $VersionNumber,

    [Parameter(Mandatory=$false)]
    [string]
    $RuntimeID,

    [Parameter(Mandatory=$false)]
    [string]
    $InstallPath = (Split-Path -Path $MyInvocation.MyCommand.Definition)
)

$ErrorActionPreference="Stop"

# In a separate method to prevent locking zip files.
function DownloadAndExtract([string]$url, [string]$targetLocation) {
    Add-Type -assembly "System.IO.Compression.FileSystem"
    Add-Type -assembly "System.IO.Compression"

    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

    Try {
        $zipStream = (New-Object System.Net.WebClient).OpenRead($url)
    }
    Catch {
        Write-Host "Info: Opening stream failed, trying again with proxy settings."
        $proxy = [System.Net.WebRequest]::GetSystemWebProxy()
        $proxy.Credentials = [System.Net.CredentialCache]::DefaultNetworkCredentials
        $webClient = New-Object System.Net.WebClient
        $webClient.UseDefaultCredentials = $false
        $webClient.proxy = $proxy

        $zipStream = $webClient.OpenRead($url)
    }
    
    $zipArchive = New-Object System.IO.Compression.ZipArchive -ArgumentList $zipStream
    [System.IO.Compression.ZipFileExtensions]::ExtractToDirectory($zipArchive, $targetLocation)
    $zipArchive.Dispose()
    $zipStream.Dispose()
}

# Checks if the existing version is the latest version.
function IsLatest([string]$installationPath, [string]$runtimeId, [string]$version) {
    $SuccessRidFile = Join-Path -Path $installationPath -ChildPath "success_rid.txt"
    if (Test-Path $SuccessRidFile) {
        $LastRid = Get-Content -Path $SuccessRidFile
        if ($LastRid -ne $runtimeId) {
            return $false
        }
    } else {
        return $false
    }

    $SuccessVersionFile = Join-Path -Path $installationPath -ChildPath "success_version.txt"
    if (Test-Path $SuccessVersionFile) {
        $LastVersion = Get-Content -Path $SuccessVersionFile
        if ($LastVersion -ne $version) {
            return $false
        }
    } else {
        return $false
    }

    return $true
}

function WriteSuccessInfo([string]$installationPath, [string]$runtimeId, [string]$version) {
    $SuccessRidFile = Join-Path -Path $installationPath -ChildPath "success_rid.txt"
    $runtimeId | Out-File -Encoding ascii $SuccessRidFile

    $SuccessVersionFile = Join-Path -Path $installationPath -ChildPath "success_version.txt"
    $version | Out-File -Encoding ascii $SuccessVersionFile
}

$ExplitVersionNumberUsed = $false
if ($Version -eq "latest") {
    $VersionNumber = "18.0.10821.2"
} elseif ($Version -eq "vs2022") {
    $VersionNumber = "18.0.10821.2"
} elseif ($Version -eq "vs2019") {
    $VersionNumber = "18.0.10821.2"
} elseif ($Version -eq "vs2017u5") {
    $VersionNumber = "18.0.10821.2"
} elseif ($Version -eq "vs2017u1") {
    $VersionNumber = "15.1.10630.1"
} else {
    $ExplitVersionNumberUsed = $true
}
Write-Host "Info: Using vsdbg version '$VersionNumber'"

if (-not $RuntimeID) {
    $RuntimeID = "win7-x64"
} elseif (-not $ExplitVersionNumberUsed) {
    $legacyLinuxRuntimeIds = @{ 
        "debian.8-x64" = "";
        "rhel.7.2-x64" = "";
        "centos.7-x64" = "";
        "fedora.23-x64" = "";
        "opensuse.13.2-x64" = "";
        "ubuntu.14.04-x64" = "";
        "ubuntu.16.04-x64" = "";
        "ubuntu.16.10-x64" = "";
        "fedora.24-x64" = "";
        "opensuse.42.1-x64" = "";
    }

    # Remap the old distro-specific runtime ids unless the caller specified an exact build number.
    # We don't do this in the exact build number case so that old builds can be used.
    if ($legacyLinuxRuntimeIds.ContainsKey($RuntimeID.ToLowerInvariant())) {
        $RuntimeID = "linux-x64"
    }
}
Write-Host "Info: Using Runtime ID '$RuntimeID'"

# if we were given a relative path, assume its relative to the script directory and create an absolute path
if (-not([System.IO.Path]::IsPathRooted($InstallPath))) {
    $InstallPath = Join-Path -Path (Split-Path -Path $MyInvocation.MyCommand.Definition) -ChildPath $InstallPath
}

if (IsLatest $InstallPath $RuntimeID $VersionNumber) {
    Write-Host "Info: Latest version of VsDbg is present. Skipping downloads"
} else {
    if (Test-Path $InstallPath) {
        Write-Host "Info: $InstallPath exists, deleting."
        Remove-Item $InstallPath -Force -Recurse -ErrorAction Stop
    }
 
    $target = ("vsdbg-" + $VersionNumber).Replace('.','-') + "/vsdbg-" + $RuntimeID + ".zip"
    $url = "https://vsdebugger-cyg0dxb6czfafzaz.b01.azurefd.net/" + $target

    DownloadAndExtract $url $InstallPath

    WriteSuccessInfo $InstallPath $RuntimeID $VersionNumber
    Write-Host "Info: Successfully installed vsdbg at '$InstallPath'"
}

# SIG # Begin signature block
# MIIpbQYJKoZIhvcNAQcCoIIpXjCCKVoCAQExDzANBglghkgBZQMEAgEFADB5Bgor
# BgEEAYI3AgEEoGswaTA0BgorBgEEAYI3AgEeMCYCAwEAAAQQH8w7YFlLCE63JNLG
# KX7zUQIBAAIBAAIBAAIBAAIBADAxMA0GCWCGSAFlAwQCAQUABCCso3KMlietkEOy
# 21jWL7OpRdpgBASgHsCQcPPY40S6oaCCDdYwgga9MIIEpaADAgECAhMzAAAAHEif
# gd+hsLd3AAAAAAAcMA0GCSqGSIb3DQEBDAUAMIGIMQswCQYDVQQGEwJVUzETMBEG
# A1UECBMKV2FzaGluZ3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEeMBwGA1UEChMVTWlj
# cm9zb2Z0IENvcnBvcmF0aW9uMTIwMAYDVQQDEylNaWNyb3NvZnQgUm9vdCBDZXJ0
# aWZpY2F0ZSBBdXRob3JpdHkgMjAxMDAeFw0yNDA4MDgyMTM2MjNaFw0zNTA2MjMy
# MjA0MDFaMF8xCzAJBgNVBAYTAlVTMR4wHAYDVQQKExVNaWNyb3NvZnQgQ29ycG9y
# YXRpb24xMDAuBgNVBAMTJ01pY3Jvc29mdCBXaW5kb3dzIENvZGUgU2lnbmluZyBQ
# Q0EgMjAyNDCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAJp9a30nwXYq
# Lq7j1TT/zCtt7vxU+CCj+7BkifS/B2gXKGU7OV9SXRJGP1yFs5p6jpsYi4cYzF56
# AV0AEmmEjV8wT2lvPU5BhN3wV30HqYPIYEj5P3WXf0kXD9fvjUf1GAtXEriJ8w7A
# LNaVEm9Rs4ePA0ZsYHaCbU5kBUJQDXv76hafOcQgdFCA3I3zYtfzX2vOwx87uDOa
# CuyKORZih9c3zTf+TLC5QYLyhVMBnDXEHDOrvaw92DSyIqpdgRWpufzqDFy1egVj
# koXZhb+9pZ9heUzNXTXhOoXzexh6YzAL4flBWm+Bc1hQyESenEvBJznV+25u3h77
# jjgMUY44+WXQ4u9qddDe/U5SeAaKRvvibmi4z7QRpLvZsla0CPiOUGz00Do5sfkC
# 0EwlsSzfM3+8A9rsyFVOgWDVPzt98OJP2EoaEOq8GE9GCoN2i7/4C2FCwff1BSCT
# JWZO1Wcr2MteJE6UxGV+ihA8nN51YPKD2dYGoewrXvRzC/1HoUeSvlZf0mf9GHEt
# vvkbJVRRo6PBf0md5t87Vb1mM/fIp1eypyaxmXkgpcBwuylsOq2kSVOJ5wBPoaEs
# sJkeMcKnEuuu++UKdDHlS0DtsYjN0QnOucvTdSsdvhzKOSjJF3XVqr9f2C945LXT
# 5rxKIHUIEDBcNYU6BKDDH6rfpKOOCSilAgMBAAGjggFGMIIBQjAOBgNVHQ8BAf8E
# BAMCAYYwEAYJKwYBBAGCNxUBBAMCAQAwHQYDVR0OBBYEFB6C3w7XjLPXAjSDDtqr
# rWW5r7jsMBkGCSsGAQQBgjcUAgQMHgoAUwB1AGIAQwBBMA8GA1UdEwEB/wQFMAMB
# Af8wHwYDVR0jBBgwFoAU1fZWy4/oolxiaNE9lJBb186aGMQwVgYDVR0fBE8wTTBL
# oEmgR4ZFaHR0cDovL2NybC5taWNyb3NvZnQuY29tL3BraS9jcmwvcHJvZHVjdHMv
# TWljUm9vQ2VyQXV0XzIwMTAtMDYtMjMuY3JsMFoGCCsGAQUFBwEBBE4wTDBKBggr
# BgEFBQcwAoY+aHR0cDovL3d3dy5taWNyb3NvZnQuY29tL3BraS9jZXJ0cy9NaWNS
# b29DZXJBdXRfMjAxMC0wNi0yMy5jcnQwDQYJKoZIhvcNAQEMBQADggIBAENf+N8/
# u+mUjDtc9btoA52RBc0XVDSBMQBMqxu56hXHBwuctUWs1XBqDDMIFCHu9c6Y/UF+
# TN8EIgjnujApKYmHP4f4EM3ARSmlzrpF5ozOJx0BA5FUv1jmpdf/2ZbqpvCxlxv/
# G1R4KjrSmmqPHzs6igw3b7RTbj7BxIS8fOIkwYWQhB2fLjlg+3HSrDGPFIhpIJWV
# amMIR7a72OGonjdf45rspwqIHuynZU4avy9ruB/Rhhbwm+fMb8BMecIaTmkohx/E
# ZZ5GNWcN6oTYW3G2BM3B3YznWkl9t4shP60fMue+2ksdHGWSE8EVTdSmGUdj0jrU
# c46lGVFJISF3/MxcxnlNeP1Khyr+ZzT4Ets/I7mufpaLnLalzMR2zIuhGOAWWswe
# sbjtFzkVUFgDR2SW903I0XKlbPEA6q8epHGJ9roxh85nsEKcBNUw4Scp68KCqSpF
# BaKiyV1skd+l8U50WNePMb9Bzz0OfASal8v5sQG+DW01kN+I+RKUIbM5I50wJjiH
# ymQFNDsbobFx9I95mCEEPU7fUZ3VT/HOUVbkmX7ltIC/eQAu5GO8fu+ceETMybvb
# oxUM4dYNC+PzooUxfmC0DuKRwB21bX9+acuIBkxIm4Ed3O19w1VLoA7UNOUuJ7z6
# NQ2W/+q7cnfOPl2QVL4qlgCblUT2vmQpllV3MIIHETCCBPmgAwIBAgITMwAAAIe8
# gm6Foa5TqAAAAAAAhzANBgkqhkiG9w0BAQwFADBfMQswCQYDVQQGEwJVUzEeMBwG
# A1UEChMVTWljcm9zb2Z0IENvcnBvcmF0aW9uMTAwLgYDVQQDEydNaWNyb3NvZnQg
# V2luZG93cyBDb2RlIFNpZ25pbmcgUENBIDIwMjQwHhcNMjUwNTA4MTgyNDU0WhcN
# MjYwNTA2MTgyNDU0WjB0MQswCQYDVQQGEwJVUzETMBEGA1UECBMKV2FzaGluZ3Rv
# bjEQMA4GA1UEBxMHUmVkbW9uZDEeMBwGA1UEChMVTWljcm9zb2Z0IENvcnBvcmF0
# aW9uMR4wHAYDVQQDExVNaWNyb3NvZnQgQ29ycG9yYXRpb24wggIiMA0GCSqGSIb3
# DQEBAQUAA4ICDwAwggIKAoICAQC0JvvNCq9eE0sLt0WdArsItuEZpbuTI0W3EVMS
# i6PTkY1xniZhsm8rsEzr/Ngq77HknKV8GYYNkVxhppwdoQilh/R0KELXZi22Qqi4
# jEUsOYgkDHPIbnUEcQvbaG1R2ijOme/uGE9DX3RBx9Pkbi8TYec1d2iDhzF/xofy
# oZHPUTvL2PzDae7ncAjjRChG7kCzobShkcMpUZPyYMXKtqCMAx4OBYdeBF6PGfft
# 4Z16crWAmSEGMCcXQ7EVxWj1W7R5sOo4TilWQBQp9yYyvKYUObpiQwUjNZb4wALe
# Y6msmJogp7LC5N6warLYTbhwJJblmhZQIhaD8UABuP030nUofVkGDqK6xC/REnOE
# KTnsE+KaRb8JDOBXWSscNMQSR7wbX2NF+hK/S4dg6NUHzr0p0k20yY8LZg0OPOeb
# Hg5WdXUqkFHNB4Ck2aOT4rhu7YYiYBewfVZXF4/XF/BemkMKgnQToJvFJYLMPZM3
# tosN1IW0Ow3Ny3RvwQfHPGQ1kSqprOl14JFTI9CO/CZzhZbgeRB9Z3dWcXt1RYpu
# NIAkMWl7gDTRYy4gWhkgmzE1x1cz1hA8hKZIvD0VmNiwqN1HHRDHn2ryxmLgZgLh
# kth4K/i3CR+xSiptNnYnpMkE0Rre89r45MKUHytz0z0yqSsey/BsJ9RhnTvqiNrX
# Ay0TBwIDAQABo4IBrzCCAaswDgYDVR0PAQH/BAQDAgeAMB8GA1UdJQQYMBYGCisG
# AQQBgjc9BgEGCCsGAQUFBwMDMAwGA1UdEwEB/wQCMAAwHQYDVR0OBBYEFCYGR6ii
# PVV4VrV7+HKNkZBNf6SoMEUGA1UdEQQ+MDykOjA4MR4wHAYDVQQLExVNaWNyb3Nv
# ZnQgQ29ycG9yYXRpb24xFjAUBgNVBAUTDTIzMDg2NSs1MDQ1ODEwHwYDVR0jBBgw
# FoAUHoLfDteMs9cCNIMO2qutZbmvuOwwagYDVR0fBGMwYTBfoF2gW4ZZaHR0cDov
# L3d3dy5taWNyb3NvZnQuY29tL3BraW9wcy9jcmwvTWljcm9zb2Z0JTIwV2luZG93
# cyUyMENvZGUlMjBTaWduaW5nJTIwUENBJTIwMjAyNC5jcmwwdwYIKwYBBQUHAQEE
# azBpMGcGCCsGAQUFBzAChltodHRwOi8vd3d3Lm1pY3Jvc29mdC5jb20vcGtpb3Bz
# L2NlcnRzL01pY3Jvc29mdCUyMFdpbmRvd3MlMjBDb2RlJTIwU2lnbmluZyUyMFBD
# QSUyMDIwMjQuY3J0MA0GCSqGSIb3DQEBDAUAA4ICAQB+y/LNkTgCe/vYXjHxIrS/
# 73+FTugtMOG7l/fuiVFw3poLaGGtcn7LF5s8/h6soz4ST82QeZA3y/ADvl5VXeO1
# 5mROcELkFZ8kbdzqXTnyuWpDOTTm54DX2XVwxQfFIxYJqBij8pwvlNAB5r3JOBOA
# fegOad1NuCoP0/aA5hgu4ci3d0i+LaBVHm2RlHM44si1KyEg8Rs9g/SQPSijwRMt
# Da7TsWmz7F1J9P+UV36yOmwQ4jepF5h8hFUSrCJ3x7tEqA0ruiy2yCT15FcaJdnQ
# 3tZ8RRUdoCJ/pxtDI3YySMLA2XYLs4qHzoF/TPPNtaWDTvr4a+rBbUZm58pV+S14
# jhtkQAoO+275/a+ESeRSEeP9+Hde3Ez06U1MRKu+uicTPfDhUggSXzcaDImnRS6i
# FjEryJGHhRyZYoMphCjLn+PBJe58nvL/HWuP7kCMbSJOYz1ghVU/k5cgTYqq/V3E
# yzIo2K0MtwoUQcZygkTdCRUv6EtqFWbxtK6fyM6mbCoRgs2tKdMl8EN5nSMRXZEr
# sx6bG6M7BgTiSP56F//HvdLyGKfAR247Nu4/1nxgcDBZVcvCJyj6FmYKnvQEWixb
# aXFlpvCMvahUFV1b/1FKk2EF4ntMScCjyTKUXSTt8cgmVfS8FB1YkZm7IsCjBkua
# GWbGZBLKlwLys7wpFhqvOzGCGu0wghrpAgEBMHYwXzELMAkGA1UEBhMCVVMxHjAc
# BgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEwMC4GA1UEAxMnTWljcm9zb2Z0
# IFdpbmRvd3MgQ29kZSBTaWduaW5nIFBDQSAyMDI0AhMzAAAAh7yCboWhrlOoAAAA
# AACHMA0GCWCGSAFlAwQCAQUAoIGuMBkGCSqGSIb3DQEJAzEMBgorBgEEAYI3AgEE
# MBwGCisGAQQBgjcCAQsxDjAMBgorBgEEAYI3AgEVMC8GCSqGSIb3DQEJBDEiBCAq
# 7SVtD1UyKhmaObpuac4LDoAOpbmk/T/4u2SfndLv+jBCBgorBgEEAYI3AgEMMTQw
# MqAUgBIATQBpAGMAcgBvAHMAbwBmAHShGoAYaHR0cDovL3d3dy5taWNyb3NvZnQu
# Y29tMA0GCSqGSIb3DQEBAQUABIICAJWpB2q8GBnF0HLgTqUowcCR8uPJBx4LwZCg
# 5SQfG6dnPgiKAdaSAXvAlMlcQwn+r/HFI97/K2WZuYWPVnF66bj6CjGnwE7zle5B
# IKP5Tu6nD5DI2mzTWr6EEmsdtgsyJXAYhWb/CMTvBqKAnjykNiq5eSF2e7zTxD9f
# Zx6cxOOdOSZVPeQydIZ7ksvRSZrdXZQskbeZAwz9rhxueCFRZF3BO6HOKjHaf88q
# Ay6vpUF14OfyDW8nzrhwwe/6GFHtd4I3C2kCABAbi2c0A2iamcwRuxwdPhNKxMEy
# s+lKnuaieN3b2GoNi6ggL0g/9zNux1tRhVCkEQ+++7/WS7Pktn1yb0LnObgevrsQ
# U86tXsfRzfMc1HZtohOAhRBVATevPNzdYfMYKeIzj84byA55L5Zju1I/lapTQVR6
# e5SV66at974/K0px0DKpKV9Kc2n4cwSqOnl4ss/bHrw7UvRhN1U7RrP6nPHRtPvp
# XmMcxCdmX2csNRQfkoAxZvL9cY+1dyjPnUouZpt0O3PmOTJcowlI9XOAq91OrJjY
# Lp/aOWCoNdO1Emnj4dazCb+CK4eKwv5zB8cjAHg9pokBrT3lKbEfgMm3YB+i6B2y
# 3KroUy5bBtrtdDAUQ46bewEXMrfX/66PGrbQyqV1U0K0EVADzjS8iYMZqd+Lq9TY
# J/dQqAPBoYIXlzCCF5MGCisGAQQBgjcDAwExgheDMIIXfwYJKoZIhvcNAQcCoIIX
# cDCCF2wCAQMxDzANBglghkgBZQMEAgEFADCCAVIGCyqGSIb3DQEJEAEEoIIBQQSC
# AT0wggE5AgEBBgorBgEEAYRZCgMBMDEwDQYJYIZIAWUDBAIBBQAEIInRbX+8/y9l
# g046V51VxM0aXcnU6dR503judrYekExyAgZoo50UPhwYEzIwMjUwODIxMDE0MjU1
# LjQ5N1owBIACAfSggdGkgc4wgcsxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNo
# aW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQgQ29y
# cG9yYXRpb24xJTAjBgNVBAsTHE1pY3Jvc29mdCBBbWVyaWNhIE9wZXJhdGlvbnMx
# JzAlBgNVBAsTHm5TaGllbGQgVFNTIEVTTjpEQzAwLTA1RTAtRDk0NzElMCMGA1UE
# AxMcTWljcm9zb2Z0IFRpbWUtU3RhbXAgU2VydmljZaCCEe0wggcgMIIFCKADAgEC
# AhMzAAACA7seXAA4bHTKAAEAAAIDMA0GCSqGSIb3DQEBCwUAMHwxCzAJBgNVBAYT
# AlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4wHAYD
# VQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xJjAkBgNVBAMTHU1pY3Jvc29mdCBU
# aW1lLVN0YW1wIFBDQSAyMDEwMB4XDTI1MDEzMDE5NDI0NloXDTI2MDQyMjE5NDI0
# NlowgcsxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYDVQQH
# EwdSZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xJTAjBgNV
# BAsTHE1pY3Jvc29mdCBBbWVyaWNhIE9wZXJhdGlvbnMxJzAlBgNVBAsTHm5TaGll
# bGQgVFNTIEVTTjpEQzAwLTA1RTAtRDk0NzElMCMGA1UEAxMcTWljcm9zb2Z0IFRp
# bWUtU3RhbXAgU2VydmljZTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIB
# AKGXQwfnACc7HxSHxG2J0XQnTJoUMclgdOk+9FHXpfUrEYNh9Pw+twaMIsKJo67c
# rUOZQhThFzmiWd2Nqmk246DPBSiPjdVtsnHk8VNj9rVnzS2mpU/Q6gomVSR8M9IE
# sWBdaPpWBrJEIg20uxRqzLTDmDKwPsgs9m6JCNpx7krEBKMp/YxVfWp8TNgFtMY0
# SKNJAIrDDJzR5q+vgWjdf/6wK64C2RNaKyxTriTysrrSOwZECmIRJ1+4evTJYCZz
# uNM4814YDHooIvaS2mcZ6AsN3UiUToG7oFLAAgUevvM7AiUWrJC4J7RJAAsJsmGx
# P3L2LLrVEkBexTS7RMLlhiZNJsQjuDXR1jHxSP6+H0icugpgLkOkpvfXVthV3RvK
# 1vOV9NGyVFMmCi2d8IAgYwuoSqT3/ZVEa72SUmLWP2dV+rJgdisw84FdytBhbSOY
# o2M4vjsJoQCs3OEMGJrXBd0kA0qoy8nylB7abz9yJvIMz7UFVmq40Ci/03i0kXgA
# K2NfSONc0NQy1JmhUVAf4WRZ189bHW4EiRz3tH7FEu4+NTKkdnkDcAAtKR7hNpEG
# 9u9MFjJbYd6c5PudgspM7iPDlCrpzDdn3NMpI9DoPmXKJil6zlFHYx0y8lLh8Jw8
# kV5pU6+5YVJD8Qa1UFKGGYsH7l7DMXN2l/VS4ma45BNPAgMBAAGjggFJMIIBRTAd
# BgNVHQ4EFgQUsilZQH4R55Db2xZ7RV3PFZAYkn0wHwYDVR0jBBgwFoAUn6cVXQBe
# Yl2D9OXSZacbUzUZ6XIwXwYDVR0fBFgwVjBUoFKgUIZOaHR0cDovL3d3dy5taWNy
# b3NvZnQuY29tL3BraW9wcy9jcmwvTWljcm9zb2Z0JTIwVGltZS1TdGFtcCUyMFBD
# QSUyMDIwMTAoMSkuY3JsMGwGCCsGAQUFBwEBBGAwXjBcBggrBgEFBQcwAoZQaHR0
# cDovL3d3dy5taWNyb3NvZnQuY29tL3BraW9wcy9jZXJ0cy9NaWNyb3NvZnQlMjBU
# aW1lLVN0YW1wJTIwUENBJTIwMjAxMCgxKS5jcnQwDAYDVR0TAQH/BAIwADAWBgNV
# HSUBAf8EDDAKBggrBgEFBQcDCDAOBgNVHQ8BAf8EBAMCB4AwDQYJKoZIhvcNAQEL
# BQADggIBAJAQxt6wPpMLTxHShgJ1ILjnYBCsZ/87z0ZDngK2ASxvHPAYNVRyaNcV
# ydolJM150EpVeQGBrBGic/UuDEfhvPNWPZ5Y2bMYjA7UWWGV0A84cDMsEQdGhJni
# l10W1pDGhptT83W9bIgKI3rQi3zmCcXkkPgwxfJ3qlLx4AMiLpO2N+Ao+i6ZZrQE
# VD9oTONSt883Wvtysr6qSYvO3D8Q1LvN6Z/LHiQZGDBjVYF8Wqb+cWUkM9AGJyp5
# Td06n2GPtaoPRFz7/hVnrBCN6wjIKS/m6FQ3LYuE0OLaV5i0CIgWmaN82TgaeAu8
# LZOP0is4y/bRKvKbkn8WHvJYCI94azfIDdBqmNlO1+vs1/OkEglDjFP+JzhYZaqE
# aVGVUEjm7o6PDdnFJkIuDe9ELgpjKmSHwV0hagqKuOJ0QaVew06j5Q/9gbkqF5uK
# 51MHEZ5x8kK65Sykh1GFK0cBCyO/90CpYEuWGiurY4Jo/7AWETdY+CefHml+W+W6
# Ohw+Cw3bj7510euXc7UUVptbybRSQMdIoKHxBPBORg7C732ITEFVaVthlHPao4gG
# Mv+jMSG0IHRq4qF9Mst640YFRoHP6hln5f1QAQKgyGQRONvph81ojVPu9UBqK6EG
# hX8kI5BP5FhmuDKTI+nOmbAw0UEPW91b/b2r2eRNagSFwQ47Qv03MIIHcTCCBVmg
# AwIBAgITMwAAABXF52ueAptJmQAAAAAAFTANBgkqhkiG9w0BAQsFADCBiDELMAkG
# A1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcTB1JlZG1vbmQx
# HjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEyMDAGA1UEAxMpTWljcm9z
# b2Z0IFJvb3QgQ2VydGlmaWNhdGUgQXV0aG9yaXR5IDIwMTAwHhcNMjEwOTMwMTgy
# MjI1WhcNMzAwOTMwMTgzMjI1WjB8MQswCQYDVQQGEwJVUzETMBEGA1UECBMKV2Fz
# aGluZ3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEeMBwGA1UEChMVTWljcm9zb2Z0IENv
# cnBvcmF0aW9uMSYwJAYDVQQDEx1NaWNyb3NvZnQgVGltZS1TdGFtcCBQQ0EgMjAx
# MDCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAOThpkzntHIhC3miy9ck
# eb0O1YLT/e6cBwfSqWxOdcjKNVf2AX9sSuDivbk+F2Az/1xPx2b3lVNxWuJ+Slr+
# uDZnhUYjDLWNE893MsAQGOhgfWpSg0S3po5GawcU88V29YZQ3MFEyHFcUTE3oAo4
# bo3t1w/YJlN8OWECesSq/XJprx2rrPY2vjUmZNqYO7oaezOtgFt+jBAcnVL+tuhi
# JdxqD89d9P6OU8/W7IVWTe/dvI2k45GPsjksUZzpcGkNyjYtcI4xyDUoveO0hyTD
# 4MmPfrVUj9z6BVWYbWg7mka97aSueik3rMvrg0XnRm7KMtXAhjBcTyziYrLNueKN
# iOSWrAFKu75xqRdbZ2De+JKRHh09/SDPc31BmkZ1zcRfNN0Sidb9pSB9fvzZnkXf
# tnIv231fgLrbqn427DZM9ituqBJR6L8FA6PRc6ZNN3SUHDSCD/AQ8rdHGO2n6Jl8
# P0zbr17C89XYcz1DTsEzOUyOArxCaC4Q6oRRRuLRvWoYWmEBc8pnol7XKHYC4jMY
# ctenIPDC+hIK12NvDMk2ZItboKaDIV1fMHSRlJTYuVD5C4lh8zYGNRiER9vcG9H9
# stQcxWv2XFJRXRLbJbqvUAV6bMURHXLvjflSxIUXk8A8FdsaN8cIFRg/eKtFtvUe
# h17aj54WcmnGrnu3tz5q4i6tAgMBAAGjggHdMIIB2TASBgkrBgEEAYI3FQEEBQID
# AQABMCMGCSsGAQQBgjcVAgQWBBQqp1L+ZMSavoKRPEY1Kc8Q/y8E7jAdBgNVHQ4E
# FgQUn6cVXQBeYl2D9OXSZacbUzUZ6XIwXAYDVR0gBFUwUzBRBgwrBgEEAYI3TIN9
# AQEwQTA/BggrBgEFBQcCARYzaHR0cDovL3d3dy5taWNyb3NvZnQuY29tL3BraW9w
# cy9Eb2NzL1JlcG9zaXRvcnkuaHRtMBMGA1UdJQQMMAoGCCsGAQUFBwMIMBkGCSsG
# AQQBgjcUAgQMHgoAUwB1AGIAQwBBMAsGA1UdDwQEAwIBhjAPBgNVHRMBAf8EBTAD
# AQH/MB8GA1UdIwQYMBaAFNX2VsuP6KJcYmjRPZSQW9fOmhjEMFYGA1UdHwRPME0w
# S6BJoEeGRWh0dHA6Ly9jcmwubWljcm9zb2Z0LmNvbS9wa2kvY3JsL3Byb2R1Y3Rz
# L01pY1Jvb0NlckF1dF8yMDEwLTA2LTIzLmNybDBaBggrBgEFBQcBAQROMEwwSgYI
# KwYBBQUHMAKGPmh0dHA6Ly93d3cubWljcm9zb2Z0LmNvbS9wa2kvY2VydHMvTWlj
# Um9vQ2VyQXV0XzIwMTAtMDYtMjMuY3J0MA0GCSqGSIb3DQEBCwUAA4ICAQCdVX38
# Kq3hLB9nATEkW+Geckv8qW/qXBS2Pk5HZHixBpOXPTEztTnXwnE2P9pkbHzQdTlt
# uw8x5MKP+2zRoZQYIu7pZmc6U03dmLq2HnjYNi6cqYJWAAOwBb6J6Gngugnue99q
# b74py27YP0h1AdkY3m2CDPVtI1TkeFN1JFe53Z/zjj3G82jfZfakVqr3lbYoVSfQ
# JL1AoL8ZthISEV09J+BAljis9/kpicO8F7BUhUKz/AyeixmJ5/ALaoHCgRlCGVJ1
# ijbCHcNhcy4sa3tuPywJeBTpkbKpW99Jo3QMvOyRgNI95ko+ZjtPu4b6MhrZlvSP
# 9pEB9s7GdP32THJvEKt1MMU0sHrYUP4KWN1APMdUbZ1jdEgssU5HLcEUBHG/ZPkk
# vnNtyo4JvbMBV0lUZNlz138eW0QBjloZkWsNn6Qo3GcZKCS6OEuabvshVGtqRRFH
# qfG3rsjoiV5PndLQTHa1V1QJsWkBRH58oWFsc/4Ku+xBZj1p/cvBQUl+fpO+y/g7
# 5LcVv7TOPqUxUYS8vwLBgqJ7Fx0ViY1w/ue10CgaiQuPNtq6TPmb/wrpNPgkNWcr
# 4A245oyZ1uEi6vAnQj0llOZ0dFtq0Z4+7X6gMTN9vMvpe784cETRkPHIqzqKOghi
# f9lwY1NNje6CbaUFEMFxBmoQtB1VM1izoXBm8qGCA1AwggI4AgEBMIH5oYHRpIHO
# MIHLMQswCQYDVQQGEwJVUzETMBEGA1UECBMKV2FzaGluZ3RvbjEQMA4GA1UEBxMH
# UmVkbW9uZDEeMBwGA1UEChMVTWljcm9zb2Z0IENvcnBvcmF0aW9uMSUwIwYDVQQL
# ExxNaWNyb3NvZnQgQW1lcmljYSBPcGVyYXRpb25zMScwJQYDVQQLEx5uU2hpZWxk
# IFRTUyBFU046REMwMC0wNUUwLUQ5NDcxJTAjBgNVBAMTHE1pY3Jvc29mdCBUaW1l
# LVN0YW1wIFNlcnZpY2WiIwoBATAHBgUrDgMCGgMVAM2vFFf+LPqyzWUEJcbw/UsX
# EPR7oIGDMIGApH4wfDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0b24x
# EDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlv
# bjEmMCQGA1UEAxMdTWljcm9zb2Z0IFRpbWUtU3RhbXAgUENBIDIwMTAwDQYJKoZI
# hvcNAQELBQACBQDsUL5TMCIYDzIwMjUwODIwMjEzNjE5WhgPMjAyNTA4MjEyMTM2
# MTlaMHcwPQYKKwYBBAGEWQoEATEvMC0wCgIFAOxQvlMCAQAwCgIBAAICEHsCAf8w
# BwIBAAICE3wwCgIFAOxSD9MCAQAwNgYKKwYBBAGEWQoEAjEoMCYwDAYKKwYBBAGE
# WQoDAqAKMAgCAQACAwehIKEKMAgCAQACAwGGoDANBgkqhkiG9w0BAQsFAAOCAQEA
# HyKXSlDDTozSYIJ1/ksaEdaJ0Jxsg9khxbpxQD8q0u1cClYZUmQgDxaKXxNT8Dp2
# ExsK3KlcUE5jSeIPLjaHd9wkMnxpJ28h442QAxt2qBYxRnjC1Pq91cYnuO7WD+6v
# sv4IQyFfpHCJ4tWZUC4QuH9LdsUhMNBz+Yd0WUmhI6MK0jus6Hldt1nShnfP6LH8
# VTKVyWOpAp/m0zYrdIZc4dhKi6C7FQ0z9piUHYY5SoNaEOFSY1FPNaRXS6MGTF3P
# X5DfjKWZhpxJ9KUBUt5QxU+maOfRUD5C67tq7WUuIg+gxmRK1VLE9rJAeIYECszE
# IIC53Q6rf2m2r1KIAbUO8jGCBA0wggQJAgEBMIGTMHwxCzAJBgNVBAYTAlVTMRMw
# EQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4wHAYDVQQKExVN
# aWNyb3NvZnQgQ29ycG9yYXRpb24xJjAkBgNVBAMTHU1pY3Jvc29mdCBUaW1lLVN0
# YW1wIFBDQSAyMDEwAhMzAAACA7seXAA4bHTKAAEAAAIDMA0GCWCGSAFlAwQCAQUA
# oIIBSjAaBgkqhkiG9w0BCQMxDQYLKoZIhvcNAQkQAQQwLwYJKoZIhvcNAQkEMSIE
# IFRxayTgDToyeabE0AU358oZGkkdT4y7OU0HqLDRgBq2MIH6BgsqhkiG9w0BCRAC
# LzGB6jCB5zCB5DCBvQQgSwPdG3GW9pPEU5lmelDDQOSw+ZV26jlLIr2H3D76Ey0w
# gZgwgYCkfjB8MQswCQYDVQQGEwJVUzETMBEGA1UECBMKV2FzaGluZ3RvbjEQMA4G
# A1UEBxMHUmVkbW9uZDEeMBwGA1UEChMVTWljcm9zb2Z0IENvcnBvcmF0aW9uMSYw
# JAYDVQQDEx1NaWNyb3NvZnQgVGltZS1TdGFtcCBQQ0EgMjAxMAITMwAAAgO7HlwA
# OGx0ygABAAACAzAiBCCjwAJ4q1j2MFY4xq7hhUQLr7Etty8bpRlh4vtDrCtZNjAN
# BgkqhkiG9w0BAQsFAASCAgAzqxYOI+6KgACWvgsp6vyPxcDAEmVVcnVvbo6aTYX9
# e3H95s/HqiMwDSL+g2vIITbbIceWxbGLOwU3FHUafWpNa8oJhvIRfiun3tpyiVrm
# vsGvfzfACYfzwi3DeIKi6iRmhIG58SeIx/q0yVTDZMWyjfaku6CwvIRu3LG1gcxU
# Ki5QJtl90JP29EyJp2qxMBN3C1Rq+xOTDiPafPusBmiZULI0IZQU5Vnm1pglsUba
# Iw/bejjVKPJO+dly1n8lqjSCN41T1S90CS9VEXYPTVgPNaRak02M5H9kBV9/LFn6
# 9a0+zTEknXWPCg8ZrAhs0dsVP2QMYfI3eCSrufUm03MXw4EGitAMahfMah2QYQDf
# WXKtOsRjXO7rGoHLpQsPAQuhczeaoIR+olmRsf1y6kqg88fGVCAjT8/DQJ9YQc3h
# 51Xj/SiDdWfunviUdnHcxN0e5QPgMXPzLr3S/DkL/ADjki9CncusCmdEB/HxPly9
# Z+RhUYr5V+ftqdirVXVWPOGYikCCtmA5ANfb++IL9lYJzMowkWQIj+Dz4UG55286
# HPpHquAg94qyfPRCEUDBS84DaFJS7KKYKSQ707EMmIC2Sht/Gj3YLvB3+Xil22RL
# n+4C0ggFuJV6l33urgAmbtgvwvZeACap6EyuIvFTvCfcQSdV38mVKpi/2cHhFZUX
# 2A==
# SIG # End signature block
