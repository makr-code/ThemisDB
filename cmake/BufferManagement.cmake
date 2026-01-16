# Buffer Management Sources
# Buffer API handlers, protocol implementations, and changefeed buffering

list(APPEND THEMIS_CORE_SOURCES
    # Buffer API handler
    ../src/server/buffer_api_handler.cpp
    
    # Binary protocol for efficient buffer communication
    ../src/server/buffer_binary_protocol.cpp
    
    # Changefeed buffer management
    ../src/cdc/changefeed_buffer.cpp
)
