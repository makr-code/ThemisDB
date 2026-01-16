# Index and Query Enhancement Sources
# Advanced vector indexing, query parsing, aggregations, and pattern matching

list(APPEND THEMIS_CORE_SOURCES
    # Advanced vector index with specialized optimizations
    ../src/index/advanced_vector_index.cpp
    
    # Query parser for custom query syntax
    ../src/query/query_parser.cpp
    
    # Aggregation functions
    ../src/timeseries/aggregates.cpp
    
    # Process mining functionality
    ../src/query/functions/process_mining_functions.cpp
    ../src/analytics/process_pattern_matcher.cpp
)
