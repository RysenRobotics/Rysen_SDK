/**
 * @file rysen_ros_example_node.cpp
 * @brief ROS2 ApexHand 客户端示例节点 / ROS2 ApexHand Client Example Node
 *
 * This example demonstrates comprehensive usage of the Rysen ApexHand ROS2 interface, including:
 * 本示例演示了 Rysen ApexHand ROS2 接口的全面使用方法，包括：
 *
 * - Connection and initialization via ROS2 services / 通过 ROS2 服务进行连接和初始化
 * - Finger enable/disable via services / 通过服务进行手指使能/禁用
 * - Motion parameter configuration via services / 通过服务进行运动参数配置
 * - Subscribing to joint, motor, and hand sensor topics / 订阅关节、电机和手传感器话题
 * - MoveJoint control via blocking service calls / 通过阻塞式服务调用进行 MoveJ 控制
 * - MoveJPositionFollow via topic publishing (non-blocking) / 通过发布话题进行位置随动控制（非阻塞）
 *
 * Copyright (c) 2024-2026, Rysen Robotics (Shenzhen) Co. Ltd.
 * All rights reserved.
 *
 * Use of this source code is governed by a BSD 3-Clause license that can be
 * found in the LICENSE file.
 */

#include <signal.h>

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/joint_state.hpp"
#include "rysen_apexhand_msgs/msg/motor_state.hpp"
#include "rysen_apexhand_msgs/msg/hand_tactile_forces.hpp"
#include "rysen_apexhand_msgs/msg/hardware_errors.hpp"
#include "rysen_apexhand_msgs/srv/connect.hpp"
#include "rysen_apexhand_msgs/srv/remove_hand.hpp"
#include "rysen_apexhand_msgs/srv/move_joint.hpp"
#include "rysen_apexhand_msgs/srv/set_all_fingers_enable.hpp"
#include "rysen_apexhand_msgs/srv/set_finger_enabled.hpp"
#include "rysen_apexhand_msgs/srv/set_max_joint_speed.hpp"
#include "rysen_apexhand_msgs/srv/set_max_joint_accel.hpp"
#include "rysen_apexhand_msgs/srv/set_max_finger_torque.hpp"
#include "rysen_apexhand_msgs/srv/get_version_info.hpp"
#include "rysen_apexhand_msgs/msg/finger_id.hpp"

// Globals / 全局变量
static volatile bool running = true;
static std::shared_ptr<rclcpp::Node> g_node;

// 全局声明订阅者，防止它们超出作用域被析构
static rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr g_joint_states_sub;
static rclcpp::Subscription<rysen_apexhand_msgs::msg::MotorState>::SharedPtr g_motor_states_sub;
static rclcpp::Subscription<rysen_apexhand_msgs::msg::HandTactileForces>::SharedPtr g_tactile_forces_sub;
static rclcpp::Subscription<rysen_apexhand_msgs::msg::HardwareErrors>::SharedPtr g_hardware_errors_sub;

// Constants / 常量
const std::vector<uint8_t> ALL_JOINT_IDS = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
                                             11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
const std::string DEFAULT_DEVICE_IP = "192.168.0.102";

/**
 * @brief Signal handler for Ctrl+C / Ctrl+C 信号处理函数
 */
void SignalHandler(int sig) {
    std::cout << "\nCaught signal " << sig << ", stopping..." << std::endl;
    running = false;
}

/**
 * @brief Call a ROS2 service and wait for response
 * 调用 ROS2 服务并等待响应 (使用后台 spinner 机制的同步等待)
 */
template <typename ServiceT>
bool CallService(const std::string& service_name,
                 typename ServiceT::Request::SharedPtr request,
                 typename ServiceT::Response::SharedPtr& response) {
    auto client = g_node->create_client<ServiceT>(service_name);
    
    // Wait for service to be available / 等待服务可用
    if (!client->wait_for_service(std::chrono::seconds(2))) {
        RCLCPP_ERROR(g_node->get_logger(), "❌ Service %s not available", service_name.c_str());
        return false;
    }

    auto future = client->async_send_request(request);
    
    // 因为我们有后台线程在 spin，所以可以直接 wait_for 阻塞等待，不会导致死锁
    if (future.wait_for(std::chrono::seconds(5)) != std::future_status::ready) {
        RCLCPP_ERROR(g_node->get_logger(), "❌ Service call to %s timed out", service_name.c_str());
        return false;
    }

    response = future.get();
    return true;
}

