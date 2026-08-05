# ThemisDB optional private plugin configuration
# Default posture: Community-safe and no-private-checkout friendly.

option(WITH_PRIVATE_PLUGINS "Enable discovery of optional private plugin-named submodules" OFF)
option(WITH_PRIVATE_COMPLIANCE "Enable default private plugin support for compliance-related plugins" OFF)
option(WITH_PRIVATE_CONNECTORS "Enable default private plugin support for connector/blob plugins" OFF)
option(WITH_PRIVATE_ACCELERATION "Enable default private plugin support for acceleration plugins" OFF)
option(WITH_PRIVATE_REGINTEL "Enable default private plugin support for regulated-intelligence plugins" OFF)
option(WITH_PRIVATE_ENTERPRISE "Enable default private plugin support for enterprise plugins" OFF)

if(WITH_PRIVATE_PLUGINS)
    set(WITH_PRIVATE_COMPLIANCE ON CACHE BOOL "Enable default private plugin support for compliance-related plugins" FORCE)
    set(WITH_PRIVATE_CONNECTORS ON CACHE BOOL "Enable default private plugin support for connector/blob plugins" FORCE)
    set(WITH_PRIVATE_ACCELERATION ON CACHE BOOL "Enable default private plugin support for acceleration plugins" FORCE)
    set(WITH_PRIVATE_REGINTEL ON CACHE BOOL "Enable default private plugin support for regulated-intelligence plugins" FORCE)
    set(WITH_PRIVATE_ENTERPRISE ON CACHE BOOL "Enable default private plugin support for enterprise plugins" FORCE)
endif()

option(WITH_PRIVATE_ETHICS_AI "Enable private plugin support for Ethics AI" ${WITH_PRIVATE_COMPLIANCE})
option(WITH_PRIVATE_USER_STORAGE_ENCRYPTED "Enable private plugin support for User Storage Encrypted" ${WITH_PRIVATE_COMPLIANCE})
option(WITH_PRIVATE_CONNECTOR_PACK "Enable private plugin support for the private connector/blob plugin set" ${WITH_PRIVATE_CONNECTORS})
option(WITH_PRIVATE_GPU_IMPACT_ANALYSIS "Enable private plugin support for GPU Impact Analysis" ${WITH_PRIVATE_ACCELERATION})
option(WITH_PRIVATE_LLM_WIKI "Enable private plugin support for LLM Wiki" ${WITH_PRIVATE_ENTERPRISE})

message(STATUS "    Private Plugin Defaults:")
message(STATUS "      Private plugin discovery:         ${WITH_PRIVATE_PLUGINS}")
message(STATUS "      Compliance-related defaults:      ${WITH_PRIVATE_COMPLIANCE}")
message(STATUS "      Connector/blob defaults:          ${WITH_PRIVATE_CONNECTORS}")
message(STATUS "      Acceleration defaults:            ${WITH_PRIVATE_ACCELERATION}")
message(STATUS "      Regulated-intel defaults:         ${WITH_PRIVATE_REGINTEL}")
message(STATUS "      Enterprise defaults:              ${WITH_PRIVATE_ENTERPRISE}")
