---
Author: "ThemisDB Contributors"
Created: "2026-11-01"
Last Updated: "2026-11-01"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References: "src/user_storage_encrypted/README.md; src/user_storage_encrypted/ARCHITECTURE.md; src/user_storage_encrypted/API.md"
source: "ai_working/user_storage_encrypted_scan/gap_scan_results.json"
source_class: "generated"
source_weight: 60
Related: "#7392; DOC-WEEKLY-2026-44"
---

> Exact model backend not verifiable from final document text alone.

# user_storage_encrypted Module - Developer Gap Note

> Auto-generated from ai_working/user_storage_encrypted_scan/gap_scan_results.json.
> This file is overwritten on each regeneration.
> Scanner findings are heuristic candidates, not independently confirmed defects or security findings.

## Scan Snapshot

- Module: user_storage_encrypted
- Generated: 2026-11-01 07:24:45
- Status: Critical scanner candidates present
- Total Findings: 22
- Actionable Findings (Critical + High): 10
- Affected Files: 9

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 1 |
| High | 9 |
| Medium | 7 |
| Low | 5 |

## Category Summary

| Category | Count |
|---|---:|
| module_doc_linkset_drift | 5 |
| generic_catch | 4 |
| circular_lock_ordering | 3 |
| resource_leaked_in_exception | 3 |
| delete_without_nullptr | 2 |
| blocking_no_timeout | 1 |
| const_cast_in_const_method | 1 |
| thin_wrapper | 1 |
| uncaught_exception | 1 |
| unchecked_result | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/user_storage_encrypted/key_rotation_scheduler.cpp | 7 | 1 | 3 | 3 | 0 |
| src/user_storage_encrypted/multi_level_storage.cpp | 6 | 0 | 3 | 3 | 0 |
| src/user_storage_encrypted/gocryptfs_backend.cpp | 2 | 0 | 2 | 0 | 0 |
| src/user_storage_encrypted/key_derivation_service.cpp | 2 | 0 | 1 | 1 | 0 |
| src/user_storage_encrypted/ARCHITECTURE.md | 1 | 0 | 0 | 0 | 1 |
| src/user_storage_encrypted/FUTURE_ENHANCEMENTS.md | 1 | 0 | 0 | 0 | 1 |
| src/user_storage_encrypted/PRODUCTION_REQUIREMENTS.md | 1 | 0 | 0 | 0 | 1 |
| src/user_storage_encrypted/README.md | 1 | 0 | 0 | 0 | 1 |
| src/user_storage_encrypted/ROADMAP.md | 1 | 0 | 0 | 0 | 1 |

## Full Scanner Findings

### src/user_storage_encrypted/key_rotation_scheduler.cpp
Total findings: 7

- Line 117: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Scanner: Uniform::phase1_error_handling
  Context: // Wake the scheduler thread so it exits promptly.

        impl_->cv.notify_all();

        if (impl_->scheduler_thread.joinable()) {

            impl_->scheduler_thread.join();

        }

    }

}
- Line 71: severity=HIGH; category=circular_lock_ordering
  Description: Potential deadlock: lock order mutex then cv_mutex
  Remediation: Enforce consistent lock ordering across all code paths
  Scanner: Uniform::phase1_thread_safety
- Line 137: severity=HIGH; category=circular_lock_ordering
  Description: Potential deadlock: lock order mutex then cv_mutex
  Remediation: Enforce consistent lock ordering across all code paths
  Scanner: Uniform::phase1_thread_safety
- Line 179: severity=HIGH; category=circular_lock_ordering
  Description: Potential deadlock: lock order mutex then cv_mutex
  Remediation: Enforce consistent lock ordering across all code paths
  Scanner: Uniform::phase1_thread_safety
- Line 247: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Scanner: Uniform::phase1_error_handling
  Context: if (schedule.callback) {

        try {

            schedule.callback(level, true, "");

        } catch (...) {}

    }



    persistRotationState(level);