/**
 * @brief Connect to the hand / 连接到机械手
 */
bool ConnectHand(const std::string& device_ip) {
    std::cout << "\n=== 连接设备 ===" << std::endl;
    std::cout << "正在连接到设备: " << device_ip << std::endl;

    auto request = std::make_shared<rysen_apexhand_msgs::srv::Connect::Request>();
    request->connect = true;
    request->ip = device_ip;
    request->connection_type = 1;  // ETHERNET

    auto response = std::make_shared<rysen_apexhand_msgs::srv::Connect::Response>();
    if (!CallService<rysen_apexhand_msgs::srv::Connect>("rysen/apexhand/connect", request,
                                                        response)) {
        return false;
    }

    if (!response->success) {
        RCLCPP_ERROR(g_node->get_logger(), "❌ 连接失败: %s", response->message.c_str());
        return false;
    }

    std::cout << "✅ 连接成功" << std::endl;
    return true;
}

/**
 * @brief Disconnect from the hand / 断开与机械手的连接
 */
bool DisconnectHand(const std::string& device_ip) {
    std::cout << "\n=== 断开连接 ===" << std::endl;

    auto request = std::make_shared<rysen_apexhand_msgs::srv::Connect::Request>();
    request->connect = false;
    request->ip = device_ip;

    auto response = std::make_shared<rysen_apexhand_msgs::srv::Connect::Response>();
    if (!CallService<rysen_apexhand_msgs::srv::Connect>("rysen/apexhand/connect", request,
                                                        response)) {
        return false;
    }

    if (!response->success) {
        RCLCPP_WARN(g_node->get_logger(), "⚠️ 断开连接警告: %s", response->message.c_str());
    } else {
        std::cout << "✅ 已断开连接" << std::endl;
    }
    return true;
}

/**
 * @brief Enable all fingers / 使能所有手指
 */
bool SetAllFingersEnabled(const std::string& device_ip, bool enable) {
    std::string op = enable ? "使能" : "禁用";
    std::cout << "\n=== " << op << "所有手指 ===" << std::endl;

    auto request = std::make_shared<rysen_apexhand_msgs::srv::SetAllFingersEnable::Request>();
    request->ip = device_ip;
    request->enable = enable;

    auto response = std::make_shared<rysen_apexhand_msgs::srv::SetAllFingersEnable::Response>();
    if (!CallService<rysen_apexhand_msgs::srv::SetAllFingersEnable>(
            "rysen/apexhand/set_all_fingers", request, response)) {
        return false;
    }

    if (!response->success) {
        RCLCPP_ERROR(g_node->get_logger(), "❌ %s所有手指失败: %s", op.c_str(),
                     response->message.c_str());
        return false;
    }

    std::cout << "✅ 所有手指已" << op << std::endl;
    return true;
}

/**
 * @brief Set finger enabled/disabled / 使能/禁用单个手指
 */
bool SetFingerEnabled(const std::string& device_ip, uint8_t finger_id, bool enable) {
    std::string op = enable ? "使能" : "禁用";
    std::string finger_name[] = {"拇指", "食指", "中指", "无名指", "小指"};
    
    std::cout << "\n=== " << op << finger_name[finger_id] << "手指 ===" << std::endl;

    auto request = std::make_shared<rysen_apexhand_msgs::srv::SetFingerEnabled::Request>();
    request->ip = device_ip;
    request->enable = enable;
    
    rysen_apexhand_msgs::msg::FingerId finger_msg;
    finger_msg.finger_id = finger_id;
    request->finger_ids.push_back(finger_msg);

    auto response = std::make_shared<rysen_apexhand_msgs::srv::SetFingerEnabled::Response>();
    if (!CallService<rysen_apexhand_msgs::srv::SetFingerEnabled>(
            "rysen/apexhand/set_finger_enabled", request, response)) {
        return false;
    }

    if (!response->success) {
        RCLCPP_ERROR(g_node->get_logger(), "❌ %s手指失败: %s", op.c_str(),
                     response->message.c_str());
        return false;
    }

    std::cout << "✅ " << finger_name[finger_id] << "已" << op << std::endl;
    return true;
}

/**
 * @brief Set maximum joint speeds / 设置最大关节速度
 */
