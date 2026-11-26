include_guard(GLOBAL)

# Source snapshots must include the initialized submodule. Git checkouts also
# verify its HEAD against the gitlink, never against a moving remote branch.
function(themis_validate_llama_submodule source_dir out_commit)
    if(NOT EXISTS "${source_dir}/CMakeLists.txt" OR
       NOT EXISTS "${source_dir}/include/llama.h")
        message(FATAL_ERROR
            "llama.cpp submodule is missing. Run: git submodule update --init -- llama.cpp")
    endif()

    file(READ "${source_dir}/include/llama.h" _llama_header)
    if(NOT _llama_header MATCHES
       "#[ \t]*define[ \t]+LLAMA_THEMIS_TRAINABLE_LORA[ \t]+1([^0-9]|$)")
        message(FATAL_ERROR
            "llama.cpp lacks the Themis trainable-LoRA API. Initialize the pinned fork submodule; upstream is not supported.")
    endif()

    set(_commit "")
    get_filename_component(_root "${source_dir}" DIRECTORY)
    if(EXISTS "${_root}/.git")
        find_package(Git REQUIRED)
        file(RELATIVE_PATH _path "${_root}" "${source_dir}")
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" -C "${_root}" ls-files --stage -- "${_path}"
            RESULT_VARIABLE _index_result OUTPUT_VARIABLE _index ERROR_VARIABLE _index_error
            OUTPUT_STRIP_TRAILING_WHITESPACE)
        if(NOT _index_result EQUAL 0 OR NOT _index MATCHES "^160000 ([0-9a-f]+) 0\t")
            message(FATAL_ERROR "llama.cpp is not a registered submodule: ${_index_error}")
        endif()
        set(_commit "${CMAKE_MATCH_1}")
        if(NOT EXISTS "${source_dir}/.git")
            message(FATAL_ERROR "llama.cpp has no submodule Git metadata. Reinitialize the submodule.")
        endif()
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" -C "${source_dir}" rev-parse HEAD
            RESULT_VARIABLE _head_result OUTPUT_VARIABLE _head ERROR_VARIABLE _head_error
            OUTPUT_STRIP_TRAILING_WHITESPACE)
        if(NOT _head_result EQUAL 0 OR NOT _head STREQUAL _commit)
            message(FATAL_ERROR
                "llama.cpp HEAD '${_head}' differs from pinned gitlink '${_commit}'. Run git submodule update --init -- llama.cpp. ${_head_error}")
        endif()
        message(STATUS "llama.cpp: verified submodule commit ${_commit}")
    else()
        message(STATUS "llama.cpp: source snapshot; patch API verified, Git identity unavailable")
    endif()
    set("${out_commit}" "${_commit}" PARENT_SCOPE)
endfunction()
