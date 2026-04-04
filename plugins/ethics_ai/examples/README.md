# Ethics AI Plugin - Example Usage

This directory contains example usage documentation for the Ethics AI Plugin.

## Quick Start Example

```cpp
#include "plugins/plugin_manager.h"
#include "plugins/ethics_ai/ethics_ai_plugin_interface.h"

int main() {
    using namespace themis::plugins;
    
    // Get plugin manager
    auto& manager = PluginManager::instance();
    
    // Load plugin
    manager.scanPluginDirectory("lib/themisdb/plugins");
    auto result = manager.loadPlugin("EthicsAI");
    
    if (auto* plugin_ptr = std::get_if<IThemisPlugin*>(&result)) {
        auto* ethics_plugin = static_cast<ethics::IEthicsAIPlugin*>(
            (*plugin_ptr)->getInstance()
        );
        
        // Load philosophy profiles
        ethics_plugin->loadPhilosophyProfiles(
            "lib/themisdb/plugins/ethics_ai/philosophies"
        );
        
        // Make a decision
        auto decision_result = ethics_plugin->makeDecision(
            "Should an autonomous vehicle prioritize passenger safety?",
            {"kant", "utilitarianism"},
            "autonomous_systems",
            true  // use RAG
        );
        
        if (auto* decision = std::get_if<ethics::EthicalDecision>(&decision_result)) {
            std::cout << "Decision: " << decision->decision_text << std::endl;
            std::cout << "Confidence: " << decision->confidence << std::endl;
            
            // Evaluate the decision
            auto eval = ethics_plugin->evaluateDecision(*decision, {});
            if (auto* result = std::get_if<ethics::EthicsEvaluationResult>(&eval)) {
                std::cout << "Quality Score: " << result->overall_score << std::endl;
            }
        }
    }
    
    return 0;
}
```

## More Examples

See the main README.md in the parent directory for comprehensive examples.
