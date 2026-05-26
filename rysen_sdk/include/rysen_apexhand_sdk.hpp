/**
 * @file    rysen_apexhand_sdk.hpp
 * @brief   Public SDK interface for the rysen_apexhand_sdk.
 *
 * Copyright (c) 2024-2026, Rysen Robotics (Shenzhen) Co. Ltd.
 * All rights reserved.
 *
 * Use of this source code is governed by a BSD 3-Clause license that can be
 * found in the LICENSE file.
 */

#ifndef RYSEN_APEXHAND_SDK_INCLUDE_RYSEN_APEXHAND_SDK_H_
#define RYSEN_APEXHAND_SDK_INCLUDE_RYSEN_APEXHAND_SDK_H_

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "rysen_apexhand_data.hpp"

// Forward declaration (ControlCommand is defined in apexhand_controller.hpp)
namespace rysen {
class ApexHandController;
}

namespace rysen {
/**
 * @brief Get Motor States Callback object
 *  获取电机状态回调函数
 * @param motor_states 电机状态（包含时间戳和电机列表）
 */
using GetMotorStatesCallback = std::function<void(const MotorStates& p_motor_states)>;
/**
 * @brief Get Hand Sensor Image Callback object
 *  获取手传感器图像回调函数
 * @param image 手传感器图像
 */
using GetHandSensorImageCallback = std::function<void(const HandSensorImage& p_image)>;
/**
 * @brief Get Joint States Callback object
 *  获取关节状态回调函数
 * @param joint_states 关节状态（包含时间戳和关节列表）
 */
using GetJointStatesCallback = std::function<void(const JointStates& p_joint_states)>;
/**
 * @brief Hardware Error Event Callback object
 *  硬件错误事件回调函数
 * @param hardware_errors 硬件错误码（设备及各手指的错误码）
 */
using HardwareErrorEventCallback = std::function<void(const HardwareErrorCodes& p_hardware_errors)>;

// 主 SDK 类
class Rysen {
   public:
    Rysen();
    ~Rysen();

