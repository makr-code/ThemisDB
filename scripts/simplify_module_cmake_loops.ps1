$ErrorActionPreference = 'Stop'

$root = 'C:/Projects/ThemisDB/tests'
$modules = @(
    'auth','jwt','jwks','oauth','oauth2','oidc','mfa','saml','rbac',
    'webauthn','mtls','tls','gssapi','ldap','kerberos','pkcs11','pki',
    'keyprovider','hsm','kdf','hkdf',
    'wire','ws','websocket','quic','udp','socket','transport',
    'http','http2','http3','cdn'
)

$updated = 0
$missing = 0

foreach ($module in $modules) {
    $cmakePath = Join-Path $root ($module + '/CMakeLists.txt')
    if (-not (Test-Path $cmakePath)) {
        $missing++
        continue
    }

    $upper = $module.ToUpperInvariant() -replace '[^A-Z0-9]', '_'

    $content = @"
# $module module tests

file(GLOB ${upper}_MODULE_TEST_SOURCES
    "
`${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
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
    [System.IO.File]::WriteAllText($cmakePath, $content.TrimStart("`r","`n"), $enc)
    $updated++
}

Write-Output ('UPDATED=' + $updated)
Write-Output ('MISSING=' + $missing)