bool SetMaxJointSpeeds(const std::string& device_ip) {
    std::cout << "\n=== 设置最大关节速度 ===" << std::endl;

    auto request = std::make_shared<rysen_apexhand_msgs::srv::SetMaxJointSpeed::Request>();
    request->ip = device_ip;
    request->get_only = false;
    request->joint_ids = ALL_JOINT_IDS;
    request->max_speeds.assign(ALL_JOINT_IDS.size(), 8.0);  // 8.0 rad/s for all joints

    auto response = std::make_shared<rysen_apexhand_msgs::srv::SetMaxJointSpeed::Response>();
    if (!CallService<rysen_apexhand_msgs::srv::SetMaxJointSpeed>(
            "rysen/apexhand/set_max_joint_speed", request, response)) {
        return false;
    }

    if (!response->success) {
        RCLCPP_ERROR(g_node->get_logger(), "❌ 设置最大关节速度失败: %s",
                     response->message.c_str());
        return false;
    }

    std::cout << "✅ 最大关节速度已设置为 8.0 rad/s" << std::endl;
    return true;
}

/**
 * @brief Set maximum joint accelerations / 设置最大关节加速度
 */
bool SetMaxJointAccels(const std::string& device_ip) {
    std::cout << "\n=== 设置最大关节加速度 ===" << std::endl;

    auto request = std::make_shared<rysen_apexhand_msgs::srv::SetMaxJointAccel::Request>();
    request->ip = device_ip;
    request->get_only = false;
    request->joint_ids = ALL_JOINT_IDS;
    request->max_accels.assign(ALL_JOINT_IDS.size(), 260.0);  // 260.0 rad/s² for all joints

    auto response = std::make_shared<rysen_apexhand_msgs::srv::SetMaxJointAccel::Response>();
    if (!CallService<rysen_apexhand_msgs::srv::SetMaxJointAccel>(
            "rysen/apexhand/set_max_joint_accel", request, response)) {
        return false;
    }

    if (!response->success) {
        RCLCPP_ERROR(g_node->get_logger(), "❌ 设置最大关节加速度失败: %s",
                     response->message.c_str());
        return false;
    }

    std::cout << "✅ 最大关节加速度已设置为 260.0 rad/s²" << std::endl;
    return true;
}

/**
 * @brief Set maximum finger torques / 设置最大手指扭矩
 */
bool SetMaxFingerTorques(const std::string& device_ip) {
    std::cout << "\n=== 设置最大手指扭矩 ===" << std::endl;

    auto request = std::make_shared<rysen_apexhand_msgs::srv::SetMaxFingerTorque::Request>();
    request->ip = device_ip;
    request->get_only = false;
    
    for (uint8_t i = 0; i < 5; ++i) {
        // 直接推入 uint8_t 基础类型即可
        request->finger_ids.push_back(i);
        request->max_torques.push_back(50.0);
    }

    auto response = std::make_shared<rysen_apexhand_msgs::srv::SetMaxFingerTorque::Response>();
    if (!CallService<rysen_apexhand_msgs::srv::SetMaxFingerTorque>(
            "rysen/apexhand/set_max_finger_torque", request, response)) {
        return false;
    }

    if (!response->success) {
        RCLCPP_ERROR(g_node->get_logger(), "❌ 设置最大手指扭矩失败: %s",
                     response->message.c_str());
        return false;
    }

    std::cout << "✅ 最大手指扭矩已设置为 50.0 Nm" << std::endl;
    return true;
}

/**
 * @brief Callback for joint states topic / 关节状态话题回调
 */
void JointStatesCallback(const sensor_msgs::msg::JointState::SharedPtr msg) {
    static int count = 0;
    if (++count % 50 == 0) {  // 降低打印频率防止刷屏
        std::cout << "[JointStates] 接收到状态: 包含 " << msg->name.size() << " 个关节" << std::endl;
    }
}

/**
 * @brief Callback for motor states topic / 电机状态话题回调
 */
void MotorStatesCallback(const rysen_apexhand_msgs::msg::MotorState::SharedPtr) {
    static int count = 0;
    if (++count % 50 == 0) { 
        std::cout << "[MotorStates] 接收到电机底层状态" << std::endl;
    }
}

/**
 * @brief Callback for tactile forces topic / 触觉力话题回调
 */
