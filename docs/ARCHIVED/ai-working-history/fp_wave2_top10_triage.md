# FP Wave2 Top10 Triage

## deadlock_risk
likely_fp=1 likely_real=0 needs_review=9
- src\auth\auth_rate_limiter.cpp:704 -> needs_review (multiple_locks_nearby)
- src\cache\adaptive_query_cache.cpp:295 -> likely_fp (single_mutex_in_context)
- src\cache\adaptive_query_cache.cpp:1035 -> needs_review (multiple_locks_nearby)
- src\cache\adaptive_query_cache.cpp:1802 -> needs_review (multiple_locks_nearby)
- src\cache\adaptive_query_cache.cpp:1949 -> needs_review (multiple_locks_nearby)
- src\cache\adaptive_query_cache.cpp:2247 -> needs_review (multiple_locks_nearby)
- src\cache\adaptive_query_cache.cpp:2305 -> needs_review (multiple_locks_nearby)
- src\cache\adaptive_query_cache.cpp:2377 -> needs_review (multiple_locks_nearby)
- src\cache\cache_replication_coordinator.cpp:281 -> needs_review (multiple_locks_nearby)
- src\cache\redis_cache_coordinator.cpp:126 -> needs_review (multiple_locks_nearby)

## lock_in_loop
likely_fp=3 likely_real=7 needs_review=0
- src\acceleration\plugin_security.cpp:706 -> likely_fp (no_lock_construct_in_loop_body_window)
- src\acceleration\plugin_security.cpp:718 -> likely_fp (no_lock_construct_in_loop_body_window)
- src\acceleration\plugin_security.cpp:906 -> likely_fp (no_lock_construct_in_loop_body_window)
- src\acceleration\vec_knn.cpp:505 -> likely_real (lock_construct_found_in_loop_body_window)
- src\analytics\anomaly_detection.cpp:1234 -> likely_real (lock_construct_found_in_loop_body_window)
- src\analytics\incremental_view.cpp:418 -> likely_real (lock_construct_found_in_loop_body_window)
- src\auth\zero_trust_auth_verifier.cpp:223 -> likely_real (lock_construct_found_in_loop_body_window)
- src\base\module_loader.cpp:1810 -> likely_real (lock_construct_found_in_loop_body_window)
- src\cache\adaptive_query_cache.cpp:1230 -> likely_real (lock_construct_found_in_loop_body_window)
- src\cache\adaptive_query_cache.cpp:1803 -> likely_real (lock_construct_found_in_loop_body_window)

## null_dereference
likely_fp=0 likely_real=10 needs_review=0
- src\main_server.cpp:2005 -> likely_real (unguarded_pointer_dereference)
- src\main_server.cpp:2007 -> likely_real (unguarded_pointer_dereference)
- src\main_server.cpp:2556 -> likely_real (unguarded_pointer_dereference)
- src\main_server.cpp:2558 -> likely_real (unguarded_pointer_dereference)
- src\main_server.cpp:2568 -> likely_real (unguarded_pointer_dereference)
- src\main_server.cpp:2657 -> likely_real (unguarded_pointer_dereference)
- src\main_server.cpp:2788 -> likely_real (unguarded_pointer_dereference)
- src\api\themisdb_grpc_service.cpp:492 -> likely_real (unguarded_pointer_dereference)
- src\aql\aql_agent.cpp:99 -> likely_real (unguarded_pointer_dereference)
- src\auth\auth_rate_limiter.cpp:589 -> likely_real (unguarded_pointer_dereference)

## uncaught_exception
likely_fp=10 likely_real=0 needs_review=0
- src\main_server.cpp:311 -> likely_fp (non_throw_line_flagged)
- src\main_server.cpp:326 -> likely_fp (non_throw_line_flagged)
- src\main_server.cpp:981 -> likely_fp (non_throw_line_flagged)
- src\main_server.cpp:983 -> likely_fp (non_throw_line_flagged)
- src\main_server.cpp:985 -> likely_fp (non_throw_line_flagged)
- src\main_server.cpp:1021 -> likely_fp (non_throw_line_flagged)
- src\main_server.cpp:1639 -> likely_fp (non_throw_line_flagged)
- src\main_server.cpp:1956 -> likely_fp (non_throw_line_flagged)
- src\main_server.cpp:1970 -> likely_fp (non_throw_line_flagged)
- src\main_server.cpp:2013 -> likely_fp (non_throw_line_flagged)
