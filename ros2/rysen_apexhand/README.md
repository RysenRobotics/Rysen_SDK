# Rysen ApexHand ROS2 节点

支持多手实例管理：每个 `ip` 对应一个独立 SDK 实例，并发布独立状态话题。

## 快速开始

```bash
cd /path/to/rysen_sdk/examples/ros2
colcon build --packages-select rysen_apexhand_msgs rysen_apexhand
source install/setup.bash
ros2 launch rysen_apexhand rysen_apexhand.launch.py
```

### 可选：同时启动 Foxglove Bridge

本 launch 可附带启动 `foxglove_bridge`，供 [Foxglove Studio](https://foxglove.dev/) 通过 WebSocket 订阅话题（需本机已安装 `ros-humble-foxglove-bridge` 或对应发行版包）。

```bash
# 安装（Ubuntu / ROS2 Humble 示例）
sudo apt install ros-humble-foxglove-bridge

# 与节点同时启动 bridge（默认端口 8765）
ros2 launch rysen_apexhand rysen_apexhand.launch.py launch_foxglove_bridge:=true

# 自定义端口
ros2 launch rysen_apexhand rysen_apexhand.launch.py \
  launch_foxglove_bridge:=true \
  foxglove_bridge_port:=8765 \
  foxglove_bridge_address:=0.0.0.0
```

在 Foxglove Studio 中打开 **Open connection → Foxglove WebSocket**，填写 `ws://<本机IP>:8765` 即可连接。

## 多手模型

- 对每个目标 IP 调用 `connect`（`connect: true`）会创建并连接对应的 `HandInstance`（独立 SDK 实例）；`connect: false` 或 `remove_hand` 可释放。
- 大多数服务请求都包含 `ip` 字段，通过 `ip` 路由到目标手。
- 每只手独立发布状态话题，避免数据混淆。

`ip_key` 规则：把 IP 中 `.` 替换为 `_`，并加前缀 `ip_`。例如 `192.168.0.102 -> ip_192_168_0_102`。

## 话题

### 发布（每手独立）

| 话题 | 类型 | 说明 |
|---|---|---|
| `rysen/apexhand/<ip_key>/joint_states` | `sensor_msgs/msg/JointState` | 关节状态 |
| `rysen/apexhand/<ip_key>/motor_states` | `rysen_apexhand_msgs/msg/MotorState` | 电机状态 |
| `rysen/apexhand/<ip_key>/hand_tactile_forces` | `rysen_apexhand_msgs/msg/HandTactileForces` | 触觉数据 |
| `rysen/apexhand/<ip_key>/hardware_errors` | `rysen_apexhand_msgs/msg/HardwareErrors` | 硬件错误事件 |

### 订阅（每手独立）

| 话题 | 类型 | 说明 |
|---|---|---|
| `rysen/apexhand/<ip_key>/move_j_position_follow_command` | `sensor_msgs/msg/JointState` | 跟随控制输入（每手独立 owner） |

## 服务

| 服务 | 类型 | 说明 |
|---|---|---|
| `rysen/apexhand/remove_hand` | `rysen_apexhand_msgs/srv/RemoveHand` | 移除手实例并释放连接 |
| `rysen/apexhand/connect` | `rysen_apexhand_msgs/srv/Connect` | 按 `ip` 连接/断开（多手时对每个 IP 各调用一次 `connect: true`） |
| `rysen/apexhand/move_joint` | `rysen_apexhand_msgs/srv/MoveJoint` | 阻塞式关节控制 |
| `rysen/apexhand/set_all_fingers` | `rysen_apexhand_msgs/srv/SetAllFingersEnable` | 全部手指使能/禁用 |
| `rysen/apexhand/set_finger_enabled` | `rysen_apexhand_msgs/srv/SetFingerEnabled` | 指定手指使能/禁用 |
| `rysen/apexhand/set_max_joint_speed` | `rysen_apexhand_msgs/srv/SetMaxJointSpeed` | 关节速度限幅 set/get |
| `rysen/apexhand/set_max_joint_accel` | `rysen_apexhand_msgs/srv/SetMaxJointAccel` | 关节加速度限幅 set/get |
| `rysen/apexhand/set_max_finger_torque` | `rysen_apexhand_msgs/srv/SetMaxFingerTorque` | 手指扭矩限幅 set/get |
| `rysen/apexhand/set_device_ip_address` | `rysen_apexhand_msgs/srv/SetDeviceIPAddress` | 修改设备端（固件）IP；成功后从节点移除该手实例，需用 `new_ip` 再 `connect` |
| `rysen/apexhand/start_tactile_calibration` | `rysen_apexhand_msgs/srv/StartTactileCalibration` | 开始触觉标定 |
| `rysen/apexhand/clear_tactile_calibration` | `rysen_apexhand_msgs/srv/ClearTactileCalibration` | 清空触觉标定 |
| `rysen/apexhand/clean_faults` | `rysen_apexhand_msgs/srv/CleanFaults` | 清故障 |
| `rysen/apexhand/get_connection_info` | `rysen_apexhand_msgs/srv/GetConnectionInfo` | 查询当前所有已添加手的连接信息 |
| `rysen/apexhand/get_version_info` | `rysen_apexhand_msgs/srv/GetVersionInfo` | 查询 SDK/手/触觉版本 |

## 服务调用示例（全量）

> 下面按默认服务名给出每一个服务的可执行示例。  
> 示例中统一使用 `192.168.0.102` 作为目标手 IP。

```bash
# 1) Connect.srv - connect=true 连接（多手时对每个 IP 调用一次）
ros2 service call /rysen/apexhand/connect rysen_apexhand_msgs/srv/Connect \
  "{connect: true, ip: '192.168.0.102', connection_type: 1}"

# 2) Connect.srv - connect=false 断开
ros2 service call /rysen/apexhand/connect rysen_apexhand_msgs/srv/Connect \
  "{connect: false, ip: '192.168.0.102', connection_type: 1}"

# 3) RemoveHand.srv（从节点移除该 IP 的实例）
ros2 service call /rysen/apexhand/remove_hand rysen_apexhand_msgs/srv/RemoveHand \
  "{ip: '192.168.0.102'}"

# 4) MoveJoint.srv
ros2 service call /rysen/apexhand/move_joint rysen_apexhand_msgs/srv/MoveJoint \
  "{ip: '192.168.0.102', joint_ids: [0,1], positions: [0.4,0.3], velocities: [1.0,1.0], accelerations: [2.0,2.0]}"

# 5) SetAllFingersEnable.srv
ros2 service call /rysen/apexhand/set_all_fingers rysen_apexhand_msgs/srv/SetAllFingersEnable \
  "{ip: '192.168.0.102', enable: true}"

# 6) SetFingerEnabled.srv
ros2 service call /rysen/apexhand/set_finger_enabled rysen_apexhand_msgs/srv/SetFingerEnabled \
  "{ip: '192.168.0.102', finger_ids: [{finger_id: 0}, {finger_id: 1}], enable: true}"

# 7) SetMaxJointSpeed.srv - set
ros2 service call /rysen/apexhand/set_max_joint_speed rysen_apexhand_msgs/srv/SetMaxJointSpeed \
  "{ip: '192.168.0.102', get_only: false, joint_ids: [0,1], max_speeds: [4.0,4.0]}"

# 8) SetMaxJointSpeed.srv - get
ros2 service call /rysen/apexhand/set_max_joint_speed rysen_apexhand_msgs/srv/SetMaxJointSpeed \
  "{ip: '192.168.0.102', get_only: true, joint_ids: [0,1], max_speeds: []}"

# 9) SetMaxJointAccel.srv - set
ros2 service call /rysen/apexhand/set_max_joint_accel rysen_apexhand_msgs/srv/SetMaxJointAccel \
  "{ip: '192.168.0.102', get_only: false, joint_ids: [0,1], max_accels: [20.0,20.0]}"

# 10) SetMaxJointAccel.srv - get
ros2 service call /rysen/apexhand/set_max_joint_accel rysen_apexhand_msgs/srv/SetMaxJointAccel \
  "{ip: '192.168.0.102', get_only: true, joint_ids: [0,1], max_accels: []}"

# 11) SetMaxFingerTorque.srv - set
ros2 service call /rysen/apexhand/set_max_finger_torque rysen_apexhand_msgs/srv/SetMaxFingerTorque \
  "{ip: '192.168.0.102', get_only: false, finger_ids: [0,1], max_torques: [50.0,50.0]}"

# 12) SetMaxFingerTorque.srv - get
ros2 service call /rysen/apexhand/set_max_finger_torque rysen_apexhand_msgs/srv/SetMaxFingerTorque \
  "{ip: '192.168.0.102', get_only: true, finger_ids: [0,1], max_torques: []}"

# 13) SetDeviceIPAddress.srv — 修改设备硬件 IP；成功后释放该 original_ip 对应的手，需用 new_ip 再 connect
ros2 service call /rysen/apexhand/set_device_ip_address rysen_apexhand_msgs/srv/SetDeviceIPAddress \
  "{original_ip: '192.168.0.102', new_ip: '192.168.0.103'}"

# 14) StartTactileCalibration.srv
ros2 service call /rysen/apexhand/start_tactile_calibration rysen_apexhand_msgs/srv/StartTactileCalibration \
  "{ip: '192.168.0.102'}"

# 15) ClearTactileCalibration.srv
ros2 service call /rysen/apexhand/clear_tactile_calibration rysen_apexhand_msgs/srv/ClearTactileCalibration \
  "{ip: '192.168.0.102'}"

# 16) CleanFaults.srv
ros2 service call /rysen/apexhand/clean_faults rysen_apexhand_msgs/srv/CleanFaults \
  "{ip: '192.168.0.102'}"

# 17) GetConnectionInfo.srv（返回所有已添加手）
ros2 service call /rysen/apexhand/get_connection_info rysen_apexhand_msgs/srv/GetConnectionInfo "{}"

# 18) GetVersionInfo.srv（指定某一只手）
ros2 service call /rysen/apexhand/get_version_info rysen_apexhand_msgs/srv/GetVersionInfo \
  "{ip: '192.168.0.102'}"
```

## 主要参数

| 参数 | 默认值 | 说明 |
|---|---|---|
| `device_ip` | `192.168.0.102` | 默认手 IP（`ip` 为空时使用） |
| `connection_type` | `1` | 默认连接类型 |
| `auto_connect` | `false` | 启动时自动连接，并自动使能自动连接到的所有手 |
| `auto_enable_on_connect` | `false` | 保留兼容参数（当前自动连接路径默认已自动使能） |
| `startup_hand_ips_csv` | `""` | 启动时自动添加并连接的多手 IP 列表（逗号分隔） |
| `auto_connect_startup_hands` | `true` | 是否自动连接 `startup_hand_ips_csv` 中的手 |
| `multi_hand_topic_prefix` | `rysen/apexhand` | 多手状态话题前缀 |
| `per_hand_topic_prefixes_csv` | `""` | 按手覆盖前缀，格式：`ip=prefix;ip=prefix` |
| `per_hand_follow_topics_csv` | `""` | 按手覆盖 follow 订阅话题，格式：`ip=topic;ip=topic` |
| `joint_states_topic` | `joint_states` | 关节状态话题后缀（可通过 launch 改名） |
| `motor_states_topic` | `motor_states` | 电机状态话题后缀（可通过 launch 改名） |
| `tactile_image_topic` | `hand_tactile_forces` | 触觉话题后缀（可通过 launch 改名） |
| `hardware_errors_topic` | `hardware_errors` | 硬件错误话题后缀（可通过 launch 改名） |
| `move_j_position_follow_command_topic` | `move_j_position_follow_command` | follow 订阅话题后缀（可通过 launch 改名） |
| `joint_states_pub_freq` | `100` | 关节状态频率 |
| `motor_states_pub_freq` | `100` | 电机状态频率 |
| `tactile_image_pub_freq` | `100` | 触觉频率 |
| `qos_depth` | `10` | 话题 QoS 深度 |
| `publish_qos_reliable` | `true` | 发布可靠性 |
| `subscribe_qos_reliable` | `false` | 订阅可靠性 |

## 相关文档

- 消息与服务定义：`../rysen_apexhand_msgs/README.md`

> 说明：默认手（`device_ip`）不允许通过 `RemoveHand` 移除。

```bash
# launch 启动多手（自动连接）
ros2 launch rysen_apexhand rysen_apexhand.launch.py \
  auto_connect:=true \
  device_ip:=192.168.0.102 \
  startup_hand_ips_csv:="192.168.0.103,192.168.0.104"

# 每只手不同前缀（同一个 launch）
ros2 launch rysen_apexhand rysen_apexhand.launch.py \
  auto_connect:=true \
  device_ip:=192.168.0.102 \
  startup_hand_ips_csv:="192.168.0.103" \
  per_hand_topic_prefixes_csv:="192.168.0.102=/handA;192.168.0.103=/handB"

# 双手专用 launch（左右手 follow 话题独立）
ros2 launch rysen_apexhand two_apex_hands.launch.py \
  left_hand_ip:=192.168.0.102 \
  right_hand_ip:=192.168.0.103 \
  left_hand_move_j_position_follow_command_topic:=/io_teleop/joint_cmd_finger_left \
  right_hand_move_j_position_follow_command_topic:=/io_teleop/joint_cmd_finger_right \
  subscribe_qos_reliable:=false \
  publish_qos_reliable:=true

# 双手专用 launch 默认行为与 rysen_apexhand.launch.py 一致
# （不传 left/right ns 和 left/right follow topic 时，仍是默认多手命名）
ros2 launch rysen_apexhand two_apex_hands.launch.py \
  left_hand_ip:=192.168.0.102 \
  right_hand_ip:=192.168.0.103

# 单个轨迹生成器 launch（可直接指定目标 follow 话题）
ros2 launch rysen_apexhand trajectory_generator.launch.py \
  move_j_position_follow_command_topic:=/io_teleop/joint_cmd_finger_left \
  publish_frequency:=20.0 \
  min_angle_deg:=0.0 \
  max_angle_deg:=80.0 \
  period_sec:=2.0 \
  joint_name:=f0_joint3 \
  frame_id:=base_link

# 或者不传 topic，默认按目标手自动拼接：
# <multi_hand_topic_prefix>/ip_<target_hand_ip>/move_j_position_follow_command
ros2 launch rysen_apexhand trajectory_generator.launch.py \
  target_hand_ip:=192.168.0.102 \
  publish_frequency:=20.0 \
  min_angle_deg:=0.0 \
  max_angle_deg:=80.0 \
  period_sec:=2.0 \
  joint_name:=f0_joint3 \
  frame_id:=base_link

# launch 改 topic 后缀（发布/订阅都会按 <prefix>/<ip_key>/<suffix> 生效）
ros2 launch rysen_apexhand rysen_apexhand.launch.py \
  joint_states_topic:=js \
  motor_states_topic:=ms \
  tactile_image_topic:=tactile \
  hardware_errors_topic:=hw_errors \
  move_j_position_follow_command_topic:=follow_cmd
```
