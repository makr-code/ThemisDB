# Dependency package build targets

include_guard(GLOBAL)
include(${CMAKE_CURRENT_LIST_DIR}/PackageCommon.cmake)

if(NOT TARGET package-deps)
    add_custom_target(package-deps COMMENT "Build dependency packages")
endif()

function(_themis_add_deps_target _target_name _triplet _output_dir)
    if(TARGET ${_target_name})
        return()
    endif()

    add_custom_target(${_target_name}
        COMMAND ${CMAKE_COMMAND}
            -DTHEMIS_VCPKG_ROOT="${THEMIS_VCPKG_ROOT}"
            -DTHEMIS_TRIPLET=${_triplet}
            -DTHEMIS_EDITION=${THEMIS_EDITION}
            -DTHEMIS_OUTPUT_DIR="${_output_dir}"
            -P "${CMAKE_SOURCE_DIR}/cmake/VcpkgPackageBuild.cmake"
        COMMENT "Building dependency packages (${_triplet})"
        VERBATIM
    )

    add_dependencies(package-deps ${_target_name})
endfunction()

_themis_add_deps_target(package-deps-windows-debug x64-windows "${THEMIS_PACKAGE_DIR_WINDOWS_DEBUG}")
_themis_add_deps_target(package-deps-windows-release x64-windows "${THEMIS_PACKAGE_DIR_WINDOWS_RELEASE}")
_themis_add_deps_target(package-deps-linux-debug x64-linux "${THEMIS_PACKAGE_DIR_LINUX_DEBUG}")
_themis_add_deps_target(package-deps-linux-release x64-linux "${THEMIS_PACKAGE_DIR_LINUX_RELEASE}")

# Backward-compatible aliases
if(NOT TARGET build-all-packages)
    add_custom_target(build-all-packages COMMENT "Compatibility alias for package-deps")
endif()
add_dependencies(build-all-packages package-deps)

if(NOT TARGET build-packages-windows-debug)
    add_custom_target(build-packages-windows-debug COMMENT "Compatibility alias")
endif()
add_dependencies(build-packages-windows-debug package-deps-windows-debug)

if(NOT TARGET build-packages-windows-release)
    add_custom_target(build-packages-windows-release COMMENT "Compatibility alias")
endif()
add_dependencies(build-packages-windows-release package-deps-windows-release)

if(NOT TARGET build-packages-linux-debug)
    add_custom_target(build-packages-linux-debug COMMENT "Compatibility alias")
endif()
add_dependencies(build-packages-linux-debug package-deps-linux-debug)

if(NOT TARGET build-packages-linux-release)
    add_custom_target(build-packages-linux-release COMMENT "Compatibility alias")
endif()
add_dependencies(build-packages-linux-release package-deps-linux-release)
