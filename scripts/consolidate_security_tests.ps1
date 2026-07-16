$ErrorActionPreference = 'Stop'

$root = 'C:/Projects/ThemisDB/tests'
$sources = @(
    'auth','jwt','jwks','oauth','oauth2','oidc','mfa','saml','rbac',
    'webauthn','mtls','tls','gssapi','ldap','kerberos','pkcs11','pki',
    'keyprovider','hsm','kdf','hkdf'
)
$dest = Join-Path $root 'security'

$moved = 0
$renamed = 0
$dedup = 0

foreach ($d in $sources) {
    $dir = Join-Path $root $d
    if (-not (Test-Path $dir)) {
        continue
    }

    Get-ChildItem $dir -File -Filter 'test_*.cpp' | ForEach-Object {
        $src = $_.FullName
        $dst = Join-Path $dest $_.Name

        if (Test-Path $dst) {
            $srcHash = (Get-FileHash -Algorithm SHA256 $src).Hash
            $dstHash = (Get-FileHash -Algorithm SHA256 $dst).Hash

            if ($srcHash -eq $dstHash) {
                Remove-Item $src -Force
                $dedup++
                return
            }

            $base = [System.IO.Path]::GetFileNameWithoutExtension($_.Name)
            $ext = $_.Extension
            $candidate = Join-Path $dest ($base + '_' + $d + $ext)
            $i = 1
            while (Test-Path $candidate) {
                $candidate = Join-Path $dest ($base + '_' + $d + '_' + $i + $ext)
                $i++
            }

            Move-Item $src $candidate
            $renamed++
            $moved++
            return
        }

        Move-Item $src $dst
        $moved++
    }
}

Write-Output ('MOVED=' + $moved)
Write-Output ('RENAMED=' + $renamed)
Write-Output ('DEDUP=' + $dedup)
Write-Output ('SECURITY_COUNT=' + ((Get-ChildItem $dest -File -Filter 'test_*.cpp' | Measure-Object).Count))
