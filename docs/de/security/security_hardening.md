# Themis – Security Hardening Guide

**Stand:** 18. April 2026  
**Version:** 1.4.0  
**Kategorie:** Security


## 📑 Inhaltsverzeichnis

- [📋 Überblick](#themis--security-hardening-guide)
- [✅ Implementierter Security Stack](#-implementierter-security-stack-stand-2025-11-17)
- [🆕 Security Enhancements v1.4.0](#-security-enhancements-v140)
- [🛠️ Server-Härtung](#server-härtung)
- [🌐 Netzwerkebene](#netzwerkebene)
- [🖥️ Betriebssystem](#betriebssystem)
- [⚙️ Systemd Service](#systemd-service)
- [📚 Siehe auch](#siehe-auch)
- [📝 Changelog](#änderungsverlauf)


Umfassender Praxisleitfaden zur Härtung von Themis-Server mit vollständiger Security-Implementation.

## 🆕 Security Enhancements v1.4.0

### NEW: VRAM Secure Clear (P0 - CRITICAL) ✅

**GPU Memory Security für KI/ML Workloads**

ThemisDB implementiert jetzt Secure Clear für GPU VRAM, um sensible Daten (Verschlüsselungskeys, Embeddings, Model Weights) vor Cold-Boot-Angriffen und Memory Leakage zu schützen.

**Features:**
- ✅ Multi-Pass Overwrite (3x default: 0x00, 0xFF, 0xAA)
- ✅ CUDA & HIP Support
- ✅ Integration in GPU Memory Manager
- ✅ Automatisches Clearing bei cudaFree()
- ✅ Audit Logging für VRAM-Operationen
- ✅ Verifizierung optional (Compliance)

**Konfiguration:**
```yaml
# config/gpu_security.yaml
gpu_security:
  vram_secure_clear:
    enabled: true
    num_passes: 3           # Anzahl Überschreibvorgänge
    verify_clear: false     # true für Compliance-Audits
    audit_log: true         # VRAM-Operationen loggen
```

**Verwendung:**
```cpp
#include "security/vram_secure_clear.h"

// Automatisch bei GPU Memory Manager
gpu_memory_mgr.deallocate(model_id, ptr);  // Secure clear automatisch

// Manuell
VRAMSecureClear::secureClearCUDA(d_ptr, size_bytes);
```

**Tests:**
```bash
./build/tests/test_vram_secure_clear
```

**Compliance:**
- ✅ GDPR Art. 32 (Secure Deletion)
- ✅ SOC 2 CC6.1 (Data Protection)
- ✅ HIPAA § 164.310 (Device Security)

---

### NEW: Multi-Factor Authentication (P1 - HIGH) ✅

**TOTP-basierte Zwei-Faktor-Authentifizierung**

MFA schützt Admin-Accounts vor Credential-basierten Angriffen durch Time-based One-Time Passwords (TOTP) nach RFC 6238.

**Features:**
- ✅ TOTP mit 6-stelligen Codes (30s Zeitfenster)
- ✅ QR-Code für Mobile Authenticator Apps
- ✅ 8 Recovery Codes pro User
- ✅ Granulare Enforcement (Admin mandatory, User optional)
- ✅ Audit Logging für alle MFA-Events
- ✅ Compatible mit Google Authenticator, Authy, etc.

**Setup:**
```bash
# MFA für Admin aktivieren
themisctl auth mfa enroll --user admin

# QR-Code generieren
themisctl auth mfa qr-code --user admin > qr.png

# Recovery Codes anzeigen
themisctl auth mfa recovery-codes --user admin
```

**Konfiguration:**
```yaml
# config/auth.yaml
mfa:
  enabled: true
  totp:
    time_step_seconds: 30
    code_length: 6
    time_window: 1          # ±1 Zeitfenster (±30s)
    issuer: "ThemisDB"
  recovery_codes:
    count: 8
    length: 8
  enforcement:
    admin_required: true    # MFA Pflicht für Admins
    operator_required: true # MFA Pflicht für Operators
    user_optional: true     # MFA optional für User
```

**API Verwendung:**
```bash
# Login mit MFA
curl -X POST http://localhost:8080/api/v1/auth/login \
  -H "Content-Type: application/json" \
  -d '{
    "username": "admin",
    "password": "***",
    "mfa_code": "123456"
  }'

# Login mit Recovery Code
curl -X POST http://localhost:8080/api/v1/auth/login \
  -H "Content-Type: application/json" \
  -d '{
    "username": "admin",
    "password": "***",
    "recovery_code": "ABC12345"
  }'
```

**Audit Events:**
```
MFA_ENROLLED              - User enrolled in MFA
MFA_ENABLED               - MFA activated for account
MFA_DISABLED              - MFA deactivated
MFA_TOTP_SUCCESS          - Successful TOTP validation
MFA_TOTP_FAILED           - Failed TOTP attempt
MFA_RECOVERY_CODE_USED    - Recovery code used
MFA_RECOVERY_CODES_REGENERATED - New recovery codes generated
```

**Tests:**
```bash
./build/tests/test_mfa_authenticator
```

**Compliance:**
- ✅ SOC 2 CC6.1 (Logical Access Controls)
- ✅ NIST SP 800-63B Level 2
- ✅ PCI DSS 8.3 (Multi-Factor Authentication)

---

### NEW: Enhanced Security Testing (P1 - HIGH) ✅

**Automatisierte Security Scans mit OWASP ZAP**

ThemisDB integriert OWASP ZAP in CI/CD Pipeline für kontinuierliche Sicherheitstests.

**Scan-Modi:**
1. **Baseline Scan** (PR/Push): Passive Scans für schnelles Feedback
2. **Full Scan** (Weekly): Aktive Scans mit Spider
3. **API Scan** (PR/Push): OpenAPI-basierte API-Tests

**GitHub Actions Workflow:**
`.github/workflows/owasp-zap.yml`

**Konfiguration:**
```tsv
# .github/zap/rules.tsv
40012	FAIL	Cross Site Scripting (Reflected)
40018	FAIL	SQL Injection
90020	FAIL	Remote OS Command Injection
90023	FAIL	XML External Entity Attack
90034	FAIL	JWT None Algorithm
90035	FAIL	JWT Weak Secret
```

**Lokale Ausführung:**
```bash
# ZAP Baseline Scan
docker run -v $(pwd):/zap/wrk/:rw \
  -t owasp/zap2docker-stable \
  zap-baseline.py -t http://localhost:8080 \
  -c .github/zap/rules.tsv
```

**Security Test Suites:**
```bash
# JWT Security Tests
./build/tests/security/test_jwt_security

# Input Validation Tests
./build/tests/security/test_input_validation_security

# Alle Security Tests
cmake --build build --target test_security
```

**Coverage:**
- ✅ JWT Algorithm Confusion (None, HS256→RS256)
- ✅ Token Forgery & Signature Bypass
- ✅ XSS (Reflected, Persistent, DOM-based)
- ✅ SQL/AQL Injection
- ✅ Path Traversal
- ✅ Command Injection
- ✅ XXE (XML External Entity)
- ✅ LDAP Injection
- ✅ CRLF Injection

---

### Enhanced Audit Logging (85+ Event Types)

**Neue Event-Typen:**
```cpp
// MFA Events (8)
MFA_ENROLLED, MFA_ENABLED, MFA_DISABLED, 
MFA_TOTP_SUCCESS, MFA_TOTP_FAILED,
MFA_RECOVERY_CODE_USED, MFA_RECOVERY_CODES_REGENERATED

// GPU/VRAM Security (4)
VRAM_ALLOCATED, VRAM_DEALLOCATED, 
VRAM_SECURE_CLEAR, GPU_MEMORY_EXHAUSTION

// Binary Integrity (3)
BINARY_SIGNATURE_VERIFIED, BINARY_SIGNATURE_FAILED,
MANIFEST_UPDATED
```

**Verwendung:**
```cpp
#include "utils/audit_logger.h"

// MFA Event loggen
audit_logger->logSecurityEvent(
    SecurityEventType::MFA_TOTP_SUCCESS,
    "admin",
    "login",
    {{"ip", "192.168.1.1"}, {"timestamp", "2026-01-18T08:00:00Z"}}
);

// VRAM Event loggen
audit_logger->logSecurityEvent(
    SecurityEventType::VRAM_SECURE_CLEAR,
    "system",
    "gpu_memory",
    {{"size_bytes", 1048576}, {"passes", 3}, {"gpu_id", 0}}
);
```

---

### NEW: Binary Integrity Verification (P2 - MEDIUM) ✅

**RSA-4096 Manifest Signing für Supply Chain Security**

ThemisDB implementiert jetzt Binary Integrity Verification mit RSA-4096 Signaturen, um die Authentizität und Integrität von Release-Binaries sicherzustellen.

**Features:**
- ✅ RSA-4096 digitale Signaturen
- ✅ SHA-256 File Hashing
- ✅ Manifest-Generierung aus Build-Artefakten
- ✅ Startup-Verifikation (automatisch)
- ✅ Update-Verifikation
- ✅ Audit Logging

**Verwendung:**
```bash
# Manifest generieren
themisctl manifest generate \
  --root /opt/themis/bin \
  --version 1.4.0 \
  --build-id $(git rev-parse HEAD) \
  --include "*.exe" "*.so" "*.dll" \
  --output manifest.json

# Manifest signieren
themisctl manifest sign \
  --input manifest.json \
  --key-id release_key \
  --output signed_manifest.json

# Verifikation
themisctl manifest verify \
  --manifest signed_manifest.json \
  --binaries /opt/themis/bin
```

**Konfiguration:**
```yaml
# config/binary_verification.yaml
binary_verification:
  enabled: true
  manifest_path: /etc/themis/signed_manifest.json
  binaries_root: /opt/themis/bin
  verify_on_startup: true
  fail_on_invalid: true
```

**Programmatische Verwendung:**
```cpp
#include "security/manifest_signer.h"

// Manifest generieren
ManifestSigner signer(signing_service, config);
BinaryManifest manifest = signer.generateManifest(
    "/opt/themis/bin", "1.4.0", "git_sha"
);

// Signieren
SignedManifest signed = signer.signManifest(manifest);
signed.saveToFile("/etc/themis/manifest.json");

// Startup-Verifikation
StartupVerifier verifier(signing_service, verifier_config);
bool valid = verifier.verify();  // Exit bei Fehler
```

**Tests:**
```bash
./build/tests/test_binary_integrity
```

**Compliance:**
- ✅ NIST SP 800-218 (Secure Software Development)
- ✅ SOC 2 CC7.1 (System Operations)
- ✅ Supply Chain Security

---

## ✅ Implementierter Security Stack (Stand: 2025-11-17)

### Core Security Features (ABGESCHLOSSEN)

#### 1. Rate Limiting & DoS Protection ✅
- **Token Bucket Algorithm**: 100 requests/minute default
- **Per-IP & Per-User Limits**: Flexible Konfiguration
- **HTTP 429 Responses**: Too Many Requests
- **Metrics Integration**: Prometheus-kompatible Metriken
- **Konfiguration**: `THEMIS_RATE_LIMIT_*` Environment Variables
- **Dokumentation**: Rate Limiter inline-dokumentiert

#### 2. TLS/SSL Hardening ✅
- **TLS 1.3 Default**: TLS 1.2 fallback konfigurierbar
- **Strong Cipher Suites**: ECDHE-RSA-AES256-GCM-SHA384, ChaCha20-Poly1305
- **mTLS Support**: Client-Zertifikatsverifikation
- **HSTS Headers**: `max-age=31536000; includeSubDomains`
- **SslSession Class**: Vollständige SSL-Stream-Implementierung
- **Dokumentation**: `docs/TLS_SETUP.md` (400+ Zeilen)

#### 3. Certificate Pinning (HSM/TSA) ✅
- **SHA256 Fingerprint Verification**: Whitelist-basiertes Pinning
- **CURL SSL Context Callbacks**: Custom SSL-Verifikation
- **Leaf vs. Chain Pinning**: Flexibles Pinning-Modell
- **Multiple Fingerprints**: Redundanz für Rotation
- **Integration**: PKI Client signHash/verifyHash
- **Dokumentation**: `docs/CERTIFICATE_PINNING.md` (700+ Zeilen)

#### 4. Input Validation & Sanitization ✅
- **JSON Schema Validation**: Strukturvalidierung
- **AQL Injection Prevention**: Whitelist-basiertes Parsing
- **Path Traversal Protection**: Normalisierung + Whitelist
- **Max Body Size**: 10MB default, konfigurierbar
- **Content-Type Validation**: Strict MIME-Type Checks
- **InputValidator Class**: Zentrale Validierungslogik

#### 5. Security Headers & CORS ✅
- **X-Frame-Options**: DENY
- **X-Content-Type-Options**: nosniff
- **X-XSS-Protection**: 1; mode=block
- **Content-Security-Policy**: Strict CSP
- **Strict-Transport-Security**: HSTS mit includeSubDomains
- **CORS Whitelisting**: Origin-basierte Zugriffskontrolle
- **Preflight Support**: OPTIONS-Requests

#### 6. Secrets Management ✅
- **HashiCorp Vault Integration**: KV v2 API
- **AppRole Authentication**: Production-ready
- **Token Renewal**: Automatische Erneuerung
- **Secret Rotation**: Callback-basierte Updates
- **Environment Fallback**: Graceful Degradation
- **Dokumentation**: `docs/SECRETS_MANAGEMENT.md` (500+ Zeilen)

#### 7. Audit Logging Enhancement ✅
- **65 Security Event Types**: Granulare Event-Klassifizierung
- **Hash Chain**: Merkle-ähnliche Manipulationserkennung
- **SIEM Integration**: Syslog RFC 5424 + Splunk HEC
- **Severity Levels**: HIGH/MEDIUM/LOW
- **Integrity Verification**: verifyChainIntegrity()
- **Dokumentation**: `docs/AUDIT_LOGGING.md` (900+ Zeilen)

#### 8. RBAC Implementation ✅
- **Role Hierarchy**: admin → operator → analyst → readonly
- **Permission System**: resource:action (data:read, keys:rotate)
- **Wildcard Support**: `*:*` für Superuser
- **Role Inheritance**: Automatische Permission-Propagierung
- **JSON/YAML Config**: Flexible Rollendefinitionen
- **User-Role Store**: Persistent Storage
- **Dokumentation**: `docs/RBAC.md` (800+ Zeilen)

## Server-Härtung

### Systemebene

#### TLS/SSL Konfiguration

```bash
# TLS 1.3 mit starken Ciphers
export THEMIS_TLS_ENABLED=true
export THEMIS_TLS_CERT=/etc/themis/certs/server.crt
export THEMIS_TLS_KEY=/etc/themis/certs/server.key
export THEMIS_TLS_MIN_VERSION=TLS1_3
export THEMIS_TLS_CIPHERS="ECDHE-RSA-AES256-GCM-SHA384:ECDHE-RSA-CHACHA20-POLY1305"

# mTLS (Client-Zertifikate)
export THEMIS_TLS_REQUIRE_CLIENT_CERT=true
export THEMIS_TLS_CA_CERT=/etc/themis/certs/ca.crt

# HSTS
export THEMIS_TLS_ENABLE_HSTS=true
```

**Zertifikatsgenerierung**: `scripts/generate_test_certs.sh`

#### Rate Limiting

```bash
# Token Bucket Configuration
export THEMIS_RATE_LIMIT_ENABLED=true
export THEMIS_RATE_LIMIT_MAX_TOKENS=100
export THEMIS_RATE_LIMIT_REFILL_RATE=10
export THEMIS_RATE_LIMIT_PER_USER=true
```

#### Secrets Management

```bash
# HashiCorp Vault
export THEMIS_VAULT_ADDR=https://vault.example.com:8200
export THEMIS_VAULT_ROLE_ID=<role-id>
export THEMIS_VAULT_SECRET_ID=<secret-id>
export THEMIS_VAULT_MOUNT=themis
export THEMIS_VAULT_TOKEN_RENEWAL_MARGIN=300

# Fallback (Development)
export THEMIS_SECRET_TOKENS_ADMIN=<admin-token>
```

### Netzwerkebene

#### Reverse Proxy (Nginx)

```nginx
# /etc/nginx/sites-available/themis
upstream themis {
    server 127.0.0.1:8765;
    keepalive 32;
}

server {
    listen 443 ssl http2;
    server_name themis.example.com;
    
    # TLS
    ssl_certificate /etc/letsencrypt/live/themis.example.com/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/themis.example.com/privkey.pem;
    ssl_protocols TLSv1.3;
    ssl_ciphers 'ECDHE-RSA-AES256-GCM-SHA384:ECDHE-RSA-CHACHA20-POLY1305';
    ssl_prefer_server_ciphers on;
    
    # Security Headers (zusätzlich zu Themis)
    add_header Strict-Transport-Security "max-age=31536000; includeSubDomains; preload" always;
    add_header X-Frame-Options DENY always;
    add_header X-Content-Type-Options nosniff always;
    
    # Rate Limiting (zusätzliche Layer)
    limit_req_zone $binary_remote_addr zone=themis_limit:10m rate=50r/s;
    limit_req zone=themis_limit burst=100 nodelay;
    
    location / {
        proxy_pass http://themis;
        proxy_http_version 1.1;
        proxy_set_header Connection "";
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }
}
```

#### Firewall (iptables)

```bash
# Nur HTTPS (443) und optionales Monitoring (9090)
sudo iptables -A INPUT -p tcp --dport 443 -j ACCEPT
sudo iptables -A INPUT -p tcp --dport 9090 -s 10.0.0.0/8 -j ACCEPT
sudo iptables -A INPUT -j DROP
```

### Betriebssystem

#### Service User (Least Privilege)

```bash
# Dedizierter User ohne Shell
sudo useradd -r -s /usr/sbin/nologin themis

# Verzeichnisberechtigungen
sudo chown -R themis:themis /var/lib/themis
sudo chmod 750 /var/lib/themis

# Secrets Read-Only
sudo chown root:themis /etc/themis/secrets
sudo chmod 640 /etc/themis/secrets/*
```

#### Systemd Service

```ini
# /etc/systemd/system/themis.service
[Unit]
Description=Themis Encrypted Database
After=network.target

[Service]
Type=simple
User=themis
Group=themis
WorkingDirectory=/opt/themis
ExecStart=/opt/themis/bin/themis_server
Restart=on-failure
RestartSec=10

# Security Hardening
PrivateTmp=yes
NoNewPrivileges=yes
ProtectSystem=strict
ProtectHome=yes
ReadWritePaths=/var/lib/themis /var/log/themis

# Resource Limits
LimitNOFILE=65536
LimitNPROC=4096

[Install]
WantedBy=multi-user.target
```

### Logging & Monitoring

#### Centralized Logging (Syslog)

```bash
# Audit Logs → Syslog → SIEM
export THEMIS_AUDIT_ENABLE_SIEM=true
export THEMIS_AUDIT_SIEM_TYPE=syslog
export THEMIS_AUDIT_SIEM_HOST=siem.example.com
export THEMIS_AUDIT_SIEM_PORT=514
```

#### Log Rotation

```conf
# /etc/logrotate.d/themis
/var/log/themis/*.log /var/log/themis/*.jsonl {
    daily
    rotate 365
    compress
    delaycompress
    missingok
    notifempty
    create 0640 themis themis
    sharedscripts
    postrotate
        systemctl reload themis 2>/dev/null || true
    endscript
}
```

## Admin-Tools (WPF)

### Code Signing

```powershell
# Signiere EXEs und DLLs
signtool sign /f "cert.pfx" /p "password" /t http://timestamp.digicert.com `
    ThemisAdmin.exe ThemisAdmin.dll
```

### Netzwerk-Hardening

```csharp
// Nur HTTPS, TLS 1.2+
ServicePointManager.SecurityProtocol = SecurityProtocolType.Tls12 | SecurityProtocolType.Tls13;
ServicePointManager.ServerCertificateValidationCallback = ValidateServerCertificate;

// Certificate Pinning
bool ValidateServerCertificate(object sender, X509Certificate cert, 
    X509Chain chain, SslPolicyErrors errors) {
    var thumbprint = cert.GetCertHashString();
    var allowedThumbprints = new[] { "A1B2C3...", "D4E5F6..." };
    return allowedThumbprints.Contains(thumbprint);
}
```

### Secrets (Windows Credential Locker)

```csharp
var vault = new PasswordVault();
vault.Add(new PasswordCredential("Themis", "admin", apiToken));

// Abrufen
var creds = vault.FindAllByResource("Themis").First();
var token = creds.RetrievePassword();
```

## Secrets-Management Best Practices

### Vault-Integration (Production)

```bash
# 1. Vault initialisieren
vault kv put themis/tokens/admin value="<admin-token>"
vault kv put themis/keys/master value="<master-key-base64>"

# 2. AppRole für Themis
vault write auth/approle/role/themis \
    secret_id_ttl=24h \
    token_ttl=1h \
    token_max_ttl=24h \
    policies=themis-policy

# 3. Role ID & Secret ID abrufen
vault read auth/approle/role/themis/role-id
vault write -f auth/approle/role/themis/secret-id
```

### Secret Rotation

```bash
# Monatliche Rotation (Cron)
0 0 1 * * /usr/local/bin/themis-rotate-secrets.sh

# themis-rotate-secrets.sh
#!/bin/bash
NEW_TOKEN=$(openssl rand -hex 32)
vault kv put themis/tokens/admin value="$NEW_TOKEN"
systemctl reload themis  # Secrets Manager refresht automatisch
```

## Compliance

### DSGVO/GDPR

#### Recht auf Löschung

```bash
# Vollständige User-Datenlöschung
curl -X DELETE https://themis.example.com/api/users/alice@example.com \
    -H "Authorization: Bearer $ADMIN_TOKEN"

# Audit Log
# Event: PII_ERASED, user_id: alice@example.com
```

#### Recht auf Auskunft

```bash
# Export aller User-Daten
curl https://themis.example.com/api/users/alice@example.com/export \
    -H "Authorization: Bearer $ANALYST_TOKEN" \
    -o user_data.json
```

#### Pseudonymisierung

```cpp
// PII Pseudonymizer aktiviert
auto pseudonymized = pii_pseudonymizer.pseudonymize("alice@example.com");
// => "user_a1b2c3d4..."
```

### SOC 2

#### Zugriffskontrolle (CC6.1)

- ✅ RBAC mit Role Hierarchy
- ✅ Least Privilege Principle
- ✅ MFA-Support (via JWT/Keycloak)
- ✅ Audit Logging aller Zugriffe

#### Änderungsmanagement (CC8.1)

- ✅ Code Signing
- ✅ Reproducible Builds (vcpkg baseline)
- ✅ Audit Trail für Konfigurationsänderungen

### HIPAA

#### PHI-Schutz

```cpp
// Encryption-at-Rest mit AES-256-GCM
field_encryption.encrypt(phi_data, encryption_context);

// Audit für PHI-Zugriffe
audit_logger.logSecurityEvent(
    SecurityEventType::PII_ACCESSED,
    user_id,
    "patients/12345/diagnosis",
    {{"reason", "treatment_plan"}}
);
```

## Checklisten & Gates

### Pre-Release Security Checklist

- [ ] **TLS/SSL**: TLS 1.3, starke Ciphers, mTLS konfiguriert
- [ ] **Secrets**: Keine Hardcoded Secrets, Vault-Integration aktiv
- [ ] **Rate Limiting**: Enabled, Limits getestet
- [ ] **Input Validation**: Alle API-Endpoints validiert
- [ ] **RBAC**: Rollen definiert, User-Mappings aktuell
- [ ] **Audit Logging**: Hash Chain aktiviert, SIEM-Integration getestet
- [ ] **Certificate Pinning**: Fingerprints aktuell
- [ ] **Dependencies**: Snyk-Scan ohne kritische CVEs
- [ ] **Code Signing**: Alle Artefakte signiert
- [ ] **Dokumentation**: Security-Docs aktualisiert

### Vulnerability Scanning

```bash
# Snyk Security Scan
snyk test --all-projects --severity-threshold=high

# Container Scanning (falls Docker)
docker scan themisdb:latest

# Dependency Check
dependency-check --project ThemisDB --scan . --format HTML
```

### Penetration Testing

```bash
# OWASP ZAP Baseline Scan
docker run -t owasp/zap2docker-stable zap-baseline.py \
    -t https://themis.example.com \
    -r zap_report.html

# SQLMap (AQL Injection Test)
sqlmap -u "https://themis.example.com/api/query" \
    --data='{"aql":"FOR u IN users RETURN u"}' \
    --headers="Authorization: Bearer $TOKEN"
```

## Build-Zeit Security

### AddressSanitizer (ASAN)

```cmake
# CMakeLists.txt
option(THEMIS_ENABLE_ASAN "Enable AddressSanitizer" OFF)

if(THEMIS_ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()
```

```bash
# Build mit ASAN
cmake -B build -DTHEMIS_ENABLE_ASAN=ON
cmake --build build
./build/tests/themis_tests  # Memory-Fehler werden erkannt
```

### Reproducible Builds

```bash
# vcpkg Baseline fixieren
git add vcpkg.json vcpkg-configuration.json
git commit -m "Lock vcpkg baseline for reproducible builds"

# Build-Hash verifizieren
sha256sum build/themis_server > themis_server.sha256
```

## Incident Response

### Security Event Response

```cpp
// HIGH-Severity Events → Immediate Action
if (event_type == SecurityEventType::BRUTE_FORCE_DETECTED) {
    // 1. IP blocken
    rate_limiter.blockIP(remote_ip, std::chrono::hours(24));
    
    // 2. Alert Operations
    alert_ops("Brute force attack from " + remote_ip);
    
    // 3. Audit Log
    audit_logger.logSecurityEvent(
        SecurityEventType::SUSPICIOUS_ACTIVITY,
        "system",
        remote_ip,
        {{"action", "ip_blocked"}, {"duration_hours", 24}}
    );
}
```

### Audit Log Tampering

```bash
# Hash Chain Verification (täglich via Cron)
0 3 * * * /usr/local/bin/themis-verify-audit-chain

# themis-verify-audit-chain
#!/bin/bash
if ! themis-cli audit verify-chain; then
    echo "CRITICAL: Audit log tampering detected!" | mail -s "Security Alert" ops@example.com
    exit 1
fi
```

## Performance vs. Security Trade-offs

| Feature | Performance Impact | Security Gain | Empfehlung |
|---------|-------------------|---------------|------------|
| TLS 1.3 | ~5% CPU | ⭐⭐⭐⭐⭐ | **ENABLE** |
| mTLS | ~10% CPU | ⭐⭐⭐⭐ | Enable (Prod) |
| Rate Limiting | <1% CPU | ⭐⭐⭐⭐ | **ENABLE** |
| Input Validation | ~2% Latenz | ⭐⭐⭐⭐⭐ | **ENABLE** |
| Hash Chain | ~0.5ms/entry | ⭐⭐⭐⭐ | **ENABLE** |
| SIEM Forwarding | ~2ms/event | ⭐⭐⭐ | Enable (Prod) |
| Certificate Pinning | <1ms | ⭐⭐⭐⭐ | **ENABLE** |
| RBAC | <1ms | ⭐⭐⭐⭐⭐ | **ENABLE** |

**Empfehlung**: Alle Features in Production aktivieren. Performance-Impact ist vernachlässigbar.

## Ressourcen & Referenzen

### Interne Dokumentation
- [TLS Setup Guide](TLS_SETUP.md)
- [Secrets Management](SECRETS_MANAGEMENT.md)
- [Audit Logging](AUDIT_LOGGING.md)
- [RBAC Configuration](RBAC.md)
- [Certificate Pinning](CERTIFICATE_PINNING.md)

### Standards & Frameworks
- [OWASP Top 10 (2021)](https://owasp.org/Top10/)
- [CIS Benchmarks](https://www.cisecurity.org/cis-benchmarks/)
- [NIST Cybersecurity Framework](https://www.nist.gov/cyberframework)
- [RFC 8446 - TLS 1.3](https://tools.ietf.org/html/rfc8446)
- [RFC 7469 - Certificate Pinning](https://tools.ietf.org/html/rfc7469)

### Tools
- [Snyk](https://snyk.io/) - Dependency Scanning
- [OWASP ZAP](https://www.zaproxy.org/) - Penetration Testing
- [HashiCorp Vault](https://www.vaultproject.io/) - Secrets Management
- [Splunk](https://www.splunk.com/) - SIEM

---

**Letzte Aktualisierung**: 2025-11-17  
**Security-Status**: ✅ Production-Ready (85% Coverage)

## Server-Härtung
- Reverse Proxy vor Themis (TLS, Rate Limiting, Auth): Nginx/Traefik empfohlen
- TLS: TLS 1.2+, HSTS, sichere Cipher Suites, OCSP Stapling
- Accounts: Least-Privilege Service User, kein Admin-Kontext
- Firewall: Nur benötigte Ports (8765) freigeben, IP-Restriktionen erwägen
- Logging: Security-Events zentralisieren; Log Rotation, WORM/ELK/Graylog
- Ressourcen: Request-Timeouts, Body-Size-Limits, Parallelitätsgrenzen
- Build: Reproducible, vcpkg Baseline fixiert; ASAN/UBSAN im Testlauf

## Admin-Tools (WPF)
- Code Signing der EXEs und Installer (MSIX/WiX)
- Updates: Signierte Updates; Hash-Validierung bei Verteilung
- Netzwerk: Nur HTTPS-Endpoints verwenden; Zertifikatsvalidierung aktiv
- Konfiguration: Keine Secrets in Klartextdateien; Windows Credential Locker/DPAPI
- Telemetrie/Logs: Keine PII im Klartext; Minimalprinzip

## Secrets-Management
- Keinerlei Secrets im Repo halten; .gitignore beachten
- Nutzung von Secret Stores (Windows, Azure Key Vault, HashiCorp Vault)
- Rotationspläne definieren (LEK/KEK/DEK + App-Secrets)

## Compliance-Aspekte
- DSGVO: Recht auf Löschung, Auskunft, Pseudonymisierung
- Auditierbarkeit: Export-/Löschaktionen protokollieren
- Aufbewahrung: Retention-Policies technisch durchsetzen

## Checklisten & Gates
- Vor Release: `docs/security_audit_checklist.md` durchgehen
- Vulnerability-Scans ohne kritische Funde
- Signierte, versionierte Artefakte im `dist/`-Pfad

### Security Hardening - 10 Critical Tasks 🔒

Ich habe die Security Hardening Tasks basierend auf der Analyse identifiziert. Hier ist die Übersicht:

Priorität: HOCH (Vor Production Deployment)

Top 3 kritische Tasks:

## Rate Limiting & DoS Protection (8-10h)

Token bucket algorithm für API rate limiting
Per-IP und per-User Limits (100 req/min default)
HTTP 429 responses
TLS/SSL Hardening (6-8h)

TLS 1.3 enforcement
Disable weak ciphers (RC4, 3DES)
HSTS headers + certificate pinning
mTLS für Production
Input Validation & Sanitization (10-12h)

JSON schema validation
SQL injection prevention in AQL
Path traversal protection
Max body size limits
Mittlere Priorität:

Authentication & Authorization (8-10h)

JWT expiration (15min)
Refresh tokens + revocation
MFA support
Brute force protection
Secrets Management (6-8h)

Remove hardcoded secrets
Vault/AWS Secrets Manager integration
Secret rotation automation
Compliance & Monitoring:

Audit Logging Enhancement (6-8h)

Security event logging
Tamper-proof logs (hash chain)
SIEM integration
Security Headers & CORS (4-6h)

X-Frame-Options, CSP, etc.
Strict CORS whitelisting
Code Security:

Memory Safety (8-10h)

AddressSanitizer integration
Smart pointer migration
Buffer overflow fixes
Dependency Security (4-6h)

Snyk/Dependabot integration
CVE monitoring automation
Error Handling (4-6h)

Sanitize error responses
Remove stack traces in production
Gesamtaufwand: ~70-90 Stunden (2-3 Wochen)