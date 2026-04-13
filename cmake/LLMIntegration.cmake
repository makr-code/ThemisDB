# LLM and AI Integration Sources
# Includes inference, LoRA fine-tuning, and assistant functionality

if(THEMIS_ENABLE_LLM)
    list(APPEND THEMIS_CORE_SOURCES
        # Model downloader for auto-download functionality
        ../src/llm/model_downloader.cpp
        
        # Documentation assistant
        ../src/llm/docs_assistant.cpp
        ../src/aql/docs_assistant_functions.cpp
        ../src/aql/classify_bridge.cpp
        
        # Model conversion and management
        ../src/llm/lora_framework/gguf_converter.cpp
        ../src/llm/llama_resource_manager.cpp
        
        # Process analysis using LLM
        ../src/analytics/llm_process_analyzer.cpp
        
        # LoRA fine-tuning API and handlers
        ../src/server/lora_api_handler.cpp
        ../src/query/functions/lora_functions.cpp
        ../src/llm/lora_security_validator.cpp
        ../src/llm/lora_certificate_store.cpp
        
        # LoRA Router - Automatic routing automation
        ../src/llm/lora_router.cpp
        
        # Adapter Registry - Adapter lifecycle management
        ../src/llm/adapter_registry.cpp
        
        # Inline Training Engine - on-the-fly LoRA fine-tuning without JSONL export
        ../src/llm/inline_training_engine.cpp

        # Distributed Training Coordinator (cross-shard, federated, fault-tolerant)
        ../src/llm/distributed_training_coordinator.cpp
        
        # Byzantine fault detection for distributed training
        ../src/llm/byzantine_detector.cpp
        
        # Multi-GPU LoRA layers
        ../src/llm/lora_framework/multi_gpu_lora_layer.cpp
        
        # FlashLoRA memory-efficient computation
        ../src/llm/lora_framework/flash_lora.cpp
        
        # RAG Enhancements - Knowledge Gap Detection & LLM-as-Judge
        ../src/rag/knowledge_gap_detector.cpp
        ../src/rag/rag_judge.cpp
        ../src/rag/llm_integration.cpp
        ../src/rag/claim_extractor.cpp
        # RAG Enhancement: Knowledge Gap Detection
        ../src/rag/knowledge_gap_detector.cpp
        
        # RAG Enhancement: LLM-as-Judge Phase 1
        ../src/rag/rag_judge.cpp
        ../src/rag/judge_config.cpp
        ../src/rag/prompt_templates.cpp
        ../src/rag/response_parser.cpp
        ../src/rag/llm_judge_integration.cpp
        
        # RAG Enhancement: LLM-as-Judge Phase 2 - Specialized Evaluators
        ../src/rag/faithfulness_evaluator.cpp
        ../src/rag/relevance_evaluator.cpp
        ../src/rag/completeness_evaluator.cpp
        ../src/rag/coherence_evaluator.cpp
        
        # RAG Enhancement: LLM-as-Judge Phase 3 - Pairwise & Ensemble
        ../src/rag/pairwise_comparator.cpp
        ../src/rag/judge_ensemble.cpp
        
        # RAG Enhancement: LLM-as-Judge Phase 4 - Rubric & CoT & G-Eval
        ../src/rag/rubric_evaluator.cpp
        ../src/rag/cot_evaluator.cpp
        ../src/rag/geval_evaluator.cpp
        
        # RAG Enhancement: Quality Control Pipeline (Phase 5)
        ../src/rag/llm_judge_client.cpp
        ../src/rag/nli_faithfulness_verifier.cpp
        ../src/rag/quality_control_pipeline.cpp
        ../src/rag/quality_control_factory.cpp
        
        # RAG Enhancement: Streaming Retrieval & Incremental Context Window Filling (Phase 2)
        ../src/rag/streaming_retriever.cpp

        # RAG Enhancement: Hybrid Retrieval (BM25 + vector, configurable RRF weights, Phase 3)
        ../src/rag/hybrid_retriever.cpp

        # RAG Enhancement: Hallucination Rate Tracking Dashboard (Phase 2)
        ../src/rag/hallucination_dashboard.cpp

        # RAG Enhancement: Knowledge Graph-Augmented Retrieval with Entity Linking (Phase 4)
        ../src/rag/knowledge_graph_retriever.cpp
        # RAG Enhancement: Configurable chunk size and overlap (Phase 3)
        ../src/rag/document_splitter.cpp

        # RAG Enhancement: Continuous Learning Integration (Phase 6)
        ../src/rag/continuous_learning_client.cpp
        
        # RAG Enhancement: Production Integration (Phase 7 - Future Works)
        ../src/rag/onnx_model_loader.cpp
        ../src/rag/http_metrics_client.cpp
        
        # LoRA Cross-Shard Synchronization (Automatic Replication & Consistency)
        ../src/llm/lora_framework/adapter_consistency_checker.cpp
        ../src/llm/lora_framework/adapter_sync_manager.cpp

        # LoRA Adapter Provenance, Snapshots, and Merkle-chained Audit Log
        ../src/llm/lora_framework/lora_provenance.cpp
        
        # NOTE: content_manager_llm.cpp commented out - requires ContentManager implementation
        # ../src/content/content_manager_llm.cpp
        
        # ML Model Management and Inference
        ../src/llm/ml_model_manager.cpp
        ../src/llm/gpu_safe_fail.cpp
        
        # Prompt Engineering and Optimization Framework
        ../src/prompt_engineering/prompt_optimizer.cpp
        ../src/prompt_engineering/prompt_evaluator.cpp
        ../src/prompt_engineering/meta_prompt_generator.cpp
        ../src/prompt_engineering/prompt_performance_tracker.cpp
        ../src/prompt_engineering/self_improvement_orchestrator.cpp
        ../src/prompt_engineering/feedback_collector.cpp
        ../src/prompt_engineering/prompt_version_control.cpp
        ../src/prompt_engineering/prompt_engineering_integration.cpp
        ../src/llm/fewshot_optimizer.cpp
    )
    
    # Flash Attention v3 Integration
    if(THEMIS_ENABLE_FLASH_ATTENTION)
        list(APPEND THEMIS_CORE_SOURCES
            ../src/llm/attention/flash_attention.cpp
            ../src/llm/attention/kv_cache_manager.cpp
        )
        
        # CUDA backend for Flash Attention
        if(THEMIS_ENABLE_CUDA)
            list(APPEND THEMIS_CORE_SOURCES
                ../src/llm/attention/cuda/flash_attention_cuda.cu
            )
        endif()
        
        message(STATUS "Flash Attention v3: Enabled")
    endif()
endif()
