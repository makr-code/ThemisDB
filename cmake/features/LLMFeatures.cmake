# ThemisDB LLM Features
# LLM integration, vision, and AI capabilities

# Main LLM feature (already set by edition, but allow user override)
if(NOT DEFINED THEMIS_ENABLE_LLM)
    option(THEMIS_ENABLE_LLM "Enable LLM plugin" OFF)
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
    option(THEMIS_ENABLE_CONTENT "Enable content ingestion module" ON)
endif()

# Flash Attention v3 (memory-efficient attention)
if(NOT DEFINED THEMIS_ENABLE_FLASH_ATTENTION)
    option(THEMIS_ENABLE_FLASH_ATTENTION "Enable Flash Attention v3 for LLM inference" ON)
endif()

# Paged KV-Cache (for Flash Attention)
if(NOT DEFINED THEMIS_ENABLE_PAGED_KV_CACHE)
    option(THEMIS_ENABLE_PAGED_KV_CACHE "Enable paged KV-cache management" ON)
endif()

# Legal LoRA Training Pipeline (multi-source ingestion + auto-labeling)
if(NOT DEFINED THEMIS_ENABLE_LEGAL_TRAINING)
    option(THEMIS_ENABLE_LEGAL_TRAINING "Enable Legal LoRA Training Pipeline" OFF)
endif()

# OCR support for legal document ingestion (optional)
if(NOT DEFINED THEMIS_ENABLE_OCR)
    option(THEMIS_ENABLE_OCR "Enable OCR support for scanned documents (requires Tesseract)" OFF)
endif()

# WikiIndexStore Phase B gate (BM25+ / HNSW / RRF path)
if(NOT DEFINED THEMIS_WIKI_PHASE_B)
    option(THEMIS_WIKI_PHASE_B "Enable WikiIndexStore Phase B backend (BM25+/HNSW/RRF)" ON)
endif()
if(THEMIS_WIKI_PHASE_B)
    add_compile_definitions(THEMIS_WIKI_PHASE_B)
    message(STATUS "  WikiIndexStore Phase B: Enabled")
else()
    message(STATUS "  WikiIndexStore Phase B: Disabled (Phase A fallback expected)")
endif()

# LLM Judge runtime gate
if(NOT DEFINED THEMIS_ENABLE_LLM_JUDGE)
    option(THEMIS_ENABLE_LLM_JUDGE "Enable LLM judge backend calls for RAG evaluation" ON)
endif()
if(THEMIS_ENABLE_LLM_JUDGE)
    add_compile_definitions(THEMIS_ENABLE_LLM_JUDGE)
    message(STATUS "  LLM Judge backend calls: Enabled")
else()
    message(STATUS "  LLM Judge backend calls: Disabled (returns llm_unavailable)")
endif()

# Stub mode: deterministic fallback for tests/dev builds only.
# Production builds (release presets) must NEVER set this flag.
# When OFF (default), the stub returns an error (fail-closed) in non-LLM builds.
# When ON, the stub returns a deterministic canned response (test/dev only).
if(NOT DEFINED THEMIS_LLM_STUB_MODE)
    option(THEMIS_LLM_STUB_MODE "Enable deterministic stub responses in non-LLM builds (TEST/DEV ONLY — never set in release presets)" OFF)
endif()
if(THEMIS_LLM_STUB_MODE)
    add_compile_definitions(THEMIS_LLM_STUB_MODE)
    message(STATUS "  LLM stub mode: ON (deterministic fallback — TEST/DEV ONLY)")
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
    if(THEMIS_ENABLE_FLASH_ATTENTION)
        message(STATUS "    Flash Attention v3: Enabled")
        if(THEMIS_ENABLE_PAGED_KV_CACHE)
            message(STATUS "      Paged KV-Cache: Enabled")
        endif()
    endif()
    if(THEMIS_ENABLE_LEGAL_TRAINING)
        message(STATUS "    Legal LoRA Training Pipeline: Enabled")
        if(THEMIS_ENABLE_OCR)
            message(STATUS "      OCR support (Tesseract): Enabled")
        endif()
    endif()
else()
    message(STATUS "  LLM: Disabled")
endif()
