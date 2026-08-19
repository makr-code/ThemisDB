# ThemisDB Gap Worklist for Remote Ollama gemma4

- [ ] Scope: actionable themis_core findings only (third_party is informational).

## Work Items

- [ ] HIGH | query | missing_doxygen_comment | src/query/query_engine.cpp:259
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/query/query_engine.cpp:259
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!engine || expression.empty()) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | index | missing_doxygen_comment | src/index/secondary_index.cpp:295
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/index/secondary_index.cpp:295
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (raw.size() < width) out.append(width - raw.size(), '0');
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | transaction | missing_doxygen_comment | src/transaction/transaction_manager.cpp:126
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/transaction/transaction_manager.cpp:126
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!deadlock_detection_enabled_.load(std::memory_order_relaxed)) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | server | missing_doxygen_comment | src/server/query_api_handler.cpp:281
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/server/query_api_handler.cpp:281
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!ob.contains("column")) return makeErrorResponse(http::status::bad_request, "order_by requires 'column'", req);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | index | missing_doxygen_comment | src/index/process_graph.cpp:311
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/index/process_graph.cpp:311
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (val.is_boolean()) return val.get<bool>();
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | index | missing_doxygen_comment | src/index/graph_index.cpp:40
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/index/graph_index.cpp:40
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (as_int.has_value()) return as_int;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | prompt_engineering | missing_doxygen_comment | src/prompt_engineering/prompt_engineering_metrics.cpp:31
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/prompt_engineering/prompt_engineering_metrics.cpp:31
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!config_.enabled) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | importers | missing_doxygen_comment | src/importers/mongo_importer.cpp:94
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/importers/mongo_importer.cpp:94
// Problem: missing_doxygen_comment
// Description: Public declaration 'sleep_for' is missing a Doxygen comment
// Context: std::this_thread::sleep_for(std::chrono::milliseconds(current_timeout));
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | server | missing_doxygen_comment | src/server/maintenance_api_handler.cpp:121
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/server/maintenance_api_handler.cpp:121
// Problem: missing_doxygen_comment
// Description: Public declaration 'for' is missing a Doxygen comment
// Context: for (auto& e : schedules) arr.push_back(scheduleToResponse(e));
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | storage | missing_doxygen_comment | src/storage/rocksdb_wrapper.cpp:839
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/storage/rocksdb_wrapper.cpp:839
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!listener) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | content | missing_doxygen_comment | src/content/content_manager.cpp:69
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/content/content_manager.cpp:69
// Problem: missing_doxygen_comment
// Description: Public declaration 'sleep_for' is missing a Doxygen comment
// Context: std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay_ms));
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | replication | missing_doxygen_comment | src/replication/replication_manager.cpp:2722
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/replication/replication_manager.cpp:2722
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!first) oss << ",";
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | importers | missing_doxygen_comment | src/importers/postgres_importer.cpp:326
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/importers/postgres_importer.cpp:326
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!std::getline(file, tl_buf)) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | projects | missing_doxygen_comment | src/projects/project_versioning.cpp:144
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/projects/project_versioning.cpp:144
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (project_id.empty()) return Status::Error("createSnapshot: project_id must not be empty");
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | server | missing_doxygen_comment | src/server/mcp_server.cpp:2786
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/server/mcp_server.cpp:2786
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!is_running_.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | storage | missing_doxygen_comment | src/storage/hierarchical_tucker_decomposer.cpp:59
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/storage/hierarchical_tucker_decomposer.cpp:59
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!n) return 0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | acceleration | missing_doxygen_comment | src/acceleration/hip_backend.cpp:612
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/acceleration/hip_backend.cpp:612
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (d_queries) hipFree(d_queries);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | index | missing_doxygen_comment | src/index/inverted_index.cpp:100
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/index/inverted_index.cpp:100
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (table.empty() || column.empty()) return Status::Error("InvertedIndex::create: table/column must not be empty");
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | performance | missing_doxygen_comment | src/performance/phase4/pmu_counters.cpp:116
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/performance/phase4/pmu_counters.cpp:116
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (fd_ < 0) return 0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | query | missing_doxygen_comment | src/query/aql_runner.cpp:67
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/query/aql_runner.cpp:67
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!ents) return out;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | auth | missing_doxygen_comment | src/auth/distributed_token_blacklist.cpp:233
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/auth/distributed_token_blacklist.cpp:233
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!sockValid(fd)) continue;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | index | missing_doxygen_comment | src/index/vector_index.cpp:154
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/index/vector_index.cpp:154
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!audit_logger_) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | query | missing_doxygen_comment | src/query/query_rewrite_rule.cpp:96
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/query/query_rewrite_rule.cpp:96
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!eq.contains("field") || !eq.contains("value")) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | server | missing_doxygen_comment | src/server/http_server.cpp:698
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/server/http_server.cpp:698
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (v && *v) return std::string(v);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | storage | missing_doxygen_comment | src/storage/gguf_metadata.cpp:157
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/storage/gguf_metadata.cpp:157
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (pos + 4 > size) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | utils | missing_doxygen_comment | src/utils/input_validator.cpp:68
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/utils/input_validator.cpp:68
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!isAsciiControl(c)) out.push_back(c);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | importers | missing_doxygen_comment | src/importers/flatfile_importer.cpp:409
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/importers/flatfile_importer.cpp:409
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!file) return result;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | acceleration | missing_doxygen_comment | src/acceleration/graphics_backends.cpp:1063
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/acceleration/graphics_backends.cpp:1063
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (initialized_ && impl_) return impl_->hasBufferDeviceAddress;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | performance | missing_doxygen_comment | src/performance/phase4/io_uring_zero_copy.cpp:86
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/performance/phase4/io_uring_zero_copy.cpp:86
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!data_) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | server | missing_doxygen_comment | src/server/mqtt_client_service.cpp:51
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/server/mqtt_client_service.cpp:51
// Problem: missing_doxygen_comment
// Description: Public declaration 'while' is missing a Doxygen comment
// Context: } while (value > 0);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | timeseries | missing_doxygen_comment | src/timeseries/prometheus_remote_write.cpp:85
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/timeseries/prometheus_remote_write.cpp:85
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!readVarint64(buf, pos, end, len)) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | voice | missing_doxygen_comment | src/voice/voice_telephony.cpp:342
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/voice/voice_telephony.cpp:342
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (impl_->on_error) impl_->on_error("Empty RTP packet");
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | acceleration | missing_doxygen_comment | src/acceleration/nccl_vector_backend.cpp:258
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/acceleration/nccl_vector_backend.cpp:258
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!pImpl->initialized) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | acceleration | missing_doxygen_comment | src/acceleration/rccl_vector_backend.cpp:285
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/acceleration/rccl_vector_backend.cpp:285
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!pImpl->initialized) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | analytics | missing_doxygen_comment | src/analytics/olap.cpp:2079
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/analytics/olap.cpp:2079
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!st.ok()) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | graph | missing_doxygen_comment | src/graph/graph_query_optimizer.cpp:50
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/graph/graph_query_optimizer.cpp:50
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (required_labels.empty()) return true;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | query | missing_doxygen_comment | src/query/functions/udf_registry.cpp:126
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/query/functions/udf_registry.cpp:126
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!expr["args"].is_array()) return "'call' node 'args' must be an array";
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | retrieval | missing_doxygen_comment | src/retrieval/src/lora_package.cc:327
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/retrieval/src/lora_package.cc:327
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (supported_architectures.empty()) return true;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | aql | missing_doxygen_comment | src/aql/llm_semantic_validator.cpp:113
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/aql/llm_semantic_validator.cpp:113
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!ast) return bindings;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | cdc | missing_doxygen_comment | src/cdc/kafka_cdc_producer.cpp:118
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/cdc/kafka_cdc_producer.cpp:118
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!set("bootstrap.servers", config_.brokers)) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | projects | missing_doxygen_comment | src/projects/collaboration_manager.cpp:81
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/projects/collaboration_manager.cpp:81
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (project_id.empty()) return Status::Error("shareProject: project_id must not be empty");
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | projects | missing_doxygen_comment | src/projects/project_lifecycle.cpp:107
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/projects/project_lifecycle.cpp:107
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (project_id.empty()) return Status::Error("applyTransition: project_id must not be empty");
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | storage | missing_doxygen_comment | src/storage/history_manager.cpp:170
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/storage/history_manager.cpp:170
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!payload) return std::nullopt;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | temporal | missing_doxygen_comment | src/temporal/interval_tree_index.cpp:44
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/temporal/interval_tree_index.cpp:44
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!n) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | tensor | missing_doxygen_comment | src/tensor/hnsw_tt_bridge.cpp:290
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/tensor/hnsw_tt_bridge.cpp:290
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!tt_store_->remove(id)) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | utils | missing_doxygen_comment | src/utils/cron_parser.cpp:296
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/utils/cron_parser.cpp:296
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!day_matches) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | voice | missing_doxygen_comment | src/voice/audio_preprocessing.cpp:108
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/voice/audio_preprocessing.cpp:108
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (samples_48k.empty()) return 0.0f;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | voice | missing_doxygen_comment | src/voice/voice_browser_streaming.cpp:212
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/voice/voice_browser_streaming.cpp:212
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (impl_->active) return impl_->stream_id;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | cache | missing_doxygen_comment | src/cache/distributed_cache_coordinator.cpp:487
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/cache/distributed_cache_coordinator.cpp:487
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!sendAll(fd, cmd)) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | gpu | missing_doxygen_comment | src/gpu/cuda_operations_stub.cpp:50
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/gpu/cuda_operations_stub.cpp:50
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (is_moved_from_) throw std::logic_error("Cannot get handle from moved-from stream");
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | index | missing_doxygen_comment | src/index/spatial_index.cpp:274
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/index/spatial_index.cpp:274
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (rtree_built_.count(table_str)) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | llm | missing_doxygen_comment | src/llm/gguf_loader.cpp:438
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/llm/gguf_loader.cpp:438
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (offset + 8 > mmap_size_) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | observability | missing_doxygen_comment | src/observability/metrics_collector.cpp:341
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/observability/metrics_collector.cpp:341
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!checkCardinality(name, key)) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | query | missing_doxygen_comment | src/query/functions/process_mining_functions.cpp:210
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/query/functions/process_mining_functions.cpp:210
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!j.is_object()) return proc;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | security | missing_doxygen_comment | src/security/hsm_provider_pkcs11.cpp:126
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/security/hsm_provider_pkcs11.cpp:126
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if(data.empty()) return "";
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | security | missing_doxygen_comment | src/security/timestamp_authority_openssl.cpp:222
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/security/timestamp_authority_openssl.cpp:222
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!gen || !gen->data) return 0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | storage | missing_doxygen_comment | src/storage/compressed_storage.cpp:107
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/storage/compressed_storage.cpp:107
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (payload_end < 9) return std::nullopt;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | training | missing_doxygen_comment | src/training/modality_parser.cpp:75
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/training/modality_parser.cpp:75
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (t.empty()) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | utils | missing_doxygen_comment | src/utils/saga_logger.cpp:73
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/utils/saga_logger.cpp:73
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (obj.contains(key) && obj[key].is_number_unsigned()) return static_cast<int64_t>(obj[key].get<uint64_t>());
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | distributed_tensor | missing_doxygen_comment | src/distributed_tensor/src/tensor_delta_log.cc:554
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/distributed_tensor/src/tensor_delta_log.cc:554
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!artifact_id_val) return std::nullopt;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | network | missing_doxygen_comment | src/network/kernel_bypass.cpp:105
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/network/kernel_bypass.cpp:105
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (core_id < 0) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | performance | missing_doxygen_comment | src/performance/numa_topology.cpp:310
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/performance/numa_topology.cpp:310
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (cpu_id < 0) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | query | missing_doxygen_comment | src/query/aql_translator.cpp:249
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/query/aql_translator.cpp:249
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (_lv < 0) return TranslationResult::Error("FULLTEXT() limit must be non-negative");
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | rag | missing_doxygen_comment | src/rag/hallucination_dashboard.cpp:124
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/rag/hallucination_dashboard.cpp:124
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (impl_->window.empty()) return 0.0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | server | missing_doxygen_comment | src/server/policy_engine.cpp:85
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/server/policy_engine.cpp:85
// Problem: missing_doxygen_comment
// Description: Public declaration 'for' is missing a Doxygen comment
// Context: for (const auto& ua : n["allowed_user_agent_patterns"]) p.allowed_user_agent_patterns.push_back(ua.as<std::string>());
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | sharding | missing_doxygen_comment | src/sharding/operational_metrics.cpp:419
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/sharding/operational_metrics.cpp:419
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!metrics) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | timeseries | missing_doxygen_comment | src/timeseries/continuous_agg.cpp:27
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/timeseries/continuous_agg.cpp:27
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (shards.empty()) return merged;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | updates | missing_doxygen_comment | src/updates/delta_update_engine.cpp:113
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/updates/delta_update_engine.cpp:113
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (rel_path.empty()) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | utils | missing_doxygen_comment | src/utils/pki_client.cpp:241
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/utils/pki_client.cpp:241
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!s) return std::string();
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | importers | missing_doxygen_comment | src/importers/mysql_importer.cpp:655
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/importers/mysql_importer.cpp:655
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!options.continue_on_error) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | index | missing_doxygen_comment | src/index/hnsw_layer_optimizer.cpp:31
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/index/hnsw_layer_optimizer.cpp:31
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!config_.enabled) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | ingestion | missing_doxygen_comment | src/ingestion/workflow_engine.cpp:95
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/ingestion/workflow_engine.cpp:95
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (pattern.empty()) return true;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | performance | missing_doxygen_comment | src/performance/advanced_cache_manager.cpp:69
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/performance/advanced_cache_manager.cpp:69
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!(bits[bit / 64] & (1ULL << (bit % 64)))) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | rag | missing_doxygen_comment | src/rag/delegate_evaluator.cpp:74
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/rag/delegate_evaluator.cpp:74
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (orig.empty()) return 0.0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | scraper | missing_doxygen_comment | src/scraper/scraper_config.cpp:38
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/scraper/scraper_config.cpp:38
// Problem: missing_doxygen_comment
// Description: Public declaration 'for' is missing a Doxygen comment
// Context: for (const auto& kw : n["keywords"]) g.keywords.push_back(kw.as<std::string>());
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | storage | missing_doxygen_comment | src/storage/key_schema.cpp:116
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/storage/key_schema.cpp:116
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (key.starts_with("idx:")) return KeyType::SECONDARY_INDEX;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | transaction | missing_doxygen_comment | src/transaction/crash_recovery_manager.cpp:118
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/transaction/crash_recovery_manager.cpp:118
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (line.empty()) return std::nullopt;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | updates | missing_doxygen_comment | src/updates/schema_migration_tester.cpp:39
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/updates/schema_migration_tester.cpp:39
// Problem: missing_doxygen_comment
// Description: Public declaration 'for' is missing a Doxygen comment
// Context: for (const auto& r : test_results) if (r.passed) ++n;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] CRITICAL | include | unhandled_critical_operation | include/api/graphql_cache.h:169
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: General
// Location: include/api/graphql_cache.h:169
// Problem: unhandled_critical_operation
// Description: Unhandled error-prone operation 'lock' in critical path
// Context: std::lock_guard<std::mutex> lock(mutex_);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | importers | missing_doxygen_comment | src/importers/schema_validator.cpp:42
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/importers/schema_validator.cpp:42
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (s.empty()) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | importers | missing_doxygen_comment | src/importers/sqlite_importer.cpp:357
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/importers/sqlite_importer.cpp:357
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!file) return json::array();
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | ingestion | missing_doxygen_comment | src/ingestion/filesystem_ingester.cpp:40
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/ingestion/filesystem_ingester.cpp:40
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (raw.size() < 4) return BinaryMimeType::UNKNOWN;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | llm | missing_doxygen_comment | src/llm/llm_factory_stub.cpp:25
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/llm/llm_factory_stub.cpp:25
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if(g_docs_factory) return g_docs_factory();
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | observability | missing_doxygen_comment | src/observability/ml_anomaly_detector.cpp:36
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/observability/ml_anomaly_detector.cpp:36
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (v < 0.0) return 0.0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | process | missing_doxygen_comment | src/process/dmn_evaluator.cpp:49
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/process/dmn_evaluator.cpp:49
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (sv.empty()) return std::nullopt;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | process | missing_doxygen_comment | src/process/process_graph_rag.cpp:66
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/process/process_graph_rag.cpp:66
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (a.empty() && b.empty()) return 1.f;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | projects | missing_doxygen_comment | src/projects/project_template.cpp:143
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/projects/project_template.cpp:143
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!def.is_object()) return Status::Error("Template definition must be a JSON object");
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | query | missing_doxygen_comment | src/query/let_evaluator.cpp:689
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/query/let_evaluator.cpp:689
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!pointInRing(px, py, rings[0])) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | rag | missing_doxygen_comment | src/rag/knowledge_graph_retriever.cpp:60
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/rag/knowledge_graph_retriever.cpp:60
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (a.empty() && b.empty()) return 1.0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | security | missing_doxygen_comment | src/security/post_quantum_crypto.cpp:270
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/security/post_quantum_crypto.cpp:270
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!kctx) throw std::runtime_error("x25519_keygen: ctx: " + ossl_error());
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | server | missing_doxygen_comment | src/server/graphql_api_handler.cpp:56
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/server/graphql_api_handler.cpp:56
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!val || val->isNull()) return json(nullptr);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | server | missing_doxygen_comment | src/server/smart_routing.cpp:185
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/server/smart_routing.cpp:185
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (backends_.empty()) return std::nullopt;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | storage | missing_doxygen_comment | src/storage/tensor_network_storage_engine.cpp:258
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/storage/tensor_network_storage_engine.cpp:258
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!backend_->put(meta_key, header)) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | temporal | missing_doxygen_comment | src/temporal/temporal_cold_store.cpp:184
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/temporal/temporal_cold_store.cpp:184
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!ofs) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | tensor | missing_doxygen_comment | src/tensor/ht_index.cpp:68
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/tensor/ht_index.cpp:68
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (results.size() > k) results.resize(k);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | themis | missing_doxygen_comment | src/themis/build_info.cpp:935
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/themis/build_info.cpp:935
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (len > 0) exe_path.assign(buf, len);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | training | missing_doxygen_comment | src/training/lora_data_selection.cpp:58
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/training/lora_data_selection.cpp:58
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (text.empty()) return 0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | utils | missing_doxygen_comment | src/utils/build_info.cpp:908
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/utils/build_info.cpp:908
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (len > 0) exe_path.assign(buf, len);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | utils | missing_doxygen_comment | src/utils/tracing.cpp:543
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/utils/tracing.cpp:543
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (h < 0 || l < 0) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | content | missing_doxygen_comment | src/content/html_processor.cpp:421
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/content/html_processor.cpp:421
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (text.empty()) return 0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | content | missing_doxygen_comment | src/content/markdown_processor.cpp:453
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/content/markdown_processor.cpp:453
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (text.empty()) return 0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | importers | missing_doxygen_comment | src/importers/oracle_importer.cpp:552
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/importers/oracle_importer.cpp:552
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!options.continue_on_error) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | include | thin_wrapper | include/api/ws_handler.h:22
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: General
// Location: include/api/ws_handler.h:22
// Problem: thin_wrapper
// Description: Class 'AuthMiddleware' appears to be a thin wrapper (depth: 4, methods: 3). Consider merging with wrapped class or adding real functionality.
// Context: namespace themis {
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | include | chimera_adapter_missing_interface | include/chimera/database_adapter.hpp:174
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: ArchitectureContract
// Location: include/chimera/database_adapter.hpp:174
// Problem: chimera_adapter_missing_interface
// Description: Chimera adapter 'IStreamingAdapter' should implement one of the required adapter interfaces
// Context: class IStreamingAdapter {
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | index | missing_doxygen_comment | src/index/ann_index.cpp:409
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/index/ann_index.cpp:409
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (cfg_.enable_ah && codebook_.num_subspaces > 0) leaves_[best_leaf].codes.push_back( codebook_.encode(vector, dim));
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | ingestion | missing_doxygen_comment | src/ingestion/entity_assembler.cpp:132
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/ingestion/entity_assembler.cpp:132
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (parts.count("section")) return "normref:" + abbr + ":§" + parts["section"];
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | ingestion | missing_doxygen_comment | src/ingestion/web_crawler_connector.cpp:124
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/ingestion/web_crawler_connector.cpp:124
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (pos + n > html.size()) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | llm | missing_doxygen_comment | src/llm/llama_lora_adapter.cpp:246
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/llm/llama_lora_adapter.cpp:246
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!g_lora_api_override_active) ensureAPIInitialized();
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | llm | missing_doxygen_comment | src/llm/lora_framework/lora_checkpoint_manager.cpp:196
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/llm/lora_framework/lora_checkpoint_manager.cpp:196
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!fs::exists(p)) return std::nullopt;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | llm | missing_doxygen_comment | src/llm/moral_analyzer.cpp:49
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/llm/moral_analyzer.cpp:49
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!status.ok) return status;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | llm | missing_doxygen_comment | src/llm/task_decomposer.cpp:204
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/llm/task_decomposer.cpp:204
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (s.empty()) return nullptr;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | query | missing_doxygen_comment | src/query/cte_subquery.cpp:323
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/query/cte_subquery.cpp:323
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!expr) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | query | missing_doxygen_comment | src/query/ddl_executor.cpp:55
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/query/ddl_executor.cpp:55
// Problem: missing_doxygen_comment
// Description: Public declaration 'for' is missing a Doxygen comment
// Context: for (const auto& kv : collections_) result.push_back(kv.first);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | query | missing_doxygen_comment | src/query/result_type_annotation.cpp:80
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/query/result_type_annotation.cpp:80
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (value.is_null()) return ResultFieldType::NULL_TYPE;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | rag | missing_doxygen_comment | src/rag/document_splitter.cpp:44
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/rag/document_splitter.cpp:44
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (text.empty()) return 0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | security | missing_doxygen_comment | src/security/aql_injection_detector.cpp:467
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/security/aql_injection_detector.cpp:467
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!expr) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | security | missing_doxygen_comment | src/security/zero_trust_policy_enforcer.cpp:292
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/security/zero_trust_policy_enforcer.cpp:292
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (a > 255 || b > 255 || c > 255 || d > 255) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | server | missing_doxygen_comment | src/server/content_api_handler.cpp:178
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/server/content_api_handler.cpp:178
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (id.empty()) return makeErrorResponse(http::status::bad_request, "Missing content id", req);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | sharding | missing_doxygen_comment | src/sharding/gossip_protocol.cpp:96
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/sharding/gossip_protocol.cpp:96
// Problem: missing_doxygen_comment
// Description: Public declaration 'sleep_for' is missing a Doxygen comment
// Context: std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | sharding | missing_doxygen_comment | src/sharding/signed_request.cpp:89
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/sharding/signed_request.cpp:89
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!bmem) return "";
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | storage | missing_doxygen_comment | src/storage/tiered_storage.cpp:86
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/storage/tiered_storage.cpp:86
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (key.empty()) return "_empty_";
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | storage | missing_doxygen_comment | src/storage/tt_quantizer.cpp:53
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/storage/tt_quantizer.cpp:53
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (bytes.size() < 33) return std::nullopt;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | temporal | missing_doxygen_comment | src/temporal/temporal_migrator.cpp:45
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/temporal/temporal_migrator.cpp:45
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (value.is_null()) return "null";
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | tensor | missing_doxygen_comment | src/tensor/hiss_structural_search.cpp:53
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/tensor/hiss_structural_search.cpp:53
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (core.data.empty()) return 0.0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | tensor | missing_doxygen_comment | src/tensor/tensor_fingerprint_graph.cpp:93
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/tensor/tensor_fingerprint_graph.cpp:93
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (a.empty() || b.empty()) return 0.0f;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | utils | missing_doxygen_comment | src/utils/audit_logger.cpp:276
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/utils/audit_logger.cpp:276
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!std::filesystem::exists(cfg_.log_path, ec)) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | analytics | missing_doxygen_comment | src/analytics/distributed_analytics.cpp:86
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/analytics/distributed_analytics.cpp:86
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (std::isnan(a) || std::isnan(b)) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | content | missing_doxygen_comment | src/content/deduplication_checker.cpp:104
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/content/deduplication_checker.cpp:104
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!storage_ || phash_hex.size() < 16) return std::nullopt;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | ethics_ai | missing_doxygen_comment | src/ethics_ai/ethics_selection_router.cpp:78
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/ethics_ai/ethics_selection_router.cpp:78
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (a.empty() || b.empty()) return 0.0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | evaluation | missing_doxygen_comment | src/evaluation/src/query_planner.cc:72
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/evaluation/src/query_planner.cc:72
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!f.isFresh(cfg.max_staleness_ms, cfg.min_residual_threshold)) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | importers | missing_doxygen_comment | src/importers/schema_inference.cpp:79
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/importers/schema_inference.cpp:79
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (a.empty() && b.empty()) return 1.0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | include | missing_doxygen_comment | include/index/learned_index.h:532
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: include/index/learned_index.h:532
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!need(4)) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | index | missing_doxygen_comment | src/index/cuda_hnsw_graph_traversal.cpp:71
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/index/cuda_hnsw_graph_traversal.cpp:71
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (denom < 1e-9f) return 1.0f;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | index | missing_doxygen_comment | src/index/index_compression.cpp:99
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/index/index_compression.cpp:99
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!bits_[combined % m_]) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | ingestion | missing_doxygen_comment | src/ingestion/ingestion_quality_judge.cpp:150
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/ingestion/ingestion_quality_judge.cpp:150
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!observer) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | llama_cpp | missing_doxygen_comment | src/llama_cpp/llama_cpp_plugin.cpp:140
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/llama_cpp/llama_cpp_plugin.cpp:140
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!model_loaded_) return std::nullopt;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | llm | missing_doxygen_comment | src/llm/llamacpp_inference_engine.cpp:183
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/llm/llamacpp_inference_engine.cpp:183
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (text.empty()) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | llm | missing_doxygen_comment | src/llm/mode_spec_loader.cpp:71
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/llm/mode_spec_loader.cpp:71
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!node || !node.IsMap()) return spec;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | network | missing_doxygen_comment | src/network/raft_load_balancer.cpp:232
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/network/raft_load_balancer.cpp:232
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!b) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | network | missing_doxygen_comment | src/network/wire_protocol_helpers.cpp:198
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/network/wire_protocol_helpers.cpp:198
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!parser.readString(request.collection)) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | observability | missing_doxygen_comment | src/observability/metric_aggregator.cpp:30
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/observability/metric_aggregator.cpp:30
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (labels.empty()) return name;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | performance | missing_doxygen_comment | src/performance/adaptive_query_compiler.cpp:152
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/performance/adaptive_query_compiler.cpp:152
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!std::holds_alternative<std::monostate>(pred.value)) return pred.value;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | performance | missing_doxygen_comment | src/performance/workload_adaptive_optimizer.cpp:185
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/performance/workload_adaptive_optimizer.cpp:185
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (adapt_running_.exchange(true)) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | projects | missing_doxygen_comment | src/projects/project_diff.cpp:53
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/projects/project_diff.cpp:53
// Problem: missing_doxygen_comment
// Description: Public declaration 'for' is missing a Doxygen comment
// Context: for (const auto& e : entries) arr.push_back(e.toJson());
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | prompt_engineering | missing_doxygen_comment | src/prompt_engineering/feedback_collector.cpp:666
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/prompt_engineering/feedback_collector.cpp:666
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!db_) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | prompt_engineering | missing_doxygen_comment | src/prompt_engineering/prompt_compressor.cpp:55
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/prompt_engineering/prompt_compressor.cpp:55
// Problem: missing_doxygen_comment
// Description: Public declaration 'while' is missing a Doxygen comment
// Context: while (ss >> word) words.push_back(word);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | query | missing_doxygen_comment | src/query/approximate_aggregator.cpp:132
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/query/approximate_aggregator.cpp:132
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!value.is_number()) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | query | missing_doxygen_comment | src/query/functions/fulltext_functions.cpp:69
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/query/functions/fulltext_functions.cpp:69
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (s.empty()) return "";
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | query | missing_doxygen_comment | src/query/materialized_view.cpp:221
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/query/materialized_view.cpp:221
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (stale_) return true;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | query | missing_doxygen_comment | src/query/mutation_executor.cpp:124
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/query/mutation_executor.cpp:124
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (err.has_value()) return *err;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | query | missing_doxygen_comment | src/query/optimizer_cost_model_enhancements.cpp:123
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/query/optimizer_cost_model_enhancements.cpp:123
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (samples.empty()) return 0.0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | query | missing_doxygen_comment | src/query/vectorized_execution.cpp:411
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/query/vectorized_execution.cpp:411
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (val.is_null()) return nullptr;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | query | missing_doxygen_comment | src/query/window_evaluator.cpp:292
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/query/window_evaluator.cpp:292
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (sortedIndices.empty()) return results;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | rag | missing_doxygen_comment | src/rag/batch_evaluator.cpp:140
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/rag/batch_evaluator.cpp:140
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (cancelled_.load()) return true;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | replication | missing_doxygen_comment | src/replication/logical_replication.cpp:78
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/replication/logical_replication.cpp:78
// Problem: missing_doxygen_comment
// Description: Public declaration 'while' is missing a Doxygen comment
// Context: while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) start++;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | scraper | missing_doxygen_comment | src/scraper/scraper_js_renderer.cpp:44
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/scraper/scraper_js_renderer.cpp:44
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (renderer_cmd_.empty()) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | scraper | missing_doxygen_comment | src/scraper/scraper_plugin.cpp:157
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/scraper/scraper_plugin.cpp:157
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!url.empty()) seeds.emplace_back(url, src->id);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | security | missing_doxygen_comment | src/security/hsm_provider.cpp:90
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/security/hsm_provider.cpp:90
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!code) return "Unknown OpenSSL error";
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | server | missing_doxygen_comment | src/server/opa_adapter.cpp:99
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/server/opa_adapter.cpp:99
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!j.contains("result")) return std::nullopt;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | server | missing_doxygen_comment | src/server/rate_limiter_v2.cpp:263
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/server/rate_limiter_v2.cpp:263
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (r) freeReplyObject(r);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | storage | missing_doxygen_comment | src/storage/base_entity.cpp:137
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/storage/base_entity.cpp:137
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!value) return std::nullopt;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | storage | missing_doxygen_comment | src/storage/blob_redundancy_manager.cpp:49
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/storage/blob_redundancy_manager.cpp:49
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (loc.is_healthy) count++;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | storage | missing_doxygen_comment | src/storage/gpu_compression.cpp:174
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/storage/gpu_compression.cpp:174
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!has_gpu_magic(compressed)) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | tensor | missing_doxygen_comment | src/tensor/tensor_index_manager.cpp:174
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/tensor/tensor_index_manager.cpp:174
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!found) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | transaction | missing_doxygen_comment | src/transaction/distributed_saga.cpp:421
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/transaction/distributed_saga.cpp:421
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!rec) return DistributedSagaStatus::Error("record not found");
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | transaction | missing_doxygen_comment | src/transaction/distributed_transaction_manager.cpp:502
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/transaction/distributed_transaction_manager.cpp:502
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!txn) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | voice | missing_doxygen_comment | src/voice/voice_batch_processor.cpp:242
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/voice/voice_batch_processor.cpp:242
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (ref_tokens.empty()) return 0.0f;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | voice | missing_doxygen_comment | src/voice/voice_meeting_support.cpp:78
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/voice/voice_meeting_support.cpp:78
// Problem: missing_doxygen_comment
// Description: Public declaration 'while' is missing a Doxygen comment
// Context: while (!current.empty() && std::isspace(static_cast<unsigned char>(current.front()))) current.erase(current.begin());
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | voice | missing_doxygen_comment | src/voice/voice_security.cpp:169
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/voice/voice_security.cpp:169
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!config_.enable_audit_logging) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | acceleration | missing_doxygen_comment | src/acceleration/cuda_backend.cpp:753
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/acceleration/cuda_backend.cpp:753
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (entries_.empty()) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | auth | missing_doxygen_comment | src/auth/redis_token_blacklist.cpp:61
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/auth/redis_token_blacklist.cpp:61
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (reply) freeReplyObject(reply);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | distributed_tensor | missing_doxygen_comment | src/distributed_tensor/src/integrity_verification.cc:86
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/distributed_tensor/src/integrity_verification.cc:86
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!isLowercaseHexCharacter(static_cast<unsigned char>(c))) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | ethics_ai | missing_doxygen_comment | src/ethics_ai/ethics_profile_registry.cpp:38
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/ethics_ai/ethics_profile_registry.cpp:38
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!node) return result;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | evaluation | missing_doxygen_comment | src/evaluation/src/retrieval_metrics.cc:200
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/evaluation/src/retrieval_metrics.cc:200
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (returned_set.count(id)) ++covered;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | governance | missing_doxygen_comment | src/governance/governance_audit_integrity.cpp:336
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/governance/governance_audit_integrity.cpp:336
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (sig_incident) return sig_incident;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | governance | missing_doxygen_comment | src/governance/operational_audit.cpp:76
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/governance/operational_audit.cpp:76
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!mdctx) return "";
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | graph | missing_doxygen_comment | src/graph/graph_query_cache.cpp:48
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/graph/graph_query_cache.cpp:48
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (key.empty()) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | importers | missing_doxygen_comment | src/importers/kafka_importer.cpp:559
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/importers/kafka_importer.cpp:559
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (url.empty()) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | importers | missing_doxygen_comment | src/importers/s3_importer.cpp:466
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/importers/s3_importer.cpp:466
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!tmp) return json::array();
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | include | missing_doxygen_comment | include/api/aql_utils.h:51
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: include/api/aql_utils.h:51
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (name.empty()) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | include | missing_doxygen_comment | include/distributed_knowledge/distributed_knowledge_api_contract.h:114
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: include/distributed_knowledge/distributed_knowledge_api_contract.h:114
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (local_ts > remote_ts) return LwwDecision::LocalWins;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | index | missing_doxygen_comment | src/index/property_graph.cpp:541
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/index/property_graph.cpp:541
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (edgeId.empty()) return true;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | ingestion | missing_doxygen_comment | src/ingestion/ingestion_coordinator.cpp:256
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/ingestion/ingestion_coordinator.cpp:256
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (deques_[idx].tasks.empty()) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | ingestion | missing_doxygen_comment | src/ingestion/ingestion_manager.cpp:118
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/ingestion/ingestion_manager.cpp:118
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!std::regex_search(json, m, key_re)) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | ingestion | missing_doxygen_comment | src/ingestion/ingestion_sinks.cpp:86
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/ingestion/ingestion_sinks.cpp:86
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!r1) return r1;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | llm | missing_doxygen_comment | src/llm/api/embedded_llm_adapter.cpp:22
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/llm/api/embedded_llm_adapter.cpp:22
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!impl) return std::string();
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | llm | missing_doxygen_comment | src/llm/lora_framework/directx_context.cpp:109
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/llm/lora_framework/directx_context.cpp:109
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!create_command_queue()) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | llm | missing_doxygen_comment | src/llm/lora_security_validator.cpp:93
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/llm/lora_security_validator.cpp:93
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (bio) BIO_free_all(bio);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | llm | missing_doxygen_comment | src/llm/model_router.cpp:86
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/llm/model_router.cpp:86
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (has_patterns && matchesAnyPattern()) return true;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | llm | missing_doxygen_comment | src/llm/security/signature_verifier.cpp:905
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/llm/security/signature_verifier.cpp:905
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!cert) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | metadata | missing_doxygen_comment | src/metadata/index_recommender.cpp:291
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/metadata/index_recommender.cpp:291
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!db_) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | metadata | missing_doxygen_comment | src/metadata/schema_consistency_checker.cpp:134
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/metadata/schema_consistency_checker.cpp:134
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (is_system_key(key_str)) return true;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | network | missing_doxygen_comment | src/network/connection_compression.cpp:93
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/network/connection_compression.cpp:93
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (samples.empty()) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | network | missing_doxygen_comment | src/network/envoy_xds.cpp:139
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/network/envoy_xds.cpp:139
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (in_str) continue;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | performance | missing_doxygen_comment | src/performance/rabitq.cpp:61
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/performance/rabitq.cpp:61
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (training_data.empty()) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | performance | missing_doxygen_comment | src/performance/workload_predictor.cpp:226
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/performance/workload_predictor.cpp:226
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (values.empty()) return 0.0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | process | missing_doxygen_comment | src/process/epk_aris_xml_importer.cpp:151
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/process/epk_aris_xml_importer.cpp:151
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (xml.size() > max_bytes) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | process | missing_doxygen_comment | src/process/process_model_manager.cpp:631
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/process/process_model_manager.cpp:631
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!doc.contains("id")) return true;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | prompt_engineering | missing_doxygen_comment | src/prompt_engineering/prompt_version_control.cpp:739
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/prompt_engineering/prompt_version_control.cpp:739
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!db_) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | query | missing_doxygen_comment | src/query/gremlin_parser.cpp:589
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/query/gremlin_parser.cpp:589
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (gremlin.empty()) return Err<GremlinASTNode>(errors::ErrorCode::ERR_QUERY_INVALID_SYNTAX, "empty Gremlin query");
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | query | missing_doxygen_comment | src/query/materialized_cte.cpp:61
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/query/materialized_cte.cpp:61
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (std::holds_alternative<std::nullptr_t>(fv)) return nullptr;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | rag | missing_doxygen_comment | src/rag/adaptive_retrieval.cpp:76
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/rag/adaptive_retrieval.cpp:76
// Problem: missing_doxygen_comment
// Description: Public declaration 'while' is missing a Doxygen comment
// Context: while (ss >> tok) tokens.push_back(tok);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | rag | missing_doxygen_comment | src/rag/continuous_learning_orchestrator.cpp:663
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/rag/continuous_learning_orchestrator.cpp:663
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (path.empty()) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | rag | missing_doxygen_comment | src/rag/learning_metrics.cpp:146
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/rag/learning_metrics.cpp:146
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (data.empty()) return 0.0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | rag | missing_doxygen_comment | src/rag/streaming_retriever.cpp:49
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/rag/streaming_retriever.cpp:49
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (ta.empty() && tb.empty()) return 1.0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | scraper | missing_doxygen_comment | src/scraper/scraper_api_client.cpp:101
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/scraper/scraper_api_client.cpp:101
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (cfg.page_size > 0) app(cfg.page_size_param, std::to_string(cfg.page_size));
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | scraper | missing_doxygen_comment | src/scraper/scraper_search_engine.cpp:62
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/scraper/scraper_search_engine.cpp:62
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (href.empty()) return base_url;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | search | missing_doxygen_comment | src/search/fuzzy_matcher.cpp:147
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/search/fuzzy_matcher.cpp:147
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (word.empty()) return "0000";
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | security | missing_doxygen_comment | src/security/cms_signing.cpp:84
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/security/cms_signing.cpp:84
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!sig_bio) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | server | missing_doxygen_comment | src/server/graph_api_handler.cpp:451
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/server/graph_api_handler.cpp:451
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!changes_array.is_array()) return cs;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | server | missing_doxygen_comment | src/server/pii_api_handler.cpp:78
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/server/pii_api_handler.cpp:78
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!db_) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | sharding | missing_doxygen_comment | src/sharding/mtls_connection_pool.cpp:218
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/sharding/mtls_connection_pool.cpp:218
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!connection) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | storage | missing_doxygen_comment | src/storage/simd_filter.cpp:405
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/storage/simd_filter.cpp:405
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (scalar_cmp(data[i], op, thr)) out.push_back(static_cast<uint32_t>(i));
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | temporal | missing_doxygen_comment | src/temporal/retention_manager.cpp:259
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/temporal/retention_manager.cpp:259
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!policy.archive_tag.empty()) return policy.archive_tag;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | themis | missing_doxygen_comment | src/themis/wire_protocol_server.cpp:415
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/themis/wire_protocol_server.cpp:415
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (disconnect_notified_) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | timeseries | missing_doxygen_comment | src/timeseries/gorilla.cpp:193
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/timeseries/gorilla.cpp:193
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (error_) return std::nullopt;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | timeseries | missing_doxygen_comment | src/timeseries/timeseries.cpp:120
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/timeseries/timeseries.cpp:120
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!db_) return results;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | training | missing_doxygen_comment | src/training/lora_adapter_merger.cpp:93
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/training/lora_adapter_merger.cpp:93
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (norm_u < 1e-12f) break;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | updates | missing_doxygen_comment | src/updates/dependency_resolver.cpp:100
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/updates/dependency_resolver.cpp:100
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (token.size() < 2) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | utils | missing_doxygen_comment | src/utils/lek_manager.cpp:356
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/utils/lek_manager.cpp:356
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!db_) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | voice | missing_doxygen_comment | src/voice/voice_intent_detector.cpp:124
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/voice/voice_intent_detector.cpp:124
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (containsAny(text, conv_kw)) return IntentCategory::CONVERSATION;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | voice | missing_doxygen_comment | src/voice/voice_session_manager.cpp:154
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/voice/voice_session_manager.cpp:154
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!timeout_config_.auto_expire) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] CRITICAL | include | exception_in_destructor | include/api/api_gateway_hook.h:99
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: General
// Location: include/api/api_gateway_hook.h:99
// Problem: exception_in_destructor
// Description: Throwing in destructor may trigger std::terminate()
// Context: n/a
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] CRITICAL | include | exception_in_destructor | include/api/api_transport_policy.h:161
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: General
// Location: include/api/api_transport_policy.h:161
// Problem: exception_in_destructor
// Description: Throwing in destructor may trigger std::terminate()
// Context: n/a
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] CRITICAL | include | exception_in_destructor | include/api/api_version_router.h:123
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: General
// Location: include/api/api_version_router.h:123
// Problem: exception_in_destructor
// Description: Throwing in destructor may trigger std::terminate()
// Context: n/a
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] CRITICAL | include | exception_in_destructor | include/api/graphql_schema_builder.h:184
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: General
// Location: include/api/graphql_schema_builder.h:184
// Problem: exception_in_destructor
// Description: Throwing in destructor may trigger std::terminate()
// Context: n/a
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] CRITICAL | include | plaintext_transmission | include/api/otlp_exporter.h:99
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: General
// Location: include/api/otlp_exporter.h:99
// Problem: plaintext_transmission
// Description: Plaintext HTTP transmission detected (should use HTTPS)
// Context: std::string endpoint       = "http://localhost:4318/v1/traces";
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | acceleration | missing_doxygen_comment | src/acceleration/directx_backend_full.cpp:668
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/acceleration/directx_backend_full.cpp:668
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | acceleration | missing_doxygen_comment | src/acceleration/kernel_registry.cpp:127
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/acceleration/kernel_registry.cpp:127
// Problem: missing_doxygen_comment
// Description: Public declaration 'for' is missing a Doxygen comment
// Context: for (const auto& [bt, _] : annDispatch_) addIfAbsent(bt);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | analytics | missing_doxygen_comment | src/analytics/nlp_text_analyzer.cpp:1294
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/analytics/nlp_text_analyzer.cpp:1294
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (ends_with("és", 3)) return strip(3, "er");
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | auth | missing_doxygen_comment | src/auth/rate_limiter_backend.cpp:150
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/auth/rate_limiter_backend.cpp:150
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (reply) freeReplyObject(reply);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | exporters | missing_doxygen_comment | src/exporters/parquet_exporter.cpp:654
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/exporters/parquet_exporter.cpp:654
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!tbl.ok()) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | geo | missing_doxygen_comment | src/geo/geo_operator_diagnostics.cpp:156
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/geo/geo_operator_diagnostics.cpp:156
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (has_suffix("FALLBACK")) return GeoIncidentSeverity::WARNING;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | governance | missing_doxygen_comment | src/governance/compliance_reporting.cpp:1286
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/governance/compliance_reporting.cpp:1286
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!needs_quoting) return val;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | gpu | missing_doxygen_comment | src/gpu/gpu_memory_allocator.cpp:190
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/gpu/gpu_memory_allocator.cpp:190
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (device_ptr) cudaFree(device_ptr);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | gpu | missing_doxygen_comment | src/gpu/gpu_resource_handles.cpp:57
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/gpu/gpu_resource_handles.cpp:57
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!stream_) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | importers | missing_doxygen_comment | src/importers/mdm_engine.cpp:45
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/importers/mdm_engine.cpp:45
// Problem: missing_doxygen_comment
// Description: Public declaration 'for' is missing a Doxygen comment
// Context: for (const auto& g : golden_records) gr_arr.push_back(g.toJson());
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | include | chimera_adapter_missing_interface | include/chimera/batch_executor.hpp:77
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: ArchitectureContract
// Location: include/chimera/batch_executor.hpp:77
// Problem: chimera_adapter_missing_interface
// Description: Chimera adapter 'IBatchAdapter' should implement one of the required adapter interfaces
// Context: class IBatchAdapter {
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | include | layer_dependency_violation | include/core/query_engine_builder.h:37
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: ArchitectureContract
// Location: include/core/query_engine_builder.h:37
// Problem: layer_dependency_violation
// Description: Module 'core' must not depend on 'query' (layer violation)
// Context: #include "query/query_engine.h"
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | include | missing_doxygen_comment | include/ethics_ai/ethics_ai_types.h:609
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: include/ethics_ai/ethics_ai_types.h:609
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (score > 0.75) return MetaVerdict::ConvergenceVerdict::CLEAR_CONSENSUS;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | include | layer_dependency_violation | include/llm/ai_orchestrator.h:744
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: ArchitectureContract
// Location: include/llm/ai_orchestrator.h:744
// Problem: layer_dependency_violation
// Description: Module 'llm' must not depend on 'server' (layer violation)
// Context: * #include "server/mcp_server.h"
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | include | missing_doxygen_comment | include/performance/rcu.h:179
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: include/performance/rcu.h:179
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!ptr) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | index | missing_doxygen_comment | src/index/gpu_memory_oversubscription.cpp:349
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/index/gpu_memory_oversubscription.cpp:349
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!pImpl_) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | ingestion | missing_doxygen_comment | src/ingestion/api_connector.cpp:81
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/ingestion/api_connector.cpp:81
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (headers) curl_slist_free_all(headers);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | ingestion | missing_doxygen_comment | src/ingestion/cdc_connector.cpp:172
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/ingestion/cdc_connector.cpp:172
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (s.empty()) return 0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | ingestion | missing_doxygen_comment | src/ingestion/steps/ner_step.cpp:108
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/ingestion/steps/ner_step.cpp:108
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!arr.is_array()) return out;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | llm | missing_doxygen_comment | src/llm/ai_orchestrator.cpp:511
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/llm/ai_orchestrator.cpp:511
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!res) return tl::unexpected(res.error());
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | llm | missing_doxygen_comment | src/llm/byzantine_detector.cpp:109
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/llm/byzantine_detector.cpp:109
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (values.empty()) return 0.0f;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | llm | missing_doxygen_comment | src/llm/json_schema_converter.cpp:324
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/llm/json_schema_converter.cpp:324
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (text.empty()) return std::nullopt;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | llm | missing_doxygen_comment | src/llm/lora_certificate_store.cpp:190
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/llm/lora_certificate_store.cpp:190
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (cert_pem.empty() || fingerprint.empty()) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | llm | missing_doxygen_comment | src/llm/vision_encoder.cpp:473
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/llm/vision_encoder.cpp:473
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!config_) return true;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | maintenance | missing_doxygen_comment | src/maintenance/database_maintenance_orchestrator.cpp:167
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/maintenance/database_maintenance_orchestrator.cpp:167
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!running_.exchange(false)) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | metadata | missing_doxygen_comment | src/metadata/schema_version_manager.cpp:262
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/metadata/schema_version_manager.cpp:262
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!r_a.ok) return VersionResult<json>::failure(r_a.error, r_a.error_message);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | network | missing_doxygen_comment | src/network/geo_topology_router.cpp:75
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/network/geo_topology_router.cpp:75
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (healthy.empty()) return nullptr;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | network | missing_doxygen_comment | src/network/wire_protocol_batch.cpp:50
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/network/wire_protocol_batch.cpp:50
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (fd_ < 0) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | network | missing_doxygen_comment | src/network/wire_protocol_performance.cpp:62
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/network/wire_protocol_performance.cpp:62
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!kind) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | plugins | missing_doxygen_comment | src/plugins/oci_registry_client.cpp:434
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/plugins/oci_registry_client.cpp:434
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (std::regex_search(www_auth, m, re)) return m[1].str();
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | plugins | missing_doxygen_comment | src/plugins/plugin_manager.cpp:48
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/plugins/plugin_manager.cpp:48
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!handle) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | process | missing_doxygen_comment | src/process/cmmn_serializer.cpp:123
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/process/cmmn_serializer.cpp:123
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (xml.size() > kMaxCmmnXmlBytes) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | query | missing_doxygen_comment | src/query/functions/tensor_functions.cpp:39
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/query/functions/tensor_functions.cpp:39
// Problem: missing_doxygen_comment
// Description: Public declaration 'for' is missing a Doxygen comment
// Context: for (const auto& v : arr) out.push_back(v.get<float>());
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | rag | missing_doxygen_comment | src/rag/calibration_manager.cpp:356
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/rag/calibration_manager.cpp:356
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (predictions.empty()) return 0.0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | rag | missing_doxygen_comment | src/rag/llm_judge_integration.cpp:126
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/rag/llm_judge_integration.cpp:126
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!r.empty()) return r;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | rag | missing_doxygen_comment | src/rag/lora_enhanced_retriever.cpp:52
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/rag/lora_enhanced_retriever.cpp:52
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (A.empty() && B.empty()) return 1.0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | rag | missing_doxygen_comment | src/rag/multi_hop_reasoner.cpp:122
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/rag/multi_hop_reasoner.cpp:122
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (q.empty()) return parts;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | rag | missing_doxygen_comment | src/rag/multi_step_rag.cpp:139
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/rag/multi_step_rag.cpp:139
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (llm_response.empty()) return aspects;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | rag | missing_doxygen_comment | src/rag/self_rag.cpp:56
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/rag/self_rag.cpp:56
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (q_tokens.empty()) return 0.0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | replication | missing_doxygen_comment | src/replication/replication_slot.cpp:247
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/replication/replication_slot.cpp:247
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!ofs.is_open()) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | scheduler | missing_doxygen_comment | src/scheduler/task_audit_manager.cpp:306
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/scheduler/task_audit_manager.cpp:306
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (line.empty()) continue;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | search | missing_doxygen_comment | src/search/llm_query_rewriter.cpp:255
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/search/llm_query_rewriter.cpp:255
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (ta.empty() && tb.empty()) return 1.0f;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | security | missing_doxygen_comment | src/security/ai_snapshot_cleanup.cpp:54
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/security/ai_snapshot_cleanup.cpp:54
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (ec) break;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | server | missing_doxygen_comment | src/server/rate_limiter.cpp:138
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/server/rate_limiter.cpp:138
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!it->second.under_penalty) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | sharding | missing_doxygen_comment | src/sharding/gossip_consensus_adapter.cpp:98
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/sharding/gossip_consensus_adapter.cpp:98
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (cluster_nodes_.empty()) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | sharding | missing_doxygen_comment | src/sharding/paxos_state_persistence.cpp:241
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/sharding/paxos_state_persistence.cpp:241
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (config_.sync_on_write) wal_->flush();
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | sharding | missing_doxygen_comment | src/sharding/redundancy_strategy.cpp:283
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/sharding/redundancy_strategy.cpp:283
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!chunk.shard_id.empty()) available++;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | storage | missing_doxygen_comment | src/storage/ggml_tensor_bridge.cpp:147
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/storage/ggml_tensor_bridge.cpp:147
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!ctx) return nullptr;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | storage | missing_doxygen_comment | src/storage/storage_engine.cpp:494
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/storage/storage_engine.cpp:494
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (stopped_early) sc_early_stops_.fetch_add(1, std::memory_order_relaxed);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | storage | missing_doxygen_comment | src/storage/tensor_compaction_filter.cpp:94
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/storage/tensor_compaction_filter.cpp:94
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (key.size() < kTTCorePrefixLen) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | storage | missing_doxygen_comment | src/storage/tensor_train_decomposer.cpp:150
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/storage/tensor_train_decomposer.cpp:150
// Problem: missing_doxygen_comment
// Description: Public declaration 'for' is missing a Doxygen comment
// Context: for (float f : c.data) writeF32(f);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | storage | missing_doxygen_comment | src/storage/wal_storage.cpp:557
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/storage/wal_storage.cpp:557
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!res) return res;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | temporal | missing_doxygen_comment | src/temporal/bitemporal_join.cpp:99
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/temporal/bitemporal_join.cpp:99
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (config_.apply_sys_time_predicate) return overlaps(l.sys_time, r.sys_time) && overlaps(l.valid_time, r.valid_time);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | temporal | missing_doxygen_comment | src/temporal/temporal_tier_manager.cpp:221
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/temporal/temporal_tier_manager.cpp:221
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (result) return result;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | timeseries | missing_doxygen_comment | src/timeseries/adaptive_flush_controller.cpp:120
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/timeseries/adaptive_flush_controller.cpp:120
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (p.metric.empty()) return "metric name cannot be empty";
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | timeseries | missing_doxygen_comment | src/timeseries/gorilla_simd.cpp:232
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/timeseries/gorilla_simd.cpp:232
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (data_.empty()) return 0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | timeseries | missing_doxygen_comment | src/timeseries/ts_operator_diagnostics.cpp:148
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/timeseries/ts_operator_diagnostics.cpp:148
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (has_suffix("CRITICAL") || contains_sub("PERSISTENT")) return TsIncidentSeverity::CRITICAL;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | training | missing_doxygen_comment | src/training/lora_checkpoint_manager.cpp:66
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/training/lora_checkpoint_manager.cpp:66
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!std::isxdigit(static_cast<unsigned char>(c)) || (std::isupper(static_cast<unsigned char>(c)))) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | user_storage_encrypted | missing_doxygen_comment | src/user_storage_encrypted/key_rotation_scheduler.cpp:255
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/user_storage_encrypted/key_rotation_scheduler.cpp:255
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!impl_->store) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | voice | missing_doxygen_comment | src/voice/voice_model_cache.cpp:190
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/voice/voice_model_cache.cpp:190
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!evictLRUOne()) break;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | voice | missing_doxygen_comment | src/voice/voice_tts_customizer.cpp:446
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/voice/voice_tts_customizer.cpp:446
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (audio.size() < 2) return 0.0f;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | acceleration | missing_doxygen_comment | src/acceleration/faiss_gpu_backend.cpp:274
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/acceleration/faiss_gpu_backend.cpp:274
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!index_) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | acceleration | missing_doxygen_comment | src/acceleration/plugin_security.cpp:98
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/acceleration/plugin_security.cpp:98
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!curl) return result;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | acceleration | missing_doxygen_comment | src/acceleration/vllm_resource_manager.cpp:190
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/acceleration/vllm_resource_manager.cpp:190
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!util.has_value()) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | acceleration | missing_doxygen_comment | src/acceleration/zluda_backend.cpp:35
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/acceleration/zluda_backend.cpp:35
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!handle) return nullptr;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | analytics | missing_doxygen_comment | src/analytics/analytics_engine.cpp:153
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/analytics/analytics_engine.cpp:153
// Problem: missing_doxygen_comment
// Description: Public declaration 'sleep_for' is missing a Doxygen comment
// Context: std::this_thread::sleep_for(std::chrono::milliseconds(100 * (retry_count + 1)));
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | cache | missing_doxygen_comment | src/cache/redis_cache_coordinator.cpp:394
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/cache/redis_cache_coordinator.cpp:394
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (r) freeReplyObject(r);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | evaluation | missing_doxygen_comment | src/evaluation/src/ablation_framework.cc:106
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/evaluation/src/ablation_framework.cc:106
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!ra || !rb) return std::nullopt;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | exporters | missing_doxygen_comment | src/exporters/huggingface_hub_client.cpp:277
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/exporters/huggingface_hub_client.cpp:277
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!curl) return 0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | governance | missing_doxygen_comment | src/governance/policy_conflict_detector.cpp:163
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/governance/policy_conflict_detector.cpp:163
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (pattern.empty()) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | importers | missing_doxygen_comment | src/importers/debezium_cdc_importer.cpp:212
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/importers/debezium_cdc_importer.cpp:212
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (config_.table_filter.empty()) return true;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | importers | missing_doxygen_comment | src/importers/elasticsearch_importer.cpp:410
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/importers/elasticsearch_importer.cpp:410
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (scroll_id.empty()) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | importers | missing_doxygen_comment | src/importers/gui_import_wizard.cpp:324
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/importers/gui_import_wizard.cpp:324
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (on_progress) on_progress(s);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | importers | missing_doxygen_comment | src/importers/wikipedia_transform.cpp:39
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/importers/wikipedia_transform.cpp:39
// Problem: missing_doxygen_comment
// Description: Public declaration 'erase' is missing a Doxygen comment
// Context: text.erase(text.begin(), std::find_if(text.begin(), text.end(), not_space));
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | include | pointer_arithmetic_unbounded | include/api/audit_logger.h:448
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: MemorySafety
// Location: include/api/audit_logger.h:448
// Problem: pointer_arithmetic_unbounded
// Description: Pointer/array access without visible bounds check
// Context: }            AuditLogBuilder& metadata(const std::string& key, const std::string& value) {          entry_.metadata[k...
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | include | unchecked_result | include/api/graphql_schema_builder.h:30
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: General
// Location: include/api/graphql_schema_builder.h:30
// Problem: unchecked_result
// Description: Result/Error variable created but never checked
// Context: * auto builder = createGraphQLSchemaBuilder();
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | include | missing_noexcept_on_move | include/api/graphql_ws_handler.h:147
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: General
// Location: include/api/graphql_ws_handler.h:147
// Problem: missing_noexcept_on_move
// Description: Move assignment should be noexcept when possible
// Context: n/a
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | include | no_retry_logic | include/api/grpc_bridge.h:162
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: ReliabilityRetry
// Location: include/api/grpc_bridge.h:162
// Problem: no_retry_logic
// Description: RPC/network call without retry logic — transient failures will propagate
// Context: * @brief Dispatch an inbound gRPC request to the registered handler.       *       * Converts `request` into an `Http...
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | include | missing_noexcept_on_move | include/api/otlp_exporter.h:181
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: General
// Location: include/api/otlp_exporter.h:181
// Problem: missing_noexcept_on_move
// Description: Move assignment should be noexcept when possible
// Context: n/a
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | include | uninitialized_variable | include/api/rate_limiter.h:114
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: General
// Location: include/api/rate_limiter.h:114
// Problem: uninitialized_variable
// Description: Variable used before visible initialization
// Context: tokens = std::min(static_cast<double>(capacity), tokens + tokens_to_add);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | include | missing_doxygen_comment | include/config/config_migration_scanner_impl.h:85
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: include/config/config_migration_scanner_impl.h:85
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!timestamp.has_value()) return "";
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | include | missing_doxygen_comment | include/core/concerns/i_context.h:240
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: include/core/concerns/i_context.h:240
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!trace_id.empty()) ctx->set(context_keys::kTraceId, trace_id);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | include | missing_doxygen_comment | include/llm/infini_attention_cpu.h:155
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: include/llm/infini_attention_cpu.h:155
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!initialized_) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | include | missing_doxygen_comment | include/llm/ssm_stub_plugin.h:116
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: include/llm/ssm_stub_plugin.h:116
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!initialized_) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | include | missing_doxygen_comment | include/performance/allocator.h:65
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: include/performance/allocator.h:65
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!ptr) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | include | missing_doxygen_comment | include/performance/huge_pages.h:60
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: include/performance/huge_pages.h:60
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!fp) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | include | missing_doxygen_comment | include/scraper/scraper_render_contract.h:83
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: include/scraper/scraper_render_contract.h:83
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (needle.empty()) return true;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | include | missing_doxygen_comment | include/security/pkcs11_wrapper.h:650
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: include/security/pkcs11_wrapper.h:650
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!api || !session.isOpen() || !publicKey) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | include | missing_doxygen_comment | include/server/route_version_router.h:205
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: include/server/route_version_router.h:205
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (isVersioned(path)) return true;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | include | missing_doxygen_comment | include/utils/openssl_deleter.h:186
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: include/utils/openssl_deleter.h:186
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!bio) return X509Ptr(nullptr);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | index | missing_doxygen_comment | src/index/index_manager.cpp:226
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/index/index_manager.cpp:226
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (s.empty() || s.size() > 512) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | index | missing_doxygen_comment | src/index/tiered_index_manager.cpp:80
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/index/tiered_index_manager.cpp:80
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (name.empty()) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | ingestion | missing_doxygen_comment | src/ingestion/huggingface_connector.cpp:76
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/ingestion/huggingface_connector.cpp:76
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (headers) curl_slist_free_all(headers);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | ingestion | missing_doxygen_comment | src/ingestion/legal_domain.cpp:527
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/ingestion/legal_domain.cpp:527
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!r.empty()) return r;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | llm | missing_doxygen_comment | src/llm/aql_train_parser.cpp:121
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/llm/aql_train_parser.cpp:121
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (left_ok && right_ok) return pos;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | llm | missing_doxygen_comment | src/llm/ethics_aware_confidence_detector.cpp:520
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/llm/ethics_aware_confidence_detector.cpp:520
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (tokens.empty()) return 0.0f;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | llm | missing_doxygen_comment | src/llm/gpu_memory_manager.cpp:853
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/llm/gpu_memory_manager.cpp:853
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!ptr) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | llm | missing_doxygen_comment | src/llm/llama_resource_manager.cpp:81
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/llm/llama_resource_manager.cpp:81
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!model_) return "none";
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | llm | missing_doxygen_comment | src/llm/llm_deployment_plugin.cpp:641
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/llm/llm_deployment_plugin.cpp:641
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (status.is_loaded) continue;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | llm | missing_doxygen_comment | src/llm/llm_response_cache.cpp:208
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/llm/llm_response_cache.cpp:208
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (metrics_collector_) metrics_collector_->recordCacheHit(cache_name_);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | llm | missing_doxygen_comment | src/llm/lookup_decoder.cpp:61
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/llm/lookup_decoder.cpp:61
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (new_tokens.empty()) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | llm | missing_doxygen_comment | src/llm/lora_framework/feedback_plugin.cpp:105
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/llm/lora_framework/feedback_plugin.cpp:105
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (text.empty()) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | llm | missing_doxygen_comment | src/llm/lora_framework/llama_tokenizer.cpp:89
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/llm/lora_framework/llama_tokenizer.cpp:89
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (add_bos) tokens.push_back(bos_token_id());
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | llm | missing_doxygen_comment | src/llm/lora_framework/resource_profiler.cpp:139
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/llm/lora_framework/resource_profiler.cpp:139
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (impl_->snapshots.empty()) return 0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | llm | missing_doxygen_comment | src/llm/production_validator.cpp:1581
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/llm/production_validator.cpp:1581
// Problem: missing_doxygen_comment
// Description: Public declaration 'for' is missing a Doxygen comment
// Context: for (const auto& id : all_ids) scheduler.cancelRequest(id);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | main_server.cpp | missing_doxygen_comment | src/main_server.cpp:992
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/main_server.cpp:992
// Problem: missing_doxygen_comment
// Description: Public declaration 'for' is missing a Doxygen comment
// Context: for (const auto& it : n) arr.push_back(to_json(it));
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | metadata | missing_doxygen_comment | src/metadata/schema_constraints.cpp:564
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/metadata/schema_constraints.cpp:564
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!t_obj.is_object()) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | metadata | missing_doxygen_comment | src/metadata/statistics_collector.cpp:211
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/metadata/statistics_collector.cpp:211
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (metrics_hook_) metrics_hook_->onError(table_name, static_cast<int>(StatsErrorCode::TABLE_NOT_FOUND));
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | network | missing_doxygen_comment | src/network/bbr_congestion_control.cpp:164
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/network/bbr_congestion_control.cpp:164
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (probe_fd < 0) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | network | missing_doxygen_comment | src/network/io_uring_batcher.cpp:84
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/network/io_uring_batcher.cpp:84
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!batcher.pending()) return true;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | network | missing_doxygen_comment | src/network/multipath_tcp.cpp:230
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/network/multipath_tcp.cpp:230
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!mptcp_enabled_) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | observability | missing_doxygen_comment | src/observability/slo_reporter.cpp:315
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/observability/slo_reporter.cpp:315
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (s.ts < cutoff) continue;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | observability | missing_doxygen_comment | src/observability/tenant_metrics_namespace.cpp:157
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/observability/tenant_metrics_namespace.cpp:157
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (config_.strict_tenant_registration) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | performance | missing_doxygen_comment | src/performance/phase3/diskann.cpp:233
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/performance/phase3/diskann.cpp:233
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!ofs) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | process | missing_doxygen_comment | src/process/fim_importer.cpp:138
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/process/fim_importer.cpp:138
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (xml.size() > kMaxSize) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | process | missing_doxygen_comment | src/process/process_linker.cpp:278
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/process/process_linker.cpp:278
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (doc.value("deleted", false)) return true;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | prompt_engineering | missing_doxygen_comment | src/prompt_engineering/adversarial_prompt_tester.cpp:80
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/prompt_engineering/adversarial_prompt_tester.cpp:80
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (needle.empty()) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | prompt_engineering | missing_doxygen_comment | src/prompt_engineering/llm_reflection_adapter.cpp:52
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/prompt_engineering/llm_reflection_adapter.cpp:52
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!llm_) return response;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | prompt_engineering | missing_doxygen_param | src/prompt_engineering/prompt_engineering_metrics.cpp:788
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/prompt_engineering/prompt_engineering_metrics.cpp:788
// Problem: missing_doxygen_param
// Description: Doxygen comment for 'if' is missing @param for: enabled
// Context: if (!config_.enabled) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | prompt_engineering | missing_doxygen_return | src/prompt_engineering/prompt_engineering_metrics.cpp:788
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/prompt_engineering/prompt_engineering_metrics.cpp:788
// Problem: missing_doxygen_return
// Description: Doxygen comment for 'if' is missing @return
// Context: if (!config_.enabled) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | prompt_engineering | missing_doxygen_comment | src/prompt_engineering/prompt_performance_tracker.cpp:273
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/prompt_engineering/prompt_performance_tracker.cpp:273
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!db_) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | prompt_engineering | missing_doxygen_comment | src/prompt_engineering/prompt_template_compiler.cpp:58
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/prompt_engineering/prompt_template_compiler.cpp:58
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (i > 0) oss << '\n';
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | prompt_engineering | missing_doxygen_comment | src/prompt_engineering/reflection_tuner.cpp:384
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/prompt_engineering/reflection_tuner.cpp:384
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (steps.empty()) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | prompt_engineering | missing_doxygen_comment | src/prompt_engineering/structured_output.cpp:183
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/prompt_engineering/structured_output.cpp:183
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (in_string) continue;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | query | missing_doxygen_comment | src/query/aql_mutation_validator.cpp:21
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/query/aql_mutation_validator.cpp:21
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (name.empty() || name.size() > 256) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | query | missing_doxygen_comment | src/query/aql_parser.cpp:1662
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/query/aql_parser.cpp:1662
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!match(TokenType::RETURN)) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | query | missing_doxygen_comment | src/query/cypher_parser.cpp:1177
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/query/cypher_parser.cpp:1177
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (ast.return_distinct) aql << "DISTINCT ";
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | query | missing_doxygen_comment | src/query/geospatial_optimizer_hints.cpp:26
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/query/geospatial_optimizer_hints.cpp:26
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (fieldName.empty()) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | query | missing_doxygen_comment | src/query/parallel_executor.cpp:151
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/query/parallel_executor.cpp:151
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (filter(e)) out.push_back(e);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | query | missing_doxygen_comment | src/query/sql_parser.cpp:115
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/query/sql_parser.cpp:115
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (i > 0) oss << ", ";
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | query | missing_doxygen_comment | src/query/tensor_aware_query_optimizer.cpp:208
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/query/tensor_aware_query_optimizer.cpp:208
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (child) rewriteNode(*child);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | replication | missing_doxygen_comment | src/replication/policy.cpp:47
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/replication/policy.cpp:47
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!r.datacenter.empty()) dcs.insert(r.datacenter);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | replication | missing_doxygen_comment | src/replication/schema_cdc.cpp:98
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/replication/schema_cdc.cpp:98
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (started_) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | rpc_grpc | missing_doxygen_comment | src/rpc_grpc/grpc_plugin.cpp:455
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/rpc_grpc/grpc_plugin.cpp:455
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!access_log_sink_) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | scheduler | missing_doxygen_comment | src/scheduler/task_anomaly_detector.cpp:623
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/scheduler/task_anomaly_detector.cpp:623
// Problem: missing_doxygen_comment
// Description: Public declaration 'for' is missing a Doxygen comment
// Context: for (double v : d) arr.push_back(v);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | scraper | missing_doxygen_comment | src/scraper/gov_source_catalog.cpp:71
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/scraper/gov_source_catalog.cpp:71
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (s.enabled) result.push_back(&s);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | search | missing_doxygen_comment | src/search/learning_to_rank.cpp:100
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/search/learning_to_rank.cpp:100
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (clicks_.empty()) return 0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | search | missing_doxygen_comment | src/search/query_expander.cpp:297
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/search/query_expander.cpp:297
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (i) oss << ' ';
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | search | missing_doxygen_comment | src/search/search_highlighter.cpp:68
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/search/search_highlighter.cpp:68
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (offsets.empty()) return text;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | security | missing_doxygen_comment | src/security/malware_scanner.cpp:542
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/security/malware_scanner.cpp:542
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (filename.empty()) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | security | missing_doxygen_comment | src/security/secret_manager.cpp:129
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/security/secret_manager.cpp:129
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!ver) return std::nullopt;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | security | missing_doxygen_comment | src/security/security_evidence_collector.cpp:97
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/security/security_evidence_collector.cpp:97
// Problem: missing_doxygen_comment
// Description: Public declaration 'for' is missing a Doxygen comment
// Context: for (const auto& r : key_rotation_log) rotations.push_back(r.toJson());
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | security | missing_doxygen_comment | src/security/usb_volume_hardening.cpp:54
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/security/usb_volume_hardening.cpp:54
// Problem: missing_doxygen_comment
// Description: Public declaration 'erase' is missing a Doxygen comment
// Context: s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | server | missing_doxygen_comment | src/server/compliance_reporting_api_handler.cpp:178
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/server/compliance_reporting_api_handler.cpp:178
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: else if (body.contains("window_start_ms") && !body["window_start_ms"].is_number_integer()) return makeErrorResponse(h...
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | server | missing_doxygen_comment | src/server/postgres_session.cpp:1956
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/server/postgres_session.cpp:1956
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (i > 0) return_clause_oss << ", ";
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | server | missing_doxygen_comment | src/server/ranger_adapter.cpp:201
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/server/ranger_adapter.cpp:201
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (rangerJson.contains("policyItems")) pushPolicies(rangerJson["policyItems"], resources, true);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | sharding | missing_doxygen_comment | src/sharding/admin_api.cpp:166
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/sharding/admin_api.cpp:166
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!bio) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | sharding | missing_doxygen_comment | src/sharding/cross_shard_transaction.cpp:2667
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/sharding/cross_shard_transaction.cpp:2667
// Problem: missing_doxygen_comment
// Description: Public declaration 'sleep_for' is missing a Doxygen comment
// Context: std::this_thread::sleep_for( std::chrono::milliseconds(rpc_config.retry_delay_ms * (1 << retries)) );
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | sharding | missing_doxygen_comment | src/sharding/secure_transport_client.cpp:244
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/sharding/secure_transport_client.cpp:244
// Problem: missing_doxygen_comment
// Description: Public declaration 'sleep_for' is missing a Doxygen comment
// Context: std::this_thread::sleep_for(std::chrono::milliseconds(delay));
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | sharding | missing_doxygen_comment | src/sharding/shard_repair_engine.cpp:375
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/sharding/shard_repair_engine.cpp:375
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!running_.load()) break;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | sharding | missing_doxygen_comment | src/sharding/two_phase_commit_coordinator.cpp:206
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/sharding/two_phase_commit_coordinator.cpp:206
// Problem: missing_doxygen_comment
// Description: Public declaration 'for' is missing a Doxygen comment
// Context: for (auto& [s, _] : ops_per_shard) v.push_back(s);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | sharding | missing_doxygen_comment | src/sharding/two_phase_commit_participant.cpp:379
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/sharding/two_phase_commit_participant.cpp:379
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!wal_) return 0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | storage | missing_doxygen_comment | src/storage/columnar_format.cpp:355
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/storage/columnar_format.cpp:355
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (data.empty()) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | storage | missing_doxygen_comment | src/storage/concurrent_write_controller.cpp:66
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/storage/concurrent_write_controller.cpp:66
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (requested > 0) return requested;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | storage | missing_doxygen_comment | src/storage/index_analyzer.cpp:94
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/storage/index_analyzer.cpp:94
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!node || !node.IsMap()) return t;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | storage | missing_doxygen_comment | src/storage/storage_audit_logger.cpp:227
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/storage/storage_audit_logger.cpp:227
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!rot.has_value()) return rot;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | temporal | missing_doxygen_comment | src/temporal/system_versioned_table.cpp:302
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/temporal/system_versioned_table.cpp:302
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (v.isCurrent()) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | temporal | missing_doxygen_comment | src/temporal/temporal_compressor.cpp:228
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/temporal/temporal_compressor.cpp:228
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (series.empty()) return nlohmann::json::object();
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | tensor | missing_doxygen_comment | src/tensor/adapter_repository.cpp:177
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/tensor/adapter_repository.cpp:177
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!existed) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | tensor | missing_doxygen_comment | src/tensor/tensor_butterfly_operator.cpp:155
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/tensor/tensor_butterfly_operator.cpp:155
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (n < 2) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | tensor | missing_doxygen_comment | src/tensor/tensor_redundancy_detection.cpp:25
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/tensor/tensor_redundancy_detection.cpp:25
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (a.empty() || b.empty()) return 0.0f;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | tensor | missing_doxygen_comment | src/tensor/utr_converter.cpp:184
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/tensor/utr_converter.cpp:184
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!seg.empty()) segments.push_back(seg);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | themis | missing_doxygen_comment | src/themis/license_info.cpp:453
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/themis/license_info.cpp:453
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!ifa->ifa_name) continue;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | timeseries | missing_doxygen_comment | src/timeseries/anomaly_detection.cpp:97
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/timeseries/anomaly_detection.cpp:97
// Problem: missing_doxygen_comment
// Description: Public declaration 'for' is missing a Doxygen comment
// Context: for (const auto& p : points) vals.push_back(p.value);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | timeseries | missing_doxygen_comment | src/timeseries/retention.cpp:41
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/timeseries/retention.cpp:41
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!store_) return 0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | timeseries | missing_doxygen_comment | src/timeseries/ts_auto_buffer.cpp:538
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/timeseries/ts_auto_buffer.cpp:538
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (wal_path.empty()) return true;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | timeseries | missing_doxygen_comment | src/timeseries/tsstore.cpp:1172
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/timeseries/tsstore.cpp:1172
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (metric.empty()) return 0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | transaction | missing_doxygen_comment | src/transaction/branch_manager.cpp:815
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/transaction/branch_manager.cpp:815
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!it_result) return result;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | transaction | missing_doxygen_comment | src/transaction/global_transaction_manager.cpp:328
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/transaction/global_transaction_manager.cpp:328
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!wal_) return 0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | transaction | missing_doxygen_comment | src/transaction/snapshot_manager.cpp:346
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/transaction/snapshot_manager.cpp:346
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (s.has_value()) snapshots.push_back(*s);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | updates | missing_doxygen_comment | src/updates/coordinated_update_manager.cpp:126
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/updates/coordinated_update_manager.cpp:126
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (s.is_local) return &s;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | utils | missing_doxygen_comment | src/utils/consistent_hash.cpp:58
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/utils/consistent_hash.cpp:58
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (nodes_.count(node)) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | utils | missing_doxygen_comment | src/utils/pii_pseudonymizer.cpp:200
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/utils/pii_pseudonymizer.cpp:200
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!txn) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | utils | missing_doxygen_comment | src/utils/update_checker.cpp:83
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/utils/update_checker.cpp:83
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (prerelease.empty() && !other.prerelease.empty()) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | utils | missing_doxygen_comment | src/utils/utils_adapters.cpp:396
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/utils/utils_adapters.cpp:396
// Problem: missing_doxygen_comment
// Description: Public declaration 'for' is missing a Doxygen comment
// Context: for (auto& s : stages_) snapshot.push_back(s.get());
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | voice | missing_doxygen_comment | src/voice/voice_audio_storage.cpp:222
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/voice/voice_audio_storage.cpp:222
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (query.empty()) return result;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | voice | missing_doxygen_comment | src/voice/voice_macro_manager.cpp:427
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/voice/voice_macro_manager.cpp:427
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!m.enabled) continue;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | voice | missing_doxygen_comment | src/voice/wake_word_detector.cpp:274
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/voice/wake_word_detector.cpp:274
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (samples.empty()) return 0.0f;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | whisper | missing_doxygen_comment | src/whisper/whisper_transcriber.cpp:172
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/whisper/whisper_transcriber.cpp:172
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (initialized_) return true;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] CRITICAL | tests | braces_imbalance | tests/api/test_api_auth_config.cpp:1
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Concurrency
// Location: tests/api/test_api_auth_config.cpp:1
// Problem: braces_imbalance
// Description: Brace imbalance detected: 7 opening braces, 5 closing braces (diff: +2)
// Context: Total opens: 7, Total closes: 5
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] CRITICAL | tests | unhandled_critical_operation | tests/api/test_api_observability.cpp:111
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: General
// Location: tests/api/test_api_observability.cpp:111
// Problem: unhandled_critical_operation
// Description: Unhandled error-prone operation 'lock' in critical path
// Context: std::unique_lock<std::mutex> lock(metrics_lock_);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | analytics | missing_doxygen_comment | src/analytics/knowledge_base.cpp:304
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/analytics/knowledge_base.cpp:304
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!seq || !seq.IsSequence()) return triples;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | analytics | missing_doxygen_comment | src/analytics/process_mining.cpp:345
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/analytics/process_mining.cpp:345
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (i > 0) oss << ",";
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | auth | missing_doxygen_comment | src/auth/auth_rate_limiter.cpp:540
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/auth/auth_rate_limiter.cpp:540
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (ex) freeReplyObject(ex);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | auth | missing_doxygen_comment | src/auth/auth_worker_thread_pool.cpp:79
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/auth/auth_worker_thread_pool.cpp:79
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (shutdown_) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | base | missing_doxygen_comment | src/base/module_loader.cpp:200
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/base/module_loader.cpp:200
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!handle) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | base | missing_doxygen_comment | src/base/module_sandbox.cpp:603
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/base/module_sandbox.cpp:603
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!platform_->cgroup_v2_active || platform_->cgroup_path.empty()) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | cache | missing_doxygen_comment | src/cache/semantic_cache.cpp:293
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/cache/semantic_cache.cpp:293
// Problem: missing_doxygen_comment
// Description: Public declaration 'while' is missing a Doxygen comment
// Context: } while (!entry_count_.compare_exchange_weak(expected, desired, std::memory_order_relaxed, std::memory_order_relaxed));
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | content | missing_doxygen_comment | src/content/content_fs.cpp:51
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/content/content_fs.cpp:51
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!mdctx) return "";
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | content | missing_doxygen_comment | src/content/content_metrics.cpp:144
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/content/content_metrics.cpp:144
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (sorted_samples.empty()) return 0.0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | content | missing_doxygen_comment | src/content/image_processor.cpp:302
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/content/image_processor.cpp:302
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (blob.size() < 24) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | content | missing_doxygen_comment | src/content/ocr_processor.cpp:132
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/content/ocr_processor.cpp:132
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (blob.empty()) return "";
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | content | missing_doxygen_comment | src/content/pdf_processor.cpp:314
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/content/pdf_processor.cpp:314
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!doc) return pages;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | distributed_tensor | missing_doxygen_comment | src/distributed_tensor/artifact_manifest.cc:26
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/distributed_tensor/artifact_manifest.cc:26
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (input.empty()) return "0000000000000000";
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | distributed_tensor | missing_doxygen_comment | src/distributed_tensor/src/manifest_store.cpp:164
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/distributed_tensor/src/manifest_store.cpp:164
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!metrics_) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | ethics_ai | missing_doxygen_comment | src/ethics_ai/mirror_school_handler.cpp:86
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/ethics_ai/mirror_school_handler.cpp:86
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (mirror_school_ids.empty()) return results;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | evaluation | missing_doxygen_comment | src/evaluation/src/approximation_rules.cc:105
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/evaluation/src/approximation_rules.cc:105
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (idx < 1 || idx > kCanonicalBoundaries.size()) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | geo | missing_doxygen_comment | src/geo/geo_faiss_knn.cpp:285
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/geo/geo_faiss_knn.cpp:285
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!impl_ || !impl_->built) return "not_built";
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | governance | missing_doxygen_comment | src/governance/audit_batch_writer.cpp:302
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/governance/audit_batch_writer.cpp:302
// Problem: missing_doxygen_comment
// Description: Public declaration 'sleep_for' is missing a Doxygen comment
// Context: std::this_thread::sleep_for( std::chrono::milliseconds(config_.flush_interval_ms) );
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | gpu | missing_doxygen_comment | src/gpu/gpu_memory_pool_safety.cpp:209
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/gpu/gpu_memory_pool_safety.cpp:209
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!head_) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | importers | missing_doxygen_comment | src/importers/huggingface_ingestion_plugin.cpp:298
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/importers/huggingface_ingestion_plugin.cpp:298
// Problem: missing_doxygen_comment
// Description: Public declaration 'sleep_for' is missing a Doxygen comment
// Context: std::this_thread::sleep_for( std::chrono::milliseconds(config_.retry_delay_ms * (1ULL << attempt)) );
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | importers | missing_doxygen_comment | src/importers/mdm_metrics.cpp:50
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/importers/mdm_metrics.cpp:50
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!callback) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | include | missing_doxygen_comment | include/content/content_plugin_interface.h:412
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: include/content/content_plugin_interface.h:412
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (text.empty()) return 0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | include | missing_doxygen_comment | include/llm/context_window_budget.h:52
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: include/llm/context_window_budget.h:52
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (text.empty()) return 0u;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | include | missing_doxygen_comment | include/llm/ssm_state_store.h:167
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: include/llm/ssm_state_store.h:167
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (vec.size() > max_snapshots_per_session_) vec.erase(vec.begin());
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | include | missing_doxygen_comment | include/query/functions/collection_functions.h:61
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: include/query/functions/collection_functions.h:61
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (str.empty()) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | include | missing_doxygen_comment | include/query/mutation_execution_plan.h:209
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: include/query/mutation_execution_plan.h:209
// Problem: missing_doxygen_comment
// Description: Public declaration 'for' is missing a Doxygen comment
// Context: for (const auto& s : steps) stepsArr.push_back(s.toJSON());
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | include | missing_doxygen_comment | include/scraper/scraper_diagnostics.h:255
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: include/scraper/scraper_diagnostics.h:255
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (isScraperFailClosed(e)) return ScraperFaultSeverity::kFatal;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | include | missing_doxygen_comment | include/security/crypto_capabilities.h:102
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: include/security/crypto_capabilities.h:102
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!ctx) return 0.0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | include | missing_doxygen_comment | include/sharding/wal_logging_helper.h:99
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: include/sharding/wal_logging_helper.h:99
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!wal) return std::nullopt;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | index | missing_doxygen_comment | src/index/edge_types.cpp:34
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/index/edge_types.cpp:34
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (initialized_) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | index | missing_doxygen_comment | src/index/hnsw_parameter_tuner.cpp:586
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/index/hnsw_parameter_tuner.cpp:586
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (sz > 0) return sz;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | index | missing_doxygen_param | src/index/inverted_index.cpp:308
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/index/inverted_index.cpp:308
// Problem: missing_doxygen_param
// Description: Doxygen comment for 'if' is missing @param for: column
// Context: if (!exists(table, column)) return Status::Error("InvertedIndex::deindex: no index for " + std::string(table) + "." +...
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | index | missing_doxygen_return | src/index/inverted_index.cpp:308
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/index/inverted_index.cpp:308
// Problem: missing_doxygen_return
// Description: Doxygen comment for 'if' is missing @return
// Context: if (!exists(table, column)) return Status::Error("InvertedIndex::deindex: no index for " + std::string(table) + "." +...
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | index | missing_doxygen_comment | src/index/multi_gpu_vector_index.cpp:816
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/index/multi_gpu_vector_index.cpp:816
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!pImpl->config.enableP2P) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | index | missing_doxygen_comment | src/index/partition_manager.cpp:36
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/index/partition_manager.cpp:36
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!manager_) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | index | missing_doxygen_comment | src/index/vector_index_manager_safety.cpp:38
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/index/vector_index_manager_safety.cpp:38
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!manager_) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | ingestion | missing_doxygen_comment | src/ingestion/object_storage_connector.cpp:77
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/ingestion/object_storage_connector.cpp:77
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (key.size() < 5) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | llm | missing_doxygen_comment | src/llm/adapter_load_balancer.cpp:304
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/llm/adapter_load_balancer.cpp:304
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (underloaded_gpus.empty()) break;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | llm | missing_doxygen_comment | src/llm/async_inference_engine.cpp:474
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/llm/async_inference_engine.cpp:474
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (async_req->callback) async_req->callback(response);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | llm | missing_doxygen_comment | src/llm/llama_grammar_adapter.cpp:186
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/llm/llama_grammar_adapter.cpp:186
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!g_grammar_api_override_active) ensureGrammarAPIInitialized();
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | llm | missing_doxygen_comment | src/llm/llm_model_audit_logger.cpp:179
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/llm/llm_model_audit_logger.cpp:179
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!impl->enabled) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | llm | missing_doxygen_comment | src/llm/lora_framework/embedding_provider.cpp:183
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/llm/lora_framework/embedding_provider.cpp:183
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!model_) return 0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | llm | missing_doxygen_comment | src/llm/lora_framework/gpu_data_loader.cpp:174
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/llm/lora_framework/gpu_data_loader.cpp:174
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (samples_.empty()) return 0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | llm | missing_doxygen_comment | src/llm/lora_framework/gradient_utils.cpp:210
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/llm/lora_framework/gradient_utils.cpp:210
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!grad_ptr) continue;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | llm | missing_doxygen_comment | src/llm/lora_framework/mixed_precision.cpp:107
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/llm/lora_framework/mixed_precision.cpp:107
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!grad_ptr) continue;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | llm | missing_doxygen_comment | src/llm/lora_framework/paged_optimizer.cpp:103
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/llm/lora_framework/paged_optimizer.cpp:103
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (param_data.empty() || grad_data.empty()) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | llm | missing_doxygen_comment | src/llm/ml_model_manager.cpp:933
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/llm/ml_model_manager.cpp:933
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!instance) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | llm | missing_doxygen_comment | src/llm/sampling_strategy.cpp:202
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/llm/sampling_strategy.cpp:202
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (nucleus.empty()) nucleus.push_back(indices[order.front()]);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | llm | missing_doxygen_comment | src/llm/speculative_decoder.cpp:259
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/llm/speculative_decoder.cpp:259
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (probs.empty()) return -1;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | network | missing_doxygen_comment | src/network/wire_protocol_connection_pool.cpp:639
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/network/wire_protocol_connection_pool.cpp:639
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!config_.enable_adaptive_sizing || !config_.adaptive_strategy) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | network | missing_doxygen_comment | src/network/wire_protocol_server.cpp:788
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/network/wire_protocol_server.cpp:788
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!running_.load(std::memory_order_acquire)) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | network | missing_doxygen_comment | src/network/wire_protocol_zero_copy.cpp:250
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/network/wire_protocol_zero_copy.cpp:250
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (wn < 0) return sf_written > 0 ? hdr_written + sf_written : -1;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | observability | missing_doxygen_comment | src/observability/advanced_metrics.cpp:28
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/observability/advanced_metrics.cpp:28
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (sorted_vals.empty()) return 0.0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | observability | missing_doxygen_comment | src/observability/alerting_engine.cpp:275
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/observability/alerting_engine.cpp:275
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!channel) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | observability | missing_doxygen_comment | src/observability/alertmanager.cpp:177
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/observability/alertmanager.cpp:177
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (http_pool_) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | observability | missing_doxygen_comment | src/observability/distributed_tracing_sdk.cpp:51
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/observability/distributed_tracing_sdk.cpp:51
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (str.empty()) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | observability | missing_doxygen_comment | src/observability/ebpf_tracer.cpp:103
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/observability/ebpf_tracer.cpp:103
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (fd < 0) return -1;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | observability | missing_doxygen_comment | src/observability/metric_anomaly_detector.cpp:99
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/observability/metric_anomaly_detector.cpp:99
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!raw) return std::nullopt;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | observability | missing_doxygen_comment | src/observability/metrics_stream_server.cpp:217
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/observability/metrics_stream_server.cpp:217
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!first) oss << ',';
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | performance | missing_doxygen_comment | src/performance/cycle_metrics.cpp:146
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/performance/cycle_metrics.cpp:146
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!event) return 0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | performance | missing_doxygen_comment | src/performance/numa_memory_manager.cpp:172
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/performance/numa_memory_manager.cpp:172
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!ptr) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | performance | missing_doxygen_comment | src/performance/phase3/bwtree.cpp:344
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/performance/phase3/bwtree.cpp:344
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!head) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | plugins | missing_doxygen_comment | src/plugins/plugin_health_monitor.cpp:659
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/plugins/plugin_health_monitor.cpp:659
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!metrics_sink_) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | plugins | missing_doxygen_comment | src/plugins/wasm_plugin_loader.cpp:373
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/plugins/wasm_plugin_loader.cpp:373
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!wasm_instance_) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | process | missing_doxygen_comment | src/process/bpmn_serializer.cpp:150
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/process/bpmn_serializer.cpp:150
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (xml.size() > kMaxBpmnXmlBytes) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | process | missing_doxygen_comment | src/process/object_centric_tracer.cpp:205
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/process/object_centric_tracer.cpp:205
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!normalized.contains("edges")) return result;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | process | missing_doxygen_comment | src/process/process_community_detector.cpp:63
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/process/process_community_detector.cpp:63
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (id.empty()) continue;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | process | missing_doxygen_comment | src/process/process_model_generator.cpp:178
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/process/process_model_generator.cpp:178
// Problem: missing_doxygen_comment
// Description: Public declaration 'for' is missing a Doxygen comment
// Context: for (const auto& e : errors) oss << " - " << e << "\n";
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | projects | missing_doxygen_comment | src/projects/in_memory_project_audit_log.cpp:92
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/projects/in_memory_project_audit_log.cpp:92
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (opts.limit > 0 && result.size() > opts.limit) result.resize(opts.limit);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | prompt_engineering | missing_doxygen_comment | src/prompt_engineering/prompt_evaluator.cpp:353
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/prompt_engineering/prompt_evaluator.cpp:353
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (std::abs(delta - 1.0) < EPS) break;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | prompt_engineering | missing_doxygen_comment | src/prompt_engineering/prompt_quality_evaluator.cpp:48
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/prompt_engineering/prompt_quality_evaluator.cpp:48
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (needle.empty()) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | prompt_engineering | missing_doxygen_comment | src/prompt_engineering/rewrite_rule_base.cpp:298
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/prompt_engineering/rewrite_rule_base.cpp:298
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!match_fn_) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | query | missing_doxygen_comment | src/query/adaptive_join.cpp:325
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/query/adaptive_join.cpp:325
// Problem: missing_doxygen_comment
// Description: Public declaration 'for' is missing a Doxygen comment
// Context: for (const auto& r : right.rows) right_ptrs.push_back(&r);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | query | missing_doxygen_comment | src/query/functions/lora_functions.cpp:398
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/query/functions/lora_functions.cpp:398
// Problem: missing_doxygen_comment
// Description: Public declaration 'while' is missing a Doxygen comment
// Context: while (iss >> w) ws.insert(w);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | query | missing_doxygen_comment | src/query/geospatial_cost_model.cpp:27
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/query/geospatial_cost_model.cpp:27
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (grid.empty()) return nullptr;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | query | missing_doxygen_comment | src/query/plan_cache.cpp:461
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/query/plan_cache.cpp:461
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (lru_list_.empty()) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | query | missing_doxygen_comment | src/query/query_optimizer.cpp:176
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/query/query_optimizer.cpp:176
// Problem: missing_doxygen_comment
// Description: Public declaration 'for' is missing a Doxygen comment
// Context: for (auto i : idx) plan.orderedPredicates.push_back(plan.details[i].pred);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | query | missing_doxygen_comment | src/query/query_profiler.cpp:25
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/query/query_profiler.cpp:25
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (operators.empty()) return nullptr;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | query | missing_doxygen_comment | src/query/sparql_parser.cpp:968
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/query/sparql_parser.cpp:968
// Problem: missing_doxygen_comment
// Description: Public declaration 'for' is missing a Doxygen comment
// Context: for (const auto& [k, _] : var_bindings) all_keys.push_back(k);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | rag | missing_doxygen_param | src/rag/calibration_manager.cpp:325
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/rag/calibration_manager.cpp:325
// Problem: missing_doxygen_param
// Description: Doxygen comment for 'if' is missing @param for: empty
// Context: if (predictions.empty()) return 0.0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | rag | missing_doxygen_return | src/rag/calibration_manager.cpp:325
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/rag/calibration_manager.cpp:325
// Problem: missing_doxygen_return
// Description: Doxygen comment for 'if' is missing @return
// Context: if (predictions.empty()) return 0.0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | rag | missing_doxygen_param | src/rag/delegate_evaluator.cpp:58
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/rag/delegate_evaluator.cpp:58
// Problem: missing_doxygen_param
// Description: Doxygen comment for 'if' is missing @param for: is_object
// Context: if (j.is_object()) return j;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | rag | missing_doxygen_comment | src/rag/document_summarizer.cpp:147
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/rag/document_summarizer.cpp:147
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!first) ss << ' ';
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | rag | missing_doxygen_comment | src/rag/evaluation_cache.cpp:56
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/rag/evaluation_cache.cpp:56
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (lru_list_.empty()) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | rag | missing_doxygen_comment | src/rag/hybrid_retriever.cpp:32
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/rag/hybrid_retriever.cpp:32
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (scores.empty()) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | replication | missing_doxygen_comment | src/replication/async_wal_shipper.cpp:172
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/replication/async_wal_shipper.cpp:172
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (segment_queue_.empty()) return 0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | scheduler | missing_doxygen_comment | src/scheduler/task_scheduler.cpp:3045
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/scheduler/task_scheduler.cpp:3045
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!config_.enable_dynamic_scaling) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | scraper | missing_doxygen_comment | src/scraper/scraper_llm_evaluator.cpp:56
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/scraper/scraper_llm_evaluator.cpp:56
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (i > 0) keywords_str << ", ";
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | search | missing_doxygen_comment | src/search/distributed_hybrid_search.cpp:449
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/search/distributed_hybrid_search.cpp:449
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!item.is_object()) continue;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | search | missing_doxygen_comment | src/search/faceted_search.cpp:169
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/search/faceted_search.cpp:169
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (remaining.empty()) break;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | search | missing_doxygen_comment | src/search/hybrid_search.cpp:415
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/search/hybrid_search.cpp:415
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (results.empty()) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | search | missing_doxygen_comment | src/search/llm_reranker_factory_stub.cpp:14
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/search/llm_reranker_factory_stub.cpp:14
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (g_reranker_factory) return g_reranker_factory(cfg);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | search | missing_doxygen_comment | src/search/multi_field_search.cpp:47
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/search/multi_field_search.cpp:47
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (scored.empty()) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | search | missing_doxygen_comment | src/search/neural_sparse_retrieval.cpp:192
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/search/neural_sparse_retrieval.cpp:192
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (results.empty()) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | search | missing_doxygen_comment | src/search/personalized_ranker.cpp:115
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/search/personalized_ranker.cpp:115
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (candidates.empty() || user_id.empty()) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | search | missing_doxygen_comment | src/search/search_analytics.cpp:89
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/search/search_analytics.cpp:89
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (events_.empty()) return m;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | search | missing_doxygen_comment | src/search/search_result_stream.cpp:130
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/search/search_result_stream.cpp:130
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!callback) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | security | missing_doxygen_comment | src/security/behavioral_anomaly_detector.cpp:223
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/security/behavioral_anomaly_detector.cpp:223
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (static_cast<int>(b.level) > static_cast<int>(a.level)) return b;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | security | missing_doxygen_comment | src/security/confidential_computing.cpp:171
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/security/confidential_computing.cpp:171
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (max_leaf[0] < 0x21) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | security | missing_doxygen_comment | src/security/intent_classifier.cpp:57
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/security/intent_classifier.cpp:57
// Problem: missing_doxygen_comment
// Description: Public declaration 'for' is missing a Doxygen comment
// Context: for (auto c : s) out.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | server | missing_doxygen_comment | src/server/audit_api_handler.cpp:27
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/server/audit_api_handler.cpp:27
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (needle.empty()) return true;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | server | missing_doxygen_comment | src/server/diff_api_handler.cpp:213
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/server/diff_api_handler.cpp:213
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (str.empty()) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | server | missing_doxygen_comment | src/server/distributed_gateway.cpp:332
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/server/distributed_gateway.cpp:332
// Problem: missing_doxygen_comment
// Description: Public declaration 'sleep_for' is missing a Doxygen comment
// Context: std::this_thread::sleep_for(delay);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | server | missing_doxygen_comment | src/server/geo_topology_api_handler.cpp:555
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/server/geo_topology_api_handler.cpp:555
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (trailing.empty()) return "";
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | server | missing_doxygen_comment | src/server/health_error_service.cpp:220
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/server/health_error_service.cpp:220
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (query.empty()) return params;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | server | missing_doxygen_comment | src/server/import_api_handler.cpp:659
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/server/import_api_handler.cpp:659
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!entry.is_object()) continue;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | server | missing_doxygen_comment | src/server/llm_grpc_service.cpp:63
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/server/llm_grpc_service.cpp:63
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (len < 0) return "";
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | server | missing_doxygen_comment | src/server/monitoring_api_handler.cpp:615
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/server/monitoring_api_handler.cpp:615
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (r.contains(k) && r[k].is_number_integer()) return static_cast<uint64_t>(r[k].get<int64_t>());
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | server | missing_doxygen_comment | src/server/rpc/rpc_service_impl.cpp:609
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/server/rpc/rpc_service_impl.cpp:609
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!iter_result) return children;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | server | missing_doxygen_comment | src/server/saga_api_handler.cpp:132
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/server/saga_api_handler.cpp:132
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (line.empty()) continue;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | server | missing_doxygen_comment | src/server/task_scheduler_api_handler.cpp:387
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/server/task_scheduler_api_handler.cpp:387
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (v.is_number_unsigned()) return v.get<size_t>();
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | server | missing_doxygen_comment | src/server/transaction_api_handler.cpp:56
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/server/transaction_api_handler.cpp:56
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!body.contains("isolation")) return IsolationLevel::ReadCommitted;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | server | missing_doxygen_comment | src/server/vector_api_handler.cpp:491
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/server/vector_api_handler.cpp:491
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (st.ok) ++deleted;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | server | missing_doxygen_comment | src/server/workload_fingerprint_engine.cpp:180
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/server/workload_fingerprint_engine.cpp:180
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (denom < 1e-12) return 0.0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | sharding | missing_doxygen_comment | src/sharding/paxos_consensus.cpp:436
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/sharding/paxos_consensus.cpp:436
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!running_.load()) break;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | sharding | missing_doxygen_comment | src/sharding/pki_shard_certificate.cpp:47
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/sharding/pki_shard_certificate.cpp:47
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!time) return "";
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | sharding | missing_doxygen_comment | src/sharding/prometheus_metrics.cpp:804
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/sharding/prometheus_metrics.cpp:804
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (labels.empty()) return "";
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | sharding | missing_doxygen_comment | src/sharding/raft_log.cpp:718
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/sharding/raft_log.cpp:718
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!entry.is_regular_file()) continue;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | sharding | missing_doxygen_comment | src/sharding/replica_consistency.cpp:160
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/sharding/replica_consistency.cpp:160
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!clock_opt) return std::nullopt;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | sharding | missing_doxygen_comment | src/sharding/replication_coordinator.cpp:190
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/sharding/replication_coordinator.cpp:190
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!shipper_) return 0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | sharding | missing_doxygen_comment | src/sharding/shard_load_balancer.cpp:202
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/sharding/shard_load_balancer.cpp:202
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (st.metrics.available) ++cnt;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | sharding | missing_doxygen_comment | src/sharding/shard_load_detector.cpp:406
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/sharding/shard_load_detector.cpp:406
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (values.empty()) return 0.0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | sharding | missing_doxygen_comment | src/sharding/shard_resource_manager.cpp:292
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/sharding/shard_resource_manager.cpp:292
// Problem: missing_doxygen_comment
// Description: Public declaration 'sleep_for' is missing a Doxygen comment
// Context: std::this_thread::sleep_for(std::chrono::milliseconds(1));
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | sharding | missing_doxygen_comment | src/sharding/sharding_manager_edition.cpp:72
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/sharding/sharding_manager_edition.cpp:72
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (node.is_healthy) count++;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | sharding | missing_doxygen_comment | src/sharding/stream_protocol.cpp:139
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/sharding/stream_protocol.cpp:139
// Problem: missing_doxygen_comment
// Description: Public declaration 'sleep_for' is missing a Doxygen comment
// Context: std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | sharding | missing_doxygen_comment | src/sharding/wal_applier.cpp:190
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/sharding/wal_applier.cpp:190
// Problem: missing_doxygen_comment
// Description: Public declaration 'sleep_for' is missing a Doxygen comment
// Context: std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | stable_diffusion | missing_doxygen_comment | src/stable_diffusion/sd_prompt_sanitizer.cpp:71
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/stable_diffusion/sd_prompt_sanitizer.cpp:71
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (blocked_keywords_.empty()) return prompt;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | storage | missing_doxygen_comment | src/storage/adaptive_compaction.cpp:102
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/storage/adaptive_compaction.cpp:102
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (sample_stop_.load(std::memory_order_relaxed)) break;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | storage | missing_doxygen_comment | src/storage/backup_manager.cpp:2795
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/storage/backup_manager.cpp:2795
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!verify) return verify;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | storage | missing_doxygen_comment | src/storage/columnar_cache.cpp:243
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/storage/columnar_cache.cpp:243
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (e.pin_count > 0) ++n;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | storage | missing_doxygen_comment | src/storage/compaction_manager.cpp:142
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/storage/compaction_manager.cpp:142
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (bg_stop_.load(std::memory_order_relaxed)) break;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | storage | missing_doxygen_comment | src/storage/database_connection_manager.cpp:140
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/storage/database_connection_manager.cpp:140
// Problem: missing_doxygen_comment
// Description: Public declaration 'sleep_for' is missing a Doxygen comment
// Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | storage | missing_doxygen_comment | src/storage/mvcc_store.cpp:152
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/storage/mvcc_store.cpp:152
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (val->empty()) return std::nullopt;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | storage | missing_doxygen_comment | src/storage/nvme_manager.cpp:669
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/storage/nvme_manager.cpp:669
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (count > 0) return count;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | storage | missing_doxygen_comment | src/storage/storage_layout_advisor.cpp:186
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/storage/storage_layout_advisor.cpp:186
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!dr_processor_) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | storage | missing_doxygen_comment | src/storage/storage_parquet_exporter.cpp:494
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/storage/storage_parquet_exporter.cpp:494
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!result) return result;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | storage | missing_doxygen_comment | src/storage/tensor_router.cpp:353
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/storage/tensor_router.cpp:353
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (data.empty() || mode_sizes.empty()) return TensorRouteDecision::KEEP;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | storage | missing_doxygen_comment | src/storage/wom_tree.cpp:772
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/storage/wom_tree.cpp:772
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (key.empty()) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | temporal | missing_doxygen_comment | src/temporal/temporal_aggregator.cpp:595
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/temporal/temporal_aggregator.cpp:595
// Problem: missing_doxygen_comment
// Description: Public declaration 'for' is missing a Doxygen comment
// Context: for (auto& v : cur_ordered) cur_values.push_back(std::get<2>(v));
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | temporal | missing_doxygen_comment | src/temporal/temporal_cdc.cpp:462
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/temporal/temporal_cdc.cpp:462
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!fd) return events;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | temporal | missing_doxygen_comment | src/temporal/temporal_query_engine.cpp:414
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/temporal/temporal_query_engine.cpp:414
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (include_deleted) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | tensor | missing_doxygen_comment | src/tensor/hyper_index_builder.cpp:486
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/tensor/hyper_index_builder.cpp:486
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (tt_train.cores.empty() || tt_train.mode_sizes.empty()) return 0.0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | tensor | missing_doxygen_comment | src/tensor/tensor_mmap_bridge.cpp:311
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/tensor/tensor_mmap_bridge.cpp:311
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!r.ptr) continue;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | tests | braces_imbalance | tests/api/test_api_security_audit.cpp:1
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Concurrency
// Location: tests/api/test_api_security_audit.cpp:1
// Problem: braces_imbalance
// Description: Brace imbalance detected: 13 opening braces, 12 closing braces (diff: +1)
// Context: Total opens: 13, Total closes: 12
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | tests | braces_imbalance | tests/api/test_api_version.cpp:1
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Concurrency
// Location: tests/api/test_api_version.cpp:1
// Problem: braces_imbalance
// Description: Brace imbalance detected: 83 opening braces, 82 closing braces (diff: +1)
// Context: Total opens: 83, Total closes: 82
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | themis | missing_doxygen_comment | src/themis/module_loader.cpp:88
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/themis/module_loader.cpp:88
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!handle) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | timeseries | missing_doxygen_comment | src/timeseries/ts_auto_buffer_adaptive.cpp:112
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/timeseries/ts_auto_buffer_adaptive.cpp:112
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!backpressure_) return true;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | timeseries | missing_doxygen_comment | src/timeseries/ts_encrypted_key_rotation.cpp:93
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/timeseries/ts_encrypted_key_rotation.cpp:93
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (stopped) break;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | toolbox | missing_doxygen_comment | src/toolbox/toolbox_builder.cpp:169
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/toolbox/toolbox_builder.cpp:169
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (mimes.empty()) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | transaction | missing_doxygen_comment | src/transaction/compensation_log.cpp:62
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/transaction/compensation_log.cpp:62
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (entry.succeeded) return true;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | transaction | missing_doxygen_comment | src/transaction/lock_manager.cpp:433
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/transaction/lock_manager.cpp:433
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (req->granted) continue;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | transaction | missing_doxygen_comment | src/transaction/saga.cpp:108
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/transaction/saga.cpp:108
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (steps_.empty()) return 0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | transaction | missing_doxygen_comment | src/transaction/transaction_auditor.cpp:39
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/transaction/transaction_auditor.cpp:39
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!enabled_.load(std::memory_order_acquire)) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | transaction | missing_doxygen_comment | src/transaction/transaction_batcher.cpp:345
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/transaction/transaction_batcher.cpp:345
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (batch.empty()) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | updates | missing_doxygen_comment | src/updates/update_state_machine.cpp:358
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/updates/update_state_machine.cpp:358
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (line.empty()) continue;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | updates | missing_doxygen_comment | src/updates/updates_diagnostic_emitter.cpp:30
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/updates/updates_diagnostic_emitter.cpp:30
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!listener) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | utils | missing_doxygen_comment | src/utils/capability_auto_generator.cpp:611
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/utils/capability_auto_generator.cpp:611
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!state_db_) return;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | utils | missing_doxygen_comment | src/utils/checksum_utils.cpp:32
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/utils/checksum_utils.cpp:32
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!ctx) return "";
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | utils | missing_doxygen_comment | src/utils/lz4_codec.cpp:192
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/utils/lz4_codec.cpp:192
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (input_size > static_cast<size_t>(LZ4_MAX_INPUT_SIZE)) return 0;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | utils | missing_doxygen_comment | src/utils/ner_detection_engine.cpp:562
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/utils/ner_detection_engine.cpp:562
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (word.empty()) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | utils | missing_doxygen_comment | src/utils/pii_detection_engine.cpp:134
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/utils/pii_detection_engine.cpp:134
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (value.empty()) return value;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | utils | missing_doxygen_comment | src/utils/rate_limiter.cpp:126
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/utils/rate_limiter.cpp:126
// Problem: missing_doxygen_comment
// Description: Public declaration 'wait_for' is missing a Doxygen comment
// Context: cv_.wait_for(lk, sleep_dur);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | utils | missing_doxygen_comment | src/utils/regex_detection_engine.cpp:519
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/utils/regex_detection_engine.cpp:519
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!pattern.enabled) continue;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | utils | missing_doxygen_comment | src/utils/retention_manager.cpp:396
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/utils/retention_manager.cpp:396
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (stopped) break;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | utils | missing_doxygen_comment | src/utils/stemmer.cpp:166
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/utils/stemmer.cpp:166
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (word.length() < 2) return false;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | utils | missing_doxygen_comment | src/utils/stopwords.cpp:22
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/utils/stopwords.cpp:22
// Problem: missing_doxygen_comment
// Description: Public declaration 'for' is missing a Doxygen comment
// Context: for (auto* w : list) s.emplace(w);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | utils | missing_doxygen_comment | src/utils/zstd_codec.cpp:537
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/utils/zstd_codec.cpp:537
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (impl_->dstream) impl_->reinit();
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | voice | missing_doxygen_comment | src/voice/voice_accessibility.cpp:354
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/voice/voice_accessibility.cpp:354
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (cues.empty()) return cues;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | voice | missing_doxygen_comment | src/voice/voice_assistant.cpp:1153
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/voice/voice_assistant.cpp:1153
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!(out_ctx->oformat->flags & AVFMT_NOFILE)) avio_closep(&out_ctx->pb);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | voice | missing_doxygen_param | src/voice/voice_telephony.cpp:442
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/voice/voice_telephony.cpp:442
// Problem: missing_doxygen_param
// Description: Doxygen comment for 'if' is missing @param for: ct
// Context: if (impl_->on_transcript) impl_->on_transcript(ct);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | voice | missing_doxygen_return | src/voice/voice_telephony.cpp:442
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/voice/voice_telephony.cpp:442
// Problem: missing_doxygen_return
// Description: Doxygen comment for 'if' is missing @return
// Context: if (impl_->on_transcript) impl_->on_transcript(ct);
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | whisper | missing_doxygen_comment | src/whisper/audio_chunk_reader.cpp:313
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/whisper/audio_chunk_reader.cpp:313
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (r->canRead(path)) return true;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

- [ ] HIGH | whisper | missing_doxygen_comment | src/whisper/whisper_plugin.cpp:328
```text
// ROUTING HINT: ollama-local
// Model: gemma4:latest
// Class: Documentation
// Location: src/whisper/whisper_plugin.cpp:328
// Problem: missing_doxygen_comment
// Description: Public declaration 'if' is missing a Doxygen comment
// Context: if (!vad_ || pcm.empty()) return pcm;
Task: Fix this finding only with minimal changes.
Output: files, diff, tests, scanner-delta; keep it concise.
Constraints: max 3 files; avoid unrelated edits; preserve API/ABI; update Doxygen for changed public C++ APIs.
```