    Rysen(const Rysen&) = delete;
    Rysen& operator=(const Rysen&) = delete;
    Rysen(Rysen&&) noexcept;
    Rysen& operator=(Rysen&&) noexcept;
    /**
     * @brief 连接机器人
     *
     * @param address 机器人地址
     * @param type 连接类型
     * @return true 连接成功
     * @return false 连接失败
     */
    ErrorCode Connect(const std::string& address,
                      ConnectionType type = ConnectionType::CONNECTION_TYPE_ETHERNET);
    /**
     * @brief 设置设备网络参数（IP/子网掩码/网关）
     * @note 需要先建立连接。设备返回成功应答后生效。
     */
    ErrorCode SetDeviceIPAddress(const std::string& ip_address);
    /**
     * @brief 断开连接
     *  断开与机器人的连接
     * @return true 断开成功
     * @return false 断开失败
     */
    ErrorCode Disconnect();
    /**
     * @brief Get the Version Info object
     *  获取版本信息
     * @return VersionInfo 版本信息
     */
    VersionInfo GetVersionInfo();
    /**
     * @brief 获取灵巧手硬件的 96位 全局唯一标识符 (UID)
     * @return std::string 格式化后的 96位十六进制字符串 (如
     * "XXXXXXXX-XXXXXXXX-XXXXXXXX")，未连接时返回空字符串
     */
    std::string GetHardwareUid() const;
    /**
     * @brief 是否连接
     *  判断是否与机器人建立了连接
     * @return true 已连接
     * @return false 未连接
     */
    bool IsConnected();
    /**
     * @brief Set the All Fingers Enabled object
     *  使能所有手指
     *  使能所有手指的控制
     * @return true 使能成功
     * @return false 使能失败
     */
    ErrorCode SetAllFingersEnabled();
    /**
     * @brief Set the All Fingers Disabled object
     *  禁用所有手指
     *  禁用所有手指的控制
     * @return true 禁用成功
     * @return false 禁用失败
     */
    ErrorCode SetAllFingersDisabled();
    /**
     * @brief Set the Finger Enabled object
     *  使能指定手指
     *  使能指定手指的控制
     * @param fingers 手指列表
     * @return true 使能成功
     * @return false 使能失败
     */
    ErrorCode SetFingerEnabled(const std::vector<FingerId>& p_fingers);
    /**
     * @brief Set the Finger Disabled object
     *  禁用指定手指
     *  禁用指定手指的控制
     * @param fingers 手指列表
     * @return true 禁用成功
     * @return false 禁用失败
     */
    ErrorCode SetFingerDisabled(const std::vector<FingerId>& p_fingers);
    /**
     * @brief 清除故障
     *  清除机器人的故障状态
     * @return true 清除成功
     * @return false 清除失败
     */
    ErrorCode CleanFaults();
    /**
     * @brief Set the Log Path object
     *  设置日志路径
     * @param path 日志路径，默认 "./log"（实例化时已自动设置）
     * @return ErrorCode 错误码
     * @note 日志等级固定为 INFO，外部调用无法修改（仅内部开发时可调整）
     */
    ErrorCode SetLogPath(const std::string& path = "./log");
    /**
     * @brief Enable or Disable Logging
     *  开启/关闭日志
     * @param enable 开启/关闭
     * @return ErrorCode 错误码
     */
    ErrorCode EnableLogging(bool enable = true);
    /**
     * @brief Set the Max Joint Speed object
     *  设置最大关节速度
     * @param speeds 最大关节速度  21个关节 （rad/s）手指，关节的形式
     * 每跟手指的dip和pip耦合
     * @return true 设置成功
     * @return false 设置失败
     */
    ErrorCode SetMaxJointSpeed(const std::vector<MaxJointSpeed>& p_joint_speeds);
    /**
     * @brief Set the Max Finger Torque object
     *  设置最大手指扭矩
     * @param torques 最大手指扭矩 五个手指， 大拇指到小拇指 0-100%
     * @return true 设置成功
     * @return false 设置失败
     */
    ErrorCode SetMaxFingerTorque(const std::vector<MaxFingerTorque>& p_finger_torques);
    /**
     * @brief Set the Max Joint Accel object
     *  设置最大关节加速度
     * @param accels 最大关节加速度   21个关节 （rad/s²）手指，关节的形式
     * 每跟手指的dip和pip耦合
     * @return true 设置成功
     * @return false 设置失败
     */
    ErrorCode SetMaxJointAccel(const std::vector<MaxJointAccel>& p_joint_accels);
    /**
     * @brief Get the Parameters object
     *  获取参数
     * @return ControlParams 参数
     */
    ParamInfo GetParameters();
    /**
     * @brief 位置控制
     *  控制机器人的关节位置，阻塞式控制，直到到达目标位置
     * @param commands 控制关节，包含位置、速度、加速度  任意关节数量
     * 每跟手指的dip和pip耦合
     * @return true 控制成功
     * @return false 控制失败
     */
    ErrorCode MoveJoint(const std::vector<JointControlParam>& p_commands);
    /**
     * @brief Get the Hardware Error Code object
     *  获取硬件错误码
     *  检查硬件错误码（设备及各手指的错误码），如果检测到错误则触发错误回调
     * @return ErrorCode 错误码，ERROR_CODE_OK 表示无错误，ERROR_CODE_HARDWARE_ERROR
     * 表示检测到硬件错误
     */
    ErrorCode GetHardwareErrorCode();
    /**
     * @brief 注册硬件错误事件回调函数
     *
     * @param cb 硬件错误事件回调函数
     * @return ErrorCode 错误码，ERROR_CODE_OK 表示注册成功
     */
    ErrorCode RegisterHardwareErrorEventCallback(HardwareErrorEventCallback cb);
    /**
     * @brief 取消注册硬件错误事件回调函数
     *
     * @return ErrorCode 错误码，ERROR_CODE_OK 表示取消注册成功
     */
    ErrorCode UnregisterHardwareErrorEventCallback();
    /**
     * @brief Get the Joint States object
     *  获取关节状态
     * @param p_states 输出参数，关节状态（包含时间戳和关节状态列表），不允许修改
     * @return ErrorCode 错误码，ERROR_CODE_OK 表示成功
     */
    ErrorCode GetJointStates(JointStates& p_states) const;
    /**
     * @brief 注册关节状态回调函数
     *
     * @param cb 关节状态回调函数
     * @return true 注册成功
     * @return false 注册失败
     */
    ErrorCode RegisterGetJointStatesCallback(GetJointStatesCallback cb, uint32_t freq_hz = 100);
    /**
     * @brief 取消注册关节状态回调函数
     *
     * @return true 取消注册成功
     * @return false 取消注册失败
     */
    ErrorCode UnregisterGetJointStatesCallback();
    /**
     * @brief Get the Motor States object
     *  获取电机状态
     * @param p_states 输出参数，电机状态（包含时间戳和电机列表），不允许修改
     * @return ErrorCode 错误码，ERROR_CODE_OK 表示成功
     */
    ErrorCode GetMotorStates(MotorStates& p_states) const;
    /**
     * @brief 注册电机状态回调函数
     *
     * @param cb 电机状态回调函数
     * @param freq_hz 回调频率 默认100Hz
     * @return true 注册成功
     * @return false 注册失败
     */
    ErrorCode RegisterGetMotorStatesCallback(GetMotorStatesCallback cb, uint32_t freq_hz = 100);
    /**
     * @brief 取消注册电机状态回调函数
     *
     * @return true 取消注册成功
     * @return false 取消注册失败
     */
    ErrorCode UnregisterGetMotorStatesCallback();
    /**
     * @brief 注册手传感器图像回调函数
     *
     * @param cb 手传感器图像回调函数
     * @param freq_hz 回调频率 默认100Hz
     * @return ErrorCode
     */
    ErrorCode RegisterGetHandSensorImageCallback(GetHandSensorImageCallback cb,
                                                 uint32_t freq_hz = 100);
    /**
     * @brief 取消注册手传感器图像回调函数
     *
     * @return true 取消注册成功
     * @return false 取消注册失败
     */
    ErrorCode UnregisterGetHandSensorImageCallback();
    /**
     * @brief Get the Hand Sensor Image object
     *  获取手传感器图像
     * @param p_image 输出参数，手传感器图像，不允许修改
     * @return ErrorCode 错误码，ERROR_CODE_OK 表示成功
     */
    ErrorCode GetHandSensorImage(HandSensorImage& p_image) const;

