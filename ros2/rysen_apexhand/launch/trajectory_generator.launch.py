#!/usr/bin/env python3
# Copyright (c) 2024-2026, Rysen Robotics (Shenzhen) Co. Ltd.
# All rights reserved.
#
# This file is part of the proprietary rysen_sdk software development kit (SDK)
# provided by Rysen Robotics (Shenzhen) Co. Ltd.
# Use, reproduction, modification, distribution, or disclosure of this file,
# in whole or in part, is strictly prohibited without prior written permission
# from Rysen Robotics (Shenzhen) Co. Ltd.

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch_ros.actions import Node
from launch.substitutions import LaunchConfiguration


def launch_setup(context, *args, **kwargs):
    follow_topic = LaunchConfiguration("move_j_position_follow_command_topic").perform(context)
    if not follow_topic:
        # Backward-compatible alias.
        follow_topic = LaunchConfiguration("follow_topic").perform(context)

    return [
        Node(
            package="rysen_apexhand",
            executable="rysen_apexhand_trajectory_generator_exe",
            name="rysen_apexhand_trajectory_generator",
            output="screen",
            emulate_tty=True,
            parameters=[{
                "follow_topic": follow_topic,
                "target_hand_ip": LaunchConfiguration("target_hand_ip"),
                "multi_hand_topic_prefix": LaunchConfiguration("multi_hand_topic_prefix"),
                "follow_topic_suffix": LaunchConfiguration("follow_topic_suffix"),
                "publish_frequency": LaunchConfiguration("publish_frequency"),
                "min_angle_deg": LaunchConfiguration("min_angle_deg"),
                "max_angle_deg": LaunchConfiguration("max_angle_deg"),
                "period_sec": LaunchConfiguration("period_sec"),
                "frame_id": LaunchConfiguration("frame_id"),
                "joint_name": LaunchConfiguration("joint_name"),
            }],
        )
    ]


def generate_launch_description() -> LaunchDescription:
    return LaunchDescription([
        DeclareLaunchArgument("move_j_position_follow_command_topic", default_value=""),
        DeclareLaunchArgument("follow_topic", default_value=""),
        DeclareLaunchArgument("target_hand_ip", default_value="192.168.0.102"),
        DeclareLaunchArgument("multi_hand_topic_prefix", default_value="rysen/apexhand"),
        DeclareLaunchArgument("follow_topic_suffix", default_value="move_j_position_follow_command"),
        DeclareLaunchArgument("publish_frequency", default_value="20.0"),
        DeclareLaunchArgument("min_angle_deg", default_value="0.0"),
        DeclareLaunchArgument("max_angle_deg", default_value="80.0"),
        DeclareLaunchArgument("period_sec", default_value="2.0"),
        DeclareLaunchArgument("frame_id", default_value="base_link"),
        DeclareLaunchArgument("joint_name", default_value="f0_joint3"),
        OpaqueFunction(function=launch_setup),
    ])
