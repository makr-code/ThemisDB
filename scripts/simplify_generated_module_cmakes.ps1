$ErrorActionPreference = 'Stop'

$testsRoot = 'C:/Projects/ThemisDB/tests'
$cmakeFiles = Get-ChildItem $testsRoot -Directory | ForEach-Object {
    Join-Path $_.FullName 'CMakeLists.txt'
} | Where-Object { Test-Path $_ }

$updated = 0
$skipped = 0
$scanned = 0

foreach ($cmakePath in $cmakeFiles) {
    $scanned++
    $content = [System.IO.File]::ReadAllText($cmakePath)
    $lineCount = ([regex]::Matches($content, "`n")).Count + 1

    # Only rewrite auto-generated dual-loop files; avoid larger hand-maintained module files.
    if ($content -notmatch '# Auto-generated module tests for prefix') { $skipped++; continue }
    if ($content -notmatch '# AUTOGEN PREFIX BLOCK:') { $skipped++; continue }
    if ($lineCount -gt 140) { $skipped++; continue }

    $module = Split-Path (Split-Path $cmakePath -Parent) -Leaf
    $upper = $module.ToUpperInvariant() -replace '[^A-Z0-9]', '_'

    $newContent = @"
# ${module} module tests

file(GLOB ${upper}_MODULE_TEST_SOURCES
    "`${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS ${upper}_MODULE_TEST_SOURCES)
    get_filename_component(_stem "`${_src}" NAME_WE)
    set(_target "module_${module}_`${_stem}_focused")
    set(_ctest "`${_stem}_${module}_FocusedTests")

    if(TARGET `${_target})
        continue()
    endif()

    add_executable(`${_target} "`${_src}")
    target_include_directories(`${_target} PRIVATE
        `${THEMIS_ROOT_DIR}/include
        `${THEMIS_ROOT_DIR}/src
    )
    target_link_libraries(`${_target} PRIVATE
        `${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(`${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE ${module}
        NAME `${_ctest}
        TARGET `${_target}
        TIER unit
        TIMEOUT 120
        LABELS ${module}
    )
endforeach()
"@

    $enc = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($cmakePath, $newContent.TrimStart("`r", "`n"), $enc)
    $updated++
}

Write-Output ('SCANNED=' + $scanned)
Write-Output ('UPDATED=' + $updated)
Write-Output ('SKIPPED=' + $skipped)
