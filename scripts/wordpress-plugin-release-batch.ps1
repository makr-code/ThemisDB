param(
    [string]$Repository = 'makr-code/wordpressPlugins'
)

$ErrorActionPreference = 'Stop'
$env:GH_PAGER = 'cat'

$nestedRoot = 'wordpress-plugins'
if (Test-Path $nestedRoot) {
    $pluginRoot = $nestedRoot
    $sharedUpdater = 'wordpress-plugins/includes/class-themisdb-plugin-updater.php'
}
else {
    $pluginRoot = '.'
    $sharedUpdater = 'includes/class-themisdb-plugin-updater.php'
}

if (-not (Test-Path $sharedUpdater)) {
    throw "Missing shared updater: $sharedUpdater"
}

$plugins = Get-ChildItem $pluginRoot -Directory |
    Where-Object { Test-Path (Join-Path $_.FullName 'update-info.json') } |
    Sort-Object Name

$results = @()
$ts = Get-Date -Format 'yyyy-MM-dd-HHmmss'

foreach ($plugin in $plugins) {
    $slug = $plugin.Name

    try {
        $metadataPath = Join-Path $plugin.FullName 'update-info.json'
        $meta = Get-Content $metadataPath -Raw | ConvertFrom-Json
        $version = [string]$meta.version

        if ([string]::IsNullOrWhiteSpace($version)) {
            throw 'update-info.json has no version'
        }

        $tag = "$slug/v$version"

        gh release view "$tag" -R $Repository *> $null
        if ($LASTEXITCODE -eq 0) {
            $results += [pscustomobject]@{
                plugin_slug = $slug
                version = $version
                tag = $tag
                status = 'exists'
                release_url = "https://github.com/$Repository/releases/tag/$tag"
                message = 'Release already exists, skipped.'
            }
            continue
        }

        $zipPath = Join-Path 'dist' "$slug.zip"
        if (Test-Path $zipPath) {
            Remove-Item $zipPath -Force
        }

        $tempRoot = Join-Path $env:TEMP ("themis-release-" + [Guid]::NewGuid().ToString('N'))
        $tempPlugin = Join-Path $tempRoot $slug
        New-Item -ItemType Directory -Path $tempPlugin -Force | Out-Null

        Copy-Item (Join-Path $plugin.FullName '*') $tempPlugin -Recurse -Force

        $tempIncludes = Join-Path $tempPlugin 'includes'
        if (-not (Test-Path $tempIncludes)) {
            New-Item -ItemType Directory -Path $tempIncludes -Force | Out-Null
        }

        Copy-Item $sharedUpdater (Join-Path $tempIncludes 'class-themisdb-plugin-updater.php') -Force

        if (-not (Test-Path 'dist')) {
            New-Item -ItemType Directory -Path 'dist' | Out-Null
        }

        Compress-Archive -Path $tempPlugin -DestinationPath $zipPath -Force
        Remove-Item $tempRoot -Recurse -Force

        $changelogPath = Join-Path $plugin.FullName 'CHANGELOG.md'
        if (Test-Path $changelogPath) {
            gh release create "$tag" "$zipPath" -R $Repository --title "$slug v$version" --notes-file "$changelogPath" *> $null
        }
        else {
            gh release create "$tag" "$zipPath" -R $Repository --title "$slug v$version" --notes "Automated plugin release for $slug v$version." *> $null
        }

        if ($LASTEXITCODE -ne 0) {
            throw 'gh release create failed'
        }

        $url = (gh release view "$tag" -R $Repository --json url --jq '.url' 2>$null | Out-String).Trim()

        $results += [pscustomobject]@{
            plugin_slug = $slug
            version = $version
            tag = $tag
            status = 'created'
            release_url = $url
            message = 'Release created.'
        }
    }
    catch {
        $results += [pscustomobject]@{
            plugin_slug = $slug
            version = ''
            tag = ''
            status = 'failed'
            release_url = ''
            message = $_.Exception.Message
        }
    }
}

$created = @($results | Where-Object status -eq 'created').Count
$exists = @($results | Where-Object status -eq 'exists').Count
$failed = @($results | Where-Object status -eq 'failed').Count

$jsonPath = "artifacts/wordpress-plugin-release-batch-$ts.json"
$txtPath = "artifacts/wordpress-plugin-release-batch-$ts.txt"

$results | ConvertTo-Json -Depth 5 | Out-File $jsonPath -Encoding utf8

$lines = @()
$lines += "BATCH_RELEASE total=$($results.Count) created=$created exists=$exists failed=$failed"
$lines += "JSON=$jsonPath"
$lines += "TXT=$txtPath"
$lines += ''
$lines += 'DETAILS:'
$lines += ($results | ForEach-Object { "{0} | {1} | {2} | {3}" -f $_.plugin_slug, $_.version, $_.status, $_.tag })
$lines | Out-File $txtPath -Encoding utf8

$lines | ForEach-Object { Write-Output $_ }
