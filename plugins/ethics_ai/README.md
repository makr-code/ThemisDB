# Ethics AI Plugin - Native C++ Implementation

## Overview

The Ethics AI Plugin is a native C++ implementation of the Ethical AI Framework for ThemisDB. It provides comprehensive ethical decision-making capabilities based on multiple philosophical schools, RAG-based context retrieval, and 5-dimension evaluation metrics.

**Now Integrated with ThemisDB Architecture** ✅
- Uses **BaseEntity** for unified storage (no SQL tables)
- Uses **AQL** for all queries (ThemisDB native)
- **Direct integration** with ThemisDB storage (no wrappers)
- Canonical runtime implementation in `src/ethics_ai/` with public headers in `include/plugins/ethics_ai/`
- `plugins/ethics_ai/` is retained as compatibility, manifest, and documentation layer

**Key Features:**
- Multi-philosophy ethical debates (10 schools: Kant, Utilitarianism, Virtue Ethics, etc.)
- RAG-based context retrieval using AQL query patterns
- Argument chain management with graph traversal
- Decision synthesis with confidence scoring
- 5-dimension evaluation metrics (Quality, Consistency, Fairness, Alignment, Transparency)
- BaseEntity storage integration (Graph, Relational, Vector, Timeline)
- YAML-based philosophy profile system (10 complete profiles)
- Prometheus metrics export
- JSON dashboard data

**Note:** This is a **native C++ implementation** without Python dependencies. The original Python-based framework can be found in `examples/24_moral_philosophy_debates/`.

## Architecture

```
EthicsAIPlugin
├── PhilosophyLoader         - Loads philosophy profiles from YAML
├── ArgumentStore            - BaseEntity storage for arguments/decisions
│   ├── Uses ThemisDB RocksDBWrapper directly
│   └── AQL query execution via QueryEngine
├── RAGContextEngine         - AQL-based context retrieval
├── EthicalDiscourseEngine   - Debate orchestration and synthesis
└── EthicsEvaluator          - 5-dimension evaluation system

ThemisDB Integration:
├── BaseEntity               - Unified storage format
├── AQL Queries              - All data access via AQL
├── RocksDBWrapper           - Direct storage access
└── QueryEngine              - AQL execution
```

### BaseEntity Storage

All ethics data is stored as ThemisDB BaseEntity instances:

```cpp
// Collections (not SQL tables):
ethics_arguments   - Ethical arguments
ethics_decisions   - Decision records
ethics_debates     - Debate sessions
ethics_profiles    - Philosophy profiles

// Keys follow ThemisDB pattern:
entity:ethics_arguments:{id}
entity:ethics_decisions:{id}
entity:ethics_profiles:{school}
```

See `THEMISDB_ARCHITECTURE_INTEGRATION.md` for detailed integration guide.

## Installation

### Build Requirements

- C++17 or later
- CMake 3.20+
- yaml-cpp (optional, for YAML philosophy profiles)
- nlohmann/json (for JSON serialization)

### Building the Plugin

```bash
mkdir build && cd build
cmake .. -DTHEMIS_BUILD_ENTERPRISE_PLUGINS=ON -DTHEMIS_PLUGIN_ETHICS_AI=ON
cmake --build . --target ethics_ai_plugin
```

### Installation

```bash
cmake --install . --prefix /usr/local
```

The plugin will be installed to:
- Binary: `/usr/local/lib/themisdb/plugins/ethics_ai_plugin.so` (or `.dll`/`.dylib`)
- Headers: `/usr/local/include/plugins/ethics_ai/`
- Metadata: `/usr/local/lib/themisdb/plugins/ethics_ai_plugin.json`
- Philosophy Profiles: `/usr/local/lib/themisdb/plugins/ethics_ai/philosophies/`

## Integration with EthicalGuidelinesManager

The Ethics AI Plugin integrates with ThemisDB's base `EthicalGuidelinesManager` to provide extended philosophical perspectives. This follows a **layered architecture** where the base system provides minimal functionality, and the plugin extends it.

