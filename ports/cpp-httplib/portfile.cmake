vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO yhirose/cpp-httplib
    REF "v${VERSION}"
    SHA512 ab6cac0c3ebc0047652d6fe1a8acda1ad2cf19c8dd651bc366b16e4446a127b6aaaeff6cfa43852fa243bab5b8e61e3303fcb6de6764d0a599334c7e657b9cfb
    HEAD_REF master
)

vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
    FEATURES
        brotli  HTTPLIB_REQUIRE_BROTLI
        openssl HTTPLIB_REQUIRE_OPENSSL
        zlib    HTTPLIB_REQUIRE_ZLIB
        zstd    HTTPLIB_REQUIRE_ZSTD
)

set(VCPKG_BUILD_TYPE release) # header-only port

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        ${FEATURE_OPTIONS}
        -DHTTPLIB_COMPILE=OFF
        -DHTTPLIB_TEST=OFF
        -DHTTPLIB_USE_OPENSSL_IF_AVAILABLE=OFF
        -DHTTPLIB_USE_ZLIB_IF_AVAILABLE=OFF
        -DHTTPLIB_USE_BROTLI_IF_AVAILABLE=OFF
        -DHTTPLIB_USE_ZSTD_IF_AVAILABLE=OFF
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME httplib CONFIG_PATH lib/cmake/httplib)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/lib")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
