# Preload zstd::zstd target for system RocksDB compatibility
# System RocksDB CMake config requires zstd::zstd target to exist
# This must be loaded BEFORE any find_package(RocksDB) call

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(ZSTD QUIET libzstd)
    if(ZSTD_FOUND)
        if(NOT TARGET zstd::zstd)
            add_library(zstd::zstd INTERFACE IMPORTED GLOBAL)
            target_include_directories(zstd::zstd INTERFACE ${ZSTD_INCLUDE_DIRS})
            target_link_libraries(zstd::zstd INTERFACE ${ZSTD_LIBRARIES})
            message(STATUS "Preloaded zstd::zstd target from pkg-config for RocksDB compatibility")
        endif()
    endif()
endif()