### Architecture Overview

```
┌─────────────────────────────────────────────────┐
│  EthicalGuidelinesManager (Minimal Base)        │
│  - Loads config/ethical_guidelines.yaml         │
│  - 5 core philosophical perspectives            │
│  - Public API: registerPhilosophy(),            │
│    mergePhilosophies(), getRegisteredPhilosophies() │
└──────────────────┬────────────────────────────┘
                   │ (extended by)
┌──────────────────▼────────────────────────────┐
│  EthicsAIPlugin (Extension Layer)              │
│  - Loads 16+ philosophy profiles from YAML     │
│  - Registers profiles with manager on init     │
│  - Manager transparently uses all philosophies │
└─────────────────────────────────────────────────┘
```

### How It Works

1. **Base System (Minimal Mode)**: The `EthicalGuidelinesManager` loads basic ethical guidelines from `config/ethical_guidelines.yaml`. It works standalone without any plugins.

2. **Plugin Registration**: When the Ethics AI Plugin initializes, it:
   - Loads philosophy profiles from `plugins/ethics_ai/philosophies/*.yaml`
   - Calls `setEthicalGuidelinesManager()` to receive a reference to the manager
   - Calls `manager->mergePhilosophies()` to register all loaded profiles
   - The manager now has access to 16+ comprehensive philosophy profiles

3. **Transparent Integration**: The manager's methods like `detectEthicalContext()` and `augmentPrompt()` transparently benefit from the extended philosophy set.

### Code Example

```cpp
#include "llm/ethical_guidelines_manager.h"
#include "plugins/ethics_ai/ethics_ai_plugin_interface.h"

// Create the base manager
auto manager = std::make_unique<llm::EthicalGuidelinesManager>(
    "config/ethical_guidelines.yaml"
);

// Load the Ethics AI Plugin
auto& plugin_manager = PluginManager::instance();
auto result = plugin_manager.loadPlugin("EthicsAI");

if (auto* plugin_ptr = std::get_if<IThemisPlugin*>(&result)) {
    auto* ethics_plugin = static_cast<ethics::IEthicsAIPlugin*>(
        (*plugin_ptr)->getInstance()
    );
    
    // Wire the plugin to the manager
    ethics_plugin->setEthicalGuidelinesManager(manager.get());
    
    // Initialize plugin with philosophy directory
    std::string config = R"({"philosophy_dir": "plugins/ethics_ai/philosophies"})";
    ethics_plugin->initialize(config.c_str());
    
    // Now the manager has access to all plugin philosophies!
    auto schools = manager->getRegisteredPhilosophies();
    std::cout << "Total philosophies: " << schools.size() << std::endl;
    // Output: Total philosophies: 16+ (base + plugin)
}
```

### Benefits of This Integration

✅ **Non-Breaking**: Existing code continues to work without modification
✅ **Minimal Base**: Core system works standalone without plugins
✅ **Extensible**: Plugins can add new philosophies dynamically
✅ **Transparent**: Manager APIs automatically benefit from plugin philosophies
✅ **Thread-Safe**: Registration is protected by mutex
✅ **Graceful Degradation**: If plugin fails to load, base system still works

## Usage

### Loading the Plugin

```cpp
#include "plugins/plugin_manager.h"
#include "plugins/ethics_ai/ethics_ai_plugin_interface.h"

using namespace themis::plugins;

// Get plugin manager
auto& manager = PluginManager::instance();

// Scan plugin directory
manager.scanPluginDirectory("lib/themisdb/plugins");

// Load the plugin
auto result = manager.loadPlugin("EthicsAI");
if (auto* plugin_ptr = std::get_if<IThemisPlugin*>(&result)) {
    auto* ethics_plugin = static_cast<ethics::IEthicsAIPlugin*>(
        (*plugin_ptr)->getInstance()
    );
    
    // Use the plugin...
}
```

### Basic Example: Making an Ethical Decision

