/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            function_registry.cpp                              ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:43:09                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     156                                            ║
    • Open Issues:     TODOs: 1, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • fc0c65a058  2026-04-07  feat(api/aql): AQL-GraphQL integration – cost model bridg... ║
    • 3da4977c8c  2026-03-14  fix(aql): address classify-bridge PR review comments ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file function_registry.cpp
 * @brief Implementation of the AQL Function Registry
 * 
 * Registers all built-in functions at application startup.
 * 
 * ## Function Categories
 * 
 * | Category   | Description                              | Count |
 * |------------|------------------------------------------|-------|
 * | String     | Text manipulation and pattern matching   | ~20   |
 * | Math       | Arithmetic, trigonometry, statistics     | ~30   |
 * | Array      | List operations and transformations      | ~20   |
 * | Date       | Date/time parsing, formatting, arithmetic| ~45   |
 * | Document   | Object manipulation and type checking    | ~20   |
 * | Geo        | Spatial/GIS functions (OGC compatible)   | ~25   |
 * | CRS        | Coordinate Reference System transforms   | ~10   |
 * | Vector     | Embeddings, similarity, ML operations    | ~20   |
 * | Graph      | Traversal, centrality, path finding      | ~15   |
 * | Relational | SQL-style joins, aggregation, window     | ~25   |
 * | File       | Path manipulation, MIME types, sizing    | ~20   |
 * | Collection | JSON-native constructors, logical ops    | ~40   |
 * | Security   | Validation, sanitization, masking        | ~15   |
 * | LoRA       | LLM adapter management and operations    | 7     |
 * 
 * Total: ~362 functions
 */

#include "query/functions/function_registry.h"
#include "query/functions/string_functions.h"
#include "query/functions/math_functions.h"
#include "query/functions/array_functions.h"
#include "query/functions/date_functions.h"
#include "query/functions/document_functions.h"
#include "query/functions/json_path_functions.h"
#include "query/functions/geo_functions.h"
#include "query/functions/crs_functions.h"
#include "query/functions/vector_functions.h"
#include "query/functions/graph_functions.h"
#include "query/functions/graph_extensions.h"
#include "query/functions/relational_functions.h"
#include "query/functions/file_functions.h"
#include "query/functions/collection_functions.h"
#include "query/functions/security_functions.h"
#include "query/functions/graphql_functions.h"
#ifdef THEMIS_ENABLE_LLM
#include "query/functions/lora_functions.h"
#include "aql/classify_bridge.h"
#endif
#include "query/functions/ethics_functions.h"
#include "query/functions/process_mining_functions.h"
#include "query/functions/fulltext_functions.h"

#include <iostream>
#include <stdexcept>

namespace themis {
namespace query {
namespace functions {

void registerBuiltinFunctions() {
    try {
        auto& registry = FunctionRegistry::instance();
        
        // Core data manipulation functions
        registerStringFunctions(registry);
        registerMathFunctions(registry);
        registerArrayFunctions(registry);
        registerDateFunctions(registry);
        registerDocumentFunctions(registry);
        registerJsonPathFunctions(registry);    // JSONPath query functions
        
        // Multi-model functions
        registerGeoFunctions(registry);         // Spatial/GIS operations
        registerCrsFunctions(registry);         // Coordinate transformations (ETRS89, UTM, etc.)
        registerVectorFunctions(registry);      // ML embeddings & similarity
        registerGraphFunctions(registry);       // Graph traversal & analysis
        registerGraphExtensions(registry);      // Advanced graph functions (betweenness centrality, community detection)
        
        // SQL-compatible functions
        registerRelationalFunctions(registry);  // Joins, aggregation, window functions
        
        // File/storage functions
        registerFileFunctions(registry);        // Path manipulation, MIME types
        
        // Collection and logical functions (JSON-native, Excel-style)
        // Includes: ARRAY, DICT, JSON, HOLIDAYS, AND, OR, IF, SWITCH, ALL, ANY, etc.
        registerCollectionFunctions(registry);
        
        // Security functions (validation, sanitization, masking)
        // Includes: IS_EMAIL, IS_URL, IS_UUID, SANITIZE, HAS_INJECTION, MASK, etc.
        registerSecurityFunctions();

        // GraphQL integration function
        // Includes: GRAPHQL(query, variables?) – execute embedded GraphQL documents
        // Cost: CostComplexity::EXTERNAL (base_cost=100) – treated as expensive I/O
        // by the optimizer; complexity guard rejects queries > kGraphQLMaxComplexity.
        registerGraphQLFunctions(registry);
        
#ifdef THEMIS_ENABLE_LLM
        // LoRA functions (LLM adapter management and operations)
        // Includes: LORA_TRAIN, LORA_QUERY, LORA_SIMILAR, LORA_PATH, LORA_STATS, LORA_RECOMMEND, LORA_LINEAGE
        registerLoRAFunctions(registry);
        
        // Wire the CLASSIFY bridge into DocsAssistantFunctions so that
        // detectIntentWithNativeNLP() uses the native NLP path instead of
        // always falling through to the LLM intent-detection round-trip.
        themis::aql::registerClassifyBridge();
#endif
        
        // Ethics AI functions (ethical decision-making and evaluation)
        // Includes: ETHICS_MAKE_DECISION, ETHICS_EVALUATE, ETHICS_GET_ARGUMENTS, ETHICS_FIND_SIMILAR_DILEMMAS, etc.
        registerEthicsFunctions(registry);
        
        // Process Mining functions (process discovery, conformance checking, pattern matching)
        // Includes: PM_EXTRACT_LOG, PM_DISCOVER_PROCESS, PM_FIND_SIMILAR, PM_COMPARE_IDEAL, PM_CONFORMANCE, etc.
        registerProcessMiningFunctions(registry);
        
        // Fulltext functions (search, phrase matching, fuzzy search, n-gram similarity)
        // Includes: FULLTEXT, PHRASE, FUZZY, NGRAM_MATCH, TOKENS, SOUNDEX, METAPHONE, DOUBLE_METAPHONE
            // TODO: registerFulltextFunctions - optional fulltext module
            // registerFulltextFunctions(registry);
    } catch (const std::exception& ex) {
        // Re-throw with more context - will be caught by FunctionRegistryInitializer
        std::cerr << "registerBuiltinFunctions() exception: " << ex.what() << std::endl;
        throw std::runtime_error(std::string("Failed to register builtin functions: ") + ex.what());
    }
}

} // namespace functions
} // namespace query
} // namespace themis
