# Remove this overlay once the vcpkg baseline fixes Boost.Locale source grouping.
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO boostorg/locale
    REF boost-${VERSION}
    SHA512 8323b1fce0695e05abb3df04eabde421ceb3315af38e37d6c37aabdb6b4b136caff04944afe827a46f2d0d46f3c9312aba0596e59d2065dd7b61a090b61768d4
    HEAD_REF master
    PATCHES fix-header-source-group.patch
)

vcpkg_check_features(
    OUT_FEATURE_OPTIONS FEATURE_OPTIONS
    FEATURES icu BOOST_LOCALE_ENABLE_ICU
)
if(VCPKG_TARGET_IS_WINDOWS)
    set(BOOST_LOCALE_ENABLE_ICONV off)
    set(BOOST_LOCALE_ENABLE_POSIX off)
else()
    set(BOOST_LOCALE_ENABLE_ICONV on)
    if(VCPKG_TARGET_IS_ANDROID)
        set(BOOST_LOCALE_ENABLE_POSIX off)
    else()
        set(BOOST_LOCALE_ENABLE_POSIX on)
    endif()
endif()
list(APPEND FEATURE_OPTIONS
    -DBOOST_LOCALE_ENABLE_ICONV=${BOOST_LOCALE_ENABLE_ICONV}
    -DBOOST_LOCALE_ENABLE_POSIX=${BOOST_LOCALE_ENABLE_POSIX}
)
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux" AND VCPKG_TARGET_IS_MINGW)
    list(APPEND FEATURE_OPTIONS "-DBOOST_LOCALE_ENABLE_STD=OFF")
endif()
boost_configure_and_install(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS ${FEATURE_OPTIONS}
)
