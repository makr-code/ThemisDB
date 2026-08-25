vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO google/benchmark
    REF "v${VERSION}"
    SHA512 0e91e0e5a2222d7650fd8bd9cafb2f0e7c1689cd1b87b2cc529c738db12bfef31162aa5a4da78f7b0aa7f0101dc08b626802c58d39862458f82f9fea9316ca25
    HEAD_REF main
)

vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
    FEATURES
        tools BENCHMARK_INSTALL_TOOLS
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DBENCHMARK_ENABLE_TESTING=OFF
        -DBENCHMARK_INSTALL_DOCS=OFF
        -DBENCHMARK_ENABLE_WERROR=OFF
        ${FEATURES}
)

vcpkg_cmake_install()
vcpkg_copy_pdbs()

vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/benchmark)
vcpkg_fixup_pkgconfig()

if(BENCHMARK_INSTALL_TOOLS)
    file(GLOB scripts "${CURRENT_PACKAGES_DIR}/share/googlebenchmark/tools/*.py")
    file(MAKE_DIRECTORY "${CURRENT_PACKAGES_DIR}/tools/${PORT}")
    foreach(script IN LISTS scripts)
      cmake_path(GET script FILENAME filename)
      file(RENAME "${script}" "${CURRENT_PACKAGES_DIR}/tools/${PORT}/${filename}")
    endforeach()
    file(RENAME "${CURRENT_PACKAGES_DIR}/share/googlebenchmark/tools/gbench" "${CURRENT_PACKAGES_DIR}/tools/${PORT}/gbench")
    file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/share/googlebenchmark")
endif()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")