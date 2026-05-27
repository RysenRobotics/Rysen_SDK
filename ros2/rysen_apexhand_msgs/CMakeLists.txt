cmake_minimum_required(VERSION 3.10)
project(rysen_apexhand_msgs)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(std_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(builtin_interfaces REQUIRED)
find_package(rosidl_default_generators REQUIRED)

# Generate messages and services
rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/FingerId.msg"
  "msg/MotorState.msg"
  "msg/TangentialForce.msg"
  "msg/TactileImage.msg"
  "msg/CommonFingerTactile.msg"
  "msg/ThumbFingerTactile.msg"
  "msg/HandTactileForces.msg"
  "msg/HardwareErrors.msg"
  "srv/MoveJoint.srv"
  "srv/SetFingerEnabled.srv"
  "srv/SetDeviceIPAddress.srv"
  "srv/Connect.srv"
  "srv/SetAllFingersEnable.srv"
  "srv/SetMaxJointSpeed.srv"
  "srv/SetMaxJointAccel.srv"
  "srv/SetMaxFingerTorque.srv"
  "srv/StartTactileCalibration.srv"
  "srv/StartIoTeleop.srv"
  "srv/ClearTactileCalibration.srv"
  "srv/CleanFaults.srv"
  "srv/GetConnectionInfo.srv"
  "srv/GetVersionInfo.srv"
  "srv/RemoveHand.srv"
  "srv/IsFingerEnabled.srv"
  DEPENDENCIES std_msgs sensor_msgs geometry_msgs builtin_interfaces
)

ament_export_dependencies(rosidl_default_runtime)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()

