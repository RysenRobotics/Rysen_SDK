cmake_minimum_required(VERSION 3.10)
project(rysen_examples LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 选择 SDK
# 情况 1：作为顶层 SDK 工程的子目录构建，直接复用现有的 rysen_sdk 目标
# 情况 2：单独分发 examples/cpp，使用本目录下的 rysen_sdk/{include,lib}
set(_SDK_TARGET "")
set(_SDK_INCLUDE_DIRS "")
set(_SDK_RPATH "")

if (TARGET rysen_sdk)
    # 复用主工程里的目标和头文件
    set(_SDK_TARGET rysen_sdk)
    set(_SDK_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/include")
else()
    # 使用根目录下打包好的 SDK
    set(_LOCAL_SDK_INC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../rysen_sdk/include")
    
    # --- 智能架构路由开始 ---
    execute_process(
        COMMAND uname -m
        OUTPUT_VARIABLE SYSTEM_ARCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(SYSTEM_ARCH MATCHES "x86_64")
        message(STATUS "🚗 Detected x86_64 architecture. Using PC libraries.")
        set(_LOCAL_SDK_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../rysen_sdk/lib/x86_64")
    elseif(SYSTEM_ARCH MATCHES "aarch64" OR SYSTEM_ARCH MATCHES "arm64")
        message(STATUS "🍓 Detected ARM64 architecture. Using Raspberry Pi libraries.")
        set(_LOCAL_SDK_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../rysen_sdk/lib/aarch64")
    else()
        message(FATAL_ERROR "❌ Unsupported architecture: ${SYSTEM_ARCH}")
    endif()
    # --- 智能架构路由结束 ---

    if (EXISTS "${_LOCAL_SDK_INC_DIR}" AND EXISTS "${_LOCAL_SDK_LIB_DIR}")
        # 选择 librysen_sdk.so*. 优先未带版本号的 so，否则取第一个匹配
        set(_LOCAL_SO "")
        if (EXISTS "${_LOCAL_SDK_LIB_DIR}/librysen_sdk.so")
            set(_LOCAL_SO "${_LOCAL_SDK_LIB_DIR}/librysen_sdk.so")
        else()
            file(GLOB _CANDIDATES "${_LOCAL_SDK_LIB_DIR}/librysen_sdk.so*")
            list(LENGTH _CANDIDATES _NUMC)
            if (_NUMC GREATER 0)
                list(GET _CANDIDATES 0 _LOCAL_SO)
            endif()
        endif()
        if (EXISTS "${_LOCAL_SO}")
            add_library(rysen_sdk SHARED IMPORTED)
            set_target_properties(rysen_sdk PROPERTIES
                IMPORTED_LOCATION "${_LOCAL_SO}"
            )
            set(_SDK_TARGET rysen_sdk)
            set(_SDK_INCLUDE_DIRS "${_LOCAL_SDK_INC_DIR}")
            # 运行时从绝对路径加载本地 lib，避免相对路径计算复杂
            file(TO_CMAKE_PATH "${_LOCAL_SDK_LIB_DIR}" _ABS_LOCAL_LIB)
            set(_SDK_RPATH "${_ABS_LOCAL_LIB}")
            message(STATUS "Found Rysen SDK (local examples/cpp package): ${_LOCAL_SO}")
        else()
            message(FATAL_ERROR "librysen_sdk.so not found under examples/cpp/rysen_sdk/lib")
        endif()
    else()
        message(FATAL_ERROR "Local rysen_sdk package not found. Expected include and lib under examples/cpp/rysen_sdk/{include,lib}")
    endif()
endif()

# === 添加例程 ===
file(GLOB EXAMPLE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# 如果外层工程已经设置了可执行文件输出目录，则沿用外层设置；
# 若本目录单独构建，则默认输出到当前构建目录下的 bin/
if (NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()

foreach(example_source ${EXAMPLE_SOURCES})
    get_filename_component(example_name ${example_source} NAME_WE)
    add_executable(${example_name} ${example_source})

    if (_SDK_INCLUDE_DIRS)
        target_include_directories(${example_name} PRIVATE
            ${_SDK_INCLUDE_DIRS}
        )
    endif()

    target_link_libraries(${example_name} PRIVATE
        ${_SDK_TARGET}
    )

    # 让可执行程序在构建目录下运行时也能找到 .so
    if (_SDK_RPATH)
        set_target_properties(${example_name}
            PROPERTIES
            BUILD_RPATH "${_SDK_RPATH}"
            INSTALL_RPATH "${_SDK_RPATH}"
        )
    endif()
endforeach()

