# LLM and AI Integration Sources
# Includes inference, LoRA fine-tuning, and assistant functionality

if(THEMIS_ENABLE_LLM)
    list(APPEND THEMIS_CORE_SOURCES
        # Documentation assistant
        ../src/llm/docs_assistant.cpp
        ../src/aql/docs_assistant_functions.cpp
        
        # Model conversion and management
        ../src/llm/lora_framework/gguf_converter.cpp
        ../src/llm/llama_resource_manager.cpp
        
        # Process analysis using LLM
        ../src/analytics/llm_process_analyzer.cpp
        
        # LoRA fine-tuning API and handlers
        ../src/server/lora_api_handler.cpp
        ../src/query/functions/lora_functions.cpp
        ../src/llm/lora_security_validator.cpp
        
        # Multi-GPU LoRA layers
        ../src/llm/lora_framework/multi_gpu_lora_layer.cpp
        
        # FlashLoRA memory-efficient computation
        ../src/llm/lora_framework/flash_lora.cpp
        
        # RAG Enhancements - Knowledge Gap Detection & LLM-as-Judge
        ../src/rag/knowledge_gap_detector.cpp
        ../src/rag/rag_judge.cpp
        ../src/rag/llm_integration.cpp
        ../src/rag/claim_extractor.cpp
        
        # NOTE: content_manager_llm.cpp commented out - requires ContentManager implementation
        # ../src/content/content_manager_llm.cpp
    )
endif()
