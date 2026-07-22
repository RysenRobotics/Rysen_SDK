#!/usr/bin/env python3
# Copyright (c) 2024-2026, Rysen Robotics (Shenzhen) Co. Ltd.
# All rights reserved.
#
# Use of this source code is governed by a BSD 3-Clause license that can be
# found in the LICENSE file.

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch_ros.actions import Node
from launch.substitutions import LaunchConfiguration


def launch_setup(context, *args, **kwargs):
    left_ip = LaunchConfiguration("left_hand_ip").perform(context)
    right_ip = LaunchConfiguration("right_hand_ip").perform(context)
    left_ns = LaunchConfiguration("left_hand_ns").perform(context)
    right_ns = LaunchConfiguration("right_hand_ns").perform(context)
    left_follow = LaunchConfiguration("left_hand_move_j_position_follow_command_topic").perform(context)
    right_follow = LaunchConfiguration("right_hand_move_j_position_follow_command_topic").perform(context)
    left_control_follow = LaunchConfiguration(
        "left_hand_move_j_control_follow_command_topic").perform(context)
    right_control_follow = LaunchConfiguration(
        "right_hand_move_j_control_follow_command_topic").perform(context)

    multi_hand_topic_prefix = LaunchConfiguration("multi_hand_topic_prefix").perform(context)
    follow_topic_suffix = LaunchConfiguration("move_j_position_follow_command_topic").perform(context)
    control_follow_topic_suffix = LaunchConfiguration(
        "move_j_control_follow_command_topic").perform(context)

    prefix_entries = []
    if left_ns:
        prefix_entries.append(f"{left_ip}=/{left_ns}")
    if right_ns:
        prefix_entries.append(f"{right_ip}=/{right_ns}")
    per_hand_prefixes_csv = ";".join(prefix_entries)

    follow_entries = []
    if left_follow:
        follow_entries.append(f"{left_ip}={left_follow}")
    if right_follow:
        follow_entries.append(f"{right_ip}={right_follow}")
    per_hand_follow_topics_csv = ";".join(follow_entries)

    control_follow_entries = []
    if left_control_follow:
        control_follow_entries.append(f"{left_ip}={left_control_follow}")
    if right_control_follow:
        control_follow_entries.append(f"{right_ip}={right_control_follow}")
    per_hand_control_follow_topics_csv = ";".join(control_follow_entries)

    node = Node(
        package="rysen_apexhand",
        executable="rysen_apexhand_node_exe",
        name="rysen_apexhand_node",
        output="screen",
        emulate_tty=True,
        parameters=[{
            "device_ip": left_ip,
            "connection_type": LaunchConfiguration("connection_type"),
            "auto_connect": True,
            "auto_enable_on_connect": LaunchConfiguration("auto_enable_on_connect"),
            "startup_hand_ips_csv": right_ip,
            "auto_connect_startup_hands": True,
            "frame_id": LaunchConfiguration("frame_id"),
            "multi_hand_topic_prefix": multi_hand_topic_prefix,
            "per_hand_topic_prefixes_csv": per_hand_prefixes_csv,
            "per_hand_follow_topics_csv": per_hand_follow_topics_csv,
            "per_hand_control_follow_topics_csv": per_hand_control_follow_topics_csv,
            "joint_states_topic": LaunchConfiguration("joint_states_topic"),
            "motor_states_topic": LaunchConfiguration("motor_states_topic"),
            "tactile_image_topic": LaunchConfiguration("tactile_image_topic"),
            "hardware_errors_topic": LaunchConfiguration("hardware_errors_topic"),
            "move_j_position_follow_command_topic": follow_topic_suffix,
            "move_j_control_follow_command_topic": control_follow_topic_suffix,
            "follow_control_owner_timeout_ms": LaunchConfiguration("follow_control_owner_timeout_ms"),
            "publish_qos_reliable": LaunchConfiguration("publish_qos_reliable"),
            "subscribe_qos_reliable": LaunchConfiguration("subscribe_qos_reliable"),
        }],
    )
    return [node]


def generate_launch_description() -> LaunchDescription:
    return LaunchDescription([
        DeclareLaunchArgument("left_hand_ip", default_value="192.168.0.102"),
        DeclareLaunchArgument("right_hand_ip", default_value="192.168.0.103"),
        DeclareLaunchArgument("left_hand_ns", default_value=""),
        DeclareLaunchArgument("right_hand_ns", default_value=""),
        DeclareLaunchArgument("left_hand_move_j_position_follow_command_topic",
                              default_value=""),
        DeclareLaunchArgument("right_hand_move_j_position_follow_command_topic",
                              default_value=""),
        DeclareLaunchArgument("left_hand_move_j_control_follow_command_topic",
                              default_value=""),
        DeclareLaunchArgument("right_hand_move_j_control_follow_command_topic",
                              default_value=""),
        DeclareLaunchArgument("multi_hand_topic_prefix", default_value="rysen/apexhand"),
        DeclareLaunchArgument("move_j_position_follow_command_topic",
                              default_value="move_j_position_follow_command"),
        DeclareLaunchArgument("move_j_control_follow_command_topic",
                              default_value="move_j_control_follow_command"),
        DeclareLaunchArgument("connection_type", default_value="1"),
        DeclareLaunchArgument("auto_enable_on_connect", default_value="false"),
        DeclareLaunchArgument("frame_id", default_value="base_link"),
        DeclareLaunchArgument("joint_states_topic", default_value="joint_states"),
        DeclareLaunchArgument("motor_states_topic", default_value="motor_states"),
        DeclareLaunchArgument("tactile_image_topic", default_value="hand_tactile_forces"),
        DeclareLaunchArgument("hardware_errors_topic", default_value="hardware_errors"),
        DeclareLaunchArgument("follow_control_owner_timeout_ms", default_value="2000"),
        DeclareLaunchArgument("publish_qos_reliable", default_value="true"),
        DeclareLaunchArgument("subscribe_qos_reliable", default_value="false"),
        OpaqueFunction(function=launch_setup),
    ])
