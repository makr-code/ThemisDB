# ThemisDB optional private plugin family configuration
# Default posture: Community-safe and no-private-checkout friendly.

option(WITH_PRIVATE_PLUGINS "Enable discovery of optional private plugin family submodules" OFF)
option(WITH_PRIVATE_COMPLIANCE "Enable optional private compliance plugin family" OFF)
option(WITH_PRIVATE_CONNECTORS "Enable optional private connector plugin family" OFF)
option(WITH_PRIVATE_ACCELERATION "Enable optional private acceleration plugin family" OFF)
option(WITH_PRIVATE_REGINTEL "Enable optional private regulated-intelligence plugin family" OFF)

if(WITH_PRIVATE_PLUGINS)
    set(WITH_PRIVATE_COMPLIANCE ON CACHE BOOL "Enable optional private compliance plugin family" FORCE)
    set(WITH_PRIVATE_CONNECTORS ON CACHE BOOL "Enable optional private connector plugin family" FORCE)
    set(WITH_PRIVATE_ACCELERATION ON CACHE BOOL "Enable optional private acceleration plugin family" FORCE)
    set(WITH_PRIVATE_REGINTEL ON CACHE BOOL "Enable optional private regulated-intelligence plugin family" FORCE)
endif()

option(WITH_PRIVATE_ETHICS_AI "Enable private compliance bundle support for Ethics AI" ${WITH_PRIVATE_COMPLIANCE})
option(WITH_PRIVATE_USER_STORAGE_ENCRYPTED "Enable private compliance bundle support for User Storage Encrypted" ${WITH_PRIVATE_COMPLIANCE})
option(WITH_PRIVATE_CONNECTOR_PACK "Enable private connector bundle support" ${WITH_PRIVATE_CONNECTORS})
option(WITH_PRIVATE_GPU_IMPACT_ANALYSIS "Enable private acceleration bundle support for GPU Impact Analysis" ${WITH_PRIVATE_ACCELERATION})

message(STATUS "    Private Plugin Families:")
message(STATUS "      Private plugin discovery:         ${WITH_PRIVATE_PLUGINS}")
message(STATUS "      Private compliance family:        ${WITH_PRIVATE_COMPLIANCE}")
message(STATUS "      Private connectors family:        ${WITH_PRIVATE_CONNECTORS}")
message(STATUS "      Private acceleration family:      ${WITH_PRIVATE_ACCELERATION}")
message(STATUS "      Private regulated-intel family:   ${WITH_PRIVATE_REGINTEL}")