void TactileForcesCallback(const rysen_apexhand_msgs::msg::HandTactileForces::SharedPtr) {
    static int count = 0;
    if (++count % 50 == 0) {
        std::cout << "[HandTactileForces] 接收到触觉传感器数据" << std::endl;
    }
}

/**
 * @brief Callback for hardware errors topic / 硬件错误话题回调
 */
void HardwareErrorsCallback(const rysen_apexhand_msgs::msg::HardwareErrors::SharedPtr) {
    RCLCPP_WARN(g_node->get_logger(), "⚠️ [HardwareErrors] 接收到硬件错误");
}

/**
 * @brief Subscribe to data topics / 订阅数据话题
 */
void SubscribeToTopics(const std::string& device_topic_prefix) {
    std::cout << "\n=== 订阅数据话题 ===" << std::endl;

    // 分配给全局指针，确保生命周期跟随程序
    g_joint_states_sub =
        g_node->create_subscription<sensor_msgs::msg::JointState>(
            device_topic_prefix + "/joint_states", 10, JointStatesCallback);

    g_motor_states_sub =
        g_node->create_subscription<rysen_apexhand_msgs::msg::MotorState>(
            device_topic_prefix + "/motor_states", 10, MotorStatesCallback);

    g_tactile_forces_sub =
        g_node->create_subscription<rysen_apexhand_msgs::msg::HandTactileForces>(
            device_topic_prefix + "/hand_tactile_forces", 10, TactileForcesCallback);

    g_hardware_errors_sub =
        g_node->create_subscription<rysen_apexhand_msgs::msg::HardwareErrors>(
            device_topic_prefix + "/hardware_errors", 10, HardwareErrorsCallback);

    std::cout << "✅ 已注册回调并订阅数据话题" << std::endl;
}

/**
 * @brief Execute MoveJoint command (blocking service call) / 执行 MoveJoint 命令（阻塞式服务调用）
 */
bool ExecuteMoveJoint(const std::string& device_ip,
                      const std::vector<std::pair<uint8_t, double>>& joint_targets) {
    std::cout << "\n=== 执行 MoveJoint 命令 ===" << std::endl;
    std::cout << "控制 " << joint_targets.size() << " 个关节..." << std::endl;

    auto request = std::make_shared<rysen_apexhand_msgs::srv::MoveJoint::Request>();
    request->ip = device_ip;

    for (const auto& target : joint_targets) {
        request->joint_ids.push_back(target.first);
        request->positions.push_back(target.second);
        request->velocities.push_back(2.0);      // 2.0 rad/s
        request->accelerations.push_back(8.0);   // 8.0 rad/s²
    }

    auto response = std::make_shared<rysen_apexhand_msgs::srv::MoveJoint::Response>();
    if (!CallService<rysen_apexhand_msgs::srv::MoveJoint>("rysen/apexhand/move_joint", request,
                                                          response)) {
        return false;
    }

    if (!response->success) {
        RCLCPP_ERROR(g_node->get_logger(), "❌ MoveJoint 失败: %s", response->message.c_str());
        return false;
    }

    std::cout << "✅ MoveJoint 完成，关节已到达目标位置" << std::endl;
    return true;
}

/**
 * @brief Perform finger reciprocating motion using MoveJPositionFollow (non-blocking via topic)
 * 通过发布话题进行手指往复运动（使用 MoveJPositionFollow，非阻塞）
 */
