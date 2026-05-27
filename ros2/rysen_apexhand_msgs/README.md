# rysen_apexhand_msgs
Message and service definition package used by the `rysen_apexhand` ROS 2 node.

## Messages (msg)
- `FingerId.msg`
- `MotorState.msg`
- `HandTactileForces.msg`
- `HardwareErrors.msg`
- `TactileImage.msg`
- `TangentialForce.msg`
- `CommonFingerTactile.msg`
- `ThumbFingerTactile.msg`

Note: Joint states use the standard message `sensor_msgs/msg/JointState` and are **not** defined in this package.

## Services (srv)
### Hand Instance & Connection
- `RemoveHand.srv`
  - Request: `string ip`
  - Response: `bool success`, `string message`
- `Connect.srv` (For multi-hand scenarios, call this service with `connect: true` for each target IP to add and connect)
  - Request: `bool connect`, `string ip`, `int32 connection_type`
  - Response: `bool success`, `string message`

### Status & Version
- `GetConnectionInfo.srv`
  - Request: Empty
  - Response: `string[] ips`, `bool[] connected`, `string[] device_ips`, `string[] hand_sides`
- `GetVersionInfo.srv`
  - Request: `string ip`
  - Response: `string sdk_version`, `string hand_firmware_version`, `string touch_sensor_version`

### Motion & Configuration
- `MoveJoint.srv`
  - Request: `string ip`, `uint8[] joint_ids`, `float64[] positions`, `float64[] velocities`, `float64[] accelerations`
  - Response: `bool success`, `string message`
- `SetFingerEnabled.srv`
  - Request: `string ip`, `FingerId[] finger_ids`, `bool enable`
  - Response: `bool success`, `string message`
- `SetAllFingersEnable.srv`
  - Request: `string ip`, `bool enable`
  - Response: `bool success`, `string message`
- `SetMaxJointSpeed.srv`
  - Request: `string ip`, `bool get_only`, `uint8[] joint_ids`, `float64[] max_speeds`
  - Response: `bool success`, `string message`, `float64[] max_speeds`
- `SetMaxJointAccel.srv`
  - Request: `string ip`, `bool get_only`, `uint8[] joint_ids`, `float64[] max_accels`
  - Response: `bool success`, `string message`, `float64[] max_accels`
- `SetMaxFingerTorque.srv`
  - Request: `string ip`, `bool get_only`, `uint8[] finger_ids`, `float64[] max_torques`
  - Response: `bool success`, `string message`, `float64[] max_torques`
- `SetDeviceIPAddress.srv`
  - Request: `string original_ip`, `string new_ip`
  - Response: `bool success`, `string message`
  - Note: After a successful IP change, the ROS 2 node releases the hand instance corresponding to `original_ip`. You must reconnect using the `new_ip`.

### Maintenance
- `StartTactileCalibration.srv`
  - Request: `string ip`
  - Response: `bool success`, `string message`
- `ClearTactileCalibration.srv`
  - Request: `string ip`
  - Response: `bool success`, `string message`
- `CleanFaults.srv`
  - Request: `string ip`
  - Response: `bool success`, `string message`

## Build
```bash
cd ros2/
colcon build --packages-select rysen_apexhand_msgs
source install/setup.bash
```

## Dependency Usage
`package.xml`:
```xml
<depend>rysen_apexhand_msgs</depend>
```

`CMakeLists.txt`:
```cmake
find_package(rysen_apexhand_msgs REQUIRED)
```