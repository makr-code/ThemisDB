#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Retired Multi-Shard RAID benchmark entry point.
.DESCRIPTION
    This legacy runner did not configure RAID and used unverified request paths.
#>

param(
    [ValidateSet('S1', 'S2', 'S3', 'S4', 'S5', 'S6', 'S7', 'S8')]
    [string]$Scenario,

    [ValidateSet('RAID0', 'RAID1', 'RAID5', 'RAID6', 'RAID10')]
    [string]$RaidLevel,

    [ValidateSet(3, 6, 12, 24)]
    [int]$NumShards = 6,

    [int]$DurationHours = 1,

    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$Arguments
)

$message = 'Retired: this runner did not configure RAID and its workload used unverified APIs. No benchmark was run and no report was generated.'
[Console]::Error.WriteLine($message)
exit 2
