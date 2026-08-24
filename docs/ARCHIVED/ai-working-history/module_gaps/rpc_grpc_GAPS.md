# rpc_grpc Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: rpc_grpc
- Generated: 2026-06-04 08:50:22
- Status: Critical Findings Present
- Total Findings: 32
- Actionable Findings (Critical + High): 8
- Affected Files: 4

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 3 |
| High | 5 |
| Medium | 12 |
| Low | 12 |

## Category Summary

| Category | Count |
|---|---:|
| hardcoded_output | 9 |
| generic_catch | 3 |
| string_concat_loop | 3 |
| uncaught_exception | 3 |
| blocking_no_timeout | 2 |
| module_doc_linkset_drift | 2 |
| delete_no_nullptr | 1 |
| delete_without_nullptr | 1 |
| endl_in_loop | 1 |
| explicit_delete | 1 |
| explicit_lock_unlock | 1 |
| manual_cleanup | 1 |
| missing_health_check | 1 |
| resource_leaked_in_exception | 1 |
| smart_ptr_misuse | 1 |
| unordered_container_iter | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| rpc_grpc/grpc_plugin.cpp | 27 | 1 | 4 | 12 | 10 |
| rpc_grpc/bidi_stream_adapter.h | 3 | 2 | 1 | 0 | 0 |
| rpc_grpc/FUTURE_ENHANCEMENTS.md | 1 | 0 | 0 | 0 | 1 |
| rpc_grpc/PRODUCTION_REQUIREMENTS.md | 1 | 0 | 0 | 0 | 1 |

## Full Scanner Findings

### rpc_grpc/grpc_plugin.cpp
Total findings: 27

- Line 517: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: return new themis::plugins::rpc::grpc_plugin::GRPCPlugin();
- Line 212: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 521: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete plugin;
- Line 521: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }



void destroyPlugin(themis::plugins::IThemisPlugin* plugin) {

    delete plugin;

}



} // extern "C"
- Line 521: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete plugin;
- Line 67: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

                int ms = std::stoi(ka_it->second);

                builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIME_MS, ms);

            } catch (...) {

                // Invalid value — fall back to gRPC default

            }

        }
- Line 67: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 76: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

                int ms = std::stoi(kt_it->second);

                builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, ms);

            } catch (...) {}

        }



        // ---- v0.2.0: admin port binding ----------------------------------------
- Line 76: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {}
- Line 93: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: builder.AddListeningPort(admin_address_,

                                             grpc::InsecureServerCredentials());

                }

            } catch (...) {

                admin_address_.clear();

            }

        }
- Line 93: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 289: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, uint64_t> reqs, errs, lats;
- Line 361: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if      (c == '"')  out += "\\\"";
- Line 362: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (c == '\\') out += "\\\\";
- Line 363: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (c == '\n') out += "\\n";
- Line 475: severity=MEDIUM; category=missing_health_check
  Description: Service initialization without nearby health/status handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: bool GRPCPlugin::initialize(const char* config_json) {
- Line 521: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: delete plugin;
- Line 116: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "gRPC server listening on " << server_address_ << std::endl;
- Line 118: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "gRPC admin port bound on " << admin_address_ << std::endl;
- Line 143: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "Shutting down gRPC server..." << std::endl;
- Line 155: severity=LOW; category=endl_in_loop
  Description: std::endl in loop (causes unnecessary flush, use '\n')
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::cout << "gRPC server stopped" << std::endl;
- Line 155: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "gRPC server stopped" << std::endl;
- Line 181: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "Registered gRPC service" << std::endl;
- Line 213: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "TLS certificates reloaded successfully" << std::endl;
- Line 400: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // Replace std::cout with std::cerr + structured warning so the message is
- Line 436: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "gRPC server configured for mutual TLS (mTLS)" << std::endl;
- Line 440: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "gRPC server configured for server-side TLS only" << std::endl;

### rpc_grpc/bidi_stream_adapter.h
Total findings: 3

- Line 149: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: {

            std::unique_lock<std::mutex> lock(queue_mutex_);

            // Wait while queue is full and stream is still open

            queue_not_full_.wait(lock, [this] {

                return finished_ || queue_.size() < max_queue_depth_;

            });
- Line 228: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: stream_->Write(msg);

            queue_not_full_.notify_one();



            lock.lock();

        }

    }
- Line 228: severity=HIGH; category=explicit_lock_unlock
  Description: Explicit lock/unlock without RAII guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: lock.lock();

### rpc_grpc/FUTURE_ENHANCEMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'FUTURE_ENHANCEMENTS.md' is missing expected cross-links: ARCHITECTURE.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### rpc_grpc/PRODUCTION_REQUIREMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'PRODUCTION_REQUIREMENTS.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
