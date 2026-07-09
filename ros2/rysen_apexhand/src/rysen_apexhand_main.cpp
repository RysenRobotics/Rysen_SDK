/**
 * @file    rysen_apexhand_main.cpp
 * @brief   Entry point for the Rysen ApexHand ROS 2 node executable.
 *
 * Copyright (c) 2024-2026, Rysen Robotics (Shenzhen) Co. Ltd.
 * All rights reserved.
 *
 * Use of this source code is governed by a BSD 3-Clause license that can be
 * found in the LICENSE file.
 */

#include "rclcpp/executors/multi_threaded_executor.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rysen_apexhand/rysen_apexhand_node.hpp"

int main(int argc, char** argv) {
    signal(SIGPIPE, SIG_IGN);
    rclcpp::init(argc, argv);

    auto node = std::make_shared<rysen_apexhand::RysenApexHandNode>();

    rclcpp::executors::MultiThreadedExecutor executor;
    executor.add_node(node);
    executor.spin();

    rclcpp::shutdown();
    return 0;
}