```cpp
#include "plugins/ethics_ai/ethics_ai_plugin_interface.h"

using namespace themis::plugins::ethics;

// Assuming ethics_plugin is already loaded...

// Initialize a debate
auto debate_result = ethics_plugin->initializeDebate(
    "Should an autonomous vehicle prioritize passenger safety over pedestrian safety?",
    {"kant", "utilitarianism", "virtue_ethics"},
    "autonomous_systems"
);

if (auto* debate = std::get_if<DebateInitialization>(&debate_result)) {
    std::cout << "Debate initialized: " << debate->debate_id << std::endl;
}

// Make a decision
auto decision_result = ethics_plugin->makeDecision(
    "Should an autonomous vehicle prioritize passenger safety over pedestrian safety?",
    {"kant", "utilitarianism", "virtue_ethics"},
    "autonomous_systems",
    true  // use RAG context
);

if (auto* decision = std::get_if<EthicalDecision>(&decision_result)) {
    std::cout << "Decision: " << decision->decision_text << std::endl;
    std::cout << "Confidence: " << decision->confidence << std::endl;
    std::cout << "Consensus: " << decision->consensus_level << std::endl;
    
    // Evaluate the decision
    auto eval_result = ethics_plugin->evaluateDecision(*decision, {});
    if (auto* eval = std::get_if<EthicsEvaluationResult>(&eval_result)) {
        std::cout << "Overall Score: " << eval->overall_score << std::endl;
        std::cout << "Decision Quality: " << eval->decision_quality_score << std::endl;
        std::cout << "Consistency: " << eval->consistency_score << std::endl;
        std::cout << "Fairness: " << eval->fairness_score << std::endl;
        std::cout << "Alignment: " << eval->alignment_score << std::endl;
        std::cout << "Transparency: " << eval->transparency_score << std::endl;
    }
}
```

### Working with Arguments

```cpp
// Create an ethical argument
EthicalArgument argument;
argument.id = "arg_001";
argument.philosophy_school = "kant";
argument.argument_type = ArgumentType::PRO;
argument.content = "All persons have inherent dignity and must be treated as ends in themselves.";
argument.principle_basis = {"categorical_imperative", "human_dignity"};
argument.strength = ArgumentStrength::STRONG;

// Store the argument
auto status = ethics_plugin->storeArgument(argument, true);
if (status.isOK()) {
    std::cout << "Argument stored successfully" << std::endl;
}

// Retrieve arguments by philosophy
auto args_result = ethics_plugin->getArgumentsByPhilosophy(
    "kant",
    {ArgumentType::PRO, ArgumentType::CONTRA},
    20
);

if (auto* args = std::get_if<std::vector<EthicalArgument>>(&args_result)) {
    for (const auto& arg : *args) {
        std::cout << "Argument: " << arg.content << std::endl;
    }
}
```

### Using RAG Context

```cpp
// Build RAG context for a dilemma
auto rag_result = ethics_plugin->buildRAGContext(
    "Should we prioritize privacy or security in data collection?",
    {"kant", "utilitarianism", "virtue_ethics"},
    "data_privacy"
);

if (auto* context = std::get_if<RAGContext>(&rag_result)) {
    std::cout << "Similar dilemmas: " << context->similar_dilemmas.size() << std::endl;
    std::cout << "Best practices: " << context->best_practices.size() << std::endl;
    
    // Access philosophy-specific arguments
    for (const auto& [school, arg_ids] : context->philosophy_arguments) {
        std::cout << school << " has " << arg_ids.size() << " arguments" << std::endl;
    }
}
```

### Philosophy Profile Management

```cpp
// Load philosophy profiles from directory
auto load_result = ethics_plugin->loadPhilosophyProfiles(
    "lib/themisdb/plugins/ethics_ai/philosophies"
);

if (auto* count = std::get_if<size_t>(&load_result)) {
    std::cout << "Loaded " << *count << " philosophy profiles" << std::endl;
}

// List loaded philosophies
auto schools = ethics_plugin->listPhilosophySchools();
for (const auto& school : schools) {
    std::cout << "Philosophy: " << school << std::endl;
}

// Get a specific profile
auto profile_result = ethics_plugin->getPhilosophyProfile("kant");
if (auto* profile = std::get_if<PhilosophyProfile>(&profile_result)) {
    std::cout << "Name: " << profile->name << std::endl;
    std::cout << "Main Theses:" << std::endl;
    for (const auto& thesis : profile->main_theses) {
        std::cout << "  - " << thesis << std::endl;
    }
}
```

