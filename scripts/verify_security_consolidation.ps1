$ErrorActionPreference = 'Stop'

$root = 'C:/Projects/ThemisDB/tests'
$sources = @(
    'auth','jwt','jwks','oauth','oauth2','oidc','mfa','saml','rbac',
    'webauthn','mtls','tls','gssapi','ldap','kerberos','pkcs11','pki',
    'keyprovider','hsm','kdf','hkdf'
)

foreach ($d in $sources) {
    $count = (Get-ChildItem (Join-Path $root $d) -File -Filter 'test_*.cpp' -ErrorAction SilentlyContinue | Measure-Object).Count
    Write-Output ($d + '=' + $count)
}

$securityCount = (Get-ChildItem (Join-Path $root 'security') -File -Filter 'test_*.cpp' | Measure-Object).Count
Write-Output ('security=' + $securityCount)
