# Legal LoRA Training Pipeline Sources
# Multi-source ingestion, auto-labeling, and incremental training for legal domain

if(THEMIS_ENABLE_LEGAL_TRAINING)
    message(STATUS "Enabling Legal LoRA Training Pipeline")
    
    list(APPEND THEMIS_CORE_SOURCES
        # =====================================================================
        # Ingestion Framework
        # =====================================================================
        
        # Core ingestion management
        ../src/ingestion/ingestion_manager.cpp
        
        # HuggingFace connector (REST API integration)
        ../src/ingestion/huggingface_connector.cpp
        
        # Filesystem ingester (PDF/DOCX with OCR support)
        ../src/ingestion/filesystem_ingester.cpp
        
        # Generic REST API connector
        ../src/ingestion/api_connector.cpp

        # Kafka consumer source connector
        ../src/ingestion/kafka_connector.cpp

        # S3 / GCS / Azure Blob object-storage source connector
        ../src/ingestion/object_storage_connector.cpp

        # JDBC-compatible database source connector (Issue: #1894)
        ../src/ingestion/database_connector.cpp

        # Web crawler / sitemap ingestion source (Issue: #1895)
        ../src/ingestion/web_crawler_connector.cpp

        # Distributed ingestion coordinator with work-stealing pool (Issue: #1897)
        ../src/ingestion/ingestion_coordinator.cpp

        # CDC source connector for live database streams (Issue: #2199; stream backend
        # gated behind THEMIS_ENABLE_CDC_STREAM — compiles without it via graceful fallback)
        ../src/ingestion/cdc_connector.cpp
        # NOTE: All ingestion sources are now unconditionally registered in
        # cmake/CMakeLists.txt (THEMIS_CORE_SOURCES).  Do NOT add them here
        # again to avoid duplicate-symbol linker errors.
        
        # =====================================================================
        # Training Framework
        # =====================================================================
        
        # Auto-labeling with Legal Modality Analyzer (PR #1 integration)
        ../src/training/auto_labeler.cpp
        
        # Knowledge graph enrichment
        ../src/training/knowledge_graph_enricher.cpp
        
        # Incremental LoRA training
        ../src/training/incremental_lora_trainer.cpp
        
        # End-to-end training pipeline orchestrator (Phase 7)
        ../src/training/training_pipeline.cpp

        # Automated Quality & Diversity data selection pipeline
        ../src/training/lora_data_selection.cpp

        # LoRA checkpoint manager with SHA-256 integrity validation (Phase 3)
        ../src/training/lora_checkpoint_manager.cpp

        # Training sample provenance and lineage tracker (Phase 3)
        ../src/training/provenance_tracker.cpp

        # Multi-modality legal document parser (Phase 3)
        ../src/training/modality_parser.cpp

        # LoRA adapter weight manipulation (real forward pass + batch updates, no simulation)
        ../src/training/lora_adapter.cpp
    )
    
    # =========================================================================
    # Optional OCR Support
    # =========================================================================
    
    if(THEMIS_ENABLE_OCR)
        message(STATUS "  -> OCR support enabled (Tesseract)")
        # TODO: Add Tesseract integration sources if needed
        # list(APPEND THEMIS_CORE_SOURCES
        #     ../src/ingestion/ocr_processor.cpp
        # )
    endif()
    
    # =========================================================================
    # Dependencies
    # =========================================================================
    
    # libcurl for HuggingFace REST API
    find_package(CURL)
    if(CURL_FOUND)
        message(STATUS "  -> libcurl found: ${CURL_VERSION_STRING}")
        list(APPEND THEMIS_CORE_LINK_LIBRARIES ${CURL_LIBRARIES})
        list(APPEND THEMIS_CORE_INCLUDE_DIRS ${CURL_INCLUDE_DIRS})
    else()
        message(WARNING "  -> libcurl not found - HuggingFace connector will be disabled")
        message(WARNING "     Install: apt-get install libcurl4-openssl-dev (Linux)")
        message(WARNING "            : brew install curl (macOS)")
    endif()
    
    # yaml-cpp for configuration files
    find_package(yaml-cpp QUIET)
    if(yaml-cpp_FOUND)
        message(STATUS "  -> yaml-cpp found: ${yaml-cpp_VERSION}")
        list(APPEND THEMIS_CORE_LINK_LIBRARIES yaml-cpp)
    else()
        message(STATUS "  -> yaml-cpp not found - using built-in YAML parser")
    endif()
    
    # Tesseract OCR (optional)
    if(THEMIS_ENABLE_OCR)
        find_package(Tesseract QUIET)
        if(Tesseract_FOUND)
            message(STATUS "  -> Tesseract OCR found: ${Tesseract_VERSION}")
            list(APPEND THEMIS_CORE_LINK_LIBRARIES ${Tesseract_LIBRARIES})
            list(APPEND THEMIS_CORE_INCLUDE_DIRS ${Tesseract_INCLUDE_DIRS})
        else()
            message(WARNING "  -> Tesseract not found - OCR will be disabled")
            message(WARNING "     Install: apt-get install tesseract-ocr (Linux)")
            message(WARNING "            : brew install tesseract (macOS)")
        endif()
    endif()
    
    # =========================================================================
    # Examples and Tests
    # =========================================================================
    
    # Add example executable
    if(THEMIS_BUILD_EXAMPLES)
        add_executable(train_legal_lora
            ../examples/legal_lora_training/train_legal_lora.cpp
        )
        target_link_libraries(train_legal_lora
            themis_core
            ${THEMIS_CORE_LINK_LIBRARIES}
        )
        target_include_directories(train_legal_lora PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${THEMIS_CORE_INCLUDE_DIRS}
        )
        
        # Add basic auto-labeler test
        add_executable(test_auto_labeler_basic
            ../examples/legal_lora_training/test_auto_labeler_basic.cpp
        )
        target_link_libraries(test_auto_labeler_basic
            themis_core
            ${THEMIS_CORE_LINK_LIBRARIES}
        )
        target_include_directories(test_auto_labeler_basic PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${THEMIS_CORE_INCLUDE_DIRS}
        )
        
        message(STATUS "  -> Example: train_legal_lora will be built")
        message(STATUS "  -> Example: test_auto_labeler_basic will be built")
    endif()
    
    # Add unit tests
    if(THEMIS_BUILD_TESTS)
        if(TARGET gtest)
            add_executable(test_legal_lora_pipeline
                ../tests/test_legal_lora_pipeline.cpp
            )
            target_link_libraries(test_legal_lora_pipeline
                themis_core
                gtest
                gtest_main
                ${THEMIS_CORE_LINK_LIBRARIES}
            )
            target_include_directories(test_legal_lora_pipeline PRIVATE
                ${CMAKE_SOURCE_DIR}/include
                ${THEMIS_CORE_INCLUDE_DIRS}
            )
            
            # Register with CTest
            add_test(NAME LegalLoRAPipeline COMMAND test_legal_lora_pipeline)
            
            message(STATUS "  -> Test: test_legal_lora_pipeline will be built")
        endif()
    endif()
    
    # Add benchmarks
    if(THEMIS_BUILD_BENCHMARKS)
        if(TARGET benchmark::benchmark)
            add_executable(bench_legal_lora_pipeline
                ../benchmarks/bench_legal_lora_pipeline.cpp
            )
            target_link_libraries(bench_legal_lora_pipeline
                themis_core
                benchmark::benchmark
                ${THEMIS_CORE_LINK_LIBRARIES}
            )
            target_include_directories(bench_legal_lora_pipeline PRIVATE
                ${CMAKE_SOURCE_DIR}/include
                ${THEMIS_CORE_INCLUDE_DIRS}
            )
            
            message(STATUS "  -> Benchmark: bench_legal_lora_pipeline will be built")
        endif()
    endif()
    
    message(STATUS "Legal LoRA Training Pipeline configuration complete")
    
else()
    message(STATUS "Legal LoRA Training Pipeline disabled (set THEMIS_ENABLE_LEGAL_TRAINING=ON to enable)")
endif()
