/**
 * @file    rysen_apexhand_node.hpp
 * @brief   ROS 2 node exposing ApexHand SDK services and publishers.
 *
 * Copyright (c) 2024-2026, Rysen Robotics (Shenzhen) Co. Ltd.
 * All rights reserved.
 *
 * Use of this source code is governed by a BSD 3-Clause license that can be
 * found in the LICENSE file.
 */

#ifndef RYSEN_APEXHAND__RYSEN_APEXHAND_NODE_HPP_
#define RYSEN_APEXHAND__RYSEN_APEXHAND_NODE_HPP_

#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "builtin_interfaces/msg/time.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rysen_apexhand/rysen_rosbag_manager.hpp"
#include "rysen_apexhand_data.hpp"
#include "rysen_apexhand_msgs/msg/hand_tactile_forces.hpp"
#include "rysen_apexhand_msgs/msg/hardware_errors.hpp"
#include "rysen_apexhand_msgs/msg/motor_state.hpp"
#include "rysen_apexhand_msgs/srv/clean_faults.hpp"
#include "rysen_apexhand_msgs/srv/clear_tactile_calibration.hpp"
#include "rysen_apexhand_msgs/srv/connect.hpp"
#include "rysen_apexhand_msgs/srv/get_connection_info.hpp"
#include "rysen_apexhand_msgs/srv/get_version_info.hpp"
#include "rysen_apexhand_msgs/srv/is_finger_enabled.hpp"
#include "rysen_apexhand_msgs/srv/move_joint.hpp"
#include "rysen_apexhand_msgs/srv/remove_hand.hpp"
#include "rysen_apexhand_msgs/srv/set_all_fingers_enable.hpp"
#include "rysen_apexhand_msgs/srv/set_device_ip_address.hpp"
#include "rysen_apexhand_msgs/srv/set_finger_enabled.hpp"
#include "rysen_apexhand_msgs/srv/set_max_finger_torque.hpp"
#include "rysen_apexhand_msgs/srv/set_max_joint_accel.hpp"
#include "rysen_apexhand_msgs/srv/set_max_joint_speed.hpp"
#include "rysen_apexhand_msgs/srv/start_playback.hpp"
#include "rysen_apexhand_msgs/srv/start_record.hpp"
#include "rysen_apexhand_msgs/srv/start_tactile_calibration.hpp"
#include "rysen_apexhand_msgs/srv/stop_playback.hpp"
#include "rysen_apexhand_msgs/srv/stop_record.hpp"
#include "rysen_apexhand_sdk.hpp"
#include "sensor_msgs/msg/joint_state.hpp"
#include "std_msgs/msg/header.hpp"

namespace rysen_apexhand {

class RysenApexHandNode : public rclcpp::Node {
   public:
    explicit RysenApexHandNode(const rclcpp::NodeOptions& options = rclcpp::NodeOptions());
    ~RysenApexHandNode();

   private:
    struct HandInstance {
        std::string ip;
        std::string topic_key;
        std::unique_ptr<rysen::Rysen> sdk;
        bool callbacks_registered{false};
        bool connected{false};
        std::string hand_side{"unknown"};
        rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr move_j_position_follow_sub;
        std::mutex follow_control_owner_mutex;
        // 使用固定大小的数组直接存储底层 GID，抛弃 std::string
        std::array<uint8_t, RMW_GID_STORAGE_SIZE> follow_control_owner_gid;
        bool has_follow_owner{false};  // 标志位，是否已经被占用
        rclcpp::Time follow_control_owner_last_seen{0, 0, RCL_SYSTEM_TIME};
        rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr joint_states_pub;
        rclcpp::Publisher<rysen_apexhand_msgs::msg::MotorState>::SharedPtr motor_states_pub;
        rclcpp::Publisher<rysen_apexhand_msgs::msg::HandTactileForces>::SharedPtr tactile_image_pub;
        rclcpp::Publisher<rysen_apexhand_msgs::msg::HardwareErrors>::SharedPtr hardware_errors_pub;
    };

