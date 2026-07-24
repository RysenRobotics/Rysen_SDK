# Rysen ApexHand ROS2 Node

Supports multi-hand instance management: each `ip` corresponds to an independent SDK instance and publishes independent state topics.

## Quick Start

```bash
cd ros2/
colcon build
source install/setup.bash
ros2 launch rysen_apexhand rysen_apexhand.launch.py
```

### Optional: Launch Foxglove Bridge Simultaneously

This launch file can optionally start the `foxglove_bridge` to allow [Foxglove Studio](https://foxglove.dev/) to subscribe to topics via WebSocket (requires `ros-humble-foxglove-bridge` or the corresponding distribution package installed locally).

```bash
# Installation (Ubuntu / ROS2 Humble example)
sudo apt install ros-humble-foxglove-bridge

# Launch the bridge with the node (default port 8765)
ros2 launch rysen_apexhand rysen_apexhand.launch.py launch_foxglove_bridge:=true

# Custom port
ros2 launch rysen_apexhand rysen_apexhand.launch.py \
  launch_foxglove_bridge:=true \
  foxglove_bridge_port:=8765 \
  foxglove_bridge_address:=0.0.0.0
```

In Foxglove Studio, open **Open connection → Foxglove WebSocket** and fill in `ws://<Local IP>:8765` to connect.

## Multi-Hand Model

- Calling `connect` (`connect: true`) for each target IP creates and connects the corresponding `HandInstance` (independent SDK instance); `connect: false` or `remove_hand` can release it.
- Most service requests include an `ip` field, which routes to the target hand via `ip`.
- Each hand publishes state topics independently to avoid data confusion.

`ip_key` rule: Replace `.` in the IP with `_` and add the prefix `ip_`. For example, `192.168.0.102 -> ip_192_168_0_102`.

## Topics

### Published (Per Hand)

| Topic | Type | Description |
|---|---|---|
| `rysen/apexhand/<ip_key>/joint_states` | `sensor_msgs/msg/JointState` | Joint states |
| `rysen/apexhand/<ip_key>/motor_states` | `rysen_apexhand_msgs/msg/MotorState` | Motor states |
| `rysen/apexhand/<ip_key>/hand_tactile_forces` | `rysen_apexhand_msgs/msg/HandTactileForces` | Tactile data |
| `rysen/apexhand/<ip_key>/hardware_errors` | `rysen_apexhand_msgs/msg/HardwareErrors` | Hardware error events |

### Subscribed (Per Hand)

| Topic | Type | Description |
|---|---|---|
| `rysen/apexhand/<ip_key>/move_j_position_follow_command` | `sensor_msgs/msg/JointState` | Smooth position follow (`name`+`position`) |
| `rysen/apexhand/<ip_key>/move_j_control_follow_command` | `sensor_msgs/msg/JointState` | Direct control follow: `position` / `velocity` / `effort`(acceleration or current substitute); shares follow owner with position follow |

## Services

| Service | Type | Description |
|---|---|---|
| `rysen/apexhand/remove_hand` | `rysen_apexhand_msgs/srv/RemoveHand` | Remove hand instance and release connection |
| `rysen/apexhand/connect` | `rysen_apexhand_msgs/srv/Connect` | Connect/disconnect by `ip` (call `connect: true` once for each IP in multi-hand scenario) |
| `rysen/apexhand/move_joint` | `rysen_apexhand_msgs/srv/MoveJoint` | Blocking joint control |
| `rysen/apexhand/set_all_fingers` | `rysen_apexhand_msgs/srv/SetAllFingersEnable` | Enable/disable all fingers |
| `rysen/apexhand/set_finger_enabled` | `rysen_apexhand_msgs/srv/SetFingerEnabled` | Enable/disable specified fingers |
| `rysen/apexhand/set_max_joint_speed` | `rysen_apexhand_msgs/srv/SetMaxJointSpeed` | Joint speed limit set/get |
| `rysen/apexhand/set_max_joint_accel` | `rysen_apexhand_msgs/srv/SetMaxJointAccel` | Joint acceleration limit set/get |
| `rysen/apexhand/set_max_finger_torque` | `rysen_apexhand_msgs/srv/SetMaxFingerTorque` | Finger torque limit set/get |
| `rysen/apexhand/set_device_ip_address` | `rysen_apexhand_msgs/srv/SetDeviceIPAddress` | Modify the device-side (firmware) IP; after success, remove the hand instance from the node and reconnect with `new_ip` |
| `rysen/apexhand/start_tactile_calibration` | `rysen_apexhand_msgs/srv/StartTactileCalibration` | Start tactile calibration |
| `rysen/apexhand/clear_tactile_calibration` | `rysen_apexhand_msgs/srv/ClearTactileCalibration` | Clear tactile calibration |
| `rysen/apexhand/clean_faults` | `rysen_apexhand_msgs/srv/CleanFaults` | Clear faults |
| `rysen/apexhand/get_connection_info` | `rysen_apexhand_msgs/srv/GetConnectionInfo` | Query connection information of all added hands |
| `rysen/apexhand/get_version_info` | `rysen_apexhand_msgs/srv/GetVersionInfo` | Query SDK/hand/tactile version |

## Service Call Examples (Full List)

> Below are executable examples for each service using the default service names.  
> `192.168.0.102` is used as the target hand IP in all examples.

```bash
# 1) Connect.srv - connect=true to connect (call once per IP in multi-hand scenario)
ros2 service call /rysen/apexhand/connect rysen_apexhand_msgs/srv/Connect \
  "{connect: true, ip: '192.168.0.102', connection_type: 1}"

# 2) Connect.srv - connect=false to disconnect
ros2 service call /rysen/apexhand/connect rysen_apexhand_msgs/srv/Connect \
  "{connect: false, ip: '192.168.0.102', connection_type: 1}"

# 3) RemoveHand.srv (remove the instance of this IP from the node)
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

# 13) SetDeviceIPAddress.srv — Modify device hardware IP; after success, release the hand corresponding to original_ip and reconnect with new_ip
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

# 17) GetConnectionInfo.srv (returns all added hands)
ros2 service call /rysen/apexhand/get_connection_info rysen_apexhand_msgs/srv/GetConnectionInfo "{}"

# 18) GetVersionInfo.srv (specify a single hand)
ros2 service call /rysen/apexhand/get_version_info rysen_apexhand_msgs/srv/GetVersionInfo \
  "{ip: '192.168.0.102'}"
```

## Key Parameters

| Parameter | Default Value | Description |
|---|---|---|
| `device_ip` | `192.168.0.102` | Default hand IP (used when `ip` is empty) |
| `connection_type` | `1` | Default connection type |
| `auto_connect` | `false` | Automatically connect on startup, and automatically enable all hands connected via auto-connect |
| `auto_enable_on_connect` | `false` | Reserved compatibility parameter (auto-enable is already default in the current auto-connect path) |
| `startup_hand_ips_csv` | `""` | List of multi-hand IPs to automatically add and connect on startup (comma-separated) |
| `auto_connect_startup_hands` | `true` | Whether to automatically connect hands in `startup_hand_ips_csv` |
| `multi_hand_topic_prefix` | `rysen/apexhand` | Multi-hand state topic prefix |
| `per_hand_topic_prefixes_csv` | `""` | Override prefix per hand, format: `ip=prefix;ip=prefix` |
| `per_hand_follow_topics_csv` | `""` | Override position-follow subscription topic per hand, format: `ip=topic;ip=topic` |
| `per_hand_control_follow_topics_csv` | `""` | Override control-follow subscription topic per hand, format: `ip=topic;ip=topic` |
| `joint_states_topic` | `joint_states` | Joint state topic suffix (can be renamed via launch) |
| `motor_states_topic` | `motor_states` | Motor state topic suffix (can be renamed via launch) |
| `tactile_image_topic` | `hand_tactile_forces` | Tactile topic suffix (can be renamed via launch) |
| `hardware_errors_topic` | `hardware_errors` | Hardware error topic suffix (can be renamed via launch) |
| `move_j_position_follow_command_topic` | `move_j_position_follow_command` | Position-follow subscription topic suffix |
| `move_j_control_follow_command_topic` | `move_j_control_follow_command` | Direct control-follow subscription topic suffix |
| `joint_states_pub_freq` | `100` | Joint state publishing frequency |
| `motor_states_pub_freq` | `100` | Motor state publishing frequency |
| `tactile_image_pub_freq` | `100` | Tactile data publishing frequency |
| `qos_depth` | `10` | Topic QoS depth |
| `publish_qos_reliable` | `true` | Publish reliability |
| `subscribe_qos_reliable` | `false` | Subscribe reliability |

## Related Documents

- Message and service definitions: `../rysen_apexhand_msgs/README.md`

> Note: The default hand (`device_ip`) cannot be removed via `RemoveHand`.

```bash
# Launch multi-hand (auto-connect)
ros2 launch rysen_apexhand rysen_apexhand.launch.py \
  auto_connect:=true \
  device_ip:=192.168.0.102 \
  startup_hand_ips_csv:="192.168.0.103,192.168.0.104"

# Different prefixes for each hand (same launch)
ros2 launch rysen_apexhand rysen_apexhand.launch.py \
  auto_connect:=true \
  device_ip:=192.168.0.102 \
  startup_hand_ips_csv:="192.168.0.103" \
  per_hand_topic_prefixes_csv:="192.168.0.102=/handA;192.168.0.103=/handB"

# Dual-hand dedicated launch (independent follow topics for left/right hands)
ros2 launch rysen_apexhand two_apex_hands.launch.py \
  left_hand_ip:=192.168.0.102 \
  right_hand_ip:=192.168.0.103 \
  left_hand_move_j_position_follow_command_topic:=/io_teleop/joint_cmd_finger_left \
  right_hand_move_j_position_follow_command_topic:=/io_teleop/joint_cmd_finger_right \
  subscribe_qos_reliable:=false \
  publish_qos_reliable:=true

# The default behavior of the dual-hand dedicated launch is consistent with rysen_apexhand.launch.py
# (when left/right ns and left/right follow topics are not passed, the default multi-hand naming is still used)
ros2 launch rysen_apexhand two_apex_hands.launch.py \
  left_hand_ip:=192.168.0.102 \
  right_hand_ip:=192.168.0.103

# Launch to modify topic suffixes (both publishing and subscribing take effect as <prefix>/<ip_key>/<suffix>)
ros2 launch rysen_apexhand rysen_apexhand.launch.py \
  joint_states_topic:=js \
  motor_states_topic:=ms \
  tactile_image_topic:=tactile \
  hardware_errors_topic:=hw_errors \
  move_j_position_follow_command_topic:=follow_cmd
```