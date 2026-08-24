# FP Delta Report (Wave 2)

Scope: full src+include scan (3183 files)

| Gap Type | Before | After | Delta | Delta % | Removed | Added |
|---|---:|---:|---:|---:|---:|---:|
| deadlock_risk | 230 | 78 | -152 | -66.09% | 226 | 74 |
| lock_in_loop | 1166 | 74 | -1092 | -93.65% | 1092 | 0 |
| null_dereference | 1634 | 654 | -980 | -59.98% | 1299 | 319 |
| uncaught_exception | 3641 | 1526 | -2115 | -58.09% | 2115 | 0 |

## Remaining deadlock_risk (Top 25)
- src\auth\jwt_validator.cpp:136
- src\cache\adaptive_query_cache.cpp:295
- src\cache\adaptive_query_cache.cpp:374
- src\cache\adaptive_query_cache.cpp:423
- src\cache\adaptive_query_cache.cpp:628
- src\cache\adaptive_query_cache.cpp:744
- src\cache\adaptive_query_cache.cpp:1035
- src\cache\adaptive_query_cache.cpp:1802
- src\cache\adaptive_query_cache.cpp:1949
- src\cache\adaptive_query_cache.cpp:2181
- src\cache\adaptive_query_cache.cpp:2247
- src\cache\adaptive_query_cache.cpp:2305
- src\cache\adaptive_query_cache.cpp:2377
- src\cache\cache_replication_coordinator.cpp:281
- src\cache\redis_cache_coordinator.cpp:126
- src\cache\redis_cache_coordinator.cpp:202
- src\content\async_ingestion_worker.cpp:154
- src\content\async_ingestion_worker.cpp:608
- src\failover\auto_failover_manager.cpp:94
- src\failover\auto_failover_manager.cpp:268
- src\governance\gdpr_subject_rights.cpp:76
- src\gpu\stream_manager.cpp:194
- src\llm\async_inference_engine.cpp:231
- src\llm\async_inference_engine.cpp:335
- src\llm\async_inference_engine.cpp:478

## Remaining lock_in_loop (Top 25)
- src\analytics\incremental_view.cpp:418
- src\auth\zero_trust_auth_verifier.cpp:223
- src\cache\adaptive_query_cache.cpp:1230
- src\cache\adaptive_query_cache.cpp:1803
- src\cache\adaptive_query_cache.cpp:1950
- src\cache\adaptive_query_cache.cpp:2306
- src\cache\cache_replication_coordinator.cpp:327
- src\failover\auto_failover_manager.cpp:550
- src\graph\tensor_deduplication_manager.cpp:1353
- src\ingestion\ingestion_coordinator.cpp:276
- src\llm\inference_engine_enhanced.cpp:1006
- src\llm\production_validator.cpp:1432
- src\llm\shared_worker_pool.cpp:75
- src\llm\shared_worker_pool.cpp:200
- src\network\kernel_bypass.cpp:631
- src\network\raft_load_balancer.cpp:410
- src\network\wire_protocol_connection_pool.cpp:513
- src\network\wire_protocol_connection_pool.cpp:647
- src\network\wire_protocol_connection_pool.cpp:685
- src\observability\alertmanager.cpp:627
- src\observability\alertmanager.cpp:644
- src\performance\advanced_cache_manager.cpp:462
- src\plugins\plugin_health_monitor.cpp:292
- src\plugins\plugin_manager.cpp:1261
- src\query\query_engine.cpp:812

## Remaining null_dereference (Top 25)
- src\main_server.cpp:2005
- src\main_server.cpp:2007
- src\main_server.cpp:2556
- src\main_server.cpp:2558
- src\main_server.cpp:2568
- src\main_server.cpp:2657
- src\main_server.cpp:2788
- src\api\themisdb_grpc_service.cpp:492
- src\aql\aql_agent.cpp:99
- src\auth\auth_rate_limiter.cpp:589
- src\auth\jwks_security.cpp:46
- src\auth\mtls_authenticator.cpp:305
- src\auth\mtls_authenticator.cpp:394
- src\auth\rate_limiter_backend.cpp:133
- src\auth\rate_limiter_backend.cpp:136
- src\auth\rate_limiter_backend.cpp:209
- src\auth\redis_token_blacklist.cpp:44
- src\auth\redis_token_blacklist.cpp:47
- src\auth\redis_token_blacklist.cpp:129
- src\auth\saml_authenticator.cpp:196
- src\auth\totp_secret_encryption.cpp:75
- src\base\hot_reload_manager.cpp:267
- src\base\hot_reload_manager.cpp:269
- src\cache\adaptive_query_cache.cpp:213
- src\cache\adaptive_query_cache.cpp:215

## Remaining uncaught_exception (Top 25)
- src\main_server.cpp:311
- src\main_server.cpp:326
- src\main_server.cpp:981
- src\main_server.cpp:983
- src\main_server.cpp:985
- src\main_server.cpp:1021
- src\main_server.cpp:1639
- src\main_server.cpp:1956
- src\main_server.cpp:1970
- src\main_server.cpp:2013
- src\main_server.cpp:2309
- src\main_server.cpp:2585
- src\main_server.cpp:2592
- src\main_server.cpp:2641
- src\main_server.cpp:2710
- src\main_server.cpp:2759
- src\main_server.cpp:2783
- src\main_server.cpp:2807
- src\main_server.cpp:3135
- src\analytics\anomaly_detection.cpp:1063
- src\analytics\anomaly_detection.cpp:1110
- src\analytics\anomaly_detection.cpp:1217
- src\analytics\anomaly_detection.cpp:1253
- src\analytics\anomaly_detection.cpp:1269
- src\analytics\cep_engine.cpp:196
