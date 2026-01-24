// Stub implementations for linking purposes
// These stubs allow themis_tests to link successfully
// They are minimal implementations that prevent linker errors

#include <memory>
#include <string>
#include <vector>

namespace themis::llm::lora {

struct Feedback {
    // Stub
};

struct TrainingTriggerPlugin {
    virtual ~TrainingTriggerPlugin() = default;
};

struct CacheAwareWeightingPlugin {
    virtual ~CacheAwareWeightingPlugin() = default;
    virtual void process(Feedback&) {}
};

struct LoRATrainingConfig {
    static LoRATrainingConfig loadFromFile(const std::string&) {
        return {};
    }
    
    std::shared_ptr<TrainingTriggerPlugin> createTrainingTriggerPlugin(const std::string&) const {
        return std::make_shared<TrainingTriggerPlugin>();
    }
    
    std::shared_ptr<CacheAwareWeightingPlugin> createCacheWeightingPlugin(const std::string&) const {
        return std::make_shared<CacheAwareWeightingPlugin>();
    }
};

struct FeedbackStorageService {
    std::vector<Feedback> getFeedbackForAdapter(const std::string&, unsigned __int64) const {
        return {};
    }
};

} // namespace themis::llm::lora
