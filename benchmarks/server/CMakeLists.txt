if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "Configuring benchmark module: server")

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_auth_token_validation.cpp")
    message(STATUS "Adding auth token validation latency benchmarks")

    add_executable(bench_auth_token_validation
        bench_auth_token_validation.cpp
    )

    target_link_libraries(bench_auth_token_validation PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(bench_auth_token_validation PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_auth_token_validation)

    install(TARGETS bench_auth_token_validation
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  Auth benchmarks: JWT RS256 (cached JWKS), JTI blacklist, TOTP, API token lookup")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_api_endpoints.cpp")
    message(STATUS "Adding API endpoint performance benchmarks")

    add_executable(bench_api_endpoints
        bench_api_endpoints.cpp
    )

    set(_bench_api_endpoint_libs
        ${BENCHMARK_LIBS}
        themis_core
        OpenSSL::SSL
        OpenSSL::Crypto
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    if(TARGET Boost::asio)
        list(APPEND _bench_api_endpoint_libs Boost::asio)
    endif()
    if(TARGET Boost::beast)
        list(APPEND _bench_api_endpoint_libs Boost::beast)
    endif()
    if(TARGET Boost::boost)
        list(APPEND _bench_api_endpoint_libs Boost::boost)
    elseif(TARGET Boost::headers)
        list(APPEND _bench_api_endpoint_libs Boost::headers)
    elseif(DEFINED Boost_INCLUDE_DIRS)
        target_include_directories(bench_api_endpoints PRIVATE ${Boost_INCLUDE_DIRS})
    endif()

    target_link_libraries(bench_api_endpoints PRIVATE ${_bench_api_endpoint_libs})

    target_compile_definitions(bench_api_endpoints PRIVATE
        THEMIS_BENCHMARK_BUILD=1
    )

    themis_apply_benchmark_release_flags(bench_api_endpoints)

    install(TARGETS bench_api_endpoints
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )

    message(STATUS "  API benchmarks: GraphQL parse/execute, correlation ID UUID, JSON serialisation, HTTP endpoint roundtrips")
endif()

themis_add_standard_benchmark(bench_postgres_e2e bench_postgres_e2e.cpp)
themis_add_standard_benchmark(bench_postgres_protocol bench_postgres_protocol.cpp)
themis_add_standard_benchmark(bench_postgres_transactions bench_postgres_transactions.cpp)
themis_add_standard_benchmark(bench_stream_protocol bench_stream_protocol.cpp)
themis_add_standard_benchmark(bench_wal_apply_grpc bench_wal_apply_grpc.cpp)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_server_hotpaths.cpp")
    message(STATUS "Adding server hot-path release-gate benchmarks (SVR-01..SVR-08)")
    add_executable(bench_server_hotpaths bench_server_hotpaths.cpp)
    target_include_directories(bench_server_hotpaths PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(bench_server_hotpaths PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        Threads::Threads
    )
    target_compile_definitions(bench_server_hotpaths PRIVATE THEMIS_BENCHMARK_BUILD=1)
    themis_apply_benchmark_release_flags(bench_server_hotpaths)
    install(TARGETS bench_server_hotpaths
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
    message(STATUS "  SVR-01: HTTP dispatch p99<=500us  SVR-02: Auth JWT p99<=2ms")
    message(STATUS "  SVR-03: Rate-limit p99<=100us  SVR-04: WebSocket p99<=200us")
    message(STATUS "  SVR-05: gRPC unary p99<=1ms  SVR-06: Shutdown drain p99<=5s")
    message(STATUS "  SVR-07: Retry budget p99<=50us  SVR-08: Route lookup p99<=10us")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_server_http3_gates.cpp")
    message(STATUS "Adding server HTTP/3 Protocol-Hardening release-gate benchmarks (SVR-H3-01..SVR-H3-02)")
    add_executable(bench_server_http3_gates bench_server_http3_gates.cpp)
    target_include_directories(bench_server_http3_gates PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(bench_server_http3_gates PRIVATE
        ${BENCHMARK_LIBS}
        themis_core
        Threads::Threads
    )
    target_compile_definitions(bench_server_http3_gates PRIVATE THEMIS_BENCHMARK_BUILD=1)
    themis_apply_benchmark_release_flags(bench_server_http3_gates)
    install(TARGETS bench_server_http3_gates
        RUNTIME DESTINATION bin/benchmarks
        COMPONENT benchmarks
    )
    message(STATUS "  SVR-H3-01: Connection-migration latency p99<=5us")
    message(STATUS "  SVR-H3-02: 0-RTT resumption decision latency p99<=1us")
endif()
