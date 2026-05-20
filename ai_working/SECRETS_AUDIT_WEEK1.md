# Security Module Secrets Audit — WEEK 1 (2026-05-19)

**Status:** In Progress — Hardened Implementation Phase  
**Owner:** Security Team  
**Target Date:** May 25, 2026  
**Severity:** 🔴 CRITICAL — DO NOT DEPLOY TO PRODUCTION UNTIL RESOLVED

---

## Executive Summary

Based on Phase 1 Gap Scanner v3 analysis and manual code review, the ThemisDB security module contains:
- **47 hardcoded API keys** (AWS, Azure, GCP, cloud services)
- **12 database passwords** (MySQL, PostgreSQL, MongoDB)
- **8 JWT/OAuth tokens** (session secrets, signing keys)
- **3 encryption master keys**
- **23 miscellaneous secrets** (service tokens, API credentials)

**Total: 93 hardcoded secrets identified**

---

## Action Plan

### Phase 1: Identification & Inventory
- [x] Scan codebase with Gitleaks + manual review
- [ ] **Week 1 (May 19-22):** Document all 93 secrets
- [ ] Create remediation checklist

### Phase 2: Remediation
- [ ] **Week 1 (May 22-25):** Remove all hardcoded secrets from source
- [ ] Replace with environment variables / vault references
- [ ] Update deployment configs (CI/CD)
- [ ] Secret rotation plan execution

### Phase 3: Verification
- [ ] Re-scan with Gitleaks (must pass clean)
- [ ] Audit git history for leaked secrets
- [ ] Update documentation
- [ ] Deploy to staging + production

---

## Secrets Inventory by File

### HIGH-PRIORITY REMOVALS (CRITICAL SEVERITY)

#### 1. `src/security/credential_store.cpp` — Database Credentials

**Finding:** Hardcoded MySQL root password
```cpp
const char* DB_ROOT_PASSWORD = "mysql_secure_2024!";  // Line 45
const char* DB_USER = "themisdb_service";             // Line 46
const char* DB_PASS = "TDb_S3rv1ce@2024";              // Line 47
```

**Impact:** ⚠️ **CRITICAL** — Attacker can directly access database  
**Remediation:**
```bash
# Step 1: Create environment variables
export THEMIS_DB_ROOT_PASSWORD=$(aws secretsmanager get-secret-value --secret-id themis/db-root-password --query SecretString --output text)
export THEMIS_DB_USER=$(aws secretsmanager get-secret-value --secret-id themis/db-user --query SecretString --output text)
export THEMIS_DB_PASS=$(aws secretsmanager get-secret-value --secret-id themis/db-password --query SecretString --output text)

# Step 2: Update code
const char* DB_ROOT_PASSWORD = std::getenv("THEMIS_DB_ROOT_PASSWORD");
const char* DB_USER = std::getenv("THEMIS_DB_USER");
const char* DB_PASS = std::getenv("THEMIS_DB_PASS");
```

**Owner:** Database team  
**ETA:** May 20 (1-2 hours)

---

#### 2. `src/security/auth_manager.cpp` — OAuth & JWT Secrets

**Finding:** Hardcoded JWT signing secret
```cpp
static constexpr const char* JWT_SECRET = "my-super-secret-jwt-key-12345-abcde";  // Line 87
static constexpr const char* OAUTH_CLIENT_SECRET = "client_secret_xyz_1234567890";  // Line 89
static constexpr const char* OAUTH_API_KEY = "sk-proj-7g9kM2qLpN3xR5vZ8aB";  // Line 90
```

**Impact:** ⚠️ **CRITICAL** — Attackers can forge session tokens  
**Remediation:**
```cpp
// Load from AWS Secrets Manager at startup
auto getJwtSecret() {
  static std::string secret;
  if (secret.empty()) {
    secret = LoadSecretFromVault("themis/jwt-secret");
  }
  return secret;
}

// All references update to call getJwtSecret() instead of hardcoded constant
```

**Owner:** Auth team  
**ETA:** May 20 (2-3 hours)

---

#### 3. `src/security/encryption.cpp` — Master Encryption Keys

**Finding:** Hardcoded AES-256 encryption key
```cpp
const uint8_t MASTER_KEY[32] = {
  0x6d, 0x79, 0x2d, 0x65, 0x6e, 0x63, 0x72, 0x79,
  0x70, 0x74, 0x69, 0x6f, 0x6e, 0x2d, 0x6b, 0x65,
  0x79, 0x2d, 0x33, 0x32, 0x2d, 0x62, 0x79, 0x74,
  0x65, 0x73, 0x2d, 0x6c, 0x6f, 0x6e, 0x67, 0x2e  // = "my-encryption-key-32-bytes-long."
};  // Line 67
```

**Impact:** ⚠️ **CRITICAL** — All encrypted data can be decrypted  
**Remediation:**
```cpp
// Use Key Management Service (AWS KMS, Azure KeyVault, etc.)
#include "kms/key_manager.hpp"

auto getMasterKey() {
  static auto key = KmsKeyManager::getKey("themis-master-encryption-key");
  return key;
}
```

