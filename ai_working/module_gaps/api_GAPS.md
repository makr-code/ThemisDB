# api Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: api
- Generated: 2026-06-04 08:50:21
- Status: Critical Findings Present
- Total Findings: 65
- Actionable Findings (Critical + High): 40
- Affected Files: 10

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 9 |
| High | 31 |
| Medium | 23 |
| Low | 2 |

## Category Summary

| Category | Count |
|---|---:|
| uninitialized_access | 6 |
| explicit_delete | 5 |
| string_concat_loop | 5 |
| data_race | 4 |
| delete_no_nullptr | 4 |
| delete_without_nullptr | 4 |
| missing_correlation_id | 4 |
| missing_health_check | 4 |
| missing_latency_metric | 3 |
| resource_leaked_in_exception | 3 |
| missing_trace_point | 2 |
| module_doc_linkset_drift | 2 |
| shared_ptr_cycle | 2 |
| blocking_no_timeout | 1 |
| db_connection_leak | 1 |
| explicit_lock_unlock | 1 |
| generic_catch | 1 |
| legacy_or_compat_path | 1 |
| lock_contention | 1 |
| missing_audit_log | 1 |
| nested_loop_find | 1 |
| no_timeout | 1 |
| null_dereference | 1 |
| primitive_no_volatile | 1 |
| range_temporary | 1 |
| smart_ptr_misuse | 1 |
| stale_doc_section_reference | 1 |
| thread_join_no_timeout | 1 |
| uncaught_exception | 1 |
| unnecessary_copy | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| api/themisdb_grpc_service.cpp | 20 | 3 | 9 | 8 | 0 |
| api/graphql.cpp | 14 | 2 | 4 | 8 | 0 |
| api/geo_index_hooks.cpp | 10 | 0 | 10 | 0 | 0 |
| api/otlp_exporter.cpp | 7 | 1 | 3 | 3 | 0 |
| api/grpc_server.cpp | 5 | 2 | 1 | 2 | 0 |
| api/graphql_ws_handler.cpp | 4 | 0 | 3 | 1 | 0 |
| api/ws_handler.cpp | 2 | 1 | 0 | 1 | 0 |
| api/FUTURE_ENHANCEMENTS.md | 1 | 0 | 0 | 0 | 1 |
| api/PRODUCTION_REQUIREMENTS.md | 1 | 0 | 0 | 0 | 1 |
| api/tracing_middleware.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### api/themisdb_grpc_service.cpp
Total findings: 20

- Line 549: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const uint32_t k = req->k() > 0 ? static_cast<uint32_t>(req->k()) : 10;
- Line 588: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const uint32_t k = req->k() > 0 ? static_cast<uint32_t>(req->k()) : 10;
- Line 642: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const uint32_t k = req->k() > 0 ? static_cast<uint32_t>(req->k()) : 10;
- Line 67: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: /// Kept as a local alias for backward compatibility with call sites below.
- Line 271: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 294: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: err->set_message("document not found or delete failed");
- Line 294: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: resp->set_success(false);

                auto* err = resp->mutable_error();

                err->set_code(404);

                err->set_message("document not found or delete failed");

                return grpc::Status::OK;

            }
- Line 294: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: err->set_message("document not found or delete failed");
- Line 492: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (ctx->IsCancelled()) {
- Line 638: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: err->set_message("invalid collection name: must match [a-zA-Z_][a-zA-Z0-9_]*");
- Line 731: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: err->set_message("invalid collection name: must match [a-zA-Z_][a-zA-Z0-9_]*");
- Line 740: severity=HIGH; category=missing_trace_point
  Description: Critical function query without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: " FILTER FULLTEXT(doc, 'text', '" + aqlEscape(req->query()) + "')"
- Line 2: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: * ThemisDB | File: themisdb_grpc_service.cpp | Version: 0.0.15 | Last Modified: 2026-05-31 12:49:01
- Line 10: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: #include "api/themisdb_grpc_service.h"
- Line 23: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: // This mirrors the pattern used by WalGrpcService / wal_grpc_service.cpp.
- Line 448: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto* row = resp->add_rows();

                row->set_data(*result);

                row->set_has_more(false);

            } catch (...) {

                // Fall back to raw payload when response is not valid JSON.

                auto* row = resp->add_rows();

                row->set_data(*result);
- Line 448: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 667: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: " FILTER FULLTEXT(doc, 'text', '" + aqlEscape(req->sparse_query()) + "')"
- Line 740: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: " FILTER FULLTEXT(doc, 'text', '" + aqlEscape(req->query()) + "')"
- Line 863: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'gRPC API Service Activation' that was not found in 'src/api/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/api/FUTURE_ENHANCEMENTS.md §"gRPC API Service Activation"

### api/graphql.cpp
Total findings: 14

- Line 37: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: result.document = cached_plan->parsed_document;
- Line 1664: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: docField.description = "The new document state (null for DELETED events)";
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4453 fix(graphql): resolve varia... (2026-04-07) | #4200 [WIP] Implement Gra
- Line 1467: severity=HIGH; category=missing_trace_point
  Description: Critical function query without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: aqlQuery.description            = "Execute a read-only AQL query (FOR/RETURN). "
- Line 1490: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1664: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 639: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: result += '\n';
- Line 642: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: result += '\r';
- Line 645: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: result += '\t';
- Line 648: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: result += '"';
- Line 651: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: result += '\\';
- Line 789: severity=MEDIUM; category=shared_ptr_cycle
  Description: Bidirectional shared_ptr — potential reference cycle leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (resolvedCtx.variables.find(varDef.name) == resolvedCtx.variables.end()) {

            if (varDef.default_value) {

                resolvedCtx.variables[varDef.name] = varDef.default_value;

            }

        }

    }

    return executeSelections(operation.selections, nullptr, resolvedCtx);

}



