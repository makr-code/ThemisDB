# SafeFormat Remediation Plan - Sprint 6 Phase 2

## Format String Gaps Identified (25+ locations)

### RAG Module (4 gaps)
1. src/rag/evaluation_report_exporter.cpp:43 - snprintf for unicode escaping
2. src/rag/flare_retrieval.cpp:190 - fprintf to stderr
3. src/rag/self_rag.cpp:94+ - Multiple fprintf calls (backtrace/debug)
4. src/rag/tensor_rag_pipeline.cpp - fprintf for debug output

### Analytics Module (2 gaps)
5. src/analytics/cep_engine.cpp:97 - snprintf for UUID formatting
6. src/analytics/streaming_window.cpp:133 - snprintf for UUID formatting

### Content Module (5 gaps)
7. src/content/archive_processor.cpp:698 - snprintf path construction
8. src/content/archive_processor.cpp:700 - snprintf path construction
9. src/content/image_processor.cpp:341 - snprintf hex color
10. src/content/mime_detector.cpp:334 - snprintf hex conversion
11. src/content/mime_detector.cpp:375 - snprintf hex digest

### Index Module (7 gaps)
12. src/index/secondary_index.cpp:423 - snprintf timestamp
13. src/index/secondary_index.cpp:2321 - snprintf morton code
14. src/index/secondary_index.cpp:2462 - snprintf timestamp
15. src/index/spatial_index.cpp:232 - snprintf morton code
16. src/index/spatial_index.cpp:238 - snprintf bucket
17. src/index/spatial_index.cpp:251 - snprintf morton code
18. src/index/spatial_index.cpp:1407 - snprintf morton code

### Network Module (8 gaps)
19. src/network/envoy_xds.cpp:66 - snprintf unicode escape
20. src/network/kernel_bypass.cpp:145 - snprintf path
21. src/network/kernel_bypass.cpp:432 - snprintf mask
22. src/network/qos_manager.cpp:668 - snprintf command
23. src/network/qos_manager.cpp:677 - snprintf command
24. src/network/qos_manager.cpp:689 - snprintf command
25. src/network/quic_server.cpp:142 - snprintf peer ID

### Additional gaps (2)
26. src/network/udp_server.cpp:67 - snprintf peer ID
27. src/network/wire_protocol_server.cpp:149 - snprintf peer ID

## Status: TO_DO
