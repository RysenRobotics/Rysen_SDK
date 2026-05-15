# rysen_apexhand_msgs

`rysen_apexhand` ROS2 节点使用的消息与服务定义包。

## 消息（msg）

- `FingerId.msg`
- `MotorState.msg`
- `HandTactileForces.msg`
- `HardwareErrors.msg`
- `TactileImage.msg`
- `TangentialForce.msg`
- `CommonFingerTactile.msg`
- `ThumbFingerTactile.msg`

说明：关节状态使用标准消息 `sensor_msgs/msg/JointState`，不在本包定义。

## 服务（srv）

### 手实例与连接

- `RemoveHand.srv`
  - Request: `string ip`
  - Response: `bool success`, `string message`
- `Connect.srv`（多手时对每个 IP 调用 `connect: true` 即可添加并连接）
  - Request: `bool connect`, `string ip`, `int32 connection_type`
  - Response: `bool success`, `string message`

### 状态与版本

- `GetConnectionInfo.srv`
  - Request: 空
  - Response: `string[] ips`, `bool[] connected`, `string[] device_ips`, `string[] hand_sides`
- `GetVersionInfo.srv`
  - Request: `string ip`
  - Response: `string sdk_version`, `string hand_firmware_version`, `string touch_sensor_version`

### 动作与配置

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
  - 说明：设置成功后 ROS 节点会释放 `original_ip` 对应的手实例，需用 `new_ip` 再 `connect`。

### 维护

- `StartTactileCalibration.srv`
  - Request: `string ip`
  - Response: `bool success`, `string message`
- `ClearTactileCalibration.srv`
  - Request: `string ip`
  - Response: `bool success`, `string message`
- `CleanFaults.srv`
  - Request: `string ip`
  - Response: `bool success`, `string message`

## 构建

```bash
cd /path/to/rysen_sdk/examples/ros2
colcon build --packages-select rysen_apexhand_msgs
source install/setup.bash
```

## 依赖使用

`package.xml`:

```xml
<depend>rysen_apexhand_msgs</depend>
```

`CMakeLists.txt`:

```cmake
find_package(rysen_apexhand_msgs REQUIRED)
```

## 相关文档

- [rysen_apexhand README](../rysen_apexhand/README.md)
