$ErrorActionPreference = 'Stop'

$testsRoot = 'C:/Projects/ThemisDB/tests'
$cmakePath = 'C:/Projects/ThemisDB/tests/CMakeLists.txt'

# Build map: file name -> first-level folder containing that file.
$fileMap = @{}
Get-ChildItem $testsRoot -Directory | ForEach-Object {
    $module = $_.Name
    Get-ChildItem $_.FullName -File -Filter 'test_*.cpp' -ErrorAction SilentlyContinue | ForEach-Object {
        $name = $_.Name
        if (-not $fileMap.ContainsKey($name)) {
            $fileMap[$name] = $module
        }
    }
}

$text = [System.IO.File]::ReadAllText($cmakePath)

foreach ($entry in $fileMap.GetEnumerator()) {
    $name = $entry.Key
    $module = $entry.Value
    $newRel = "$module/$name"

    $text = $text.Replace("`${CMAKE_CURRENT_SOURCE_DIR}/$name", "`${CMAKE_CURRENT_SOURCE_DIR}/$newRel")
    $text = $text.Replace("`${THEMIS_ROOT_DIR}/tests/$name", "`${THEMIS_ROOT_DIR}/tests/$newRel")
}

# Rewrite lines that contain only a bare test source token.
$text = [regex]::Replace(
    $text,
    '(?m)^(\s+)(test_[A-Za-z0-9_]+\.cpp)(\s*)$',
    {
        param($m)
        $indent = $m.Groups[1].Value
        $name = $m.Groups[2].Value
        $trail = $m.Groups[3].Value
        if ($fileMap.ContainsKey($name)) {
            return "$indent$($fileMap[$name])/$name$trail"
        }
        return $m.Value
    }
)

# Rewrite bare filename tokens (e.g. add_executable(foo test_bar.cpp))
# while keeping already-qualified paths untouched.
$text = [regex]::Replace(
    $text,
    '(?<![A-Za-z0-9_./-])(test_[A-Za-z0-9_]+\.cpp)(?![A-Za-z0-9_./-])',
    {
        param($m)
        $name = $m.Groups[1].Value
        if ($fileMap.ContainsKey($name)) {
            return "$($fileMap[$name])/$name"
        }
        return $name
    }
)

$enc = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText($cmakePath, $text, $enc)
Write-Output 'Rewrote tests/CMakeLists.txt source paths to modular locations.'