    /**
     * @brief 开始触觉标定（法向力零偏 + 切向力 Bias）
     *
     * @return ErrorCode 错误码
     */
    ErrorCode StartTactileCalibration();

    /**
     * @brief 清空触觉标定（法向力零偏 + 切向力 Bias）
     *
     * @return ErrorCode 错误码
     */
    ErrorCode ClearTactileCalibration();

    /**
     * @brief 获取手方向
     *
     * @return HandDir::LEFT or HandDir::RIGHT
     */
    HandDir GetHandDir() const;

    /**
     * @brief Move J Follow
     *  跟随关节位置控制
     * @param follow_param 目标关节位置 21个关节
     * @return ErrorCode 错误码
     */
    ErrorCode MoveJPositionFollow(const std::vector<MoveJPositionFollowParam>& p_follow_param);

    /**
     * @brief 从 URDF 文件加载机器人模型（手部刚体/关节模型）。
     *
     * SDK 默认不会自动加载任何 URDF。若需要刚体/关节模型（例如用于轨迹规划、
     * 碰撞检测或可视化），需要显式调用本函数，并传入 URDF 文件路径。
     *
     * 可在 Connect 之前或之后调用本函数，传入绝对路径或相对当前工作目录的路径。
     *
     * @param urdf_path URDF 文件路径，不能为空；例如：
     *                  "config/apex_hand_right.urdf" 或 "config/apex_hand_left.urdf"
     * @return ErrorCode 错误码：
     *   - ERROR_CODE_OK               加载成功
     *   - ERROR_CODE_INVALID_ARGUMENT urdf_path 为空
     *   - ERROR_CODE_CONFIG_ERROR     URDF 文件不存在或解析失败
     */
    ErrorCode SetRobotModelFromURDF(const std::string& urdf_path);

    /**
     * @brief Check if a specific finger is enabled
     * @param finger_id Finger ID to check
     * @return true if enabled, false otherwise
     */
    bool IsFingerEnabled(FingerId finger_id) const;

   private:
    struct Impl;
    std::unique_ptr<Impl> p_impl_;
};

}  // namespace rysen

#endif  // RYSEN_APEXHAND_SDK_INCLUDE_RYSEN_APEXHAND_SDK_H_
