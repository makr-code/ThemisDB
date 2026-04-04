# Distributed Training Sources
# Multi-GPU data loading, training coordination, and optimization
# Requires: THEMIS_ENABLE_DISTRIBUTED_TRAINING

if(THEMIS_ENABLE_DISTRIBUTED_TRAINING)
    list(APPEND THEMIS_CORE_SOURCES
        # Data loading for distributed training
        ../src/llm/lora_framework/distributed_dataloader.cpp
        
        # Multi-GPU trainer coordination
        ../src/llm/lora_framework/multi_gpu_trainer.cpp
        
        # Multi-GPU orchestration
        ../src/llm/lora_framework/multi_gpu.cpp
        
        # Communication optimization
        ../src/llm/lora_framework/custom_allreduce.cpp
        
        # Distributed training coordinator (cross-shard, federated, fault-tolerant)
        ../src/llm/distributed_training_coordinator.cpp
        
        # Byzantine fault detection for distributed training
        ../src/llm/byzantine_detector.cpp
    )
endif()
