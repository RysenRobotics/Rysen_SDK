/**
 * @file    trajectory_generator_node.cpp
 * @brief   Implementation of the trajectory generator ROS 2 node.
 *
 * Copyright (c) 2024-2026, Rysen Robotics (Shenzhen) Co. Ltd.
 * All rights reserved.
 *
 * This file is part of the proprietary rysen_sdk software development kit (SDK)
 * provided by Rysen Robotics (Shenzhen) Co. Ltd.
 * Use, reproduction, modification, distribution, or disclosure of this file,
 * in whole or in part, is strictly prohibited without prior written permission
 * from Rysen Robotics (Shenzhen) Co. Ltd.
 */

#include "rysen_apexhand/trajectory_generator_node.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace rysen_apexhand {

TrajectoryGeneratorNode::TrajectoryGeneratorNode(const rclcpp::NodeOptions& options)
    : Node("trajectory_generator", options) {
    this->declare_parameter<double>("publish_frequency", 20.0);
    this->declare_parameter<double>("min_angle_deg", 0.0);
    this->declare_parameter<double>("max_angle_deg", 80.0);
    this->declare_parameter<double>("period_sec", 2.0);
    this->declare_parameter<std::string>("frame_id", "base_link");
    this->declare_parameter<std::string>("follow_topic", "");
    this->declare_parameter<std::string>("target_hand_ip", "192.168.0.102");
    this->declare_parameter<std::string>("multi_hand_topic_prefix", "rysen/apexhand");
    this->declare_parameter<std::string>("follow_topic_suffix", "move_j_position_follow_command");
    this->declare_parameter<std::string>("joint_name", "");
    this->declare_parameter<std::vector<std::string>>("joint_names", std::vector<std::string>{});

    publish_frequency_ = this->get_parameter("publish_frequency").as_double();
    min_angle_deg_ = this->get_parameter("min_angle_deg").as_double();
    max_angle_deg_ = this->get_parameter("max_angle_deg").as_double();
    period_sec_ = this->get_parameter("period_sec").as_double();
    frame_id_ = this->get_parameter("frame_id").as_string();
    const std::string follow_topic_param = this->get_parameter("follow_topic").as_string();
    const std::string target_hand_ip = this->get_parameter("target_hand_ip").as_string();
    const std::string multi_hand_topic_prefix =
        this->get_parameter("multi_hand_topic_prefix").as_string();
    const std::string follow_topic_suffix = this->get_parameter("follow_topic_suffix").as_string();

    std::string single_joint_name = this->get_parameter("joint_name").as_string();
    if (!single_joint_name.empty()) {
        joint_names_ = {single_joint_name};
    } else {
        joint_names_ = this->get_parameter("joint_names").as_string_array();
        if (joint_names_.empty()) {
            RCLCPP_WARN(this->get_logger(),
                        "No joint names specified, using default: thumb_cmc_flex");
            joint_names_ = {"thumb_cmc_flex"};
        }
    }

    amplitude_deg_ = (max_angle_deg_ - min_angle_deg_) / 2.0;
    offset_deg_ = (max_angle_deg_ + min_angle_deg_) / 2.0;

    std::string follow_topic = follow_topic_param;
    if (follow_topic.empty()) {
        std::string ip_key = target_hand_ip;
        std::replace(ip_key.begin(), ip_key.end(), '.', '_');
        ip_key = "ip_" + ip_key;
        follow_topic = multi_hand_topic_prefix + "/" + ip_key + "/" + follow_topic_suffix;
    }
    trajectory_pub_ = this->create_publisher<sensor_msgs::msg::JointState>(
        follow_topic, rclcpp::QoS(10).reliable());

    auto period = std::chrono::milliseconds(static_cast<int>(1000.0 / publish_frequency_));
    timer_ = this->create_wall_timer(period, [this]() { PublishTrajectory(); });

    phase_ = 0.0;
    last_timestamp_ = std::chrono::steady_clock::now();

    std::string joints_str;
    for (size_t i = 0; i < joint_names_.size(); ++i) {
        if (i > 0)
            joints_str += ", ";
        joints_str += joint_names_[i];
    }

    RCLCPP_INFO(this->get_logger(),
                "Trajectory generator started: topic=%s, joints=[%s], range=[%.1f, %.1f] deg, "
                "period=%.2f s, freq=%.1f Hz",
                follow_topic.c_str(), joints_str.c_str(), min_angle_deg_, max_angle_deg_,
                period_sec_, publish_frequency_);
}

void TrajectoryGeneratorNode::PublishTrajectory() {
    auto now = std::chrono::steady_clock::now();
    auto time_since_last =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - last_timestamp_);
    last_timestamp_ = now;

    const double phase_increment = 2.0 * M_PI * (time_since_last.count() / 1000.0) / period_sec_;
    phase_ += phase_increment;

    if (phase_ >= 2.0 * M_PI) {
        phase_ -= 2.0 * M_PI;
    }

    const double position_deg = amplitude_deg_ * std::sin(phase_) + offset_deg_;
    const double degree_to_rad = M_PI / 180.0;
    const double position = position_deg * degree_to_rad;

    auto msg = std::make_shared<sensor_msgs::msg::JointState>();
    msg->header.stamp = this->now();
    msg->header.frame_id = frame_id_;
    msg->name = joint_names_;
    msg->position.resize(joint_names_.size(), position);
    msg->velocity.resize(joint_names_.size(), 0.0);
    msg->effort.resize(joint_names_.size(), 0.0);

    trajectory_pub_->publish(*msg);
}

}  // namespace rysen_apexhand