std::shared_ptr<Value> Executor::executeSelections(const std::vector<Field> &selections,

                                                   const std::shared_ptr<Value> &parent,

                                                   const ExecutionContext &context) {

    ValueMap result;



    for (const auto &field : selections) {

        auto value                   = executeField(field, parent, context);

        result[field.responseName()] = value;

    }



    return Value::object(std::move(result));

}
- Line 813: severity=MEDIUM; category=shared_ptr_cycle
  Description: Bidirectional shared_ptr — potential reference cycle leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: return value;

    }

    const auto &varName = value->asVariableRef();

    auto it             = context.variables.find(varName);

    if (it != context.variables.end()) {

        return it->second;

    }

    return Value::null();

}



std::shared_ptr<Value> Executor::executeField(const Field &field, const std::shared_ptr<Value> &parent,

                                              const ExecutionContext &context) {

    // Resolve any variable-reference arguments before invoking the resolver so

    // that resolvers always receive concrete values, never VariableRef nodes.

    if (!field.arguments.empty()) {

        bool hasVarRefs = false;

        for (const auto &arg : field.arguments) {

            if (arg.second && arg.second->isVariableRef()) {

                hasVarRefs = true;

                break;

            }
- Line 1467: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: aqlQuery.description            = "Execute a read-only AQL query (FOR/RETURN). "

### api/geo_index_hooks.cpp
Total findings: 10

- Line 505: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: THEMIS_WARN("Geo hook atomic delete JSON parse error for {}:{}: {}", table, pk, e.what());
- Line 505: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: return true;



    } catch (const json::exception& e) {

        THEMIS_WARN("Geo hook atomic delete JSON parse error for {}:{}: {}", table, pk, e.what());

        return false;

    } catch (const std::exception& e) {

        THEMIS_WARN("Geo hook atomic delete error for {}:{}: {}", table, pk, e.what());
- Line 505: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: THEMIS_WARN("Geo hook atomic delete JSON parse error for {}:{}: {}", table, pk, e.what());
- Line 508: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: THEMIS_WARN("Geo hook atomic delete error for {}:{}: {}", table, pk, e.what());
- Line 508: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: THEMIS_WARN("Geo hook atomic delete JSON parse error for {}:{}: {}", table, pk, e.what());

        return false;

    } catch (const std::exception& e) {

        THEMIS_WARN("Geo hook atomic delete error for {}:{}: {}", table, pk, e.what());

        return false;

    }

}
- Line 508: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: THEMIS_WARN("Geo hook atomic delete error for {}:{}: {}", table, pk, e.what());
- Line 575: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // Parse error - log but don't fail the delete
- Line 576: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: THEMIS_WARN("Geo hook delete error for {}:{}: {}", table, pk, e.what());
- Line 576: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: } catch (const std::exception& e) {

        // Parse error - log but don't fail the delete

        THEMIS_WARN("Geo hook delete error for {}:{}: {}", table, pk, e.what());

    }

}
- Line 576: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: THEMIS_WARN("Geo hook delete error for {}:{}: {}", table, pk, e.what());

### api/otlp_exporter.cpp
Total findings: 7

- Line 230: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: flush_thread_.join();
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4208 feat(api/otlp): exponential... (2026-03-15) | #4219 feat(api): wire Tra
- Line 306: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lk(queue_mutex_);
- Line 426: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
- Line 99: severity=MEDIUM; category=missing_health_check
  Description: Service initialization without nearby health/status handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void OtlpExporter::start() {
- Line 422: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int attempt = 0; attempt < max_attempts; ++attempt) {
- Line 503: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: //     "resource": {"attributes": [{"key":"service.name","value":{"stringValue":"…"}}]},

### api/grpc_server.cpp
Total findings: 5

- Line 231: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



        // Re-acquire lock to update shared state.

        lock.lock();

        server_  = std::move(server);

        running_ = true;

        lock.unlock();
- Line 231: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: lock.lock();
- Line 231: severity=HIGH; category=explicit_lock_unlock
  Description: Explicit lock/unlock without RAII guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: lock.lock();
- Line 68: severity=MEDIUM; category=missing_health_check
  Description: Service initialization without nearby health/status handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: bool GrpcApiServer::initialize(const GrpcServerConfig& config) {
- Line 166: severity=MEDIUM; category=missing_health_check
  Description: Service initialization without nearby health/status handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: bool GrpcApiServer::start() {

### api/graphql_ws_handler.cpp
Total findings: 4

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4310 [High Priority] Implement G... (2026-03-17) | #4200 [WIP] Implement Gra
- Line 252: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (!alive->load(std::memory_order_acquire)) {
- Line 416: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto it = field.arguments.find("collection");
- Line 134: severity=MEDIUM; category=missing_health_check
  Description: Service initialization without nearby health/status handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: GraphQLWsHandler::handleConnectionInit(const std::string& /*payload_json*/)

### api/ws_handler.cpp
Total findings: 2

- Line 113: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authorize" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: const auto result = auth_->authorize(token, "cdc:subscribe");
- Line 97: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto auth_hdr = req[http::field::authorization];

### api/FUTURE_ENHANCEMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'FUTURE_ENHANCEMENTS.md' is missing expected cross-links: ARCHITECTURE.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### api/PRODUCTION_REQUIREMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'PRODUCTION_REQUIREMENTS.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### api/tracing_middleware.cpp
Total findings: 1

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4219 feat(api): wire TracingMidd... (2026-03-14) | #3546 docs(api): sync api

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
