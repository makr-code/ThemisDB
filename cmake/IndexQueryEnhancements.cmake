# Index and Query Enhancement Sources
# Advanced vector indexing, query parsing, aggregations, and pattern matching

list(APPEND THEMIS_CORE_SOURCES
    # Advanced vector index with specialized optimizations
    ../src/index/advanced_vector_index.cpp
    
    # IndexManager with Dependency Injection (Phase 4)
    ../src/index/index_manager.cpp
    
    # Aggregation functions
    ../src/timeseries/aggregates.cpp
    
    # Process mining functionality
    ../src/query/functions/process_mining_functions.cpp
    ../src/analytics/process_pattern_matcher.cpp
)