    // 1. Bag 管理器实例
    std::unique_ptr<ApexHandBagManager> bag_manager_;

    // 2. 声明 4 个服务端
    rclcpp::Service<rysen_apexhand_msgs::srv::StartRecord>::SharedPtr start_record_srv_;
    rclcpp::Service<rysen_apexhand_msgs::srv::StopRecord>::SharedPtr stop_record_srv_;
    rclcpp::Service<rysen_apexhand_msgs::srv::StartPlayback>::SharedPtr start_playback_srv_;
    rclcpp::Service<rysen_apexhand_msgs::srv::StopPlayback>::SharedPtr stop_playback_srv_;

    // 3. 声明 4 个处理函数
    void HandleStartRecord(
        const std::shared_ptr<rysen_apexhand_msgs::srv::StartRecord::Request> request,
        std::shared_ptr<rysen_apexhand_msgs::srv::StartRecord::Response> response);
    void HandleStopRecord(
        const std::shared_ptr<rysen_apexhand_msgs::srv::StopRecord::Request> request,
        std::shared_ptr<rysen_apexhand_msgs::srv::StopRecord::Response> response);
    void HandleStartPlayback(
        const std::shared_ptr<rysen_apexhand_msgs::srv::StartPlayback::Request> request,
        std::shared_ptr<rysen_apexhand_msgs::srv::StartPlayback::Response> response);
    void HandleStopPlayback(
        const std::shared_ptr<rysen_apexhand_msgs::srv::StopPlayback::Request> request,
        std::shared_ptr<rysen_apexhand_msgs::srv::StopPlayback::Response> response);

    std::mutex hands_mutex_;
    std::unordered_map<std::string, std::unique_ptr<HandInstance>> hands_;
    std::string default_ip_;
    // 缓存后端版本号
    std::string ros_backend_version_;

    rclcpp::Service<rysen_apexhand_msgs::srv::RemoveHand>::SharedPtr remove_hand_srv_;
    rclcpp::Service<rysen_apexhand_msgs::srv::Connect>::SharedPtr connect_srv_;
    rclcpp::Service<rysen_apexhand_msgs::srv::MoveJoint>::SharedPtr move_joint_srv_;
    rclcpp::Service<rysen_apexhand_msgs::srv::SetAllFingersEnable>::SharedPtr set_all_fingers_srv_;
    rclcpp::Service<rysen_apexhand_msgs::srv::SetFingerEnabled>::SharedPtr set_finger_enabled_srv_;
    rclcpp::Service<rysen_apexhand_msgs::srv::SetMaxJointSpeed>::SharedPtr set_max_joint_speed_srv_;
    rclcpp::Service<rysen_apexhand_msgs::srv::SetMaxJointAccel>::SharedPtr set_max_joint_accel_srv_;
    rclcpp::Service<rysen_apexhand_msgs::srv::SetMaxFingerTorque>::SharedPtr
        set_max_finger_torque_srv_;
    rclcpp::Service<rysen_apexhand_msgs::srv::SetDeviceIPAddress>::SharedPtr
        set_device_ip_address_srv_;
    rclcpp::Service<rysen_apexhand_msgs::srv::StartTactileCalibration>::SharedPtr
        start_tactile_calibration_srv_;
    rclcpp::Service<rysen_apexhand_msgs::srv::ClearTactileCalibration>::SharedPtr
        clear_tactile_calibration_srv_;
    rclcpp::Service<rysen_apexhand_msgs::srv::CleanFaults>::SharedPtr clean_faults_srv_;
    rclcpp::Service<rysen_apexhand_msgs::srv::GetConnectionInfo>::SharedPtr
        get_connection_info_srv_;
    rclcpp::Service<rysen_apexhand_msgs::srv::GetVersionInfo>::SharedPtr get_version_info_srv_;
    rclcpp::Service<rysen_apexhand_msgs::srv::IsFingerEnabled>::SharedPtr is_finger_enabled_srv_;

