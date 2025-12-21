cmake_minimum_required(VERSION 3.20)

# Image Analysis Plugins for ThemisDB
project(themis_image_analysis_plugins VERSION 1.0.0)

# Options
option(THEMIS_BUILD_IMAGE_PLUGINS "Build image analysis plugins" ON)
option(THEMIS_BUILD_ONNX_CLIP_PLUGIN "Build ONNX CLIP plugin" ON)
option(THEMIS_BUILD_LLAMACPP_VISION_PLUGIN "Build llama.cpp vision plugin" OFF)
option(THEMIS_BUILD_OPENCV_DNN_PLUGIN "Build OpenCV DNN plugin" OFF)
option(THEMIS_BUILD_STABLE_DIFFUSION_PLUGIN "Build Stable Diffusion ONNX plugin" OFF)

if(THEMIS_BUILD_IMAGE_PLUGINS)
    message(STATUS "Building ThemisDB Image Analysis Plugins")
    
    # Find common dependencies
    find_package(nlohmann_json REQUIRED)
    
    # ONNX CLIP Plugin
    if(THEMIS_BUILD_ONNX_CLIP_PLUGIN)
        message(STATUS "  - ONNX CLIP Plugin")
        add_subdirectory(onnx_clip)
    endif()
    
    # llama.cpp Vision Plugin
    if(THEMIS_BUILD_LLAMACPP_VISION_PLUGIN)
        message(STATUS "  - llama.cpp Vision Plugin")
        add_subdirectory(llamacpp_vision)
    endif()
    
    # OpenCV DNN Plugin
    if(THEMIS_BUILD_OPENCV_DNN_PLUGIN)
        message(STATUS "  - OpenCV DNN Plugin")
        add_subdirectory(opencv_dnn)
    endif()
    
    # Stable Diffusion ONNX Plugin
    if(THEMIS_BUILD_STABLE_DIFFUSION_PLUGIN)
        message(STATUS "  - Stable Diffusion ONNX Plugin")
        add_subdirectory(stable_diffusion_onnx)
    endif()
    
endif()
