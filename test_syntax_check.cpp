// Test syntax and include all the modified files
#include "llm/lora_framework/training_service_registry.h"
#include "llm/lora_framework/lora_training_service.h"
#include "llm/distributed_training_coordinator.h"

int main() {
    // Quick instantiation test
    auto& registry = themis::llm::lora::TrainingServiceRegistry::getInstance();
    (void)registry;
    return 0;
}