### Monitoring and Metrics

```cpp
// Get Prometheus metrics
std::string prometheus_metrics = ethics_plugin->getPrometheusMetrics();
std::cout << prometheus_metrics << std::endl;

// Get dashboard JSON
std::string dashboard_json = ethics_plugin->getDashboardJSON();
std::cout << dashboard_json << std::endl;

// Get statistics
auto stats = ethics_plugin->getStatistics();
for (const auto& [key, value] : stats) {
    std::cout << key << ": " << value << std::endl;
}
```

## Configuration

The plugin can be configured via JSON configuration:

```json
{
  "philosophy_dir": "lib/themisdb/plugins/ethics_ai/philosophies",
  "rag_enabled": true,
  "vector_search_enabled": true,
  "graph_traversal_enabled": true,
  "monitoring_enabled": true,
  "default_similarity_threshold": 0.65,
  "default_rag_limit": 10,
  "default_argument_limit": 20
}
```

Pass configuration during initialization:

```cpp
std::string config = R"({
    "philosophy_dir": "/path/to/philosophies",
    "rag_enabled": true
})";

plugin->initialize(config.c_str());
```

## Philosophy Profiles

Philosophy profiles are defined in YAML format. Example structure:

```yaml
school_id: kant
name: Kantian Ethics
main_theses:
  - "Act only according to that maxim by which you can at the same time will that it should become a universal law"
  - "Treat humanity, whether in your own person or that of another, always as an end and never as a means only"
secondary_theses:
  - "Respect for rational agency"
  - "Moral autonomy and self-legislation"
decision_framework:
  primary_test: "Categorical Imperative universalizability"
  secondary_test: "Respect for persons as ends-in-themselves"
strengths:
  - "Clear moral principles"
  - "Universal applicability"
weaknesses:
  - "Can be rigid in edge cases"
  - "May conflict with consequences"
```

## API Reference

See header files in `include/plugins/ethics_ai/` for detailed API documentation:

- `ethics_ai_types.h` - Core data structures
- `ethics_ai_plugin_interface.h` - Main plugin interface

## Testing

```bash
# Run unit tests
ctest -R ethics_ai

# Run specific test
./build/tests/test_ethics_ai_plugin
```

## Future Enhancements

### Phase 1 (Complete)
- [x] Core data structures
- [x] Plugin interface
- [x] Philosophy loader
- [x] Argument store (in-memory)
- [x] RAG context engine (stub)
- [x] Discourse engine (basic)
- [x] Evaluator (5 dimensions)
- [x] Plugin implementation

### Phase 2 (Planned)
- [ ] Integration with actual ThemisDB storage managers
- [ ] Full AQL query implementation for RAG patterns
- [ ] Vector embedding generation
- [ ] Graph traversal implementation
- [ ] Timeline tracking
- [ ] Comprehensive test suite
- [ ] Performance benchmarks

### Phase 3 (Future)
- [ ] Prompt optimization framework
- [ ] LoRa training integration
- [ ] Advanced evaluation metrics
- [ ] Real-time monitoring dashboard
- [ ] Production deployment tools

## Contributing

Contributions are welcome! Please ensure:

1. Code follows C++17 standards
2. All tests pass
3. Documentation is updated
4. No Python dependencies introduced

## License

MIT License - See LICENSE file for details

## Related

- Original Python implementation: `examples/24_moral_philosophy_debates/`
- Plugin system documentation: `docs/plugins/`
- ThemisDB documentation: `docs/`

## Contact

For questions or issues, please contact the ThemisDB team or open an issue on GitHub.
