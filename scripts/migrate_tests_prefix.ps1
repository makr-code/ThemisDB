$ErrorActionPreference = 'Stop'

$testsRoot = 'C:/Projects/ThemisDB/tests'
$rootFiles = Get-ChildItem $testsRoot -File -Filter 'test_*.cpp'

$createdDirs = New-Object System.Collections.Generic.HashSet[string]
$createdCmake = New-Object System.Collections.Generic.HashSet[string]
$patchedCmake = New-Object System.Collections.Generic.HashSet[string]
$moved = 0

function New-ModuleCMake {
    param(
        [string]$Module,
        [string]$FilePath
    )

    $upper = ($Module.ToUpper() -replace '[^A-Z0-9]', '_')
    $content = @"
# Auto-generated module tests for prefix '$Module'

file(GLOB ${upper}_PREFIX_TEST_SOURCES
    "`$`{CMAKE_CURRENT_SOURCE_DIR}/test_${Module}_*.cpp"
)

foreach(_src IN LISTS ${upper}_PREFIX_TEST_SOURCES)
    get_filename_component(_stem "`$`{_src}" NAME_WE)
    set(_target "module_${Module}_`$`{_stem}_focused")
    set(_ctest "`$`{_stem}_${Module}_FocusedTests")

    if(TARGET `$`{_target})
        continue()
    endif()

    add_executable(`$`{_target} "`$`{_src}")
    target_include_directories(`$`{_target} PRIVATE
        `$`{THEMIS_ROOT_DIR}/include
        `$`{THEMIS_ROOT_DIR}/src
    )
    target_link_libraries(`$`{_target} PRIVATE
        `$`{TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(`$`{_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE ${Module}
        NAME `$`{_ctest}
        TARGET `$`{_target}
        TIER unit
        TIMEOUT 120
        LABELS ${Module}
    )
endforeach()
"@

    $enc = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($FilePath, $content, $enc)
}

function Ensure-PrefixBlock {
    param(
        [string]$Module,
        [string]$FilePath
    )

    $marker = "# AUTOGEN PREFIX BLOCK: $Module"
    $text = [System.IO.File]::ReadAllText($FilePath)
    if ($text.Contains($marker)) {
        return $false
    }

    $upper = ($Module.ToUpper() -replace '[^A-Z0-9]', '_')
    $append = @"

$marker
file(GLOB ${upper}_AUTOGEN_PREFIX_TEST_SOURCES
    "`$`{CMAKE_CURRENT_SOURCE_DIR}/test_${Module}_*.cpp"
)

foreach(_src IN LISTS ${upper}_AUTOGEN_PREFIX_TEST_SOURCES)
    get_filename_component(_stem "`$`{_src}" NAME_WE)
    set(_target "module_${Module}_`$`{_stem}_autofocused")
    set(_ctest "`$`{_stem}_${Module}_AutoFocusedTests")

    if(TARGET `$`{_target})
        continue()
    endif()

    add_executable(`$`{_target} "`$`{_src}")
    target_include_directories(`$`{_target} PRIVATE
        `$`{THEMIS_ROOT_DIR}/include
        `$`{THEMIS_ROOT_DIR}/src
    )
    target_link_libraries(`$`{_target} PRIVATE
        `$`{TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(`$`{_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE ${Module}
        NAME `$`{_ctest}
        TARGET `$`{_target}
        TIER unit
        TIMEOUT 120
        LABELS ${Module} autogen
    )
endforeach()
"@

    $enc = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($FilePath, ($text + $append), $enc)
    return $true
}

foreach ($f in $rootFiles) {
    $base = $f.BaseName -replace '^test_', ''
    $prefix = ($base -split '_')[0].ToLower()
    if ([string]::IsNullOrWhiteSpace($prefix)) {
        continue
    }

    $dir = Join-Path $testsRoot $prefix
    if (-not (Test-Path $dir)) {
        New-Item -ItemType Directory -Path $dir | Out-Null
        $createdDirs.Add($prefix) | Out-Null
    }

    $cmake = Join-Path $dir 'CMakeLists.txt'
    if (-not (Test-Path $cmake)) {
        New-ModuleCMake -Module $prefix -FilePath $cmake
        $createdCmake.Add($prefix) | Out-Null
    }
    else {
        if (Ensure-PrefixBlock -Module $prefix -FilePath $cmake) {
            $patchedCmake.Add($prefix) | Out-Null
        }
    }

    $destPath = Join-Path $dir $f.Name
    if (Test-Path $destPath) {
        $srcHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $f.FullName).Hash
        $dstHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $destPath).Hash
        if ($srcHash -eq $dstHash) {
            Remove-Item -LiteralPath $f.FullName
        }
        else {
            $nameNoExt = [System.IO.Path]::GetFileNameWithoutExtension($f.Name)
            $ext = [System.IO.Path]::GetExtension($f.Name)
            $candidate = Join-Path $dir ("${nameNoExt}_root${ext}")
            $i = 1
            while (Test-Path $candidate) {
                $candidate = Join-Path $dir ("${nameNoExt}_root${i}${ext}")
                $i++
            }
            Move-Item -LiteralPath $f.FullName -Destination $candidate
        }
    }
    else {
        Move-Item -LiteralPath $f.FullName -Destination $destPath
    }
    $moved++
}

Write-Output ('MOVED=' + $moved)
Write-Output ('CREATED_DIRS=' + $createdDirs.Count)
Write-Output ('CREATED_CMAKES=' + $createdCmake.Count)
Write-Output ('PATCHED_CMAKES=' + $patchedCmake.Count)
Write-Output ('ROOT_TEST_CPP_AFTER=' + ((Get-ChildItem $testsRoot -File -Filter 'test_*.cpp' | Measure-Object).Count))