    void HandleMoveJoint(
        const std::shared_ptr<rysen_apexhand_msgs::srv::MoveJoint::Request> request,
        std::shared_ptr<rysen_apexhand_msgs::srv::MoveJoint::Response> response);
    void HandleConnect(const std::shared_ptr<rysen_apexhand_msgs::srv::Connect::Request> request,
                       std::shared_ptr<rysen_apexhand_msgs::srv::Connect::Response> response);
    void HandleRemoveHand(
        const std::shared_ptr<rysen_apexhand_msgs::srv::RemoveHand::Request> request,
        std::shared_ptr<rysen_apexhand_msgs::srv::RemoveHand::Response> response);
    void HandleSetAllFingers(
        const std::shared_ptr<rysen_apexhand_msgs::srv::SetAllFingersEnable::Request> request,
        std::shared_ptr<rysen_apexhand_msgs::srv::SetAllFingersEnable::Response> response);
    void HandleSetFingerEnabled(
        const std::shared_ptr<rysen_apexhand_msgs::srv::SetFingerEnabled::Request> request,
        std::shared_ptr<rysen_apexhand_msgs::srv::SetFingerEnabled::Response> response);
    void HandleSetMaxJointSpeed(
        const std::shared_ptr<rysen_apexhand_msgs::srv::SetMaxJointSpeed::Request> request,
        std::shared_ptr<rysen_apexhand_msgs::srv::SetMaxJointSpeed::Response> response);
    void HandleSetMaxJointAccel(
        const std::shared_ptr<rysen_apexhand_msgs::srv::SetMaxJointAccel::Request> request,
        std::shared_ptr<rysen_apexhand_msgs::srv::SetMaxJointAccel::Response> response);
    void HandleSetMaxFingerTorque(
        const std::shared_ptr<rysen_apexhand_msgs::srv::SetMaxFingerTorque::Request> request,
        std::shared_ptr<rysen_apexhand_msgs::srv::SetMaxFingerTorque::Response> response);
    void HandleSetDeviceIpAddress(
        const std::shared_ptr<rysen_apexhand_msgs::srv::SetDeviceIPAddress::Request> request,
        std::shared_ptr<rysen_apexhand_msgs::srv::SetDeviceIPAddress::Response> response);
    void HandleStartTactileCalibration(
        const std::shared_ptr<rysen_apexhand_msgs::srv::StartTactileCalibration::Request> request,
        std::shared_ptr<rysen_apexhand_msgs::srv::StartTactileCalibration::Response> response);
    void HandleClearTactileCalibration(
        const std::shared_ptr<rysen_apexhand_msgs::srv::ClearTactileCalibration::Request> request,
        std::shared_ptr<rysen_apexhand_msgs::srv::ClearTactileCalibration::Response> response);
    void HandleCleanFaults(
        const std::shared_ptr<rysen_apexhand_msgs::srv::CleanFaults::Request> request,
        std::shared_ptr<rysen_apexhand_msgs::srv::CleanFaults::Response> response);
    void HandleGetConnectionInfo(
        const std::shared_ptr<rysen_apexhand_msgs::srv::GetConnectionInfo::Request> request,
        std::shared_ptr<rysen_apexhand_msgs::srv::GetConnectionInfo::Response> response);
    void HandleGetVersionInfo(
        const std::shared_ptr<rysen_apexhand_msgs::srv::GetVersionInfo::Request> request,
        std::shared_ptr<rysen_apexhand_msgs::srv::GetVersionInfo::Response> response);
    void HandleIsFingerEnabled(
        const std::shared_ptr<rysen_apexhand_msgs::srv::IsFingerEnabled::Request> request,
        std::shared_ptr<rysen_apexhand_msgs::srv::IsFingerEnabled::Response> response);

    void OnMoveJPositionFollowCommand(HandInstance* hand,
                                      const sensor_msgs::msg::JointState::SharedPtr msg,
                                      const rclcpp::MessageInfo& message_info);

