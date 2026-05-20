/*
 * ThemisDB | File: function_registry.cpp | Version: 0.0.47 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 86/100 | Lines: 158
 * Open Issues: TODOs=2, Stubs=3, Gaps=7, Unimpl=0, Mock=1, Sim=1, Debt=0
 * Gap Correlation: internal=7 | external_v3=16 | delta=9 | status=divergent
 * External Severity (v3): C=2, H=11, M=3
 * PR: #1141 Complete AQL function registration: fulltext, ethics, and process m... (2026-03-11T17:51:06Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
#include "query/functions/tensor_functions.h"

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
        
        // Tensor Network functions (TT-compressed domain operations)
        // Includes: TENSOR_SIMILARITY, TENSOR_NORM, TENSOR_SLICE, TENSOR_COMPRESS, TENSOR_INFO
        // Ref: Oseledets (2011) TT-SVD; Holtz et al. (2012) TT-format algebra
        registerTensorFunctions(registry);
        
        // Fulltext functions (search, phrase matching, fuzzy search, n-gram similarity)
        // Includes: FULLTEXT, PHRASE, FUZZY, NGRAM_MATCH, TOKENS, SOUNDEX, METAPHONE, DOUBLE_METAPHONE
        registerFulltextFunctions(registry);
    } catch (const std::exception& ex) {
        // Re-throw with more context - will be caught by FunctionRegistryInitializer
        std::cerr << "registerBuiltinFunctions() exception: " << ex.what() << std::endl;
        throw std::runtime_error(std::string("Failed to register builtin functions: ") + ex.what());
    }
}

} // namespace functions
} // namespace query
} // namespace themis
