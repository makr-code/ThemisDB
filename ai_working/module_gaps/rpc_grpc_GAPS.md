# rpc_grpc Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: rpc_grpc
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 25
- Actionable Findings (Critical + High): 15
- Affected Files: 1

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 1 |
| High | 14 |
| Medium | 10 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| audit_logging | 9 |
| reliability | 4 |
| performance | 3 |
| performance_patterns | 2 |
| raii | 2 |
| container | 1 |
| determinism | 1 |
| exception_safety | 1 |
| memory | 1 |
| observability | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/rpc_grpc/grpc_plugin.cpp | 25 | 1 | 14 | 10 | 0 |

## Full Scanner Findings

### src/rpc_grpc/grpc_plugin.cpp
Total findings: 25

- Line 519: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: return new themis::plugins::rpc::grpc_plugin::GRPCPlugin();
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 118: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "gRPC server listening on " << server_address_ << std::endl;
  Confidence: band=very_high; score=0.9
- Line 120: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "gRPC admin port bound on " << admin_address_ << std::endl;
  Confidence: band=very_high; score=0.9
- Line 145: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "Shutting down gRPC server..." << std::endl;
  Confidence: band=very_high; score=0.9
- Line 157: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "gRPC server stopped" << std::endl;
  Confidence: band=very_high; score=0.9
- Line 183: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "Registered gRPC service" << std::endl;
  Confidence: band=very_high; score=0.9
- Line 215: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "TLS certificates reloaded successfully" << std::endl;
  Confidence: band=very_high; score=0.9
- Line 313: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& kv : errs)
  Confidence: band=very_high; score=0.9
- Line 320: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& kv : lats)
  Confidence: band=very_high; score=0.9
- Line 391: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: if (!file) throw std::runtime_error("Failed to open file: " + path);
- Line 402: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Replace std::cout with std::cerr + structured warning so the message is
  Confidence: band=very_high; score=0.9
- Line 438: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "gRPC server configured for mutual TLS (mTLS)" << std::endl;
  Confidence: band=very_high; score=0.9
- Line 442: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "gRPC server configured for server-side TLS only" << std::endl;
  Confidence: band=very_high; score=0.9
- Line 523: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: plugin = nullptr;
  Context: delete plugin;
- Line 69: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 78: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 95: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 291: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, uint64_t> reqs, errs, lats;
  Confidence: band=medium; score=0.66
- Line 363: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if      (c == '"')  out += "\\\"";
- Line 364: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\\') out += "\\\\";
- Line 365: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\n') out += "\\n";
- Line 450: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ssl_opts.pem_key_cert_pairs.push_back(pair);
- Line 477: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: bool GRPCPlugin::initialize(const char* config_json) {
  Confidence: band=medium; score=0.66
- Line 523: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete plugin;

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