- Line 276: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Scanner: Uniform::phase1_error_handling
  Context: if (now_ms - schedule.last_check_ms >= interval_ms) {

                    try {

                        schedule.callback(schedule.level, true, "");

                    } catch (...) {

                        // Callback must not propagate exceptions.

                    }
- Line 357: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Scanner: Uniform::phase1_error_handling
  Context: if (j.contains("interval_days")) {

            it->second.interval_days = j["interval_days"].get<int>();

        }

    } catch (...) {

        // Ignore corrupted persisted state.

    }

}

### src/user_storage_encrypted/multi_level_storage.cpp
Total findings: 6

- Line 646: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception path may leak resource
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 656: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception path may leak resource
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 688: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception path may leak resource
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 40: severity=MEDIUM; category=thin_wrapper
  Description: Class 'MultiLevelEncryptedStorage' appears to be a thin wrapper (depth: 3, methods: 2). Consider merging with wrapped class or adding real functionality.
  Remediation: Add meaningful functionality to wrapper, or remove the abstraction layer and use wrapped class directly.
  Scanner: Uniform::wrapper_abstraction_excess
  Context: namespace user_storage {
- Line 1011: severity=MEDIUM; category=delete_without_nullptr
  Description: delete without explicit nullification
  Remediation: Set pointer to nullptr after delete: user = nullptr;
  Scanner: Uniform::phase1_memory_safety
  Context: }

    

    if (std::remove(path.c_str()) != 0) {

        return Result<void>::error("Failed to delete user file");

    }

    

    return Result<void>();
- Line 1112: severity=MEDIUM; category=delete_without_nullptr
  Description: delete without explicit nullification
  Remediation: Set pointer to nullptr after delete: group = nullptr;
  Scanner: Uniform::phase1_memory_safety
  Context: }

    

    if (std::remove(path.c_str()) != 0) {

        return Result<void>::error("Failed to delete group file");

    }

    

    return Result<void>();

### src/user_storage_encrypted/gocryptfs_backend.cpp
Total findings: 2

- Line 309: severity=HIGH; category=const_cast_in_const_method
  Description: const_cast used inside const-qualified method
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::const_correctness
  Context: auto result = const_cast<GocryptfsBackend*>(this)->executeCommandSafe(
- Line 908: severity=HIGH; category=unchecked_result
  Description: Result/Error variable created but never checked
  Remediation: Check result or explicitly ignore with [[nodiscard]] cast
  Scanner: Uniform::phase1_ai_error_handling_consistency
  Context: auto result = executeCommandWithStdin(args, key_hex);

### src/user_storage_encrypted/key_derivation_service.cpp
Total findings: 2

- Line 351: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Scanner: Uniform::phase1_error_handling
  Context: secureZero(password.data(),password.size());



    if (rc != ARGON2_OK) {

        throw std::runtime_error(

            std::string("Argon2id derivation failed: ") + argon2_error_message(rc)

        );

    }
- Line 164: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Scanner: Uniform::phase1_error_handling
  Context: if (fn) [[unlikely]] {

            try {

                return fn(master_key, salt);

            } catch (...) {

                // Fall through to built-in implementation.

            }

        }

### src/user_storage_encrypted/ARCHITECTURE.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'ARCHITECTURE.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### src/user_storage_encrypted/FUTURE_ENHANCEMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'FUTURE_ENHANCEMENTS.md' is missing expected cross-links: ARCHITECTURE.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### src/user_storage_encrypted/PRODUCTION_REQUIREMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'PRODUCTION_REQUIREMENTS.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### src/user_storage_encrypted/README.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'README.md' is missing expected cross-links: ARCHITECTURE.md, FUTURE_ENHANCEMENTS.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### src/user_storage_encrypted/ROADMAP.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'ROADMAP.md' is missing expected cross-links: ARCHITECTURE.md, FUTURE_ENHANCEMENTS.md, README.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
