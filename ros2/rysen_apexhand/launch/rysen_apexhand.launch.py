#!/usr/bin/env python3
# Copyright (c) 2024-2026, Rysen Robotics (Shenzhen) Co. Ltd.
# All rights reserved.
#
# Use of this source code is governed by a BSD 3-Clause license that can be
# found in the LICENSE file.

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.conditions import IfCondition
from launch.launch_description_sources import AnyLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description() -> LaunchDescription:
    """Launch Rysen ApexHand ROS2 node."""
    declare_device_ip = DeclareLaunchArgument(
        "device_ip", default_value="192.168.0.102",
        description="Rysen device IP address");
    declare_frame_id = DeclareLaunchArgument(
        "frame_id", default_value="base_link", description="TF frame id for published messages");
    declare_multi_hand_topic_prefix = DeclareLaunchArgument(
        "multi_hand_topic_prefix", default_value="rysen/apexhand")
    declare_per_hand_topic_prefixes_csv = DeclareLaunchArgument(
        "per_hand_topic_prefixes_csv", default_value="",
        description="Per-hand topic prefixes, format: ip=prefix;ip=prefix")
    declare_per_hand_follow_topics_csv = DeclareLaunchArgument(
        "per_hand_follow_topics_csv", default_value="",
        description="Per-hand follow topics, format: ip=topic;ip=topic")

    declare_joint_states_topic = DeclareLaunchArgument(
        "joint_states_topic", default_value="joint_states");
    declare_motor_states_topic = DeclareLaunchArgument(
        "motor_states_topic", default_value="motor_states");
    declare_tactile_image_topic = DeclareLaunchArgument(
        "tactile_image_topic", default_value="hand_tactile_forces");
    declare_hardware_errors_topic = DeclareLaunchArgument(
        "hardware_errors_topic", default_value="hardware_errors");
    declare_move_j_position_follow_command_topic = DeclareLaunchArgument(
        "move_j_position_follow_command_topic",
        default_value="move_j_position_follow_command");
    declare_follow_control_owner_timeout_ms = DeclareLaunchArgument(
        "follow_control_owner_timeout_ms", default_value="100")
    declare_is_finger_enabled_service = DeclareLaunchArgument(
        "is_finger_enabled_service", 
        default_value="rysen/apexhand/is_finger_enabled",
        description="Service name to check if a finger is enabled")
    declare_move_joint_service = DeclareLaunchArgument(
        "move_joint_service", default_value="rysen/apexhand/move_joint");
    declare_remove_hand_service = DeclareLaunchArgument(
        "remove_hand_service", default_value="rysen/apexhand/remove_hand")
    declare_connect_service = DeclareLaunchArgument(
        "connect_service", default_value="rysen/apexhand/connect")
    declare_set_all_fingers_service = DeclareLaunchArgument(
        "set_all_fingers_service", default_value="rysen/apexhand/set_all_fingers")
    declare_set_finger_enabled_service = DeclareLaunchArgument(
        "set_finger_enabled_service", default_value="rysen/apexhand/set_finger_enabled")
    declare_set_max_joint_speed_service = DeclareLaunchArgument(
        "set_max_joint_speed_service", default_value="rysen/apexhand/set_max_joint_speed")
    declare_set_max_joint_accel_service = DeclareLaunchArgument(
        "set_max_joint_accel_service", default_value="rysen/apexhand/set_max_joint_accel")
    declare_set_max_finger_torque_service = DeclareLaunchArgument(
        "set_max_finger_torque_service", default_value="rysen/apexhand/set_max_finger_torque")
    declare_set_device_ip_address_service = DeclareLaunchArgument(
        "set_device_ip_address_service",
        default_value="rysen/apexhand/set_device_ip_address")
    declare_start_tactile_calibration_service = DeclareLaunchArgument(
        "start_tactile_calibration_service",
        default_value="rysen/apexhand/start_tactile_calibration")
    declare_clear_tactile_calibration_service = DeclareLaunchArgument(
        "clear_tactile_calibration_service",
        default_value="rysen/apexhand/clear_tactile_calibration")
    declare_clean_faults_service = DeclareLaunchArgument(
        "clean_faults_service", default_value="rysen/apexhand/clean_faults")
    declare_get_connection_info_service = DeclareLaunchArgument(
        "get_connection_info_service", default_value="rysen/apexhand/get_connection_info")
    declare_get_version_info_service = DeclareLaunchArgument(
        "get_version_info_service", default_value="rysen/apexhand/get_version_info")

    # connection_type is an int param in the node (1=Ethernet)
    declare_connection_type = DeclareLaunchArgument(
        "connection_type", default_value="1");
    declare_auto_connect = DeclareLaunchArgument("auto_connect", default_value="false")
    declare_auto_enable_on_connect = DeclareLaunchArgument(
        "auto_enable_on_connect", default_value="false")
    declare_startup_hand_ips_csv = DeclareLaunchArgument(
        "startup_hand_ips_csv", default_value="",
        description="Comma-separated IPs to auto-add/connect at startup")
    declare_auto_connect_startup_hands = DeclareLaunchArgument(
        "auto_connect_startup_hands", default_value="true")

    # Optional: start foxglove_bridge so Foxglove Studio can connect (Open Connection → Foxglove WebSocket → ws://host:port)
    declare_launch_foxglove_bridge = DeclareLaunchArgument(
        "launch_foxglove_bridge", default_value="false",
        description="If true, also start foxglove_bridge (install: sudo apt install ros-${ROS_DISTRO}-foxglove-bridge)")
    declare_foxglove_bridge_port = DeclareLaunchArgument(
        "foxglove_bridge_port", default_value="8765",
        description="WebSocket port for foxglove_bridge")
    declare_foxglove_bridge_address = DeclareLaunchArgument(
        "foxglove_bridge_address", default_value="0.0.0.0",
        description="Bind address for foxglove_bridge")

    rysen_node = Node(
        package="rysen_apexhand",
        executable="rysen_apexhand_node_exe",
        name="rysen_apexhand_node",
        output="screen",
        emulate_tty=True,
        parameters=[
            {
                "device_ip": LaunchConfiguration("device_ip"),
                "connection_type": LaunchConfiguration("connection_type"),
                "auto_connect": LaunchConfiguration("auto_connect"),
                "auto_enable_on_connect": LaunchConfiguration("auto_enable_on_connect"),
                "startup_hand_ips_csv": LaunchConfiguration("startup_hand_ips_csv"),
                "auto_connect_startup_hands": LaunchConfiguration("auto_connect_startup_hands"),
                "frame_id": LaunchConfiguration("frame_id"),
                "multi_hand_topic_prefix": LaunchConfiguration("multi_hand_topic_prefix"),
                "per_hand_topic_prefixes_csv": LaunchConfiguration("per_hand_topic_prefixes_csv"),
                "per_hand_follow_topics_csv": LaunchConfiguration("per_hand_follow_topics_csv"),
                "joint_states_topic": LaunchConfiguration("joint_states_topic"),
                "motor_states_topic": LaunchConfiguration("motor_states_topic"),
                "tactile_image_topic": LaunchConfiguration("tactile_image_topic"),
                "hardware_errors_topic": LaunchConfiguration("hardware_errors_topic"),
                "move_j_position_follow_command_topic":
                    LaunchConfiguration("move_j_position_follow_command_topic"),
                "follow_control_owner_timeout_ms":
                    LaunchConfiguration("follow_control_owner_timeout_ms"),
                "is_finger_enabled_service": LaunchConfiguration("is_finger_enabled_service"),
                "move_joint_service": LaunchConfiguration("move_joint_service"),
                "remove_hand_service": LaunchConfiguration("remove_hand_service"),
                "connect_service": LaunchConfiguration("connect_service"),
                "set_all_fingers_service": LaunchConfiguration("set_all_fingers_service"),
                "set_finger_enabled_service": LaunchConfiguration("set_finger_enabled_service"),
                "set_max_joint_speed_service": LaunchConfiguration("set_max_joint_speed_service"),
                "set_max_joint_accel_service": LaunchConfiguration("set_max_joint_accel_service"),
                "set_max_finger_torque_service":
                    LaunchConfiguration("set_max_finger_torque_service"),
                "set_device_ip_address_service":
                    LaunchConfiguration("set_device_ip_address_service"),
                "start_tactile_calibration_service":
                    LaunchConfiguration("start_tactile_calibration_service"),
                "clear_tactile_calibration_service":
                    LaunchConfiguration("clear_tactile_calibration_service"),
                "clean_faults_service": LaunchConfiguration("clean_faults_service"),
                "get_connection_info_service":
                    LaunchConfiguration("get_connection_info_service"),
                "get_version_info_service":
                    LaunchConfiguration("get_version_info_service"),
            }
        ],
    )

    foxglove_bridge = IncludeLaunchDescription(
        AnyLaunchDescriptionSource(
            PathJoinSubstitution(
                [FindPackageShare("foxglove_bridge"), "launch", "foxglove_bridge_launch.xml"]
            )
        ),
        condition=IfCondition(LaunchConfiguration("launch_foxglove_bridge")),
        launch_arguments={
            "port": LaunchConfiguration("foxglove_bridge_port"),
            "address": LaunchConfiguration("foxglove_bridge_address"),
        }.items(),
    )

    return LaunchDescription(
        [
            declare_device_ip,
            declare_connection_type,
            declare_auto_connect,
            declare_auto_enable_on_connect,
            declare_startup_hand_ips_csv,
            declare_auto_connect_startup_hands,
            declare_frame_id,
            declare_multi_hand_topic_prefix,
            declare_per_hand_topic_prefixes_csv,
            declare_per_hand_follow_topics_csv,
            declare_joint_states_topic,
            declare_motor_states_topic,
            declare_tactile_image_topic,
            declare_hardware_errors_topic,
            declare_move_j_position_follow_command_topic,
            declare_follow_control_owner_timeout_ms,
            declare_is_finger_enabled_service,
            declare_move_joint_service,
            declare_remove_hand_service,
            declare_connect_service,
            declare_set_all_fingers_service,
            declare_set_finger_enabled_service,
            declare_set_max_joint_speed_service,
            declare_set_max_joint_accel_service,
            declare_set_max_finger_torque_service,
            declare_set_device_ip_address_service,
            declare_start_tactile_calibration_service,
            declare_clear_tactile_calibration_service,
            declare_clean_faults_service,
            declare_get_connection_info_service,
            declare_get_version_info_service,
            declare_launch_foxglove_bridge,
            declare_foxglove_bridge_port,
            declare_foxglove_bridge_address,
            rysen_node,
            foxglove_bridge,
        ]
    )