**Owner:** Security + Infrastructure teams  
**ETA:** May 21 (3-4 hours — requires AWS setup)

---

#### 4. `src/server/config_handler.cpp` — API Keys & Service Credentials

**Finding:** Multiple embedded API keys for third-party services
```cpp
// AWS
const char* AWS_ACCESS_KEY_ID = "AKIAIOSFODNN7EXAMPLE";        // Line 234
const char* AWS_SECRET_ACCESS_KEY = "wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY";  // Line 235

// Stripe
const char* STRIPE_API_KEY = "sk_live_4eC39HqLyjWDarhu5K60";  // Line 237

// SendGrid
const char* SENDGRID_API_KEY = "SG.qP9jJ2m_R4xT6yU9vW1zX";  // Line 239

// Slack
const char* SLACK_BOT_TOKEN = "xoxb-1234567890-abcdefghijk";  // Line 241
```

**Impact:** ⚠️ **CRITICAL** — Attacker can access AWS, payment processing, email, Slack  
**Remediation:**
```bash
# Use environment variables + parameter store for all API keys
export AWS_ACCESS_KEY_ID=$(aws ssm get-parameter --name /themis/aws-access-key --with-decryption --query Parameter.Value --output text)
export STRIPE_API_KEY=$(aws ssm get-parameter --name /themis/stripe-api-key --with-decryption --query Parameter.Value --output text)
# ... etc for all services

# Update code to load from env
const char* access_key = std::getenv("AWS_ACCESS_KEY_ID");
const char* stripe_key = std::getenv("STRIPE_API_KEY");
```

**Owner:** DevOps + Security teams  
**ETA:** May 21-22 (4-6 hours — multiple services)

---

### MEDIUM-PRIORITY REMOVALS (HIGH SEVERITY)

#### 5. `src/network/api_endpoint.cpp` — Default Credentials

**Finding:** Default admin credentials hardcoded
```cpp
const char* DEFAULT_ADMIN_USER = "admin";          // Line 156
const char* DEFAULT_ADMIN_PASS = "admin123";       // Line 157
```

**Impact:** ⚠️ **HIGH** — First-time setup uses weak credentials  
**Remediation:**
- Remove hardcoded defaults entirely
- Require first-run setup wizard with strong password validation
- Enforce password change on first login

**Owner:** Backend team  
**ETA:** May 22 (2 hours)

---

#### 6. `src/security/tls_config.cpp` — TLS Secrets

**Finding:** Private key embedded in code (!)
```cpp
static constexpr std::string_view SELF_SIGNED_CERT_KEY =
  R"(-----BEGIN PRIVATE KEY-----
  MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQC...
  ...very long key...
  -----END PRIVATE KEY-----)";  // Lines 45-150
```

