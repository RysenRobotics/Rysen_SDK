/**
 * @file rysen_example.cpp
 * @brief Rysen ApexHand SDK C++ Example / Rysen ApexHand SDK C++ 示例程序
 *
 * This example demonstrates comprehensive usage of the Rysen ApexHand SDK, including:
 * 本示例演示了 Rysen ApexHand SDK 的全面使用方法，包括：
 *
 * - Connection and initialization / 连接和初始化
 * - Finger enable/disable / 手指使能/禁用
 * - Motion parameter configuration / 运动参数配置
 * - Joint, motor, and hand sensor image callbacks / 关节、电机和手传感器图像回调
 * - MoveJ control commands / MoveJ 控制命令
 * - Get functions (GetJointStates, GetMotorStates, GetHandSensorImage) / Get 函数
 * - MoveJPositionFollow (position following control) / MoveJPositionFollow（位置跟随控制）
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

#include <signal.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <rysen_apexhand_sdk.hpp>
#include <string>
#include <thread>

// Global flag for graceful shutdown / 全局标志位，用于优雅退出
static volatile bool running = true;

/**
 * @brief Signal handler for Ctrl+C / Ctrl+C 信号处理函数
 * @param sig Signal number / 信号编号
 */
void SignalHandler(int sig) {
    std::cout << "Caught signal " << sig << ", stopping..." << std::endl;
    running = false;
}

/**
 * @brief Main function / 主函数
 * @return 0 on success, 1 on error / 成功返回 0，失败返回 1
 */
