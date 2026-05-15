/**
 * @file    rysen_apexhand_data.hpp
 * @brief   Public data structures for the rysen_apexhand_sdk.
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

#ifndef RYSEN_APEXHAND_SDK_INCLUDE_RYSEN_APEXHAND_DATA_H_
#define RYSEN_APEXHAND_SDK_INCLUDE_RYSEN_APEXHAND_DATA_H_

#include <array>
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace rysen {

enum class LogLevel : uint8_t {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_ERROR = 3,
    LOG_LEVEL_FATAL = 4

};

// 连接类型 rs485 ethernet
enum class ConnectionType : uint8_t { CONNECTION_TYPE_RS485 = 0, CONNECTION_TYPE_ETHERNET = 1 };
// 错误码
enum class ErrorCode : uint64_t {
    ERROR_CODE_OK = 0,            // 操作成功
    ERROR_CODE_COMM_ERROR = 1,    // 通讯错误（连接中断、发送/接收失败等）
    ERROR_CODE_TIMEOUT = 2,       // 超时错误（等待设备响应超时）
    ERROR_CODE_OUT_OF_RANGE = 3,  // 参数超出允许范围（位置/速度/加速度等）
    ERROR_CODE_OVER_SPEED = 4,    // 速度/加速度过大（超过规划/跟随允许范围）
    ERROR_CODE_OTHER_ERROR = 5,   // 其他错误
    ERROR_CODE_INVALID_ARGUMENT = 6,  // 非法参数（数量不匹配、频率不合法、空列表等）
    ERROR_CODE_CONFIG_ERROR = 7,  // 配置/环境错误（日志配置失败、传感器不可用等）
    ERROR_CODE_NOT_IMPLEMENTED = 8,  // 功能未实现（预留接口）
    ERROR_CODE_URDF_ERROR = 9,       // URDF 文件错误（文件不存在、解析失败等）
    ERROR_CODE_HARDWARE_ERROR = 10,  // 硬件错误（通过 HardwareErrorCodes 检测到硬件故障）
    ERROR_CODE_NETWORK_LATENCY_HIGH = 11  // 预连接 TCP 延迟测量超过阈值（Connect 前检查）
};

// 左右手
enum class HandDir {
    LEFT = 0,
    RIGHT = 1,
};

// 手指标识（命名见 NAMING_CONVENTION.md）
enum class FingerId : uint8_t {
    FINGER_ID_THUMB = 0,
    FINGER_ID_INDEX = 1,
    FINGER_ID_MIDDLE = 2,
    FINGER_ID_RING = 3,
    FINGER_ID_LITTLE = 4
};

// 电机标识 16 个电机（命名与顺序见 NAMING_CONVENTION.md / ROS motor_states）
enum class MotorId : uint8_t {
    MOTOR_ID_THUMB_CMC_ABD = 0,           // f0_motor0 拇指腕掌关节 侧摆
    MOTOR_ID_THUMB_CMC_ROT = 1,           // f0_motor1 拇指腕掌关节 旋转
    MOTOR_ID_THUMB_CMC_FLEX = 2,          // f0_motor2 拇指腕掌关节 弯曲
    MOTOR_ID_THUMB_MCP_FLEX = 3,          // f0_motor3 拇指掌指关节 弯曲
    MOTOR_ID_INDEX_MCP_ABD_FLEX_0 = 4,    // f1_motor0 食指掌指关节 侧摆 弯曲
    MOTOR_ID_INDEX_MCP_ABD_FLEX_1 = 5,    // f1_motor1 食指掌指关节 侧摆 弯曲
    MOTOR_ID_INDEX_PIP_FLEX = 6,          // f1_motor2 食指近端指间关节 弯曲
    MOTOR_ID_MIDDLE_MCP_ABD_FLEX_0 = 7,   // f2_motor0 中指掌指关节 侧摆 弯曲
    MOTOR_ID_MIDDLE_MCP_ABD_FLEX_1 = 8,   // f2_motor1 中指掌指关节 侧摆 弯曲
    MOTOR_ID_MIDDLE_PIP_FLEX = 9,         // f2_motor2 中指近端指间关节 弯曲
    MOTOR_ID_RING_MCP_ABD_FLEX_0 = 10,    // f3_motor0 无名指掌指关节 侧摆 弯曲
    MOTOR_ID_RING_MCP_ABD_FLEX_1 = 11,    // f3_motor1 无名指掌指关节 侧摆 弯曲
    MOTOR_ID_RING_PIP_FLEX = 12,          // f3_motor2 无名指近端指间关节 弯曲
    MOTOR_ID_LITTLE_MCP_ABD_FLEX_0 = 13,  // f4_motor0 小指掌指关节 侧摆 弯曲
    MOTOR_ID_LITTLE_MCP_ABD_FLEX_1 = 14,  // f4_motor1 小指掌指关节 侧摆 弯曲
    MOTOR_ID_LITTLE_PIP_FLEX = 15,        // f4_motor2 小指近端指间关节 弯曲
};

// 关节标识 21 个关节
enum class JointId : uint8_t {
    JOINT_ID_THUMB_CMC_ABD = 0,   // f0_joint0 拇指腕掌关节 侧摆
    JOINT_ID_THUMB_CMC_ROT = 1,   // f0_joint1 拇指腕掌关节 旋转
    JOINT_ID_THUMB_CMC_FLEX = 2,  // f0_joint2 拇指腕掌关节 弯曲
    JOINT_ID_THUMB_MCP_FLEX = 3,  // f0_joint3 拇指掌指关节 弯曲
    JOINT_ID_THUMB_IP_FLEX = 4,   // f0_joint4 拇指指间关节 弯曲

    JOINT_ID_INDEX_MCP_ABD = 5,   // f1_joint0 食指掌指关节 侧摆
    JOINT_ID_INDEX_MCP_FLEX = 6,  // f1_joint1 食指掌指关节 弯曲
    JOINT_ID_INDEX_PIP_FLEX = 7,  // f1_joint2 食指近端指间关节 弯曲
    JOINT_ID_INDEX_DIP_FLEX = 8,  // f1_joint3 食指远端指间关节 弯曲

    JOINT_ID_MIDDLE_MCP_ABD = 9,    // f2_joint0 中指掌指关节 侧摆
    JOINT_ID_MIDDLE_MCP_FLEX = 10,  // f2_joint1 中指掌指关节 弯曲
    JOINT_ID_MIDDLE_PIP_FLEX = 11,  // f2_joint2 中指近端指间关节 弯曲
    JOINT_ID_MIDDLE_DIP_FLEX = 12,  // f2_joint3 中指远端指间关节 弯曲

    JOINT_ID_RING_MCP_ABD = 13,   // f3_joint0 无名指掌指关节 侧摆
    JOINT_ID_RING_MCP_FLEX = 14,  // f3_joint1 无名指掌指关节 弯曲
    JOINT_ID_RING_PIP_FLEX = 15,  // f3_joint2 无名指近端指间关节 弯曲
    JOINT_ID_RING_DIP_FLEX = 16,  // f3_joint3 无名指远端指间关节 弯曲

    JOINT_ID_LITTLE_MCP_ABD = 17,   // f4_joint0 小指掌指关节 侧摆
    JOINT_ID_LITTLE_MCP_FLEX = 18,  // f4_joint1 小指掌指关节 弯曲
    JOINT_ID_LITTLE_PIP_FLEX = 19,  // f4_joint2 小指近端指间关节 弯曲
    JOINT_ID_LITTLE_DIP_FLEX = 20,  // f4_joint3 小指远端指间关节 弯曲
};

// 固件/库版本信息
struct VersionInfo {
    std::string touch_sensor_version;
    std::string hand_firmware_version;
    std::string sdk_version;
};

// 参数查询结构
struct ParamInfo {
    std::vector<double> max_speed{};
    std::vector<double> max_accel{};
    std::vector<double> max_current{};
    // TODO
};

// 单关节状态
struct JointState {
    JointId joint_id;
    double position = 0.0;
    double velocity = 0.0;
    double acceleration = 0.0;
    double torque = 0.0;
};

// 一组关节状态数据（包含时间戳）
struct JointStates {
    std::chrono::steady_clock::time_point timestamp;
    std::vector<JointState> joint_states;
};

// 单关节控制参数
struct JointControlParam {
    JointId joint_id;
    double position = 0.0;
    double velocity = 0.0;
    double acceleration = 0.0;
};

// MoveJPositionFollow 的参数
struct MoveJPositionFollowParam {
    JointId id;             // 关节ID
    double position = 0.0;  // 目标关节位置
    double torque = 200.0;  // 目标关节扭矩 Nmm
};

// 触觉切向力 方向 和大小
struct TangentialForce {
    double theta = 0.0;      // 切向力方向 0-2pi
    double magnitude = 0.0;  // 切向力大小
};

// 二维力图像 切向力
struct TactileImage {
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<uint16_t> gray_image;   // 灰度图像数据
    TangentialForce tangential_forces;  // 切向力数据
};

// 四指：对应 NAMING_CONVENTION 中 <finger>_prox_pad / mid_pad / dist_pad
struct CommonFingerSensorImage {
    TactileImage prox_pad;
    TactileImage mid_pad;
    TactileImage dist_pad;
};

// 拇指：thumb_prox_pad / thumb_mid_pad / thumb_dist_pad
struct ThumbFingerSensorImage {
    TactileImage prox_pad;
    TactileImage mid_pad;
    TactileImage dist_pad;
};

struct HandSensorImage {
    std::chrono::steady_clock::time_point timestamp;
    CommonFingerSensorImage index_image;   // 食指
    CommonFingerSensorImage middle_image;  // 中指
    CommonFingerSensorImage ring_image;    // 无名指
    CommonFingerSensorImage little_image;  // 小拇指
    ThumbFingerSensorImage thumb_image;    // 大拇指
    TactileImage palm_center;              // 手掌中心区域
};

struct MotorState {
    MotorId motor_id;
    double temperature = 0.0;
    double current = 0.0;
};

// 一组电机状态数据（包含时间戳）
struct MotorStates {
    std::chrono::steady_clock::time_point timestamp;
    std::vector<MotorState> motors;
};

struct MaxJointSpeed {
    JointId joint_id;
    double speed = 0.0;
};
struct MaxJointAccel {
    JointId joint_id;
    double accel = 0.0;
};
struct MaxFingerTorque {
    FingerId finger_id;
    double torque = 0.0;
};

// 硬件错误码结构
struct HardwareErrorCodes {
    uint64_t device_error_code = 0;  // 设备错误码
    uint64_t thumb_error_code = 0;   // 大拇指错误码
    uint64_t index_error_code = 0;   // 食指错误码
    uint64_t middle_error_code = 0;  // 中指错误码
    uint64_t ring_error_code = 0;    // 无名指错误码
    uint64_t little_error_code = 0;  // 小拇指错误码
};

}  // namespace rysen

#endif  // RYSEN_APEXHAND_SDK_INCLUDE_RYSEN_APEXHAND_DATA_H_
