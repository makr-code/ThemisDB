# ThemisDB Network and Protocol Features
# gRPC, HTTP, WebSocket, MQTT, PostgreSQL Wire, etc.

# gRPC (already set by edition, but allow user override if not forced)
if(NOT DEFINED THEMIS_ENABLE_GRPC)
    option(THEMIS_ENABLE_GRPC "Enable gRPC" ON)
endif()

# HTTP Server (built-in httplib-based)
if(NOT DEFINED THEMIS_ENABLE_HTTP_SERVER)
    option(THEMIS_ENABLE_HTTP_SERVER "Enable built-in HTTP server (httplib-based)" ON)
endif()

# HTTP/2
if(NOT DEFINED THEMIS_ENABLE_HTTP2)
    option(THEMIS_ENABLE_HTTP2 "Enable HTTP/2" OFF)
endif()

# HTTP/3
if(NOT DEFINED THEMIS_ENABLE_HTTP3)
    option(THEMIS_ENABLE_HTTP3 "Enable HTTP/3" OFF)
endif()

# WebSocket
if(NOT DEFINED THEMIS_ENABLE_WEBSOCKET)
    option(THEMIS_ENABLE_WEBSOCKET "Enable WebSocket" OFF)
endif()

# MQTT
if(NOT DEFINED THEMIS_ENABLE_MQTT)
    option(THEMIS_ENABLE_MQTT "Enable MQTT" OFF)
endif()

# PostgreSQL Wire Protocol
if(NOT DEFINED THEMIS_ENABLE_POSTGRES_WIRE)
    option(THEMIS_ENABLE_POSTGRES_WIRE "Enable PostgreSQL Wire" OFF)
endif()

# MCP (Model Context Protocol)
if(NOT DEFINED THEMIS_ENABLE_MCP)
    option(THEMIS_ENABLE_MCP "Enable MCP" OFF)
endif()

# Server-Sent Events
if(NOT DEFINED THEMIS_ENABLE_SSE)
    option(THEMIS_ENABLE_SSE "Enable Server-Sent Events" OFF)
endif()

# GraphQL
if(NOT DEFINED THEMIS_ENABLE_GRAPHQL)
    option(THEMIS_ENABLE_GRAPHQL "Enable GraphQL" OFF)
endif()

# Display network features
message(STATUS "  Network Protocols:")
if(THEMIS_ENABLE_GRPC)
    message(STATUS "    gRPC: Enabled")
endif()
if(THEMIS_ENABLE_HTTP_SERVER)
    message(STATUS "    HTTP Server: Enabled")
endif()
if(THEMIS_ENABLE_HTTP2)
    message(STATUS "    HTTP/2: Enabled")
endif()
if(THEMIS_ENABLE_HTTP3)
    message(STATUS "    HTTP/3: Enabled")
endif()
if(THEMIS_ENABLE_WEBSOCKET)
    message(STATUS "    WebSocket: Enabled")
endif()
if(THEMIS_ENABLE_MQTT)
    message(STATUS "    MQTT: Enabled")
endif()
if(THEMIS_ENABLE_POSTGRES_WIRE)
    message(STATUS "    PostgreSQL Wire: Enabled")
endif()
if(THEMIS_ENABLE_MCP)
    message(STATUS "    MCP: Enabled")
endif()
if(THEMIS_ENABLE_SSE)
    message(STATUS "    SSE: Enabled")
endif()
if(THEMIS_ENABLE_GRAPHQL)
    message(STATUS "    GraphQL: Enabled")
endif()
