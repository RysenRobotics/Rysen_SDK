/**
 * @file    rysen_apexhand_main.cpp
 * @brief   Entry point for the Rysen ApexHand ROS 2 node executable.
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

#include "rclcpp/executors/multi_threaded_executor.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rysen_apexhand/rysen_apexhand_node.hpp"

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);

    auto node = std::make_shared<rysen_apexhand::RysenApexHandNode>();

    rclcpp::executors::MultiThreadedExecutor executor;
    executor.add_node(node);
    executor.spin();

    rclcpp::shutdown();
    return 0;
}
