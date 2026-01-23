# ThemisDB LLM Features
# LLM integration, vision, and AI capabilities

# Main LLM feature (already set by edition, but allow user override)
if(NOT DEFINED THEMIS_ENABLE_LLM)
    option(THEMIS_ENABLE_LLM "Enable LLM plugin" ON)
endif()

# Vision support (CLIP-based, requires LLM)
if(NOT DEFINED THEMIS_ENABLE_VISION)
    option(THEMIS_ENABLE_VISION "Enable CLIP-based vision (LLaVA) support" OFF)
endif()

# Content processors
if(NOT DEFINED THEMIS_ENABLE_CONTENT_PROCESSORS)
    option(THEMIS_ENABLE_CONTENT_PROCESSORS "Include content processors" OFF)
endif()

# Content ingestion module
if(NOT DEFINED THEMIS_ENABLE_CONTENT)
    option(THEMIS_ENABLE_CONTENT "Enable content ingestion module" OFF)
endif()

# Display LLM features
if(THEMIS_ENABLE_LLM)
    message(STATUS "  LLM: Enabled")
    if(THEMIS_ENABLE_VISION)
        message(STATUS "    Vision (CLIP/LLaVA): Enabled")
    endif()
    if(THEMIS_ENABLE_CONTENT_PROCESSORS)
        message(STATUS "    Content processors: Enabled")
    endif()
    if(THEMIS_ENABLE_CONTENT)
        message(STATUS "    Content ingestion: Enabled")
    endif()
else()
    message(STATUS "  LLM: Disabled")
endif()