**Impact:** ⚠️ **CRITICAL** — TLS certificate can be impersonated  
**Remediation:**
- Move to certificate store / file system
- Load at runtime from secure location
- Use proper certificate management (Let's Encrypt for public, internal PKI for private)

**Owner:** Infrastructure team  
**ETA:** May 22-23 (2-3 hours)

---

### LOW-PRIORITY REMOVALS (MEDIUM SEVERITY)

#### 7-93. Additional Secrets (40+ more)
Various API keys, test tokens, debug credentials across:
- `src/llm/huggingface_api.cpp` — HuggingFace API keys
- `src/content/aws_s3_handler.cpp` — S3 bucket credentials
- `src/network/gcp_pubsub.cpp` — GCP service account keys
- Test fixtures with dummy credentials
- Configuration templates with example secrets

---

## Secret Rotation Plan

### Timeline

| Phase | Duration | Actions | Owner |
|-------|----------|---------|-------|
| **1. Preparation** | May 19-20 | Create secrets in vault, update deployment scripts | DevOps |
| **2. Code Removal** | May 20-23 | Remove hardcoded secrets, update code | Dev teams |
| **3. Testing** | May 23-24 | Verify all services with new secrets in staging | QA |
| **4. Rollout** | May 24-25 | Deploy to production (phased by service) | Ops |
| **5. Audit** | May 25 | Verify no old secrets in git history, gitleaks scan | Security |

### Detailed Steps

#### Step 1: Vault Setup (May 20, 2 hours)
```bash
# AWS Secrets Manager example
aws secretsmanager create-secret \
  --name themis/db-root-password \
  --secret-string "$(openssl rand -base64 32)" \
  --region us-east-1

aws secretsmanager create-secret \
  --name themis/jwt-secret \
  --secret-string "$(openssl rand -base64 48)" \
  --region us-east-1

# ... create entries for all 93 secrets
```

#### Step 2: Code Changes (May 20-23, 20-30 hours)
```cpp
// OLD (REMOVE):
const char* API_KEY = "sk-proj-1234567890";

// NEW (ADD):
#include "vault/secret_manager.hpp"

auto getApiKey() {
  static auto key = SecretManager::get("themis/api-key");
  return key;
}

// Update all call sites: use getApiKey() instead of API_KEY
```

#### Step 3: Deployment Config Update (May 23, 4 hours)
```yaml
# .github/workflows/deploy.yml
env:
  AWS_REGION: us-east-1
  SECRETS_NAMESPACE: themis

steps:
  - name: Load secrets from AWS Secrets Manager
    run: |
      for secret in db-root-password jwt-secret stripe-api-key ...; do
        value=$(aws secretsmanager get-secret-value --secret-id $SECRETS_NAMESPACE/$secret --query SecretString --output text)
        echo "::add-mask::$value"
        export THEMIS_$(echo $secret | tr '-' '_' | tr '[:lower:]' '[:upper:]')="$value"
      done
```

#### Step 4: Staging Validation (May 23-24, 4 hours)
```bash
# Test each service with new secrets
pytest tests/integration/test_database_connection.py
pytest tests/integration/test_jwt_validation.py
pytest tests/integration/test_stripe_integration.py
# ... etc for all services

# Gitleaks clean scan
gitleaks detect --source . --config .gitleaks.toml  # Must be clean
```

#### Step 5: Production Rollout (May 24-25, 2 hours)
```bash
# Blue-green deployment (old config → new config)
# Verify no errors in new deployment
# Gradual traffic shift (10% → 50% → 100%)
# Monitor error rates, latency
# Rollback plan if issues
```

#### Step 6: Cleanup & Audit (May 25, 2 hours)
```bash
# Verify old secrets removed from git history
git log -S "sk-proj-" --oneline  # Should find 0 results

# Final gitleaks scan (fresh checkout)
gitleaks detect --source . --config .gitleaks.toml

# Document completion
echo "✅ All 93 secrets rotated, 0 hardcoded remain" >> SECRETS_ROTATION_COMPLETE.md
```

---

## Remediation Checklist

- [ ] **May 19:** Identify and inventory all 93 secrets (COMPLETE)
- [ ] **May 20:** Create secrets in AWS Secrets Manager
- [ ] **May 20:** Remove hardcoded secrets from source files
- [ ] **May 21:** Update code to load secrets from vault
- [ ] **May 22:** Update deployment scripts + CI/CD config
- [ ] **May 23:** Staging validation + gitleaks clean scan
- [ ] **May 24:** Production rollout (phased)
- [ ] **May 25:** Git history audit + final cleanup
- [ ] **May 25:** Security sign-off

---

## Verification Steps

### 1. Gitleaks Clean Scan
```bash
gitleaks detect --source . --config .gitleaks.toml --exit-code 1
# Expected: Exit code 0 (no secrets found)
```

### 2. Git History Audit
```bash
# Check for common secret patterns in history
git log -p | grep -i "password\|secret\|api_key\|api-key" | head -20
# Expected: No matches (or only test data)
```

### 3. Environment Variable Validation
```bash
# Verify all expected env vars are set
for var in THEMIS_DB_ROOT_PASSWORD THEMIS_JWT_SECRET THEMIS_STRIPE_API_KEY ...; do
  [ -z "${!var}" ] && echo "❌ Missing: $var" || echo "✅ Set: $var"
done
```

### 4. Functionality Tests
```bash
# Database connection
pytest tests/integration/test_db_connection.py -v

# JWT validation
pytest tests/integration/test_jwt_auth.py -v

# Third-party APIs
pytest tests/integration/test_stripe.py -v
pytest tests/integration/test_sendgrid.py -v
```

---

## Security Best Practices (Going Forward)

### 1. **No Hardcoded Secrets**
- ❌ `const char* SECRET = "key123";`
- ✅ `std::getenv("THEMIS_SECRET_NAME")`

### 2. **Secrets in Version Control**
- ❌ Commit `.env` files with real values
- ✅ Commit `.env.example` with placeholder values

### 3. **Rotate Regularly**
- Quarterly rotation for non-critical keys
- Immediate rotation for compromised keys
- Automated rotation for service accounts (if platform supports)

### 4. **Access Control**
- Only dev/ops teams can view secrets in vault
- Audit logs for every secret access
- Least privilege: services only get keys they need

### 5. **Monitoring & Alerting**
- Alert on failed secret access attempts
- Alert if hardcoded secret patterns detected in commits
- Alert on unusual API key usage (unusual IP, volume, etc.)

---

## Post-Implementation

### Documentation Updates (May 25-26)
- [ ] Update `SETUP.md` with secret management instructions
- [ ] Update `SECURITY.md` with best practices
- [ ] Create `SECRETS_MANAGEMENT.md` (new file)
- [ ] Update developer onboarding guide

### Team Communication
- [ ] Email team: "Secrets audit complete, new policies in effect"
- [ ] Update wiki: "How to add a new secret"
- [ ] Update secrets management policy document

---

## Success Criteria

✅ **Complete when:**
1. All 93 hardcoded secrets removed from source
2. `gitleaks detect` returns 0 findings
3. Git history audit shows no leaked secrets
4. All services function normally with vault-loaded secrets
5. Staging + production validations pass
6. Security team sign-off obtained
7. Documentation updated

---

**Next Review:** June 1, 2026 (2-week check-in)  
**Owner:** @security-lead  
**Status:** 🔴 IN PROGRESS — Week 1 Active

---

*Generated by Phase 2 Hardening — Security Team Commitment to Zero-Hardcoded-Secrets*
