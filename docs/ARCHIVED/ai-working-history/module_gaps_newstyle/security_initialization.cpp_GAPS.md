# security_initialization.cpp Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: security_initialization.cpp
- Generated: 2026-06-03 20:28:49
- Status: High-Priority Findings Present
- Total Findings: 14
- Actionable Findings (Critical + High): 14
- Affected Files: 1

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 0 |
| High | 14 |
| Medium | 0 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| uncaught_exception | 14 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| security_initialization.cpp | 14 | 0 | 14 | 0 | 0 |

## Full Scanner Findings

### security_initialization.cpp
Total findings: 14

- Line 130: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // In production, reject mock/local providers

        if (production_mode && key_provider_type_.value() == KeyProviderType::LOCAL) {

            throw std::runtime_error(

                "Production mode violation: LOCAL (mock) key provider is not allowed in production. "

                "Use VAULT or HSM key provider instead. "

                "Set THEMIS_PRODUCTION_MODE=0 or THEMIS_ENVIRONMENT=development for testing."
- Line 139: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: } else {

        // No key provider configured

        if (production_mode) {

            throw std::runtime_error(

                "Production mode violation: No key provider configured. "

                "Call withKeyProvider() with VAULT or HSM configuration before build(). "

                "Mock/default key providers are not allowed in production."
- Line 153: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Cast to KeyProvider interface

    auto key_provider_concrete = std::dynamic_pointer_cast<KeyProvider>(key_provider_impl);

    if (!key_provider_concrete) {

        throw std::runtime_error("Key provider does not implement KeyProvider interface");

    }

    

    // Create field encryption
- Line 174: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Load RBAC policy if file is specified

    if (!rbac_policy_file_.empty()) {

        if (!layer.rbac->loadConfig(rbac_policy_file_)) {

            throw std::runtime_error("Failed to load RBAC policy from: " + rbac_policy_file_);

        }

    }
- Line 183: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Validate JWT configuration

        auto validation = core::ConfigValidator::validateJWTConfig(jwt_config_, production_mode);

        if (!validation.valid) {

            throw std::runtime_error("Invalid JWT configuration:\n" + validation.formatErrors());

        }

        

        layer.jwt = std::make_shared<auth::JWTValidator>(jwt_config_);
- Line 190: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: } else {

        // No JWT configured

        if (production_mode) {

            throw std::runtime_error(

                "Production mode violation: No JWT validation configured. "

                "Call withJWT() with proper configuration before build(). "

                "JWT validation is required in production mode."
- Line 212: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::string SecurityLayerBuilder::loadFile(const std::string& path) {

    std::ifstream file(path);

    if (!file.is_open()) {

        throw std::runtime_error("Failed to open file: " + path);

    }

    

    std::stringstream buffer;
- Line 258: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // They are validated but not used until VaultKeyProvider::Config is extended

            

            if (vault_config.vault_addr.empty() || vault_config.vault_token.empty()) {

                throw std::runtime_error("VAULT key provider requires vault_addr and vault_token in config");

            }

            

            return std::make_shared<VaultKeyProvider>(vault_config);
- Line 268: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Check if HSM support is enabled

            const char* hsm_enabled = std::getenv("THEMIS_HSM_ENABLED");

            if (!hsm_enabled || std::string(hsm_enabled) != "1") {

                throw std::runtime_error(

                    "HSM key provider is not enabled. "

                    "Set THEMIS_HSM_ENABLED=1 to enable HSM support. "

                    "Note: HSM support requires PKCS#11 libraries to be installed."
- Line 291: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

            

            if (library_path.empty()) {

                throw std::runtime_error("HSM key provider requires library_path in config");

            }



            std::error_code library_ec;
- Line 298: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: const std::filesystem::path library_fs_path(library_path);

            if (!std::filesystem::exists(library_fs_path, library_ec) ||

                !std::filesystem::is_regular_file(library_fs_path, library_ec)) {

                throw std::runtime_error("HSM key provider library_path does not point to an existing file: " +

                                         library_path);

            }
- Line 324: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto hsm = std::make_shared<security::HSMProvider>(hsm_config);

                if (!hsm->initialize()) {

                    throw std::runtime_error("HSM provider initialization failed");

                }



                return std::make_shared<security::HSMKeyProviderAdapter>(hsm);
- Line 329: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: return std::make_shared<security::HSMKeyProviderAdapter>(hsm);

            } catch (const std::exception& e) {

                throw std::runtime_error("Failed to initialize HSM key provider: " + std::string(e.what()));

            }

        }
- Line 334: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

        

        default:

            throw std::runtime_error("Unknown key provider type");

    }

}

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
