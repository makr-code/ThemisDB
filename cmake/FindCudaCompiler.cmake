#[=[
FindCudaCompiler.cmake
Single source of truth for CUDA compiler detection on all platforms.
Replaces duplicated detection logic (previously at cmake/CMakeLists.txt lines ~165 and ~295).

Usage:
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindCudaCompiler.cmake)
    find_cuda_compiler()
    # After: CMAKE_CUDA_COMPILER and CUDAToolkit_ROOT are set if CUDA found
]=]

function(find_cuda_compiler)
    # Already found (don't re-detect)
    if(DEFINED CMAKE_CUDA_COMPILER AND CMAKE_CUDA_COMPILER)
        message(STATUS "[FindCudaCompiler] CUDA compiler already found: ${CMAKE_CUDA_COMPILER}")
        return()
    endif()

    set(_cuda_nvcc_candidates)
    
    # ========== Priority 1: CUDACXX environment variable ==========
    if(DEFINED ENV{CUDACXX} AND EXISTS "$ENV{CUDACXX}")
        list(APPEND _cuda_nvcc_candidates "$ENV{CUDACXX}")
        message(STATUS "[FindCudaCompiler] Candidate 1 (CUDACXX env): $ENV{CUDACXX}")
    endif()
    
    # ========== Priority 2: Windows Registry Query ==========
    # Query NVIDIA registry key for CUDA installation path
    if(WIN32 AND NOT _cuda_nvcc_candidates)
        execute_process(
            COMMAND powershell -NoProfile -Command "
                \$reg = Get-ItemProperty 'Registry::HKLM\\SOFTWARE\\NVIDIA Corporation\\CUDA' -ErrorAction SilentlyContinue
                if (\$reg) { Write-Host \$reg.'(Default)' }
            "
            OUTPUT_VARIABLE _nvidia_cuda_root
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        if(_nvidia_cuda_root)
            set(_cuda_nvcc "${_nvidia_cuda_root}/bin/nvcc.exe")
            if(EXISTS "${_cuda_nvcc}")
                list(APPEND _cuda_nvcc_candidates "${_cuda_nvcc}")
                message(STATUS "[FindCudaCompiler] Candidate 2 (Windows Registry): ${_cuda_nvcc}")
            endif()
        endif()
    endif()
    
    # ========== Priority 3: CUDA_PATH environment variable ==========
    if(DEFINED ENV{CUDA_PATH} AND NOT _cuda_nvcc_candidates)
        set(_cuda_path "$ENV{CUDA_PATH}")
        if(WIN32)
            set(_cuda_nvcc "${_cuda_path}/bin/nvcc.exe")
        else()
            set(_cuda_nvcc "${_cuda_path}/bin/nvcc")
        endif()
        
        if(EXISTS "${_cuda_nvcc}")
            list(APPEND _cuda_nvcc_candidates "${_cuda_nvcc}")
            message(STATUS "[FindCudaCompiler] Candidate 3 (CUDA_PATH env): ${_cuda_nvcc}")
        endif()
    endif()
    
    # ========== Priority 4: Standard installation paths ==========
    if(NOT _cuda_nvcc_candidates)
        if(WIN32)
            # Windows standard paths derived from environment roots
            set(_cuda_roots)
            foreach(_cuda_program_files_root IN ITEMS "$ENV{ProgramFiles}" "$ENV{ProgramFiles\(x86\)}" "$ENV{ProgramW6432}")
                if(_cuda_program_files_root)
                    file(GLOB _cuda_roots_glob "${_cuda_program_files_root}/NVIDIA GPU Computing Toolkit/CUDA/*")
                    list(APPEND _cuda_roots ${_cuda_roots_glob})
                endif()
            endforeach()
            list(REMOVE_DUPLICATES _cuda_roots)
            foreach(_root ${_cuda_roots})
                set(_nvcc "${_root}/bin/nvcc.exe")
                if(EXISTS "${_nvcc}")
                    list(APPEND _cuda_nvcc_candidates "${_nvcc}")
                    message(STATUS "[FindCudaCompiler] Candidate (Standard paths): ${_nvcc}")
                endif()
            endforeach()
        else()
            # Unix-like standard paths
            file(GLOB _cuda_roots "/usr/local/cuda*"
                                  "/opt/cuda*"
                                  "/opt/nvidia/cuda*"
                                  "${HOME}/cuda*")
            foreach(_root ${_cuda_roots})
                set(_nvcc "${_root}/bin/nvcc")
                if(EXISTS "${_nvcc}")
                    list(APPEND _cuda_nvcc_candidates "${_nvcc}")
                    message(STATUS "[FindCudaCompiler] Candidate (Standard paths): ${_nvcc}")
                endif()
            endforeach()
        endif()
    endif()
    
    # ========== Select best candidate (prefer newest version) ==========
    if(_cuda_nvcc_candidates)
        # Sort by CUDA version (newest first) - versions typically in path like CUDA/11.8, CUDA/12.0
        list(SORT _cuda_nvcc_candidates ORDER DESCENDING)
        list(GET _cuda_nvcc_candidates 0 _selected_nvcc)
        
        set(CMAKE_CUDA_COMPILER "${_selected_nvcc}" PARENT_SCOPE)
        message(STATUS "[FindCudaCompiler] Selected: ${_selected_nvcc}")
        
        # Extract CUDA root directory (typically 2 levels up from bin/nvcc)
        get_filename_component(_cuda_bin_dir "${_selected_nvcc}" DIRECTORY)
        get_filename_component(_cuda_root_dir "${_cuda_bin_dir}" DIRECTORY)
        
        if(NOT DEFINED CUDAToolkit_ROOT)
            set(CUDAToolkit_ROOT "${_cuda_root_dir}" PARENT_SCOPE)
            message(STATUS "[FindCudaCompiler] CUDAToolkit_ROOT: ${_cuda_root_dir}")
        endif()
        
        # Try to detect CUDA version from nvcc
        execute_process(
            COMMAND "${_selected_nvcc}" --version
            OUTPUT_VARIABLE _nvcc_version_output
            ERROR_QUIET
        )
        if(_nvcc_version_output MATCHES "release ([0-9]+\\.[0-9]+)")
            set(_cuda_version "${CMAKE_MATCH_1}")
            message(STATUS "[FindCudaCompiler] Detected CUDA version: ${_cuda_version}")
        endif()
    else()
        message(STATUS "[FindCudaCompiler] No CUDA compiler found")
    endif()
endfunction()

# Auto-invoke if not called explicitly
if(NOT COMMAND find_cuda_compiler_initialized)
    function(find_cuda_compiler_initialized)
    endfunction()
endif()
