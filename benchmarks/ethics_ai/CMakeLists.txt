if(TARGET benchmark::benchmark AND TARGET themis_core)
    if(NOT TARGET bench_ethics_ai_ldm)
        add_executable(bench_ethics_ai_ldm bench_ldm.cpp)
        target_include_directories(bench_ethics_ai_ldm PRIVATE
            ${THEMIS_ROOT_DIR}/include
            ${THEMIS_ROOT_DIR}/src
        )
        target_sources(bench_ethics_ai_ldm PRIVATE
            ${THEMIS_ROOT_DIR}/src/ethics_ai/ethics_ai_types.cpp
            ${THEMIS_ROOT_DIR}/src/ethics_ai/philosophy_loader.cpp
            ${THEMIS_ROOT_DIR}/src/ethics_ai/argument_store.cpp
            ${THEMIS_ROOT_DIR}/src/ethics_ai/rag_context_engine.cpp
            ${THEMIS_ROOT_DIR}/src/ethics_ai/discourse_engine.cpp
            ${THEMIS_ROOT_DIR}/src/ethics_ai/chain_visualizer.cpp
            ${THEMIS_ROOT_DIR}/src/ethics_ai/ethics_profile_registry.cpp
            ${THEMIS_ROOT_DIR}/src/ethics_ai/ethics_selection_router.cpp
            ${THEMIS_ROOT_DIR}/src/ethics_ai/discourse_orchestrator.cpp
            ${THEMIS_ROOT_DIR}/src/ethics_ai/meta_verdict_builder.cpp
            ${THEMIS_ROOT_DIR}/src/ethics_ai/mirror_school_handler.cpp
        )
        target_link_libraries(bench_ethics_ai_ldm PRIVATE
            benchmark::benchmark
            themis_core
            Threads::Threads
        )
        target_compile_definitions(bench_ethics_ai_ldm PRIVATE THEMIS_TEST_BUILD=1)
        set_target_properties(bench_ethics_ai_ldm PROPERTIES CXX_STANDARD 20)

        if(TARGET yaml-cpp::yaml-cpp)
            target_link_libraries(bench_ethics_ai_ldm PRIVATE yaml-cpp::yaml-cpp)
            target_compile_definitions(bench_ethics_ai_ldm PRIVATE HAVE_YAML_CPP)
        endif()

        install(TARGETS bench_ethics_ai_ldm
            RUNTIME DESTINATION bin/benchmarks
        )
    endif()
endif()
