# Buffer Management Sources
# Buffer API handlers, protocol implementations, and changefeed buffering

list(APPEND THEMIS_CORE_SOURCES
    # Buffer API handler
    ../src/server/buffer_api_handler.cpp

    # Changefeed buffer management
    ../src/cdc/changefeed_buffer.cpp
)

if(msgpack_FOUND OR msgpack-cxx_FOUND)
    list(APPEND THEMIS_CORE_SOURCES
        ../src/server/buffer_binary_protocol.cpp
    )
endif()
