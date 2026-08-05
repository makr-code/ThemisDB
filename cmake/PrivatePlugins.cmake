include_guard(GLOBAL)

# Wave-1 private plugin aggregate repository layout (provisioned 2026-07):
#
#   makr-code/themisdb_ethic_ai   → plugins/themisdb_ethic_ai/
#   makr-code/themisdb_storage    → plugins/themisdb_storage/
#       user_storage_encrypted/
#       azure_blob_storage/
#       s3_blob_storage/
#   makr-code/themisdb_importer   → plugins/themisdb_importer/
#       mysql/
#       mongo/
#       kafka/
#       s3/
#   makr-code/themisdb_llm_wiki   → plugins/themisdb_llm_wiki/
#
# Each submodule is commit-pinned and optional (no-hard-fail when absent).

function(_themis_add_optional_private_plugin_dir label source_dir binary_dir)
    if(EXISTS "${source_dir}/CMakeLists.txt")
        message(STATUS "  - ${label} (private plugin)")
        add_subdirectory("${source_dir}" "${binary_dir}")
    else()
        message(STATUS "  - ${label} not available (optional private source missing: ${source_dir})")
    endif()
endfunction()

function(themis_register_private_plugins plugins_root)
    set(_private_storage_added FALSE)
    set(_private_importer_added FALSE)

    # ethics_ai — aggregate repo root is the plugin dir
    if(WITH_PRIVATE_ETHICS_AI)
        _themis_add_optional_private_plugin_dir("Private plugin: ethics_ai"
            "${plugins_root}/themisdb_ethic_ai"
            "${CMAKE_CURRENT_BINARY_DIR}/private_ethics_ai")
    endif()

    # storage plugins — aggregate repo (contains shared + plugin subdirs)
    if(WITH_PRIVATE_USER_STORAGE_ENCRYPTED)
        _themis_add_optional_private_plugin_dir("Private plugin aggregate: themisdb_storage"
            "${plugins_root}/themisdb_storage"
            "${CMAKE_CURRENT_BINARY_DIR}/private_themisdb_storage")
        set(_private_storage_added TRUE)
    endif()

    if(WITH_PRIVATE_CONNECTOR_PACK)
        # importer plugins — aggregate repo (contains shared + plugin subdirs)
        _themis_add_optional_private_plugin_dir("Private plugin aggregate: themisdb_importer"
            "${plugins_root}/themisdb_importer"
            "${CMAKE_CURRENT_BINARY_DIR}/private_themisdb_importer")
        set(_private_importer_added TRUE)

        # connector/blob pack also depends on storage aggregate (azure/s3)
        if(NOT _private_storage_added)
            _themis_add_optional_private_plugin_dir("Private plugin aggregate: themisdb_storage"
                "${plugins_root}/themisdb_storage"
                "${CMAKE_CURRENT_BINARY_DIR}/private_themisdb_storage")
            set(_private_storage_added TRUE)
        endif()
    endif()

    # llm_wiki plugin — enterprise aggregrate repo
    if(WITH_PRIVATE_LLM_WIKI)
        _themis_add_optional_private_plugin_dir("Private plugin: LLM Wiki"
            "${plugins_root}/themisdb_llm_wiki"
            "${CMAKE_CURRENT_BINARY_DIR}/private_llm_wiki")
    endif()

    if(WITH_PRIVATE_REGINTEL)
        _themis_add_optional_private_plugin_dir("Private regulated-intelligence plugins"
            "${plugins_root}/private/regintel"
            "${CMAKE_CURRENT_BINARY_DIR}/private_regintel")
    endif()

    if(THEMIS_BUILD_GPU_IMPACT_PLUGIN OR WITH_PRIVATE_GPU_IMPACT_ANALYSIS)
        if(EXISTS "${plugins_root}/private/gpu_impact_analysis/CMakeLists.txt")
            _themis_add_optional_private_plugin_dir("GPU Impact Analysis Plugin"
                "${plugins_root}/private/gpu_impact_analysis"
                "${CMAKE_CURRENT_BINARY_DIR}/private_gpu_impact_analysis")
        elseif(EXISTS "${plugins_root}/enterprise/gpu_impact_analysis/CMakeLists.txt")
            _themis_add_optional_private_plugin_dir("GPU Impact Analysis Plugin"
                "${plugins_root}/enterprise/gpu_impact_analysis"
                "${CMAKE_CURRENT_BINARY_DIR}/gpu_impact_analysis")
        else()
            message(STATUS "  - GPU Impact Analysis Plugin not available (optional private source missing)")
        endif()
    endif()
endfunction()
