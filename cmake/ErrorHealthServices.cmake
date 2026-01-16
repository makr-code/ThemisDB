# Error Handling and Health Services
# Error registry, health monitoring, and error reporting

list(APPEND THEMIS_CORE_SOURCES
    # Error API handler for error reporting
    ../src/server/error_api_handler.cpp
    
    # Health error service for monitoring
    ../src/server/health_error_service.cpp
)
