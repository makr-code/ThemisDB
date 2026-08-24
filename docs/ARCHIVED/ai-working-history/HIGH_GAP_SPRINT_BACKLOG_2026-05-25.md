# High-Gap Sprint Backlog (2026-05-25)

Scope: server, llm, query, sharding, index (nach Rescan)

## Priorisierung nach HIGH-Gaps
1. llm: HIGH=18885, CRITICAL=1414, TOTAL=23543
2. server: HIGH=14066, CRITICAL=396, TOTAL=17603
3. query: HIGH=12959, CRITICAL=579, TOTAL=15305
4. sharding: HIGH=7800, CRITICAL=323, TOTAL=10099
5. index: HIGH=6503, CRITICAL=249, TOTAL=8395

## Modul llm
- Snapshot: HIGH=18885, CRITICAL=1414, TOTAL=23543
- Top HIGH-Typen:
  - unknown: 17531
  - uncaught_exception: 377
  - pointer_arithmetic: 324
  - null_dereference: 268
  - db_connection_leak: 97
  - uninitialized_access: 62
- Top CRITICAL-Typen:
  - unknown: 880
  - data_race: 440
  - iterator_invalidation: 43
  - no_timeout: 33
  - smart_ptr_misuse: 8
  - sql_injection: 7
- Hotspot-Dateien (HIGH):
  - src\llm\multi_lora_manager.cpp: 1011
  - src\llm\llama_wrapper.cpp: 744
  - src\llm\lora_framework\lora_training_service.cpp: 565
  - src\llm\inference_engine_enhanced.cpp: 490
  - src\llm\aql_train_parser.cpp: 432
  - src\llm\gpu_memory_manager.cpp: 420
- Hotspot-Dateien (CRITICAL):
  - src\llm\lora_framework\kernels\vulkan_kernels.cpp: 116
  - src\llm\vision_config.cpp: 89
  - src\llm\lora_framework\kernels\directx_kernels.cpp: 80
  - src\llm\lora_framework\gpu_lora_layers.cpp: 72
  - src\llm\lora_framework\lora_training_service.cpp: 62
  - src\llm\multi_lora_manager.cpp: 60
- Nächste konkrete Maßnahmen:
  - Input-/Bounds-Checks in den Top-Hotspots zuerst schließen.
  - Timeout/Locking-Pfade für CRITICAL concurrency/reliability Befunde priorisieren.
  - Nach Fixes Modul-Rescan nur für dieses Modul fahren und Delta protokollieren.

## Modul server
- Snapshot: HIGH=14066, CRITICAL=396, TOTAL=17603
- Top HIGH-Typen:
  - unknown: 13190
  - null_dereference: 415
  - pointer_arithmetic: 219
  - no_retry_logic: 87
  - uncaught_exception: 70
  - range_temporary: 19
- Top CRITICAL-Typen:
  - data_race: 150
  - unknown: 142
  - iterator_invalidation: 56
  - no_timeout: 25
  - smart_ptr_misuse: 12
  - new_without_delete: 6
- Hotspot-Dateien (HIGH):
  - src\server\http_server.cpp: 2279
  - src\server\query_api_handler.cpp: 1674
  - src\server\mcp_server.cpp: 573
  - src\server\voice_api_handler.cpp: 373
  - src\server\postgres_session.cpp: 304
  - src\server\rpc\rpc_service_impl.cpp: 290
- Hotspot-Dateien (CRITICAL):
  - src\server\query_api_handler.cpp: 98
  - src\server\http_server.cpp: 69
  - src\server\cache_admin_api_handler.cpp: 17
  - src\server\http3_session.cpp: 11
  - src\server\llm_api_handler.cpp: 9
  - src\server\sse_connection_manager.cpp: 8
- Nächste konkrete Maßnahmen:
  - Input-/Bounds-Checks in den Top-Hotspots zuerst schließen.
  - Timeout/Locking-Pfade für CRITICAL concurrency/reliability Befunde priorisieren.
  - Nach Fixes Modul-Rescan nur für dieses Modul fahren und Delta protokollieren.

## Modul query
- Snapshot: HIGH=12959, CRITICAL=579, TOTAL=15305
- Top HIGH-Typen:
  - unknown: 12558
  - uncaught_exception: 123
  - null_dereference: 84
  - no_retry_logic: 73
  - pointer_arithmetic: 51
  - o_n_squared: 27
