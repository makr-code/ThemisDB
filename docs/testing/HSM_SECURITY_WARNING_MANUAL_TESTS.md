# HSM Security Warning - Manual Testing Guide

This guide provides manual testing procedures to verify the HSM security warning implementation (FIND-002).

## Prerequisites

- ThemisDB server built from source
- Access to terminal/command line
- Ability to set environment variables

## Test Cases

### Test 1: Startup Warning Banner (Development Mode)

**Objective**: Verify warning banner displays at startup with stub HSM

**Steps**:
1. Ensure `THEMIS_PRODUCTION_MODE` is NOT set:
   ```bash
   unset THEMIS_PRODUCTION_MODE
   unset THEMIS_ENVIRONMENT
   ```

2. Start server with default configuration:
   ```bash
   ./build/themis_server --config config/config.yaml
   ```

**Expected Result**:
- Server starts successfully
- Warning banner displays in logs:
  ```
  ╔════════════════════════════════════════════════════════════════════════════╗
  ║  ⚠️  WARNING: INSECURE HSM CONFIGURATION DETECTED                          ║
  ║                                                                            ║
  ║  The HSM provider is set to 'stub' which is DEVELOPMENT ONLY.              ║
  ║  Master encryption keys are NOT protected by hardware security.            ║
  ║                                                                            ║
  ║  FOR PRODUCTION USE:                                                       ║
  ║  - Configure a real HSM provider (PKCS#11, AWS KMS, Azure Key Vault)       ║
  ║  - See: docs/security/HSM_PRODUCTION_SETUP.md                              ║
  ║                                                                            ║
  ║  To suppress this warning in development, use: --allow-stub-hsm            ║
  ╚════════════════════════════════════════════════════════════════════════════╝
  ```
- Log shows: `HSM Provider Status:` with `Provider Type: stub (DEVELOPMENT ONLY)`

---

### Test 2: Warning Suppression with Flag

**Objective**: Verify --allow-stub-hsm flag suppresses warnings

**Steps**:
1. Start server with flag:
   ```bash
   ./build/themis_server --config config/config.yaml --allow-stub-hsm
   ```

**Expected Result**:
- Server starts successfully
- Warning banner does NOT display
- Log shows: `HSM stub provider allowed by --allow-stub-hsm flag (DEVELOPMENT ONLY)`

---

### Test 3: Production Mode Blocking (No Flag)

**Objective**: Verify server refuses to start in production mode with stub HSM

**Steps**:
1. Set production mode:
   ```bash
   export THEMIS_PRODUCTION_MODE=true
   ```

2. Start server WITHOUT flag:
   ```bash
   ./build/themis_server --config config/config.yaml
   ```

**Expected Result**:
- Server FAILS to start (exits with code 1)
- CRITICAL error banner displays:
  ```
  ╔═══════════════════════════════════════════════════════════════╗
  ║  🛑  CRITICAL SECURITY FAILURE  🛑                            ║
  ╠═══════════════════════════════════════════════════════════════╣
  ║  HSM stub provider is NOT ALLOWED in production mode!        ║
  ║                                                               ║
  ║  COMPLIANCE VIOLATIONS:                                      ║
  ║  ❌ NIST SP 800-53 SC-12 (Key Management)                    ║
  ║  ❌ ISO 27001 A.8.24 (Cryptography)                          ║
  ║  ❌ PCI DSS Requirement 3.6 (Key Protection)                 ║
  ║  ❌ GDPR Article 32 (Security of Processing)                 ║
  ...
  ```
- Log shows: `Server startup aborted due to HSM security violation`

---

### Test 4: Production Mode with Override Flag

**Objective**: Verify server starts in production with flag (not recommended)

**Steps**:
1. Ensure production mode is set:
   ```bash
   export THEMIS_PRODUCTION_MODE=true
   ```

2. Start server WITH flag:
   ```bash
   ./build/themis_server --config config/config.yaml --allow-stub-hsm
   ```

**Expected Result**:
- Server starts successfully
- WARNING banner displays (not CRITICAL):
  ```
  ╔═══════════════════════════════════════════════════════════════╗
  ║  ⚠️  PRODUCTION SAFETY OVERRIDE ACTIVE  ⚠️                    ║
  ╠═══════════════════════════════════════════════════════════════╣
  ║  --allow-stub-hsm flag used in PRODUCTION mode!              ║
  ║  HSM security is DISABLED.                                   ║
  ║  This violates security compliance requirements.             ║
  ...
  ```

