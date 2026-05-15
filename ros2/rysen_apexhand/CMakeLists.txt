cmake_minimum_required(VERSION 3.10)
project(rysen_apexhand)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_CXX_STANDARD VERSION_LESS 17)
  message(FATAL_ERROR "rysen_apexhand requires C++17 or higher")
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(builtin_interfaces REQUIRED)
find_package(rysen_apexhand_msgs REQUIRED)

# Find Rysen SDK (local package)
set(SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../rysen_sdk")
set(SDK_INCLUDE_DIR "${SDK_DIR}/include")

# --- 智能架构路由开始 ---
execute_process(
    COMMAND uname -m
    OUTPUT_VARIABLE SYSTEM_ARCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(SYSTEM_ARCH MATCHES "x86_64")
    message(STATUS "🚗 Detected x86_64 architecture. Using PC libraries.")
    set(SDK_LIB_DIR "${SDK_DIR}/lib/x86_64")
elseif(SYSTEM_ARCH MATCHES "aarch64" OR SYSTEM_ARCH MATCHES "arm64")
    message(STATUS "🍓 Detected ARM64 architecture. Using Raspberry Pi libraries.")
    set(SDK_LIB_DIR "${SDK_DIR}/lib/aarch64")
else()
    message(FATAL_ERROR "❌ Unsupported architecture: ${SYSTEM_ARCH}")
endif()
# --- 智能架构路由结束 ---

if(NOT EXISTS "${SDK_INCLUDE_DIR}" OR NOT EXISTS "${SDK_LIB_DIR}")
  message(FATAL_ERROR "Rysen SDK not found or architecture missing at ${SDK_LIB_DIR}")
endif()

# Find SDK library
file(GLOB SDK_LIB_FILES "${SDK_LIB_DIR}/librysen_sdk.so*")
if(NOT SDK_LIB_FILES)
  message(FATAL_ERROR "librysen_sdk.so not found in ${SDK_LIB_DIR}")
endif()
list(GET SDK_LIB_FILES 0 SDK_LIB_FILE)

# Create imported SDK target
add_library(rysen_sdk SHARED IMPORTED)
set_target_properties(rysen_sdk PROPERTIES
  IMPORTED_LOCATION "${SDK_LIB_FILE}"
)
get_filename_component(SDK_LIB_DIR_ABS "${SDK_LIB_DIR}" ABSOLUTE)

message(STATUS "Found Rysen SDK: ${SDK_LIB_FILE}")

# Build the node library
add_library(${PROJECT_NAME}_node SHARED
  src/rysen_apexhand_node.cpp
)

target_include_directories(${PROJECT_NAME}_node
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    "${SDK_INCLUDE_DIR}"
)

target_compile_features(${PROJECT_NAME}_node PUBLIC c_std_99 cxx_std_17)

ament_target_dependencies(${PROJECT_NAME}_node
  rclcpp
  std_msgs
  sensor_msgs
  geometry_msgs
  builtin_interfaces
  rysen_apexhand_msgs
)

target_link_libraries(${PROJECT_NAME}_node
  rysen_sdk
)

# Set RPATH for node library
set_target_properties(${PROJECT_NAME}_node PROPERTIES
  BUILD_RPATH "${SDK_LIB_DIR_ABS}"
  INSTALL_RPATH "$ORIGIN/rysen_sdk"
)

# Create executables
add_executable(${PROJECT_NAME}_node_exe
  src/rysen_apexhand_main.cpp
)

target_include_directories(${PROJECT_NAME}_node_exe
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    "${SDK_INCLUDE_DIR}"
)

ament_target_dependencies(${PROJECT_NAME}_node_exe
  rclcpp
  std_msgs
  sensor_msgs
  geometry_msgs
  builtin_interfaces
  rysen_apexhand_msgs
)

target_link_libraries(${PROJECT_NAME}_node_exe
  ${PROJECT_NAME}_node
  rysen_sdk
)

# Set RPATH for executables
set_target_properties(${PROJECT_NAME}_node_exe PROPERTIES
  BUILD_RPATH "${SDK_LIB_DIR_ABS}"
  INSTALL_RPATH "$ORIGIN/../rysen_sdk"
)

# Trajectory generator executable (does not require SDK, only publishes ROS2 messages)
add_executable(${PROJECT_NAME}_trajectory_generator_exe
  src/trajectory_generator_main.cpp
  src/trajectory_generator_node.cpp
)

target_include_directories(${PROJECT_NAME}_trajectory_generator_exe
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

ament_target_dependencies(${PROJECT_NAME}_trajectory_generator_exe
  rclcpp
  sensor_msgs
)

# ==========================================================
# Choreography Node (动作编排节点)
# ==========================================================
add_executable(${PROJECT_NAME}_choreography_node_exe
  src/choreography_main.cpp
  src/choreography_node.cpp
)

# 引入 thirdparty 目录作为头文件搜索路径
set(THIRDPARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty")

target_include_directories(${PROJECT_NAME}_choreography_node_exe
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${THIRDPARTY_DIR}>
    $<INSTALL_INTERFACE:include>
)

ament_target_dependencies(${PROJECT_NAME}_choreography_node_exe
  rclcpp
  sensor_msgs
  rysen_apexhand_msgs
)

# Install targets
install(TARGETS
  ${PROJECT_NAME}_node
  ${PROJECT_NAME}_node_exe
  ${PROJECT_NAME}_trajectory_generator_exe
  ${PROJECT_NAME}_choreography_node_exe
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

# Install directories
install(DIRECTORY launch/
  DESTINATION share/${PROJECT_NAME}/launch
)

install(DIRECTORY include/
  DESTINATION include
)

# Install SDK headers and libraries
install(DIRECTORY ${SDK_INCLUDE_DIR}/
  DESTINATION include/rysen_sdk
  FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)

install(DIRECTORY ${SDK_LIB_DIR}/
  DESTINATION lib/rysen_sdk
  FILES_MATCHING PATTERN "*.so*"
)

# Testing
if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

# Export
ament_export_include_directories(include)
ament_export_libraries(${PROJECT_NAME}_node)
ament_export_dependencies(
  rclcpp
  std_msgs
  sensor_msgs
  geometry_msgs
  builtin_interfaces
  rysen_apexhand_msgs
)

ament_package()