    void PublishJointStates(HandInstance* hand, const rysen::JointStates& states);
    void PublishMotorStates(HandInstance* hand, const rysen::MotorStates& states);
    void PublishTactileImage(HandInstance* hand, const rysen::HandSensorImage& image);
    void PublishHardwareErrors(HandInstance* hand,
                               const rysen::HardwareErrorCodes& hardware_errors);

    std_msgs::msg::Header ConvertTime(const std::chrono::steady_clock::time_point& time_point);
    rysen_apexhand_msgs::msg::TactileImage ConvertTactileImage(const rysen::TactileImage& tactile);
    rysen_apexhand_msgs::msg::CommonFingerTactile ConvertCommonFingerTactile(
        const rysen::CommonFingerSensorImage& finger_image);
    rysen_apexhand_msgs::msg::ThumbFingerTactile ConvertThumbFingerTactile(
        const rysen::ThumbFingerSensorImage& thumb_image);
    std::string ErrorCodeToString(const rysen::ErrorCode& error);
    rclcpp::Time GetSystemNow() const;
    void SdkSteadyToSystemStamp(const std::chrono::steady_clock::time_point& sdk_steady,
                                builtin_interfaces::msg::Time& out);

    std::vector<std::string> joint_names_;
    std::vector<std::string> motor_names_;

    std::chrono::steady_clock::time_point time_sync_ref_steady_;
    rclcpp::Time time_sync_ref_system_;
    std::atomic<bool> time_sync_initialized_{false};

    std::string frame_id_;

    std::unordered_map<std::string, rysen::JointId> joint_name_to_id_map_;
    std::unordered_map<std::string, std::string> per_hand_topic_prefix_overrides_;
    std::unordered_map<std::string, std::string> per_hand_follow_topic_overrides_;

    std::string NormalizeIp(const std::string& ip) const;
    std::string IpToTopicKey(const std::string& ip) const;
    HandInstance* FindHand(const std::string& ip);
    HandInstance* EnsureHandSdkOnly(const std::string& ip);
    void AttachHandTopics(HandInstance* hand);
    void DetachHandTopics(HandInstance* hand);
    void ReleaseHandSdk(HandInstance* hand);
    void ResetHandSdkAfterFailedConnect(HandInstance* hand);
    void CompleteSuccessfulConnection(HandInstance* hand);
    /** Connects one hand at ip_norm; on failure removes slot for non-default IPs, resets SDK for
     * default_ip_. */
    rysen::ErrorCode ConnectToHand(const std::string& ip_norm, int connection_type);
    bool ResolveHandForCommand(const std::string& ip, HandInstance** out, std::string& err_msg);
    void RegisterSdkCallbacks(HandInstance* hand);

    int follow_control_owner_timeout_ms_{2000};

    void InitializeJointNameMapping();

    // Compliance Control
    static constexpr std::array<double, 21> kJointBalanceTorque = {
        500.0,  // THUMB_CMC_ABD
        500.0,  // THUMB_CMC_ROT
        200.0,  // THUMB_CMC_FLEX
        100.0,  // THUMB_MCP_FLEX
        100.0,  // THUMB_IP_FLEX
        300.0,  // INDEX_MCP_ABD
        300.0,  // INDEX_MCP_FLEX
        200.0,  // INDEX_PIP_FLEX
        200.0,  // INDEX_DIP_FLEX
        300.0,  // MIDDLE_MCP_ABD
        300.0,  // MIDDLE_MCP_FLEX
        200.0,  // MIDDLE_PIP_FLEX
        200.0,  // MIDDLE_DIP_FLEX
        300.0,  // RING_MCP_ABD
        300.0,  // RING_MCP_FLEX
        200.0,  // RING_PIP_FLEX
        200.0,  // RING_DIP_FLEX
        300.0,  // LITTLE_MCP_ABD
        300.0,  // LITTLE_MCP_FLEX
        200.0,  // LITTLE_PIP_FLEX
        200.0   // LITTLE_DIP_FLEX
    };
};

}  // namespace rysen_apexhand

#endif  // RYSEN_APEXHAND__RYSEN_APEXHAND_NODE_HPP_
