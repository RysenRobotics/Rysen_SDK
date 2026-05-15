/**
 * @file    trajectory_generator_node.hpp
 * @brief   ROS 2 node that publishes sinusoidal joint trajectories for testing.
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

#ifndef RYSEN_APEXHAND__TRAJECTORY_GENERATOR_NODE_HPP_
#define RYSEN_APEXHAND__TRAJECTORY_GENERATOR_NODE_HPP_

#include <chrono>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/joint_state.hpp"

namespace rysen_apexhand {

class TrajectoryGeneratorNode : public rclcpp::Node {
   public:
    explicit TrajectoryGeneratorNode(const rclcpp::NodeOptions& options = rclcpp::NodeOptions());

   private:
    void PublishTrajectory();

    rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr trajectory_pub_;
    rclcpp::TimerBase::SharedPtr timer_;

    double publish_frequency_;
    double min_angle_deg_;
    double max_angle_deg_;
    double period_sec_;
    std::vector<std::string> joint_names_;
    std::string frame_id_;

    double amplitude_deg_;
    double offset_deg_;

    double phase_;
    std::chrono::steady_clock::time_point last_timestamp_;
};

}  // namespace rysen_apexhand

#endif  // RYSEN_APEXHAND__TRAJECTORY_GENERATOR_NODE_HPP_