int main([[maybe_unused]] int argc, char** argv) {
    // Register signal handler for Ctrl+C / 注册 Ctrl+C 信号处理函数
    signal(SIGINT, SignalHandler);

    // Create SDK instance / 创建 SDK 实例
    std::cout << "\n=== 创建 SDK 实例 ===" << std::endl;
    rysen::Rysen sdk;

    // Set logging before Connect so that connection logs are also written to file
    // 在 Connect 之前设置日志，这样连接相关日志也会写入文件
    std::cout << "\n=== 设置日志 ===" << std::endl;
    sdk.SetLogPath("./log");
    sdk.EnableLogging(true);
    std::cout << "✅ 日志已启用，路径: ./log" << std::endl;

    // Optional: load robot model from URDF (for planning/kinematics)
    // 可选：从 URDF 加载机器人模型（用于规划/运动学）
    // 使用“当前可执行文件所在目录”为基准的相对路径，避免依赖运行时工作目录。
    {
        std::filesystem::path exe_path = std::filesystem::canonical(argv[0]);
        std::filesystem::path exe_dir = exe_path.parent_path();
        std::filesystem::path urdf_fs_path = exe_dir / "../../../urdf/apex_hand_right.urdf";
        // std::filesystem::path urdf_fs_path = "../../urdf/apex_hand_right.urdf";
        const std::string urdf_path = urdf_fs_path.string();
        std::cout << "\n=== 从 URDF 加载机器人模型 ===" << std::endl;
        auto urdf_ret = sdk.SetRobotModelFromURDF(urdf_path);
        if (urdf_ret != rysen::ErrorCode::ERROR_CODE_OK) {
            std::cerr << "⚠️ 从 URDF 加载机器人模型失败，错误码: "
                      << static_cast<int>(urdf_ret) << "，继续仅基于关节命令运行示例" << std::endl;
        } else {
            std::cout << "✅ 已从 URDF 加载机器人模型: " << urdf_path << std::endl;
        }
    }

    // Connect to device / 连接设备
    // Note: Connection is required before receiving data / 注意：需要先连接才能接收数据
    std::cout << "\n=== 连接设备 ===" << std::endl;
    std::string device_ip = "192.168.0.102";  // Modify to your device IP / 可以修改为你的设备IP
    std::cout << "正在连接到设备: " << device_ip << std::endl;

    auto ret = sdk.Connect(device_ip, rysen::ConnectionType::CONNECTION_TYPE_ETHERNET);
    if (ret != rysen::ErrorCode::ERROR_CODE_OK) {
        std::cerr << "❌ 连接失败，错误码: " << static_cast<int>(ret) << std::endl;
        return 1;
    }
    std::cout << "✅ 连接成功" << std::endl;

    // ========== Finger Enable/Disable Tests / 手指使能/禁用测试 ==========
    //  std::this_thread::sleep_for(std::chrono::milliseconds(500));  // 等待5秒，确保数据线程启动
    // Test enable all fingers / 测试使能所有手指
    std::cout << "\n=== 测试 SetAllFingersEnabled 接口 ===" << std::endl;
    ret = sdk.SetAllFingersEnabled();
    if (ret != rysen::ErrorCode::ERROR_CODE_OK) {
        std::cerr << "❌ 使能所有手指失败，错误码: " << static_cast<int>(ret) << std::endl;
    } else {
        std::cout << "✅ 所有手指已使能" << std::endl;
    }

    // Test disable all fingers / 测试禁用所有手指
    std::cout << "\n=== 测试 SetAllFingersDisabled 接口 ===" << std::endl;
    ret = sdk.SetAllFingersDisabled();
    if (ret != rysen::ErrorCode::ERROR_CODE_OK) {
        std::cerr << "❌ 禁用所有手指失败，错误码: " << static_cast<int>(ret) << std::endl;
    } else {
        std::cout << "✅ 所有手指已禁用" << std::endl;
    }

    // Enable single finger / 使能单个手指
    std::cout << "\n=== 测试 SetFingerEnabled 接口 ===" << std::endl;
    ret = sdk.SetFingerEnabled({rysen::FingerId::FINGER_ID_INDEX});
    if (ret != rysen::ErrorCode::ERROR_CODE_OK) {
        std::cerr << "❌ 使能手指失败，错误码: " << static_cast<int>(ret) << std::endl;
    } else {
        std::cout << "✅ 手指已使能" << std::endl;
    }

    // Disable single finger / 禁用单个手指
    std::cout << "\n=== 测试 SetFingerDisabled 接口 ===" << std::endl;
    ret = sdk.SetFingerDisabled({rysen::FingerId::FINGER_ID_INDEX});
    if (ret != rysen::ErrorCode::ERROR_CODE_OK) {
        std::cerr << "❌ 禁用手指失败，错误码: " << static_cast<int>(ret) << std::endl;
    } else {
        std::cout << "✅ 手指已禁用" << std::endl;
    }
    // Enable all fingers again for subsequent operations / 再次使能所有手指，准备后续操作
    std::cout << "\n=== 再次使能所有手指 ===" << std::endl;
    ret = sdk.SetAllFingersEnabled();
    if (ret != rysen::ErrorCode::ERROR_CODE_OK) {
        std::cerr << "❌ 使能失败，错误码: " << static_cast<int>(ret) << std::endl;
    } else {
        std::cout << "✅ 所有手指已使能" << std::endl;
    }

    // ========== Motion Parameter Configuration / 运动参数配置 ==========
    // Configure maximum joint speeds, accelerations, and finger torques
    // 配置最大关节速度、加速度和手指扭矩
    std::cout << "\n=== 设置运动参数 ===" << std::endl;

    // Set maximum joint speeds (21 joints) / 设置最大关节速度（21个关节）
    std::vector<rysen::MaxJointSpeed> speeds;
    speeds.reserve(21);
    for (int i = 0; i < 21; ++i) {
        speeds.push_back(
            {static_cast<rysen::JointId>(i), 8.0});  // Default speed 4.0 rad/s / 默认速度 4.0 rad/s
    }

    // Set maximum joint accelerations (21 joints) / 设置最大关节加速度（21个关节）
    std::vector<rysen::MaxJointAccel> accels;
    accels.reserve(21);
    for (int i = 0; i < 21; ++i) {
        accels.push_back({static_cast<rysen::JointId>(i),
                          260.0});  // Default acceleration 20.0 rad/s² / 默认加速度 20.0 rad/s²
    }

    // Set maximum finger torques (5 fingers) / 设置最大手指扭矩（5个手指）
    std::vector<rysen::MaxFingerTorque> torques;
    torques.reserve(5);
    for (int i = 0; i < 5; ++i) {
        torques.push_back(
            {static_cast<rysen::FingerId>(i), 50.0});  // Default torque 100% / 默认扭矩 100%
    }

    sdk.SetMaxJointSpeed(speeds);
    sdk.SetMaxJointAccel(accels);
    sdk.SetMaxFingerTorque(torques);
    std::cout << "✅ 运动参数已设置" << std::endl;

    // ========== Register Callbacks / 注册回调函数 ==========

    // Register joint states callback (100Hz) / 注册关节状态回调（100Hz）
    // The callback will be called periodically to receive joint position, velocity, and
    // acceleration data 回调函数将周期性调用，接收关节位置、速度和加速度数据
    std::cout << "\n=== 注册关节状态回调 (100Hz) ===" << std::endl;
    // Record program start time for calculating relative timestamps /
    // 记录程序开始时间，用于计算相对时间戳
    auto program_start = std::chrono::steady_clock::now();

    sdk.RegisterGetJointStatesCallback(
        [program_start](const rysen::JointStates& states) {
            auto elapsed = states.timestamp - program_start;
            auto timestamp_ms =
                std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
            std::cout << "[JointStates] 时间戳: " << timestamp_ms
                      << " ms, 关节数量: " << states.joint_states.size() << std::endl;
        },
        100);
    std::cout << "✅ 关节状态回调已注册" << std::endl;

    // Register motor states callback (50Hz) / 注册电机状态回调（50Hz）
    // The callback will be called periodically to receive motor temperature and current data
    // 回调函数将周期性调用，接收电机温度和电流数据
    std::cout << "\n=== 注册电机状态回调 (50Hz) ===" << std::endl;
    sdk.RegisterGetMotorStatesCallback(
        [program_start](const rysen::MotorStates& states) {
            auto elapsed = states.timestamp - program_start;
            auto timestamp_ms =
                std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
            std::cout << "[MotorStates] 时间戳: " << timestamp_ms
                      << " ms, 电机数量: " << states.motors.size() << std::endl;
        },
        50);
    std::cout << "✅ 电机状态回调已注册" << std::endl;

    // Register hand sensor image callback (50Hz) / 注册手传感器图像回调（50Hz）
    // The callback will be called periodically to receive tactile sensor image data
    // Note: Requires tactile sensor version to be available / 注意：需要触觉传感器版本号可用
    // 回调函数将周期性调用，接收触觉传感器图像数据
    std::cout << "\n=== 注册手传感器图像回调 (50Hz) ===" << std::endl;
    ret = sdk.RegisterGetHandSensorImageCallback(
        [program_start](const rysen::HandSensorImage& image) {
            auto elapsed = image.timestamp - program_start;
            auto timestamp_ms =
                std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
            std::cout << "[HandSensorImage] 时间戳: " << timestamp_ms << " ms" << std::endl;
        },
        50);
    if (ret != rysen::ErrorCode::ERROR_CODE_OK) {
        std::cerr << "❌ 注册手传感器图像回调失败，错误码: " << static_cast<int>(ret) << std::endl;
        std::cerr << "   提示: 如果触觉传感器版本号未获取到，触觉相关接口无法使用" << std::endl;
    } else {
        std::cout << "✅ 手传感器图像回调已注册" << std::endl;
    }

    // Wait for data to stabilize / 等待数据稳定
    std::cout << "\n=== 等待数据稳定（2秒）===" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // ========== MoveJ Control Test / MoveJ 控制测试 ==========
    // MoveJ is a blocking function that moves joints to target positions and waits until they reach
    // MoveJ 是阻塞函数，将关节移动到目标位置并等待到达
    std::cout << "\n=== 测试 MoveJ 控制（所有21个关节） ===" << std::endl;
    std::vector<rysen::JointControlParam> movej_commands;

    // Thumb (5个关节)
    movej_commands.push_back({rysen::JointId::JOINT_ID_THUMB_CMC_ABD, 1.2, 2.0, 8.0});
    movej_commands.push_back({rysen::JointId::JOINT_ID_THUMB_CMC_ROT, 0.2, 2.0, 8.0});
    movej_commands.push_back({rysen::JointId::JOINT_ID_THUMB_CMC_FLEX, 0.25, 2.0, 8.0});
    movej_commands.push_back({rysen::JointId::JOINT_ID_THUMB_MCP_FLEX, 0.4, 2.0, 8.0});
    movej_commands.push_back({rysen::JointId::JOINT_ID_THUMB_IP_FLEX, 0.4, 2.0, 8.0});

    // Index (4个关节)
    movej_commands.push_back({rysen::JointId::JOINT_ID_INDEX_MCP_ABD, 0.3, 2.0, 8.0});
    movej_commands.push_back({rysen::JointId::JOINT_ID_INDEX_MCP_FLEX, 0.8, 2.0, 8.0});
    movej_commands.push_back({rysen::JointId::JOINT_ID_INDEX_PIP_FLEX, 0.8, 2.0, 8.0});
    movej_commands.push_back({rysen::JointId::JOINT_ID_INDEX_DIP_FLEX, 0.8, 2.0, 8.0});

    // Middle (4个关节)
    movej_commands.push_back({rysen::JointId::JOINT_ID_MIDDLE_MCP_ABD, 0.15, 2.0, 8.0});
    movej_commands.push_back({rysen::JointId::JOINT_ID_MIDDLE_MCP_FLEX, 0.8, 2.0, 8.0});
    movej_commands.push_back({rysen::JointId::JOINT_ID_MIDDLE_PIP_FLEX, 0.8, 2.0, 8.0});
    movej_commands.push_back({rysen::JointId::JOINT_ID_MIDDLE_DIP_FLEX, 0.8, 2.0, 8.0});

    // Ring (4个关节)
    movej_commands.push_back({rysen::JointId::JOINT_ID_RING_MCP_ABD, -0.15, 2.0, 8.0});
    movej_commands.push_back({rysen::JointId::JOINT_ID_RING_MCP_FLEX, 0.8, 2.0, 8.0});
    movej_commands.push_back({rysen::JointId::JOINT_ID_RING_PIP_FLEX, 0.8, 2.0, 8.0});
    movej_commands.push_back({rysen::JointId::JOINT_ID_RING_DIP_FLEX, 0.8, 2.0, 8.0});

    // Pinky (4个关节)
    movej_commands.push_back({rysen::JointId::JOINT_ID_LITTLE_MCP_ABD, -0.3, 2.0, 8.0});
    movej_commands.push_back({rysen::JointId::JOINT_ID_LITTLE_MCP_FLEX, 0.8, 2.0, 8.0});
    movej_commands.push_back({rysen::JointId::JOINT_ID_LITTLE_PIP_FLEX, 0.8, 2.0, 8.0});
    movej_commands.push_back({rysen::JointId::JOINT_ID_LITTLE_DIP_FLEX, 0.8, 2.0, 8.0});

    std::cout << "发送 MoveJ 命令，控制 " << movej_commands.size() << " 个关节..." << std::endl;
    std::cout << "等待关节到达目标位置（MoveJoint 是阻塞函数，会等待直到到达）..." << std::endl;
    ret = sdk.MoveJoint(movej_commands);
    if (ret != rysen::ErrorCode::ERROR_CODE_OK) {
        std::cerr << "❌ MoveJ 控制失败，错误码: " << static_cast<int>(ret) << std::endl;
    } else {
        std::cout << "✅ MoveJ 完成，关节已到达目标位置" << std::endl;
    }

    // Test again: return to initial position / 再次测试：回到初始位置
    // Move all 21 joints back to 0 position / 将所有21个关节都回到0位置
    std::cout << "\n=== 测试 MoveJ 回到初始位置（所有21个关节） ===" << std::endl;
    movej_commands.clear();

    // Thumb (5个关节)
    movej_commands.push_back({rysen::JointId::JOINT_ID_THUMB_CMC_ABD, 0.0, 0.5, 2.0});
    movej_commands.push_back({rysen::JointId::JOINT_ID_THUMB_CMC_ROT, 0.0, 0.5, 2.0});
    movej_commands.push_back({rysen::JointId::JOINT_ID_THUMB_CMC_FLEX, 0.0, 0.5, 2.0});
    movej_commands.push_back({rysen::JointId::JOINT_ID_THUMB_MCP_FLEX, 0.0, 0.5, 2.0});
    movej_commands.push_back({rysen::JointId::JOINT_ID_THUMB_IP_FLEX, 0.0, 0.5, 2.0});

    // Index (4个关节)
    movej_commands.push_back({rysen::JointId::JOINT_ID_INDEX_MCP_ABD, 0.0, 0.5, 2.0});
    movej_commands.push_back({rysen::JointId::JOINT_ID_INDEX_MCP_FLEX, 0.0, 0.5, 2.0});
    movej_commands.push_back({rysen::JointId::JOINT_ID_INDEX_PIP_FLEX, 0.0, 0.5, 2.0});
    movej_commands.push_back({rysen::JointId::JOINT_ID_INDEX_DIP_FLEX, 0.0, 0.5, 2.0});

    // Middle (4个关节)
    movej_commands.push_back({rysen::JointId::JOINT_ID_MIDDLE_MCP_ABD, 0.0, 0.5, 2.0});
    movej_commands.push_back({rysen::JointId::JOINT_ID_MIDDLE_MCP_FLEX, 0.0, 0.5, 2.0});
    movej_commands.push_back({rysen::JointId::JOINT_ID_MIDDLE_PIP_FLEX, 0.0, 0.5, 2.0});
    movej_commands.push_back({rysen::JointId::JOINT_ID_MIDDLE_DIP_FLEX, 0.0, 0.5, 2.0});

    // Ring (4个关节)
    movej_commands.push_back({rysen::JointId::JOINT_ID_RING_MCP_ABD, 0.0, 0.5, 2.0});
    movej_commands.push_back({rysen::JointId::JOINT_ID_RING_MCP_FLEX, 0.0, 0.5, 2.0});
    movej_commands.push_back({rysen::JointId::JOINT_ID_RING_PIP_FLEX, 0.0, 0.5, 2.0});
    movej_commands.push_back({rysen::JointId::JOINT_ID_RING_DIP_FLEX, 0.0, 0.5, 2.0});

    // Pinky (4个关节)
    movej_commands.push_back({rysen::JointId::JOINT_ID_LITTLE_MCP_ABD, 0.0, 0.5, 2.0});
    movej_commands.push_back({rysen::JointId::JOINT_ID_LITTLE_MCP_FLEX, 0.0, 0.5, 2.0});
    movej_commands.push_back({rysen::JointId::JOINT_ID_LITTLE_PIP_FLEX, 0.0, 0.5, 2.0});
    movej_commands.push_back({rysen::JointId::JOINT_ID_LITTLE_DIP_FLEX, 0.0, 0.5, 2.0});

    std::cout << "发送 MoveJ 命令，回到初始位置（MoveJoint 是阻塞函数，会等待直到到达）..."
              << std::endl;
    ret = sdk.MoveJoint(movej_commands);

    if (ret != rysen::ErrorCode::ERROR_CODE_OK) {
        std::cerr << "❌ MoveJ 控制失败，错误码: " << static_cast<int>(ret) << std::endl;
    } else {
        std::cout << "✅ MoveJ 完成，关节已回到初始位置" << std::endl;
    }

    // ========== Get Functions Test / Get 函数测试 ==========
    // Test synchronous data retrieval functions / 测试同步数据获取函数
    std::cout << "\n=== 测试 Get 函数 ===" << std::endl;

    // Test GetJointStates / 测试 GetJointStates
    // Synchronously get current joint states / 同步获取当前关节状态
    std::cout << "\n--- 测试 GetJointStates ---" << std::endl;
    rysen::JointStates joint_states;
    ret = sdk.GetJointStates(joint_states);
    if (ret == rysen::ErrorCode::ERROR_CODE_OK) {
        std::cout << "✅ 成功获取关节状态，关节数量: " << joint_states.joint_states.size()
                  << std::endl;
        if (!joint_states.joint_states.empty()) {
            size_t print_count = std::min(static_cast<size_t>(5), joint_states.joint_states.size());
            std::cout << "前 " << print_count << " 个关节的状态:" << std::endl;
            for (size_t i = 0; i < print_count; ++i) {
                const auto& joint = joint_states.joint_states[i];
                std::cout << "  关节[" << static_cast<int>(joint.joint_id) << "] "
                          << "位置: " << joint.position << " rad, "
                          << "速度: " << joint.velocity << " rad/s, "
                          << "加速度: " << joint.acceleration << " rad/s², "
                          << "力矩: " << joint.torque << " Nmm" << std::endl;
            }
        }
    } else {
        std::cerr << "❌ 获取关节状态失败，错误码: " << static_cast<int>(ret) << std::endl;
    }

    // Test GetMotorStates / 测试 GetMotorStates
    // Synchronously get current motor states / 同步获取当前电机状态
    std::cout << "\n--- 测试 GetMotorStates ---" << std::endl;
    rysen::MotorStates motor_states;
    ret = sdk.GetMotorStates(motor_states);
    if (ret == rysen::ErrorCode::ERROR_CODE_OK) {
        std::cout << "✅ 成功获取电机状态，电机数量: " << motor_states.motors.size() << std::endl;
        if (!motor_states.motors.empty()) {
            size_t print_count = std::min(static_cast<size_t>(5), motor_states.motors.size());
            std::cout << "前 " << print_count << " 个电机的状态:" << std::endl;
            for (size_t i = 0; i < print_count; ++i) {
                const auto& motor = motor_states.motors[i];
                std::cout << "  电机[" << static_cast<int>(motor.motor_id) << "] "
                          << "温度: " << motor.temperature << " °C, "
                          << "电流: " << motor.current << " A" << std::endl;
            }
        }
    } else {
        std::cerr << "❌ 获取电机状态失败，错误码: " << static_cast<int>(ret) << std::endl;
    }

    // Test GetHandSensorImage / 测试 GetHandSensorImage
    // Synchronously get current tactile sensor image / 同步获取当前触觉传感器图像
    std::cout << "\n--- 测试 GetHandSensorImage ---" << std::endl;
    rysen::HandSensorImage sensor_image;
    ret = sdk.GetHandSensorImage(sensor_image);
    if (ret == rysen::ErrorCode::ERROR_CODE_OK) {
        std::cout << "✅ 成功获取手传感器图像" << std::endl;
        std::cout << "  食指 PIP 图像尺寸: " << sensor_image.index_image.prox_pad.width << "x"
                  << sensor_image.index_image.prox_pad.height << std::endl;
        std::cout << "  图像数据大小: " << sensor_image.index_image.prox_pad.gray_image.size()
                  << " 像素" << std::endl;
    } else {
        std::cerr << "❌ 获取手传感器图像失败，错误码: " << static_cast<int>(ret) << std::endl;
    }

    // ========== MoveJPositionFollow Test / MoveJPositionFollow 测试 ==========
    // MoveJPositionFollow is a non-blocking function that continuously follows target positions
    // It uses smooth interpolation for trajectory planning
    // MoveJPositionFollow 是非阻塞函数，持续跟踪目标位置，使用平滑插值进行轨迹规划
    std::cout << "\n=== 测试 MoveJPositionFollow（单个关节往复运动，一系列中间点） ==="
              << std::endl;
    std::cout << "控制食指 MCP_1 关节往复运动，包含多个中间点，时间间隔不同..." << std::endl;

    // Move to starting position first / 先移动到起始位置
    std::cout << "\n--- 移动到起始位置 (0.0 rad) ---" << std::endl;
    std::vector<rysen::JointControlParam> start_pos;
    start_pos.push_back({rysen::JointId::JOINT_ID_INDEX_PIP_FLEX, 0.0, 1.0, 2.0});
    ret = sdk.MoveJoint(start_pos);
    if (ret != rysen::ErrorCode::ERROR_CODE_OK) {
        std::cerr << "❌ 移动到起始位置失败，错误码: " << static_cast<int>(ret) << std::endl;
    } else {
        std::cout << "✅ 已到达起始位置" << std::endl;
    }

    // Reciprocating motion cycles (2 cycles) / 往复运动循环（2次）
    const int cycle_count = 2;

    // Define waypoint structure / 定义路径点结构
    struct Waypoint {
        double position;  // Target position (rad) / 目标位置 (rad)
        int delay_ms;  // Wait time after reaching this point (ms) / 到达该点后的等待时间 (ms)
    };

    // Define path: from 0.0 to 0.8 and back to 0.0, with dense intermediate points
    // 定义路径：从 0.0 到 0.8 再回到 0.0，包含更多密集的中间点
    std::vector<Waypoint> waypoints = {
        {0.0, 100},  // 起始点
        {0.1, 50},   // 中间点
        {0.2, 50},   // 中间点
        {0.3, 50},   // 中间点
        {0.4, 50},   // 中间点
        {0.5, 50},   // 中间点
        {0.6, 50},   // 中间点
        {0.7, 50},   // 中间点
        {0.8, 100},  // 最大位置
        {0.7, 50},   // 返回路径
        {0.6, 50},   // 返回路径
        {0.5, 50},   // 返回路径
        {0.4, 50},   // 返回路径
        {0.3, 50},   // 返回路径
        {0.2, 50},   // 返回路径
        {0.1, 50},   // 返回路径
        {0.0, 100}   // 回到起始位置
    };

    // 定义5个手指对应的主要MCP关节ID（每个手指完成一个完整的往复运动循环）
    const std::vector<rysen::JointId> finger_joints = {
        rysen::JointId::JOINT_ID_THUMB_CMC_ABD,    // 拇指
        rysen::JointId::JOINT_ID_INDEX_MCP_FLEX,   // 食指
        rysen::JointId::JOINT_ID_MIDDLE_MCP_FLEX,  // 中指
        rysen::JointId::JOINT_ID_RING_MCP_FLEX,    // 无名指
        rysen::JointId::JOINT_ID_LITTLE_MCP_FLEX   // 小指
    };
    const std::vector<std::string> finger_names = {"拇指", "食指", "中指", "无名指", "小指"};

    // 每个手指完成 cycle_count 次往复运动循环
    for (size_t finger_idx = 0; finger_idx < finger_joints.size() && running; ++finger_idx) {
        rysen::JointId target_joint = finger_joints[finger_idx];
        const std::string& finger_name = finger_names[finger_idx];

        std::cout << "\n=== " << finger_name << " 开始往复运动 ===" << std::endl;

        for (int cycle = 0; cycle < cycle_count && running; ++cycle) {
            std::cout << "\n--- " << finger_name << " 往复运动循环 " << (cycle + 1) << "/"
                      << cycle_count << " ---" << std::endl;

            // Go through all waypoints sequentially / 依次经过所有中间点
            for (size_t i = 0; i < waypoints.size() && running; ++i) {
                const auto& waypoint = waypoints[i];
                std::cout << "  点 " << (i + 1) << "/" << waypoints.size()
                          << ": 位置=" << waypoint.position << " rad, 等待=" << waypoint.delay_ms
                          << " ms" << std::endl;

                std::vector<rysen::MoveJPositionFollowParam> follow_params;
                follow_params.push_back({target_joint, waypoint.position});
                ret = sdk.MoveJPositionFollow(follow_params);
                if (ret != rysen::ErrorCode::ERROR_CODE_OK) {
                    std::cerr << "❌ MoveJPositionFollow 失败，错误码: " << static_cast<int>(ret)
                              << std::endl;
                    break;
                }

                // Wait for the specified time interval (last point doesn't need to wait)
                // 等待指定的时间间隔（最后一个点不需要等待）
                if (i < waypoints.size() - 1) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(waypoint.delay_ms));
                }
            }
            std::cout << "✅ " << finger_name << " 往复运动循环 " << (cycle + 1) << " 完成"
                      << std::endl;

            // Wait between cycles / 循环之间的等待
            if (cycle < cycle_count - 1) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        }

        std::cout << "✅ " << finger_name << " 往复运动完成" << std::endl;

        // Wait between fingers / 手指之间的等待
        if (finger_idx < finger_joints.size() - 1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

    std::cout << "\n✅ MoveJPositionFollow 往复运动测试完成" << std::endl;

    // ========== Cleanup / 清理资源 ==========
    std::cout << "\n=== 清理资源 ===" << std::endl;

    // Disable all fingers / 禁用所有手指
    sdk.SetAllFingersDisabled();
    std::cout << "✅ 所有手指已禁用" << std::endl;

    // Disconnect / 断开连接
    std::cout << "正在断开连接..." << std::endl;
    ret = sdk.Disconnect();
    if (ret != rysen::ErrorCode::ERROR_CODE_OK) {
        std::cerr << "❌ 断开连接失败，错误码: " << static_cast<int>(ret) << std::endl;
    } else {
        std::cout << "✅ 已断开连接" << std::endl;
    }

    std::cout << "\n=== 测试完成 ===" << std::endl;
    return 0;
}