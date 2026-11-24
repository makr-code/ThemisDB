$script:BenchmarkWorkloadCatalogDirectory = $PSScriptRoot

function Get-BenchmarkJsonPropertyCount {
    param($Value)
    $count = 0
    if ($Value -is [System.Management.Automation.PSCustomObject]) {
        foreach ($property in $Value.PSObject.Properties) {
            $count++
            $count += Get-BenchmarkJsonPropertyCount $property.Value
        }
    } elseif ($Value -is [array]) {
        foreach ($item in $Value) { $count += Get-BenchmarkJsonPropertyCount $item }
    }
    return $count
}

function Read-BenchmarkJsonUnique {
    param([string]$text)
    $value = ConvertFrom-Json -InputObject $text -ErrorAction Stop
    $tokens = [regex]::Matches($text, '"(?:\\.|[^"\\])*"|[{}\[\],:]')
    $propertyCount = 0
    for ($i = 0; $i -lt $tokens.Count - 1; $i++) {
        if ($tokens[$i].Value.StartsWith('"') -and $tokens[$i + 1].Value -ceq ':') {
            $propertyCount++
        }
    }
    if ($propertyCount -ne (Get-BenchmarkJsonPropertyCount $value)) {
        throw 'Duplicate or invalid JSON object properties'
    }
    return $value
}

function Resolve-BenchmarkWorkloadId {
    param(
        [Parameter(Mandatory = $true)][string]$ExecutableName,
        [Parameter(Mandatory = $true)][string]$ObservedName,
        [string]$CatalogDirectory = $script:BenchmarkWorkloadCatalogDirectory
    )

    try {
        $mappingText = Get-Content -LiteralPath (Join-Path $CatalogDirectory 'benchmark_suite_map.json') -Raw -Encoding UTF8
        $catalogText = Get-Content -LiteralPath (Join-Path $CatalogDirectory 'workload_profiles.json') -Raw -Encoding UTF8
        if ($null -eq $script:BenchmarkWorkloadCache -or
            $script:BenchmarkWorkloadCache.MappingText -cne $mappingText -or
            $script:BenchmarkWorkloadCache.CatalogText -cne $catalogText) {
            $mapping = Read-BenchmarkJsonUnique $mappingText
            $catalog = Read-BenchmarkJsonUnique $catalogText
            $script:BenchmarkWorkloadCache = @{
                MappingText = $mappingText; CatalogText = $catalogText
                Mapping = $mapping; Catalog = $catalog
            }
        } else {
            $mapping = $script:BenchmarkWorkloadCache.Mapping
            $catalog = $script:BenchmarkWorkloadCache.Catalog
        }
        if ($mapping.workload_profile_catalog -cne 'workload_profiles.json' -or
            $catalog.profiles -isnot [array] -or $catalog.profiles.Count -eq 0 -or
            $null -eq $mapping.observed_case_workload_links) { throw 'Invalid workload catalog or links' }

        $ids = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
        foreach ($profile in $catalog.profiles) {
            if ($profile.workload_id -isnot [string] -or
                [string]::IsNullOrEmpty($profile.workload_id) -or -not $ids.Add($profile.workload_id)) { throw 'Invalid or duplicate workload profile ID' }
        }

        $targetName = [System.IO.Path]::GetFileNameWithoutExtension($ExecutableName)
        $candidates = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
        foreach ($target in $mapping.observed_case_workload_links.PSObject.Properties) {
            if ([string]::IsNullOrEmpty($target.Name)) { throw 'Invalid workload link target' }
            $entries = if ($target.Value -is [array]) { $target.Value } else { @($target.Value) }
            foreach ($entry in $entries) {
                if ($entry.workload_id -isnot [string] -or -not $ids.Contains($entry.workload_id) -or
                    $entry.exact_observed_cases -isnot [array] -or $entry.exact_observed_cases.Count -eq 0) { throw 'Invalid workload link or profile reference' }
                foreach ($case in $entry.exact_observed_cases) {
                    if ($case -isnot [string] -or [string]::IsNullOrEmpty($case)) { throw 'Invalid workload link case' }
                    if ([string]::Equals($target.Name, $targetName, [System.StringComparison]::Ordinal) -and
                        [string]::Equals($case, $ObservedName, [System.StringComparison]::Ordinal)) {
                        [void]$candidates.Add($entry.workload_id)
                    }
                }
            }
        }
        if ($candidates.Count -eq 1) { return @($candidates)[0] }
    } catch {
        [Console]::Error.WriteLine("Warning: workload links unavailable: $_")
        return $null
    }
    return $null
}