void PerformMoveJPositionFollowDemo(const std::string& device_topic_prefix) {
    std::cout << "\n=== 测试 MoveJPositionFollow（手指往复运动，通过话题发布）===" << std::endl;
    std::cout << "注意：MoveJPositionFollow 是非阻塞接口，通过发布话题实现" << std::endl;

    auto follow_pub = g_node->create_publisher<sensor_msgs::msg::JointState>(
        device_topic_prefix + "/move_j_position_follow_command", 10);

    // Define waypoints / 定义路径点
    struct Waypoint {
        double position;
        int delay_ms;
    };

    std::vector<Waypoint> waypoints = {
        {0.0, 100},   {0.1, 50},  {0.2, 50},  {0.3, 50},  {0.4, 50},   {0.5, 50},
        {0.6, 50},    {0.7, 50},  {0.8, 100}, {0.7, 50},  {0.6, 50},   {0.5, 50},
        {0.4, 50},    {0.3, 50},  {0.2, 50},  {0.1, 50},  {0.0, 100}};

    // 修复：这里使用规范的字符串名称，而不是简单的 "joint_x"
    const std::vector<std::string> finger_joint_names = {
        "thumb_mcp_flex",
        "index_mcp_flex",
        "middle_mcp_flex",
        "ring_mcp_flex",
        "little_mcp_flex"
    };

    const std::vector<std::string> finger_names = {"拇指", "食指", "中指", "无名指", "小指"};

    // Each finger performs 1 reciprocating cycle / 每个手指完成 1 个往复运动循环
    for (size_t finger_idx = 0; finger_idx < finger_joint_names.size() && running; ++finger_idx) {
        const std::string& target_joint_name = finger_joint_names[finger_idx];
        const std::string& finger_name = finger_names[finger_idx];

        std::cout << "\n--- " << finger_name << " 往复运动 ---" << std::endl;

        // Go through all waypoints / 依次经过所有路径点
        for (size_t i = 0; i < waypoints.size() && running; ++i) {
            const auto& waypoint = waypoints[i];

            auto msg = std::make_shared<sensor_msgs::msg::JointState>();
            msg->name.push_back(target_joint_name);
            msg->position.push_back(waypoint.position);

            follow_pub->publish(*msg);

            std::cout << "  点 " << (i + 1) << "/" << waypoints.size()
                      << ": 位置=" << std::fixed << std::setprecision(2) << waypoint.position
                      << " rad" << std::endl;

            if (i < waypoints.size() - 1) {
                std::this_thread::sleep_for(std::chrono::milliseconds(waypoint.delay_ms));
            }
        }

        std::cout << "✅ " << finger_name << " 往复运动完成" << std::endl;

        if (finger_idx < finger_joint_names.size() - 1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

    std::cout << "\n✅ MoveJPositionFollow 演示完成" << std::endl;
}

bool RemoveHand(const std::string& device_ip) {
    std::cout << "\n=== 从节点移除设备实例 ===" << std::endl;
    auto request = std::make_shared<rysen_apexhand_msgs::srv::RemoveHand::Request>();
    request->ip = device_ip;

    auto response = std::make_shared<rysen_apexhand_msgs::srv::RemoveHand::Response>();
    if (!CallService<rysen_apexhand_msgs::srv::RemoveHand>("rysen/apexhand/remove_hand", request, response)) {
        return false;
    }
    std::cout << (response->success ? "✅ 移除成功" : "❌ 移除失败") << std::endl;
    return response->success;
}

/**
 * @brief Main function / 主函数
 */
int main(int argc, char* argv[]) {
    // Initialize ROS2 / 初始化 ROS2
    rclcpp::init(argc, argv);
    
    g_node = rclcpp::Node::make_shared("ros_example_node");
    
    // Register signal handler / 注册信号处理函数
    signal(SIGINT, SignalHandler);

    //使用 Executor 管理节点，方便后续安全退出
    rclcpp::executors::SingleThreadedExecutor executor;
    executor.add_node(g_node);
    std::thread spin_thread([&executor]() {
        executor.spin();
    });

    std::string device_ip = DEFAULT_DEVICE_IP;
    std::string raw_topic_prefix = "rysen/apexhand/ip_" + device_ip;
    std::replace(raw_topic_prefix.begin(), raw_topic_prefix.end(), '.', '_');
    std::string device_topic_prefix = raw_topic_prefix; // Default topic prefix

    // Parse command line arguments / 解析命令行参数
    std::cout << "\n=== ROS2 ApexHand 客户端示例节点 ===" << std::endl;
    std::cout << "设备IP: " << device_ip << std::endl;
    std::cout << "话题前缀: " << device_topic_prefix << std::endl;

    // ========== Connection / 连接 ==========
    if (!ConnectHand(device_ip)) {
        RCLCPP_ERROR(g_node->get_logger(), "❌ 无法连接到设备，退出");
        running = false;
        rclcpp::shutdown();
        spin_thread.join();
        return 1;
    }

    // ========== Subscribe to topics (immediately start receiving data) / 立即订阅话题
    SubscribeToTopics(device_topic_prefix);

    // ========== Finger enable/disable tests / 手指使能/禁用测试 ==========
    std::cout << "\n=== 测试 SetAllFingersEnabled/Disabled 接口 ===" << std::endl;
    
    SetAllFingersEnabled(device_ip, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    SetAllFingersEnabled(device_ip, false);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Test single finger enable / 测试单个手指使能
    SetFingerEnabled(device_ip, 1, true);   // Enable index finger / 使能食指
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    SetFingerEnabled(device_ip, 1, false);  // Disable index finger / 禁用食指
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Enable all fingers again / 再次使能所有手指
    SetAllFingersEnabled(device_ip, true);

    // ========== Motion parameter configuration / 运动参数配置 ==========
    std::cout << "\n=== 设置运动参数 ===" << std::endl;
    SetMaxJointSpeeds(device_ip);
    SetMaxJointAccels(device_ip);
    SetMaxFingerTorques(device_ip);

    // Wait for data to stabilize / 等待数据稳定
    std::cout << "\n=== 等待数据稳定（2秒）===" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::cout << "✅ 数据已稳定" << std::endl;

    // ========== MoveJoint control test / MoveJoint 控制测试 ==========
    std::cout << "\n=== 测试 MoveJoint（阻塞式服务调用）===" << std::endl;

    // Move to target position / 移动到目标位置
    std::vector<std::pair<uint8_t, double>> targets_1 = {
        {0, 1.2},   // Thumb CMC ABD
        {1, 0.2},   // Thumb CMC ROT
        {2, 0.25},  // Thumb CMC FLEX
        {3, 0.4},   // Thumb MCP FLEX
        {4, 0.4},   // Thumb IP FLEX
        {5, 0.3},   // Index MCP ABD
        {6, 0.8},   // Index MCP FLEX
        {7, 0.8},   // Index PIP FLEX
        {8, 0.8},   // Index DIP FLEX

        {9, 0.15},   // Middle MCP ABD
        {10, 0.8},  // Middle MCP FLEX
        {11, 0.8},  // Middle PIP FLEX
        {12, 0.8},  // Middle DIP FLEX
        {13, -0.15},  // Ring MCP ABD
        {14, 0.8},  // Ring MCP FLEX
        {15, 0.8},  // Ring PIP FLEX
        {16, 0.8},  // Ring DIP FLEX
        {17, -0.3},  // Little MCP ABD
        {18, 0.8},  // Little MCP FLEX
        {19, 0.8},  // Little PIP FLEX
        {20, 0.8}   // Little DIP FLEX
    };
    ExecuteMoveJoint(device_ip, targets_1);

    // Return to initial position / 回到初始位置
    std::cout << "\n=== 测试 MoveJoint 回到初始位置 ===" << std::endl;
    std::vector<std::pair<uint8_t, double>> targets_initial(ALL_JOINT_IDS.size(), {0, 0.0});
    for (size_t i = 0; i < ALL_JOINT_IDS.size(); ++i) {
        targets_initial[i].first = ALL_JOINT_IDS[i];
    }
    ExecuteMoveJoint(device_ip, targets_initial);

    // ========== MoveJPositionFollow test (non-blocking via topic) / MoveJPositionFollow 测试
    if (running) {
        PerformMoveJPositionFollowDemo(device_topic_prefix);
    }

    // 保持连接，接收一会数据再退出
    std::cout << "\n=== 测试结束，持续接收数据5秒后退出 ===" << std::endl;
    for(int i = 0; i < 50 && running; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // ========== Cleanup / 清理资源 ==========
    std::cout << "\n=== 清理资源 ===" << std::endl;
    SetAllFingersEnabled(device_ip, false);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    DisconnectHand(device_ip);

    RemoveHand(device_ip);

    std::cout << "\n=== 进程结束 ===" << std::endl;

    // 遵循严格的 ROS 2 资源释放顺序
    running = false;
    
    // 1. 安全取消后台执行器，打断 spin 阻塞，并等待线程安全退出
    executor.cancel();
    if (spin_thread.joinable()) {
        spin_thread.join();
    }

    // 2. 在 rclcpp::shutdown() 之前，手动释放所有的 ROS 2 全局对象
    g_joint_states_sub.reset();
    g_motor_states_sub.reset();
    g_tactile_forces_sub.reset();
    g_hardware_errors_sub.reset();
    g_node.reset();

    // 3. 此时底层的收发实体已清空，可以安全关闭通信上下文了
    rclcpp::shutdown();  

    return 0;
}