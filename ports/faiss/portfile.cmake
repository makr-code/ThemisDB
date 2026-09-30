vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO facebookresearch/faiss
    REF "v${VERSION}"
    SHA512 64d333e3cf561a65a9dcb78bb04f76073047b1149ce4778e4d65aa809928bedbd43b2b0a3362e8336664feae3d09167702ef68abddce3c86bc70cdb9551bc65c
    HEAD_REF master
    PATCHES
        msvc-template.diff
        undef-small.diff
        fix-lapack-detect.diff
)

vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
    FEATURES
        gpu     FAISS_ENABLE_GPU
)

if ("gpu" IN_LIST FEATURES)
    vcpkg_find_cuda(OUT_CUDA_TOOLKIT_ROOT cuda_toolkit_root)
    list(APPEND FEATURE_OPTIONS
        "-DCMAKE_CUDA_COMPILER=${NVCC}"
        "-DCUDAToolkit_ROOT=${cuda_toolkit_root}"
    )
endif()

set(_faiss_static_prefix "${CMAKE_STATIC_LIBRARY_PREFIX}")
set(_faiss_static_suffix "${CMAKE_STATIC_LIBRARY_SUFFIX}")
if(VCPKG_TARGET_IS_WINDOWS)
    set(_faiss_f2c_basename "libf2c")
else()
    set(_faiss_f2c_basename "f2c")
endif()

set(FAISS_BLAS_LIBS "${CURRENT_INSTALLED_DIR}/lib/${_faiss_static_prefix}openblas${_faiss_static_suffix}")
set(FAISS_LAPACK_LIBS
    "${CURRENT_INSTALLED_DIR}/lib/${_faiss_static_prefix}lapack${_faiss_static_suffix}\\;${CURRENT_INSTALLED_DIR}/lib/${_faiss_static_prefix}${_faiss_f2c_basename}${_faiss_static_suffix}")

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        ${FEATURE_OPTIONS}
        -DFAISS_ENABLE_MKL=OFF
        -DFAISS_ENABLE_PYTHON=OFF
        -DBUILD_TESTING=OFF
        -DBLA_VENDOR=OpenBLAS
        -DBLAS_LIBRARIES=${FAISS_BLAS_LIBS}
        -DLAPACK_LIBRARIES=${FAISS_LAPACK_LIBS}
)

vcpkg_cmake_install()
vcpkg_copy_pdbs()
vcpkg_cmake_config_fixup()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