---

### Test 5: Periodic Warning Logging

**Objective**: Verify ERROR-level warnings log every 5 minutes

**Steps**:
1. Start server in development mode without flag:
   ```bash
   unset THEMIS_PRODUCTION_MODE
   ./build/themis_server --config config/config.yaml
   ```

2. Wait at least 5 minutes (300 seconds)

3. Check server logs

**Expected Result**:
- Every 5 minutes, ERROR-level log appears:
  ```
  [ERROR] [SECURITY] ⚠️  PRODUCTION SECURITY ALERT: HSM stub provider active! ...
  ```
- Log message contains: `Master keys are NOT hardware-protected`
- Log references: `docs/security/HSM_PRODUCTION_SETUP.md`

---

### Test 6: Prometheus Metrics

**Objective**: Verify HSM metrics are exposed at /metrics endpoint

**Steps**:
1. Start server:
   ```bash
   ./build/themis_server --config config/config.yaml
   ```

2. Query metrics endpoint:
   ```bash
   curl http://localhost:8080/metrics
   ```

**Expected Result**:
Metrics output includes:
```
# HELP themis_hsm_insecure_config Indicates if HSM is in insecure configuration
# TYPE themis_hsm_insecure_config gauge
themis_hsm_insecure_config 1

# HELP themis_hsm_provider_type HSM provider type information
# TYPE themis_hsm_provider_type gauge
themis_hsm_provider_type{provider="stub"} 1

# HELP hsm_security_stub_active Indicates if HSM stub provider is active
# TYPE hsm_security_stub_active gauge
hsm_security_stub_active 1

# HELP hsm_compliance_status HSM compliance status
# TYPE hsm_compliance_status gauge
hsm_compliance_status{standard="nist_sp_800_53_sc_12"} 0
hsm_compliance_status{standard="iso_27001_a_8_24"} 0
hsm_compliance_status{standard="pci_dss_3_6"} 0
hsm_compliance_status{standard="gdpr_art_32"} 0
```

---

### Test 7: Help Output

**Objective**: Verify --allow-stub-hsm appears in help

**Steps**:
```bash
./build/themis_server --help
```

**Expected Result**:
Help output includes:
```
  --allow-stub-hsm  Allow insecure stub HSM provider (development only)
```

---

## Test Matrix

| Test Case | Environment | Flag | Expected Startup | Expected Warnings |
|-----------|-------------|------|------------------|-------------------|
| 1 | Development | None | ✅ Success | ⚠️ Banner + Periodic |
| 2 | Development | --allow-stub-hsm | ✅ Success | ❌ None |
| 3 | Production | None | ❌ Blocked | 🛑 Critical Error |
| 4 | Production | --allow-stub-hsm | ✅ Success | ⚠️ Override Warning |

---

## Verification Checklist

- [ ] Test 1: Warning banner displays in development mode
- [ ] Test 2: Flag suppresses warnings
- [ ] Test 3: Production mode blocks startup without flag
- [ ] Test 4: Production mode allows with flag (logs warning)
- [ ] Test 5: Periodic warnings appear every 5 minutes
- [ ] Test 6: Prometheus metrics exposed correctly
- [ ] Test 7: Help text includes --allow-stub-hsm flag

---

## Cleanup

After testing, reset environment:
```bash
unset THEMIS_PRODUCTION_MODE
unset THEMIS_ENVIRONMENT
```

---

## Notes

- **Production Testing**: Do NOT test production mode blocking on actual production systems
- **Real HSM**: For testing with real HSM, configure `config/security.yaml` with proper PKCS#11 settings
- **Log Files**: Check server log files for complete output if terminal doesn't show all messages
- **Metrics Port**: Default metrics port is 8080, adjust curl commands if configured differently

---

## Troubleshooting

**Issue**: Warning banner doesn't display
- **Solution**: Check that HSM provider is actually set to 'stub' in config

**Issue**: Periodic warnings don't appear
- **Solution**: Ensure warning thread started successfully, check logs for "HSM security warning thread started"

**Issue**: Metrics endpoint returns 404
- **Solution**: Verify server started successfully and metrics endpoint is enabled

**Issue**: Server won't start even with --allow-stub-hsm
- **Solution**: Check for other configuration errors in logs