- Top CRITICAL-Typen:
  - unknown: 380
  - data_race: 112
  - iterator_invalidation: 57
  - no_timeout: 17
  - array_bounds: 12
  - smart_ptr_misuse: 1
- Hotspot-Dateien (HIGH):
  - src\query\query_engine.cpp: 2288
  - src\query\aql_translator.cpp: 1171
  - src\query\aql_parser.cpp: 725
  - src\query\let_evaluator.cpp: 692
  - src\query\cypher_parser.cpp: 374
  - src\query\query_federation.cpp: 367
- Hotspot-Dateien (CRITICAL):
  - src\query\adaptive_join.cpp: 105
  - src\query\aql_translator.cpp: 95
  - src\query\query_engine.cpp: 92
  - src\query\query_federation.cpp: 46
  - src\query\cte_subquery.cpp: 37
  - src\query\aql_runner.cpp: 33
- Nächste konkrete Maßnahmen:
  - Input-/Bounds-Checks in den Top-Hotspots zuerst schließen.
  - Timeout/Locking-Pfade für CRITICAL concurrency/reliability Befunde priorisieren.
  - Nach Fixes Modul-Rescan nur für dieses Modul fahren und Delta protokollieren.

## Modul sharding
- Snapshot: HIGH=7800, CRITICAL=323, TOTAL=10099
- Top HIGH-Typen:
  - unknown: 7294
  - pointer_arithmetic: 150
  - uncaught_exception: 55
  - uninitialized_access: 55
  - range_temporary: 47
  - lock_contention: 46
- Top CRITICAL-Typen:
  - unknown: 181
  - data_race: 63
  - no_timeout: 37
  - iterator_invalidation: 31
  - array_bounds: 8
  - sql_injection: 1
- Hotspot-Dateien (HIGH):
  - src\sharding\redundancy_strategy.cpp: 597
  - src\sharding\cross_shard_transaction.cpp: 567
  - src\sharding\paxos_consensus.cpp: 240
  - src\sharding\stream_protocol.cpp: 231
  - src\sharding\shard_router.cpp: 203
  - src\sharding\shard_rpc_client.cpp: 175
- Hotspot-Dateien (CRITICAL):
  - src\sharding\shard_router.cpp: 32
  - src\sharding\redundancy_strategy.cpp: 29
  - src\sharding\gossip_config_manager.cpp: 18
  - src\sharding\cross_shard_transaction.cpp: 14
  - src\sharding\distributed_transaction.cpp: 12
  - src\sharding\metadata_shard.cpp: 12
- Nächste konkrete Maßnahmen:
  - Input-/Bounds-Checks in den Top-Hotspots zuerst schließen.
  - Timeout/Locking-Pfade für CRITICAL concurrency/reliability Befunde priorisieren.
  - Nach Fixes Modul-Rescan nur für dieses Modul fahren und Delta protokollieren.

## Modul index
- Snapshot: HIGH=6503, CRITICAL=249, TOTAL=8395
- Top HIGH-Typen:
  - unknown: 6267
  - pointer_arithmetic: 66
  - uncaught_exception: 40
  - o_n_squared: 31
  - null_dereference: 29
  - db_connection_leak: 19
- Top CRITICAL-Typen:
  - data_race: 139
  - unknown: 49
  - iterator_invalidation: 35
  - smart_ptr_misuse: 14
  - no_timeout: 7
  - missing_dtor: 4
- Hotspot-Dateien (HIGH):
  - src\index\secondary_index.cpp: 1058
  - src\index\vector_index.cpp: 633
  - src\index\process_graph.cpp: 614
  - src\index\graph_index.cpp: 543
  - src\index\gpu_vector_index.cpp: 281
  - src\index\property_graph.cpp: 254
- Hotspot-Dateien (CRITICAL):
  - src\index\secondary_index.cpp: 68
  - src\index\vector_index.cpp: 43
  - src\index\cuda_hnsw_graph_traversal.cpp: 18
  - src\index\graph_index.cpp: 18
  - src\index\gpu_vector_index.cpp: 14
  - src\index\gpu_memory_oversubscription.cpp: 12
- Nächste konkrete Maßnahmen:
  - Input-/Bounds-Checks in den Top-Hotspots zuerst schließen.
  - Timeout/Locking-Pfade für CRITICAL concurrency/reliability Befunde priorisieren.
  - Nach Fixes Modul-Rescan nur für dieses Modul fahren und Delta protokollieren.
