# Gap Scanner FP Tuning Delta Report

| Scanner | Gap Type | Before | After | Delta | Delta % |
|---|---:|---:|---:|---:|---:|
| concurrency | data_race | 1617 | 1220 | -397 | -24.55% |
| reliability | no_timeout | 378 | 330 | -48 | -12.7% |
| memory | pointer_arithmetic | 2022 | 845 | -1177 | -58.21% |
| security | null_dereference | 1469 | 570 | -899 | -61.2% |
| container | iterator_invalidation | 677 | 117 | -560 | -82.72% |

False-positive candidates (heuristic, top 30): 30

- [data_race] src\analytics\olap.cpp:296 (lock_scope_present)
- [data_race] src\analytics\olap.cpp:297 (lock_scope_present)
- [data_race] src\gpu\time_slice_scheduler.cpp:188 (lock_scope_present)
- [data_race] src\llm\inline_training_engine.cpp:261 (lock_scope_present)
- [data_race] src\llm\lora_framework\lora_feedback_storage.cpp:35 (lock_scope_present)
- [data_race] src\llm\lora_framework\lora_feedback_storage.cpp:442 (lock_scope_present)
- [data_race] src\network\wire_protocol_v2.cpp:170 (lock_scope_present)
- [data_race] src\rag\continuous_learning_orchestrator.cpp:181 (lock_scope_present)
- [data_race] src\security\hsm_provider_pkcs11.cpp:661 (lock_scope_present)
- [data_race] src\security\hsm_provider_pkcs11.cpp:1023 (lock_scope_present)
- [data_race] src\sharding\cross_shard_transaction.cpp:2623 (lock_scope_present)
- [data_race] src\sharding\metadata_shard.cpp:149 (lock_scope_present)
- [iterator_invalidation] src\graph\graph_query_optimizer.cpp:2081 (erase_after_find_pattern)
- [iterator_invalidation] src\query\query_cache.cpp:325 (erase_after_find_pattern)
- [iterator_invalidation] src\transaction\lock_manager.cpp:142 (erase_after_find_pattern)
- [iterator_invalidation] src\transaction\lock_manager.cpp:183 (erase_after_find_pattern)
- [no_timeout] src\base\remote_registry_client.cpp:149 (timeout_already_present)
- [no_timeout] src\base\remote_registry_client.cpp:177 (timeout_already_present)
- [no_timeout] src\llm\async_inference_engine.cpp:673 (timeout_already_present)
- [no_timeout] src\network\wire_protocol_connection_pool.cpp:410 (timeout_already_present)
- [no_timeout] src\rag\continuous_learning_client.cpp:55 (timeout_already_present)
- [no_timeout] src\server\mcp_server.cpp:2831 (timeout_already_present)
- [no_timeout] src\server\mqtt_client_service.cpp:434 (timeout_already_present)
- [no_timeout] src\server\postgres_session.cpp:123 (timeout_already_present)
- [no_timeout] src\sharding\gossip_protocol.cpp:667 (local_pem_file_io)
- [no_timeout] src\sharding\raft_wal_integration.cpp:90 (timeout_already_present)
- [no_timeout] src\updates\hot_reload_engine.cpp:471 (timeout_already_present)
- [no_timeout] src\updates\parallel_downloader.cpp:382 (timeout_already_present)
- [no_timeout] src\utils\grpc_channel_pool.cpp:84 (timeout_already_present)
- [no_timeout] src\utils\rate_limiter.cpp:53 (timeout_already_present)
