/**
 * @file    rysen_apexhand_node.cpp
 * @brief   Implementation of the Rysen ApexHand ROS 2 node.
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

#include "rysen_apexhand/rysen_apexhand_node.hpp"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace {

std::string TrimWhitespace(const std::string& s) {
    const auto first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

rclcpp::QoS QoSFromDepthAndReliable(int depth, bool reliable) {
    const size_t d = static_cast<size_t>(std::max(1, depth));
    rclcpp::QoS q(d);
    if (reliable) {
        q.reliable();
    } else {
        q.best_effort();
    }
    return q;
}

std::unordered_map<std::string, std::string> ParseIpPrefixOverrides(const std::string& csv) {
    std::unordered_map<std::string, std::string> out;
    std::stringstream ss(csv);
    std::string entry;
    while (std::getline(ss, entry, ';')) {
        const auto eq = entry.find('=');
        if (eq == std::string::npos) {
            continue;
        }
        std::string ip = entry.substr(0, eq);
        std::string prefix = entry.substr(eq + 1);
        ip.erase(0, ip.find_first_not_of(" \t\r\n"));
        const auto ip_last = ip.find_last_not_of(" \t\r\n");
        if (ip_last == std::string::npos) {
            continue;
        }
        ip.erase(ip_last + 1);
        prefix.erase(0, prefix.find_first_not_of(" \t\r\n"));
        const auto prefix_last = prefix.find_last_not_of(" \t\r\n");
        if (prefix_last == std::string::npos) {
            continue;
        }
        prefix.erase(prefix_last + 1);
        out[ip] = prefix;
    }
    return out;
}

std::string ResolveTopicName(const std::string& configured_name,
                             const std::string& default_suffix,
                             const std::string& prefix,
                             const std::string& ip_key) {
    if (configured_name.empty() || configured_name == default_suffix) {
        return prefix + "/" + ip_key + "/" + default_suffix;
    }
    std::string resolved = configured_name;
    const std::string placeholder = "{ip_key}";
    size_t pos = 0;
    while ((pos = resolved.find(placeholder, pos)) != std::string::npos) {
        resolved.replace(pos, placeholder.size(), ip_key);
        pos += ip_key.size();
    }
    return resolved;
}

}  // namespace

namespace rysen_apexhand {

rclcpp::Time RysenApexHandNode::GetSystemNow() const {
    static rclcpp::Clock system_clock(RCL_SYSTEM_TIME);
    return system_clock.now();
}

RysenApexHandNode::RysenApexHandNode(const rclcpp::NodeOptions& options)
    : Node("rysen_apexhand_node", options) {
    this->declare_parameter<std::string>("device_ip", "192.168.0.102");
    this->declare_parameter<int>("connection_type", 1);
    this->declare_parameter<int>("joint_states_pub_freq", 100);
    this->declare_parameter<int>("motor_states_pub_freq", 100);
    this->declare_parameter<int>("tactile_image_pub_freq", 100);
    this->declare_parameter<bool>("auto_connect", false);
    this->declare_parameter<bool>("auto_enable_on_connect", false);
    this->declare_parameter<std::string>("log_path", "./log");
    this->declare_parameter<std::string>("frame_id", "base_link");
    this->declare_parameter<std::string>("multi_hand_topic_prefix", "rysen/apexhand");
    this->declare_parameter<std::string>("per_hand_topic_prefixes_csv", "");
    this->declare_parameter<std::string>("per_hand_follow_topics_csv", "");
    this->declare_parameter<std::string>("joint_states_topic", "joint_states");
    this->declare_parameter<std::string>("motor_states_topic", "motor_states");
    this->declare_parameter<std::string>("tactile_image_topic", "hand_tactile_forces");
    this->declare_parameter<std::string>("hardware_errors_topic", "hardware_errors");
    this->declare_parameter<int>("qos_depth", 10);
    this->declare_parameter<bool>("publish_qos_reliable", true);
    this->declare_parameter<bool>("subscribe_qos_reliable", false);
    this->declare_parameter<int>("follow_control_owner_timeout_ms",
                                 100);  //释放控制权时间缩小为0.1s
    this->declare_parameter<std::string>("move_j_position_follow_command_topic",
                                         "move_j_position_follow_command");

    this->declare_parameter<std::string>("remove_hand_service", "rysen/apexhand/remove_hand");
    this->declare_parameter<std::string>("move_joint_service", "rysen/apexhand/move_joint");
    this->declare_parameter<std::string>("connect_service", "rysen/apexhand/connect");
    this->declare_parameter<std::string>("set_all_fingers_service",
                                         "rysen/apexhand/set_all_fingers");
    this->declare_parameter<std::string>("set_finger_enabled_service",
                                         "rysen/apexhand/set_finger_enabled");
    this->declare_parameter<std::string>("is_finger_enabled_service",
                                         "rysen/apexhand/is_finger_enabled");
    this->declare_parameter<std::string>("set_max_joint_speed_service",
                                         "rysen/apexhand/set_max_joint_speed");
    this->declare_parameter<std::string>("set_max_joint_accel_service",
                                         "rysen/apexhand/set_max_joint_accel");
    this->declare_parameter<std::string>("set_max_finger_torque_service",
                                         "rysen/apexhand/set_max_finger_torque");
    this->declare_parameter<std::string>("set_device_ip_address_service",
                                         "rysen/apexhand/set_device_ip_address");
    this->declare_parameter<std::string>("start_tactile_calibration_service",
                                         "rysen/apexhand/start_tactile_calibration");
    this->declare_parameter<std::string>("clear_tactile_calibration_service",
                                         "rysen/apexhand/clear_tactile_calibration");
    this->declare_parameter<std::string>("clean_faults_service", "rysen/apexhand/clean_faults");
    this->declare_parameter<std::string>("get_connection_info_service",
                                         "rysen/apexhand/get_connection_info");
    this->declare_parameter<std::string>("get_version_info_service",
                                         "rysen/apexhand/get_version_info");
    this->declare_parameter<std::string>("startup_hand_ips_csv", "");
    this->declare_parameter<bool>("auto_connect_startup_hands", true);

    joint_names_ = {"f0_joint0", "f0_joint1", "f0_joint2", "f0_joint3", "f0_joint4", "f1_joint0",
                    "f1_joint1", "f1_joint2", "f1_joint3", "f2_joint0", "f2_joint1", "f2_joint2",
                    "f2_joint3", "f3_joint0", "f3_joint1", "f3_joint2", "f3_joint3", "f4_joint0",
                    "f4_joint1", "f4_joint2", "f4_joint3"};
    motor_names_ = {
        "thumb_cmc_abd_motor",   "thumb_cmc_rot_motor",         "thumb_cmc_flex_motor",
        "thumb_mcp_flex_motor",  "index_mcp_abd_flex_motor_0",  "index_mcp_abd_flex_motor_1",
        "index_pip_flex_motor",  "middle_mcp_abd_flex_motor_0", "middle_mcp_abd_flex_motor_1",
        "middle_pip_flex_motor", "ring_mcp_abd_flex_motor_0",   "ring_mcp_abd_flex_motor_1",
        "ring_pip_flex_motor",   "little_mcp_abd_flex_motor_0", "little_mcp_abd_flex_motor_1",
        "little_pip_flex_motor"};
    InitializeJointNameMapping();

    frame_id_ = this->get_parameter("frame_id").as_string();
    follow_control_owner_timeout_ms_ = std::max(
        100, static_cast<int>(this->get_parameter("follow_control_owner_timeout_ms").as_int()));
    default_ip_ = TrimWhitespace(this->get_parameter("device_ip").as_string());
    if (default_ip_.empty()) {
        default_ip_ = "192.168.0.102";
    }
    per_hand_topic_prefix_overrides_ =
        ParseIpPrefixOverrides(this->get_parameter("per_hand_topic_prefixes_csv").as_string());
    per_hand_follow_topic_overrides_ =
        ParseIpPrefixOverrides(this->get_parameter("per_hand_follow_topics_csv").as_string());

    const auto bind2 = [this](auto fn) {
        return std::bind(fn, this, std::placeholders::_1, std::placeholders::_2);
    };
    remove_hand_srv_ = this->create_service<rysen_apexhand_msgs::srv::RemoveHand>(
        this->get_parameter("remove_hand_service").as_string(),
        bind2(&RysenApexHandNode::HandleRemoveHand));
    connect_srv_ = this->create_service<rysen_apexhand_msgs::srv::Connect>(
        this->get_parameter("connect_service").as_string(),
        bind2(&RysenApexHandNode::HandleConnect));
    move_joint_srv_ = this->create_service<rysen_apexhand_msgs::srv::MoveJoint>(
        this->get_parameter("move_joint_service").as_string(),
        bind2(&RysenApexHandNode::HandleMoveJoint));
    set_all_fingers_srv_ = this->create_service<rysen_apexhand_msgs::srv::SetAllFingersEnable>(
        this->get_parameter("set_all_fingers_service").as_string(),
        bind2(&RysenApexHandNode::HandleSetAllFingers));
    set_finger_enabled_srv_ = this->create_service<rysen_apexhand_msgs::srv::SetFingerEnabled>(
        this->get_parameter("set_finger_enabled_service").as_string(),
        bind2(&RysenApexHandNode::HandleSetFingerEnabled));
    is_finger_enabled_srv_ = this->create_service<rysen_apexhand_msgs::srv::IsFingerEnabled>(
        this->get_parameter("is_finger_enabled_service").as_string(),
        bind2(&RysenApexHandNode::HandleIsFingerEnabled));
    set_max_joint_speed_srv_ = this->create_service<rysen_apexhand_msgs::srv::SetMaxJointSpeed>(
        this->get_parameter("set_max_joint_speed_service").as_string(),
        bind2(&RysenApexHandNode::HandleSetMaxJointSpeed));
    set_max_joint_accel_srv_ = this->create_service<rysen_apexhand_msgs::srv::SetMaxJointAccel>(
        this->get_parameter("set_max_joint_accel_service").as_string(),
        bind2(&RysenApexHandNode::HandleSetMaxJointAccel));
    set_max_finger_torque_srv_ = this->create_service<rysen_apexhand_msgs::srv::SetMaxFingerTorque>(
        this->get_parameter("set_max_finger_torque_service").as_string(),
        bind2(&RysenApexHandNode::HandleSetMaxFingerTorque));
    set_device_ip_address_srv_ = this->create_service<rysen_apexhand_msgs::srv::SetDeviceIPAddress>(
        this->get_parameter("set_device_ip_address_service").as_string(),
        bind2(&RysenApexHandNode::HandleSetDeviceIpAddress));
    start_tactile_calibration_srv_ =
        this->create_service<rysen_apexhand_msgs::srv::StartTactileCalibration>(
            this->get_parameter("start_tactile_calibration_service").as_string(),
            bind2(&RysenApexHandNode::HandleStartTactileCalibration));
    clear_tactile_calibration_srv_ =
        this->create_service<rysen_apexhand_msgs::srv::ClearTactileCalibration>(
            this->get_parameter("clear_tactile_calibration_service").as_string(),
            bind2(&RysenApexHandNode::HandleClearTactileCalibration));
    clean_faults_srv_ = this->create_service<rysen_apexhand_msgs::srv::CleanFaults>(
        this->get_parameter("clean_faults_service").as_string(),
        bind2(&RysenApexHandNode::HandleCleanFaults));
    get_connection_info_srv_ = this->create_service<rysen_apexhand_msgs::srv::GetConnectionInfo>(
        this->get_parameter("get_connection_info_service").as_string(),
        bind2(&RysenApexHandNode::HandleGetConnectionInfo));
    get_version_info_srv_ = this->create_service<rysen_apexhand_msgs::srv::GetVersionInfo>(
        this->get_parameter("get_version_info_service").as_string(),
        bind2(&RysenApexHandNode::HandleGetVersionInfo));

    if (this->get_parameter("auto_connect").as_bool()) {
        const int ct = this->get_parameter("connection_type").as_int();
        const auto rc = ConnectToHand(default_ip_, ct);
        if (rc == rysen::ErrorCode::ERROR_CODE_OK) {
            HandInstance* hand = FindHand(default_ip_);
            if (hand && hand->sdk) {
                (void)hand->sdk->SetAllFingersEnabled();
            }
        } else {
            RCLCPP_WARN(this->get_logger(), "auto_connect failed for default device_ip %s: %s",
                        default_ip_.c_str(), ErrorCodeToString(rc).c_str());
        }
    }

    if (this->get_parameter("auto_connect_startup_hands").as_bool()) {
        const std::string startup_csv = this->get_parameter("startup_hand_ips_csv").as_string();
        std::stringstream ss(startup_csv);
        std::string token;
        const int ct = this->get_parameter("connection_type").as_int();
        while (std::getline(ss, token, ',')) {
            token.erase(0, token.find_first_not_of(" \t\r\n"));
            const auto last = token.find_last_not_of(" \t\r\n");
            if (last == std::string::npos) {
                continue;
            }
            token.erase(last + 1);
            if (token.empty() || token == default_ip_) {
                continue;
            }
            const auto rc = ConnectToHand(token, ct);
            if (rc == rysen::ErrorCode::ERROR_CODE_OK) {
                HandInstance* hand = FindHand(token);
                if (hand && hand->sdk) {
                    (void)hand->sdk->SetAllFingersEnabled();
                }
            } else {
                RCLCPP_WARN(this->get_logger(), "Startup connect failed for %s: %s", token.c_str(),
                            ErrorCodeToString(rc).c_str());
            }
        }
    }
}

RysenApexHandNode::~RysenApexHandNode() {
    std::lock_guard<std::mutex> lock(hands_mutex_);
    for (auto& kv : hands_) {
        if (kv.second) {
            ReleaseHandSdk(kv.second.get());
        }
    }
}

std::string RysenApexHandNode::NormalizeIp(const std::string& ip) const {
    const std::string t = TrimWhitespace(ip);
    return t.empty() ? default_ip_ : t;
}

std::string RysenApexHandNode::IpToTopicKey(const std::string& ip) const {
    std::string key = ip;
    std::replace(key.begin(), key.end(), '.', '_');
    return "ip_" + key;
}

RysenApexHandNode::HandInstance* RysenApexHandNode::FindHand(const std::string& ip) {
    std::lock_guard<std::mutex> lock(hands_mutex_);
    const std::string key = NormalizeIp(ip);
    auto it = hands_.find(key);
    return (it != hands_.end()) ? it->second.get() : nullptr;
}

RysenApexHandNode::HandInstance* RysenApexHandNode::EnsureHandSdkOnly(const std::string& ip) {
    const std::string key = NormalizeIp(ip);
    std::lock_guard<std::mutex> lock(hands_mutex_);
    auto it = hands_.find(key);
    if (it != hands_.end()) {
        return it->second.get();
    }

    auto hand = std::make_unique<HandInstance>();
    HandInstance* hand_ptr = hand.get();
    hand->ip = key;
    hand->topic_key = IpToTopicKey(key);
    hand->sdk = std::make_unique<rysen::Rysen>();

    hands_[key] = std::move(hand);
    return hand_ptr;
}

void RysenApexHandNode::AttachHandTopics(HandInstance* hand) {
    if (!hand || hand->joint_states_pub) {
        return;
    }

    std::string prefix = this->get_parameter("multi_hand_topic_prefix").as_string();
    const auto prefix_it = per_hand_topic_prefix_overrides_.find(hand->ip);
    if (prefix_it != per_hand_topic_prefix_overrides_.end() && !prefix_it->second.empty()) {
        prefix = prefix_it->second;
    }
    const std::string joint_states_suffix = this->get_parameter("joint_states_topic").as_string();
    const std::string motor_states_suffix = this->get_parameter("motor_states_topic").as_string();
    const std::string tactile_suffix = this->get_parameter("tactile_image_topic").as_string();
    const std::string hardware_errors_suffix =
        this->get_parameter("hardware_errors_topic").as_string();
    const int qos_depth = std::max(1, static_cast<int>(this->get_parameter("qos_depth").as_int()));
    const rclcpp::QoS pub_qos =
        QoSFromDepthAndReliable(qos_depth, this->get_parameter("publish_qos_reliable").as_bool());
    hand->joint_states_pub = this->create_publisher<sensor_msgs::msg::JointState>(
        ResolveTopicName(joint_states_suffix, "joint_states", prefix, hand->topic_key), pub_qos);
    hand->motor_states_pub = this->create_publisher<rysen_apexhand_msgs::msg::MotorState>(
        ResolveTopicName(motor_states_suffix, "motor_states", prefix, hand->topic_key), pub_qos);
    hand->tactile_image_pub = this->create_publisher<rysen_apexhand_msgs::msg::HandTactileForces>(
        ResolveTopicName(tactile_suffix, "hand_tactile_forces", prefix, hand->topic_key), pub_qos);
    hand->hardware_errors_pub = this->create_publisher<rysen_apexhand_msgs::msg::HardwareErrors>(
        ResolveTopicName(hardware_errors_suffix, "hardware_errors", prefix, hand->topic_key),
        pub_qos);
    const rclcpp::QoS sub_qos =
        QoSFromDepthAndReliable(qos_depth, this->get_parameter("subscribe_qos_reliable").as_bool());
    const std::string follow_topic_suffix =
        this->get_parameter("move_j_position_follow_command_topic").as_string();
    std::string follow_topic = ResolveTopicName(
        follow_topic_suffix, "move_j_position_follow_command", prefix, hand->topic_key);
    const auto follow_it = per_hand_follow_topic_overrides_.find(hand->ip);
    if (follow_it != per_hand_follow_topic_overrides_.end() && !follow_it->second.empty()) {
        follow_topic = follow_it->second;
    }
    HandInstance* hand_ptr = hand;
    hand->move_j_position_follow_sub = this->create_subscription<sensor_msgs::msg::JointState>(
        follow_topic, sub_qos,
        [this, hand_ptr](const sensor_msgs::msg::JointState::SharedPtr msg,
                         const rclcpp::MessageInfo& info) {
            OnMoveJPositionFollowCommand(hand_ptr, msg, info);
        });
    hand->follow_control_owner_last_seen = GetSystemNow();
}

void RysenApexHandNode::DetachHandTopics(HandInstance* hand) {
    if (!hand) {
        return;
    }
    hand->move_j_position_follow_sub.reset();
    hand->joint_states_pub.reset();
    hand->motor_states_pub.reset();
    hand->tactile_image_pub.reset();
    hand->hardware_errors_pub.reset();
}

void RysenApexHandNode::ReleaseHandSdk(HandInstance* hand) {
    if (!hand) {
        return;
    }
    if (hand->sdk) {
        if (hand->sdk->IsConnected()) {
            (void)hand->sdk->SetAllFingersDisabled();
        }
        (void)hand->sdk->Disconnect();
        hand->sdk.reset();
    }
    hand->connected = false;
    hand->callbacks_registered = false;
    DetachHandTopics(hand);
}

void RysenApexHandNode::CompleteSuccessfulConnection(HandInstance* hand) {
    if (!hand) {
        return;
    }
    AttachHandTopics(hand);
    RegisterSdkCallbacks(hand);
}

void RysenApexHandNode::ResetHandSdkAfterFailedConnect(HandInstance* hand) {
    if (!hand) {
        return;
    }
    hand->connected = false;
    hand->callbacks_registered = false;
    DetachHandTopics(hand);
    if (hand->sdk) {
        (void)hand->sdk->Disconnect();
        hand->sdk = std::make_unique<rysen::Rysen>();
    }
}

rysen::ErrorCode RysenApexHandNode::ConnectToHand(const std::string& ip_norm, int connection_type) {
    HandInstance* hand = FindHand(ip_norm);
    if (!hand) {
        hand = EnsureHandSdkOnly(ip_norm);
    }
    if (!hand || !hand->sdk) {
        return rysen::ErrorCode::ERROR_CODE_COMM_ERROR;
    }
    if (hand->sdk->IsConnected()) {
        return rysen::ErrorCode::ERROR_CODE_OK;
    }
    const rysen::ErrorCode error =
        hand->sdk->Connect(ip_norm, static_cast<rysen::ConnectionType>(connection_type));
    if (error == rysen::ErrorCode::ERROR_CODE_OK) {
        hand->connected = true;
        CompleteSuccessfulConnection(hand);
    } else if (ip_norm == default_ip_) {
        ResetHandSdkAfterFailedConnect(hand);
    } else {
        std::lock_guard<std::mutex> lock(hands_mutex_);
        auto it = hands_.find(ip_norm);
        if (it != hands_.end() && it->second) {
            ReleaseHandSdk(it->second.get());
            hands_.erase(it);
        }
    }
    return error;
}

bool RysenApexHandNode::ResolveHandForCommand(const std::string& ip_raw,
                                              HandInstance** out,
                                              std::string& err_msg) {
    if (!out) {
        return false;
    }
    *out = nullptr;
    HandInstance* h = FindHand(ip_raw);
    if (!h) {
        err_msg = "no hand for this ip; call connect first";
        return false;
    }
    if (!h->sdk || !h->sdk->IsConnected()) {
        err_msg = "hand not connected";
        return false;
    }
    *out = h;
    return true;
}

void RysenApexHandNode::RegisterSdkCallbacks(HandInstance* hand) {
    if (!hand || !hand->sdk || hand->callbacks_registered) {
        return;
    }
    const int joint_states_freq = this->get_parameter("joint_states_pub_freq").as_int();
    const int motor_states_freq = this->get_parameter("motor_states_pub_freq").as_int();
    const int tactile_freq = this->get_parameter("tactile_image_pub_freq").as_int();
    const uint32_t joint_freq =
        (joint_states_freq > 0) ? static_cast<uint32_t>(joint_states_freq) : 100u;
    const uint32_t motor_freq =
        (motor_states_freq > 0) ? static_cast<uint32_t>(motor_states_freq) : 100u;
    const uint32_t tactile_freq_u = (tactile_freq > 0) ? static_cast<uint32_t>(tactile_freq) : 50u;

    hand->sdk->RegisterGetJointStatesCallback(
        [this, hand](const rysen::JointStates& s) { PublishJointStates(hand, s); }, joint_freq);
    hand->sdk->RegisterGetMotorStatesCallback(
        [this, hand](const rysen::MotorStates& s) { PublishMotorStates(hand, s); }, motor_freq);
    hand->sdk->RegisterGetHandSensorImageCallback(
        [this, hand](const rysen::HandSensorImage& i) { PublishTactileImage(hand, i); },
        tactile_freq_u);
    hand->sdk->RegisterHardwareErrorEventCallback(
        [this, hand](const rysen::HardwareErrorCodes& e) { PublishHardwareErrors(hand, e); });
    hand->callbacks_registered = true;
}

void RysenApexHandNode::HandleMoveJoint(
    const std::shared_ptr<rysen_apexhand_msgs::srv::MoveJoint::Request> request,
    std::shared_ptr<rysen_apexhand_msgs::srv::MoveJoint::Response> response) {
    HandInstance* hand = nullptr;
    std::string err;
    if (!ResolveHandForCommand(request->ip, &hand, err)) {
        response->success = false;
        response->message = err;
        return;
    }
    if (request->joint_ids.size() != request->positions.size() ||
        request->joint_ids.size() != request->velocities.size() ||
        request->joint_ids.size() != request->accelerations.size()) {
        response->success = false;
        response->message = "Joint IDs, positions, velocities, and accelerations size mismatch";
        return;
    }
    std::vector<rysen::JointControlParam> commands;
    for (size_t i = 0; i < request->joint_ids.size(); ++i) {
        rysen::JointControlParam cmd;
        cmd.joint_id = static_cast<rysen::JointId>(request->joint_ids[i]);
        cmd.position = request->positions[i];
        cmd.velocity = request->velocities[i];
        cmd.acceleration = request->accelerations[i];
        commands.push_back(cmd);
    }
    rysen::ErrorCode error = hand->sdk->MoveJoint(commands);
    response->success = (error == rysen::ErrorCode::ERROR_CODE_OK);
    response->message = ErrorCodeToString(error);
}

void RysenApexHandNode::HandleRemoveHand(
    const std::shared_ptr<rysen_apexhand_msgs::srv::RemoveHand::Request> request,
    std::shared_ptr<rysen_apexhand_msgs::srv::RemoveHand::Response> response) {
    if (request->ip.empty()) {
        response->success = false;
        response->message = "ip is empty";
        return;
    }

    const std::string ip_norm = NormalizeIp(request->ip);
    std::lock_guard<std::mutex> lock(hands_mutex_);
    auto it = hands_.find(ip_norm);
    if (it == hands_.end() || !it->second) {
        response->success = false;
        response->message = "hand not found";
        return;
    }

    ReleaseHandSdk(it->second.get());
    hands_.erase(it);
    response->success = true;
    response->message = "hand removed";
}

void RysenApexHandNode::HandleConnect(
    const std::shared_ptr<rysen_apexhand_msgs::srv::Connect::Request> request,
    std::shared_ptr<rysen_apexhand_msgs::srv::Connect::Response> response) {
    const std::string address = NormalizeIp(request->ip);

    if (!request->connect) {
        HandInstance* hand = FindHand(request->ip);
        if (!hand || !hand->sdk) {
            response->success = false;
            response->message = "no hand for this ip";
            return;
        }
        const rysen::ErrorCode error = hand->sdk->Disconnect();
        hand->connected = false;
        hand->callbacks_registered = false;
        DetachHandTopics(hand);
        response->success = (error == rysen::ErrorCode::ERROR_CODE_OK);
        response->message = ErrorCodeToString(error);
        return;
    }

    HandInstance* existing = FindHand(address);
    if (existing && existing->sdk && existing->sdk->IsConnected()) {
        response->success = false;
        response->message = "already connected; remove_hand before reconnecting";
        return;
    }

    const int connection_type = request->connection_type <= 0
                                    ? this->get_parameter("connection_type").as_int()
                                    : request->connection_type;
    const rysen::ErrorCode error = ConnectToHand(address, connection_type);
    response->success = (error == rysen::ErrorCode::ERROR_CODE_OK);
    response->message = ErrorCodeToString(error);
}

void RysenApexHandNode::HandleSetAllFingers(
    const std::shared_ptr<rysen_apexhand_msgs::srv::SetAllFingersEnable::Request> request,
    std::shared_ptr<rysen_apexhand_msgs::srv::SetAllFingersEnable::Response> response) {
    HandInstance* hand = nullptr;
    std::string err;
    if (!ResolveHandForCommand(request->ip, &hand, err)) {
        response->success = false;
        response->message = err;
        return;
    }
    rysen::ErrorCode error =
        request->enable ? hand->sdk->SetAllFingersEnabled() : hand->sdk->SetAllFingersDisabled();
    response->success = (error == rysen::ErrorCode::ERROR_CODE_OK);
    response->message = ErrorCodeToString(error);
}

void RysenApexHandNode::HandleSetMaxJointSpeed(
    const std::shared_ptr<rysen_apexhand_msgs::srv::SetMaxJointSpeed::Request> request,
    std::shared_ptr<rysen_apexhand_msgs::srv::SetMaxJointSpeed::Response> response) {
    HandInstance* hand = nullptr;
    std::string err;
    if (!ResolveHandForCommand(request->ip, &hand, err)) {
        response->success = false;
        response->message = err;
        return;
    }
    if (request->get_only) {
        const rysen::ParamInfo p = hand->sdk->GetParameters();
        response->max_speeds.reserve(request->joint_ids.size());
        for (const auto id : request->joint_ids) {
            const size_t idx = static_cast<size_t>(id);
            if (idx >= p.max_speed.size()) {
                response->success = false;
                response->message = "joint_id out of range in get_only";
                return;
            }
            response->max_speeds.push_back(p.max_speed[idx]);
        }
        response->success = true;
        response->message = "OK";
        return;
    }
    if (request->joint_ids.size() != request->max_speeds.size()) {
        response->success = false;
        response->message = "Size mismatch: joint_ids and max_speeds";
        return;
    }
    std::vector<rysen::MaxJointSpeed> max_speeds;
    max_speeds.reserve(request->joint_ids.size());
    for (size_t i = 0; i < request->joint_ids.size(); ++i) {
        max_speeds.push_back(
            {static_cast<rysen::JointId>(request->joint_ids[i]), request->max_speeds[i]});
    }
    const rysen::ErrorCode error = hand->sdk->SetMaxJointSpeed(max_speeds);
    response->success = (error == rysen::ErrorCode::ERROR_CODE_OK);
    response->message = ErrorCodeToString(error);
}

void RysenApexHandNode::HandleSetMaxJointAccel(
    const std::shared_ptr<rysen_apexhand_msgs::srv::SetMaxJointAccel::Request> request,
    std::shared_ptr<rysen_apexhand_msgs::srv::SetMaxJointAccel::Response> response) {
    HandInstance* hand = nullptr;
    std::string err;
    if (!ResolveHandForCommand(request->ip, &hand, err)) {
        response->success = false;
        response->message = err;
        return;
    }
    if (request->get_only) {
        const rysen::ParamInfo p = hand->sdk->GetParameters();
        response->max_accels.reserve(request->joint_ids.size());
        for (const auto id : request->joint_ids) {
            const size_t idx = static_cast<size_t>(id);
            if (idx >= p.max_accel.size()) {
                response->success = false;
                response->message = "joint_id out of range in get_only";
                return;
            }
            response->max_accels.push_back(p.max_accel[idx]);
        }
        response->success = true;
        response->message = "OK";
        return;
    }
    if (request->joint_ids.size() != request->max_accels.size()) {
        response->success = false;
        response->message = "Size mismatch: joint_ids and max_accels";
        return;
    }
    std::vector<rysen::MaxJointAccel> max_accels;
    max_accels.reserve(request->joint_ids.size());
    for (size_t i = 0; i < request->joint_ids.size(); ++i) {
        max_accels.push_back(
            {static_cast<rysen::JointId>(request->joint_ids[i]), request->max_accels[i]});
    }
    const rysen::ErrorCode error = hand->sdk->SetMaxJointAccel(max_accels);
    response->success = (error == rysen::ErrorCode::ERROR_CODE_OK);
    response->message = ErrorCodeToString(error);
}

void RysenApexHandNode::HandleSetMaxFingerTorque(
    const std::shared_ptr<rysen_apexhand_msgs::srv::SetMaxFingerTorque::Request> request,
    std::shared_ptr<rysen_apexhand_msgs::srv::SetMaxFingerTorque::Response> response) {
    HandInstance* hand = nullptr;
    std::string err;
    if (!ResolveHandForCommand(request->ip, &hand, err)) {
        response->success = false;
        response->message = err;
        return;
    }
    if (request->get_only) {
        const rysen::ParamInfo p = hand->sdk->GetParameters();
        response->max_torques.reserve(request->finger_ids.size());
        for (const auto id : request->finger_ids) {
            const size_t idx = static_cast<size_t>(id);
            if (idx >= p.max_current.size()) {
                response->success = false;
                response->message = "finger_id out of range in get_only";
                return;
            }
            response->max_torques.push_back(p.max_current[idx]);
        }
        response->success = true;
        response->message = "OK";
        return;
    }
    if (request->finger_ids.size() != request->max_torques.size()) {
        response->success = false;
        response->message = "Size mismatch: finger_ids and max_torques";
        return;
    }
    std::vector<rysen::MaxFingerTorque> max_torques;
    max_torques.reserve(request->finger_ids.size());
    for (size_t i = 0; i < request->finger_ids.size(); ++i) {
        max_torques.push_back(
            {static_cast<rysen::FingerId>(request->finger_ids[i]), request->max_torques[i]});
    }
    const rysen::ErrorCode error = hand->sdk->SetMaxFingerTorque(max_torques);
    response->success = (error == rysen::ErrorCode::ERROR_CODE_OK);
    response->message = ErrorCodeToString(error);
}

void RysenApexHandNode::HandleSetFingerEnabled(
    const std::shared_ptr<rysen_apexhand_msgs::srv::SetFingerEnabled::Request> request,
    std::shared_ptr<rysen_apexhand_msgs::srv::SetFingerEnabled::Response> response) {
    HandInstance* hand = nullptr;
    std::string err;
    if (!ResolveHandForCommand(request->ip, &hand, err)) {
        response->success = false;
        response->message = err;
        return;
    }
    if (request->finger_ids.empty()) {
        response->success = false;
        response->message = "No finger IDs provided";
        return;
    }

    std::vector<rysen::FingerId> fingers;
    fingers.reserve(request->finger_ids.size());
    for (const auto& finger_id_msg : request->finger_ids) {
        if (finger_id_msg.finger_id > 4) {
            response->success = false;
            response->message = "Invalid finger ID: " + std::to_string(finger_id_msg.finger_id);
            return;
        }
        fingers.push_back(static_cast<rysen::FingerId>(finger_id_msg.finger_id));
    }

    rysen::ErrorCode error;
    if (request->enable) {
        error = hand->sdk->SetFingerEnabled(fingers);
    } else {
        error = hand->sdk->SetFingerDisabled(fingers);
    }

    response->success = (error == rysen::ErrorCode::ERROR_CODE_OK);
    response->message = ErrorCodeToString(error);
}

void RysenApexHandNode::HandleIsFingerEnabled(
    const std::shared_ptr<rysen_apexhand_msgs::srv::IsFingerEnabled::Request> request,
    std::shared_ptr<rysen_apexhand_msgs::srv::IsFingerEnabled::Response> response) {
    HandInstance* hand = nullptr;
    std::string err;

    // 1. 检查机械手是否在线/连接
    if (!ResolveHandForCommand(request->ip, &hand, err)) {
        response->success = false;
        response->message = err;
        response->is_enabled = false;
        return;
    }

    // 2. 校验手指 ID 是否合法 (0~4)
    if (request->finger_id > 4) {
        response->success = false;
        response->message = "Invalid finger ID: " + std::to_string(request->finger_id);
        response->is_enabled = false;
        return;
    }

    // 3. 调用底层 SDK 获取使能状态
    rysen::FingerId target_finger = static_cast<rysen::FingerId>(request->finger_id);
    response->is_enabled = hand->sdk->IsFingerEnabled(target_finger);

    response->success = true;
    response->message = "OK";
}

void RysenApexHandNode::HandleSetDeviceIpAddress(
    const std::shared_ptr<rysen_apexhand_msgs::srv::SetDeviceIPAddress::Request> request,
    std::shared_ptr<rysen_apexhand_msgs::srv::SetDeviceIPAddress::Response> response) {
    HandInstance* hand = nullptr;
    std::string err;
    if (!ResolveHandForCommand(request->original_ip, &hand, err)) {
        response->success = false;
        response->message = err;
        return;
    }
    if (request->new_ip.empty()) {
        response->success = false;
        response->message = "new_ip is empty";
        return;
    }
    RCLCPP_WARN(this->get_logger(),
                "SetDeviceIPAddress: setting IP to %s (SDK will close connection after success)",
                request->new_ip.c_str());
    rysen::ErrorCode error = hand->sdk->SetDeviceIPAddress(request->new_ip);
    response->success = (error == rysen::ErrorCode::ERROR_CODE_OK);
    response->message = ErrorCodeToString(error);
    if (error == rysen::ErrorCode::ERROR_CODE_OK) {
        const std::string key = NormalizeIp(request->original_ip);
        {
            std::lock_guard<std::mutex> lock(hands_mutex_);
            auto it = hands_.find(key);
            if (it != hands_.end() && it->second) {
                ReleaseHandSdk(it->second.get());
                hands_.erase(it);
            }
        }
        response->message += " Hand released from node; call connect with ip '" + request->new_ip +
                             "' to reconnect.";
        RCLCPP_INFO(this->get_logger(),
                    "SetDeviceIPAddress succeeded; hand %s released from node (reconnect at %s)",
                    key.c_str(), request->new_ip.c_str());
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    } else if (!hand->sdk->IsConnected()) {
        hand->connected = false;
        hand->callbacks_registered = false;
        DetachHandTopics(hand);
        response->message += " (disconnected)";
    }
}

void RysenApexHandNode::HandleStartTactileCalibration(
    const std::shared_ptr<rysen_apexhand_msgs::srv::StartTactileCalibration::Request> request,
    std::shared_ptr<rysen_apexhand_msgs::srv::StartTactileCalibration::Response> response) {
    HandInstance* hand = nullptr;
    std::string err;
    if (!ResolveHandForCommand(request->ip, &hand, err)) {
        response->success = false;
        response->message = err;
        return;
    }
    rysen::ErrorCode error = hand->sdk->StartTactileCalibration();
    response->success = (error == rysen::ErrorCode::ERROR_CODE_OK);
    response->message = ErrorCodeToString(error);
}

void RysenApexHandNode::HandleClearTactileCalibration(
    const std::shared_ptr<rysen_apexhand_msgs::srv::ClearTactileCalibration::Request> request,
    std::shared_ptr<rysen_apexhand_msgs::srv::ClearTactileCalibration::Response> response) {
    HandInstance* hand = nullptr;
    std::string err;
    if (!ResolveHandForCommand(request->ip, &hand, err)) {
        response->success = false;
        response->message = err;
        return;
    }
    rysen::ErrorCode error = hand->sdk->ClearTactileCalibration();
    response->success = (error == rysen::ErrorCode::ERROR_CODE_OK);
    response->message = ErrorCodeToString(error);
}

void RysenApexHandNode::HandleCleanFaults(
    const std::shared_ptr<rysen_apexhand_msgs::srv::CleanFaults::Request> request,
    std::shared_ptr<rysen_apexhand_msgs::srv::CleanFaults::Response> response) {
    HandInstance* hand = nullptr;
    std::string err;
    if (!ResolveHandForCommand(request->ip, &hand, err)) {
        response->success = false;
        response->message = err;
        return;
    }
    rysen::ErrorCode error = hand->sdk->CleanFaults();
    response->success = (error == rysen::ErrorCode::ERROR_CODE_OK);
    response->message = ErrorCodeToString(error);
}

void RysenApexHandNode::HandleGetConnectionInfo(
    const std::shared_ptr<rysen_apexhand_msgs::srv::GetConnectionInfo::Request> request,
    std::shared_ptr<rysen_apexhand_msgs::srv::GetConnectionInfo::Response> response) {
    (void)request;
    std::lock_guard<std::mutex> lock(hands_mutex_);
    response->ips.reserve(hands_.size());
    response->connected.reserve(hands_.size());
    response->device_ips.reserve(hands_.size());
    response->hand_sides.reserve(hands_.size());
    for (const auto& kv : hands_) {
        const auto& hand = kv.second;
        if (!hand || !hand->sdk) {
            continue;
        }
        const bool is_connected = hand->sdk->IsConnected();
        response->ips.push_back(hand->ip);
        response->connected.push_back(is_connected);
        response->device_ips.push_back(is_connected ? hand->ip : "");
        if (!is_connected) {
            response->hand_sides.push_back("unknown");
            continue;
        }
        const rysen::HandDir hand_dir = hand->sdk->GetHandDir();
        response->hand_sides.push_back(hand_dir == rysen::HandDir::LEFT ? "left" : "right");
    }
}

void RysenApexHandNode::HandleGetVersionInfo(
    const std::shared_ptr<rysen_apexhand_msgs::srv::GetVersionInfo::Request> request,
    std::shared_ptr<rysen_apexhand_msgs::srv::GetVersionInfo::Response> response) {
    HandInstance* hand = nullptr;
    std::string err;
    if (!ResolveHandForCommand(request->ip, &hand, err)) {
        response->sdk_version.clear();
        response->hand_firmware_version.clear();
        response->touch_sensor_version.clear();
        RCLCPP_WARN(this->get_logger(), "GetVersionInfo: %s", err.c_str());
        return;
    }
    const rysen::VersionInfo version = hand->sdk->GetVersionInfo();
    response->sdk_version = version.sdk_version;
    response->hand_firmware_version = version.hand_firmware_version;
    response->touch_sensor_version = version.touch_sensor_version;
}

void RysenApexHandNode::OnMoveJPositionFollowCommand(
    HandInstance* hand,
    const sensor_msgs::msg::JointState::SharedPtr msg,
    const rclcpp::MessageInfo& message_info) {
    if (!hand || !hand->sdk || !hand->sdk->IsConnected()) {
        return;
    }
    std::ostringstream oss;
    const auto& gid = message_info.get_rmw_message_info().publisher_gid;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < RMW_GID_STORAGE_SIZE; ++i) {
        oss << std::setw(2) << static_cast<int>(gid.data[i]);
    }
    const std::string requester_id = oss.str();
    const rclcpp::Time now = GetSystemNow();
    {
        std::lock_guard<std::mutex> lock(hand->follow_control_owner_mutex);
        if (!hand->follow_control_owner_id.empty()) {
            const int64_t elapsed_ms =
                (now - hand->follow_control_owner_last_seen).nanoseconds() / 1000000;
            if (elapsed_ms > follow_control_owner_timeout_ms_) {
                RCLCPP_INFO(this->get_logger(),
                            "Follow owner timeout for hand %s, releasing owner: %s",
                            hand->ip.c_str(), hand->follow_control_owner_id.c_str());
                hand->follow_control_owner_id.clear();
            }
        }
        if (hand->follow_control_owner_id.empty()) {
            hand->follow_control_owner_id = requester_id;
            RCLCPP_INFO(this->get_logger(), "Follow owner acquired for hand %s: %s",
                        hand->ip.c_str(), hand->follow_control_owner_id.c_str());
        } else if (hand->follow_control_owner_id != requester_id) {
            RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 2000,
                                 "Follow ignored for hand %s: owner is another publisher",
                                 hand->ip.c_str());
            return;
        }
        hand->follow_control_owner_last_seen = now;
    }

    if (msg->name.size() != msg->position.size()) {
        RCLCPP_ERROR(this->get_logger(),
                     "MoveJPositionFollowCommand: Joint names and positions size mismatch");
        return;
    }

    std::vector<rysen::MoveJPositionFollowParam> follow_params;
    for (size_t i = 0; i < msg->name.size(); ++i) {
        auto it = joint_name_to_id_map_.find(msg->name[i]);
        if (it != joint_name_to_id_map_.end()) {
            rysen::MoveJPositionFollowParam param;
            param.id = it->second;
            param.position = msg->position[i];
            param.torque = kJointBalanceTorque.at(static_cast<size_t>(param.id));
            follow_params.push_back(param);
        } else {
            RCLCPP_WARN(this->get_logger(), "Unknown joint name: %s, skipping",
                        msg->name[i].c_str());
        }
    }

    if (follow_params.empty()) {
        RCLCPP_WARN(this->get_logger(), "No valid joints found in MoveJPositionFollowCommand");
        return;
    }

    rysen::ErrorCode error = hand->sdk->MoveJPositionFollow(follow_params);
    // if (error != rysen::ErrorCode::ERROR_CODE_OK) {
    //     RCLCPP_ERROR(this->get_logger(), "MoveJPositionFollowCommand failed: %s",
    //                  ErrorCodeToString(error).c_str());
    // }
    if (error != rysen::ErrorCode::ERROR_CODE_OK) {
        // 将无脑的 RCLCPP_ERROR 改为 RCLCPP_WARN_THROTTLE，限制为每 2000 毫秒（2秒）最多打印一次
        RCLCPP_WARN_THROTTLE(
            this->get_logger(), *this->get_clock(), 2000,
            "MoveJPositionFollowCommand returned warning/error: %s (values were clamped safely)",
            ErrorCodeToString(error).c_str());
    }
}

void RysenApexHandNode::PublishJointStates(HandInstance* hand, const rysen::JointStates& states) {
    if (!hand || !hand->joint_states_pub) {
        return;
    }
    auto msg = std::make_shared<sensor_msgs::msg::JointState>();
    msg->header = ConvertTime(states.timestamp);
    msg->header.frame_id = frame_id_;

    msg->name.reserve(states.joint_states.size());
    msg->position.reserve(states.joint_states.size());
    msg->velocity.reserve(states.joint_states.size());
    msg->effort.reserve(states.joint_states.size());

    for (const auto& joint_state : states.joint_states) {
        size_t joint_idx = static_cast<size_t>(joint_state.joint_id);
        if (joint_idx < joint_names_.size()) {
            msg->name.push_back(joint_names_[joint_idx]);
            msg->position.push_back(joint_state.position);
            msg->velocity.push_back(joint_state.velocity);
            msg->effort.push_back(joint_state.torque);
        }
    }
    hand->joint_states_pub->publish(*msg);
}

void RysenApexHandNode::PublishMotorStates(HandInstance* hand, const rysen::MotorStates& states) {
    if (!hand || !hand->motor_states_pub) {
        return;
    }
    auto msg = std::make_shared<rysen_apexhand_msgs::msg::MotorState>();
    msg->header = ConvertTime(states.timestamp);
    msg->header.frame_id = frame_id_;

    msg->name.reserve(states.motors.size());
    msg->temperature.reserve(states.motors.size());
    msg->current.reserve(states.motors.size());

    for (const auto& motor_state : states.motors) {
        size_t motor_idx = static_cast<size_t>(motor_state.motor_id);
        if (motor_idx < motor_names_.size()) {
            msg->name.push_back(motor_names_[motor_idx]);
            msg->temperature.push_back(motor_state.temperature);
            msg->current.push_back(motor_state.current);
        }
    }
    hand->motor_states_pub->publish(*msg);
}

void RysenApexHandNode::PublishTactileImage(HandInstance* hand,
                                            const rysen::HandSensorImage& image) {
    if (!hand || !hand->tactile_image_pub) {
        return;
    }
    auto msg = std::make_shared<rysen_apexhand_msgs::msg::HandTactileForces>();

    builtin_interfaces::msg::Time stamp = ConvertTime(image.timestamp).stamp;
    msg->stamp = stamp;

    msg->index = ConvertCommonFingerTactile(image.index_image);
    msg->middle = ConvertCommonFingerTactile(image.middle_image);
    msg->ring = ConvertCommonFingerTactile(image.ring_image);
    msg->little = ConvertCommonFingerTactile(image.little_image);
    msg->thumb = ConvertThumbFingerTactile(image.thumb_image);

    msg->palm_center = ConvertTactileImage(image.palm_center);

    hand->tactile_image_pub->publish(*msg);
}

void RysenApexHandNode::PublishHardwareErrors(HandInstance* hand,
                                              const rysen::HardwareErrorCodes& hardware_errors) {
    if (!hand || !hand->hardware_errors_pub) {
        return;
    }
    if (hardware_errors.device_error_code != 0 || hardware_errors.thumb_error_code != 0 ||
        hardware_errors.index_error_code != 0 || hardware_errors.middle_error_code != 0 ||
        hardware_errors.ring_error_code != 0 || hardware_errors.little_error_code != 0) {
        rysen_apexhand_msgs::msg::HardwareErrors msg;
        msg.header.stamp = GetSystemNow();
        msg.header.frame_id = frame_id_;
        msg.device_error_code = hardware_errors.device_error_code;
        msg.thumb_error_code = hardware_errors.thumb_error_code;
        msg.index_error_code = hardware_errors.index_error_code;
        msg.middle_error_code = hardware_errors.middle_error_code;
        msg.ring_error_code = hardware_errors.ring_error_code;
        msg.little_error_code = hardware_errors.little_error_code;
        hand->hardware_errors_pub->publish(msg);

        RCLCPP_ERROR(this->get_logger(),
                     "SDK Hardware Error (device:0x%016lX, thumb:0x%016lX, index:0x%016lX, "
                     "middle:0x%016lX, ring:0x%016lX, little:0x%016lX)",
                     hardware_errors.device_error_code, hardware_errors.thumb_error_code,
                     hardware_errors.index_error_code, hardware_errors.middle_error_code,
                     hardware_errors.ring_error_code, hardware_errors.little_error_code);
    }
}

std_msgs::msg::Header RysenApexHandNode::ConvertTime(
    const std::chrono::steady_clock::time_point& time_point) {
    std_msgs::msg::Header header;
    SdkSteadyToSystemStamp(time_point, header.stamp);
    header.frame_id = frame_id_;
    return header;
}

void RysenApexHandNode::SdkSteadyToSystemStamp(
    const std::chrono::steady_clock::time_point& sdk_steady, builtin_interfaces::msg::Time& out) {
    if (!time_sync_initialized_.load()) {
        time_sync_ref_steady_ = sdk_steady;
        time_sync_ref_system_ = GetSystemNow();
        time_sync_initialized_.store(true);
    }
    const auto delta_ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(sdk_steady - time_sync_ref_steady_);
    const rclcpp::Time system_time = time_sync_ref_system_ + rclcpp::Duration(delta_ns);
    const int64_t ns = system_time.nanoseconds();
    out.sec = static_cast<int32_t>(ns / 1000000000LL);
    out.nanosec = static_cast<uint32_t>(ns % 1000000000LL);
}

rysen_apexhand_msgs::msg::TactileImage RysenApexHandNode::ConvertTactileImage(
    const rysen::TactileImage& tactile) {
    rysen_apexhand_msgs::msg::TactileImage msg;
    msg.width = tactile.width;
    msg.height = tactile.height;

    msg.gray_image.resize(tactile.gray_image.size());
    for (size_t i = 0; i < tactile.gray_image.size(); ++i) {
        msg.gray_image[i] = tactile.gray_image[i];
    }

    msg.tangential_forces.theta = tactile.tangential_forces.theta;
    msg.tangential_forces.magnitude = tactile.tangential_forces.magnitude;

    return msg;
}

rysen_apexhand_msgs::msg::CommonFingerTactile RysenApexHandNode::ConvertCommonFingerTactile(
    const rysen::CommonFingerSensorImage& finger_image) {
    rysen_apexhand_msgs::msg::CommonFingerTactile msg;
    msg.prox_pad = ConvertTactileImage(finger_image.prox_pad);
    msg.mid_pad = ConvertTactileImage(finger_image.mid_pad);
    msg.dist_pad = ConvertTactileImage(finger_image.dist_pad);
    return msg;
}

rysen_apexhand_msgs::msg::ThumbFingerTactile RysenApexHandNode::ConvertThumbFingerTactile(
    const rysen::ThumbFingerSensorImage& thumb_image) {
    rysen_apexhand_msgs::msg::ThumbFingerTactile msg;
    msg.prox_pad = ConvertTactileImage(thumb_image.prox_pad);
    msg.mid_pad = ConvertTactileImage(thumb_image.mid_pad);
    msg.dist_pad = ConvertTactileImage(thumb_image.dist_pad);
    return msg;
}

void RysenApexHandNode::InitializeJointNameMapping() {
    joint_name_to_id_map_.clear();

    for (size_t i = 0; i < joint_names_.size(); ++i) {
        joint_name_to_id_map_[joint_names_[i]] = static_cast<rysen::JointId>(i);
    }

    static const std::vector<std::string> canonical_joint_names = {
        "thumb_cmc_abd",   "thumb_cmc_rot",  "thumb_cmc_flex",  "thumb_mcp_flex",
        "thumb_ip_flex",   "index_mcp_abd",  "index_mcp_flex",  "index_pip_flex",
        "index_dip_flex",  "middle_mcp_abd", "middle_mcp_flex", "middle_pip_flex",
        "middle_dip_flex", "ring_mcp_abd",   "ring_mcp_flex",   "ring_pip_flex",
        "ring_dip_flex",   "little_mcp_abd", "little_mcp_flex", "little_pip_flex",
        "little_dip_flex"};
    for (size_t i = 0; i < canonical_joint_names.size(); ++i) {
        joint_name_to_id_map_[canonical_joint_names[i]] = static_cast<rysen::JointId>(i);
    }
}

std::string RysenApexHandNode::ErrorCodeToString(const rysen::ErrorCode& error) {
    switch (error) {
        case rysen::ErrorCode::ERROR_CODE_OK:
            return "OK";
        case rysen::ErrorCode::ERROR_CODE_COMM_ERROR:
            return "Communication error";
        case rysen::ErrorCode::ERROR_CODE_TIMEOUT:
            return "Timeout";
        case rysen::ErrorCode::ERROR_CODE_OUT_OF_RANGE:
            return "Out of range";
        case rysen::ErrorCode::ERROR_CODE_OVER_SPEED:
            return "Over speed";
        case rysen::ErrorCode::ERROR_CODE_OTHER_ERROR:
            return "Other error";
        case rysen::ErrorCode::ERROR_CODE_INVALID_ARGUMENT:
            return "Invalid argument";
        case rysen::ErrorCode::ERROR_CODE_CONFIG_ERROR:
            return "Config error";
        case rysen::ErrorCode::ERROR_CODE_NOT_IMPLEMENTED:
            return "Not implemented";
        case rysen::ErrorCode::ERROR_CODE_URDF_ERROR:
            return "URDF error";
        case rysen::ErrorCode::ERROR_CODE_HARDWARE_ERROR:
            return "Hardware error";
        case rysen::ErrorCode::ERROR_CODE_NETWORK_LATENCY_HIGH:
            return "Network latency high";
        default:
            return "Unknown error";
    }
}

}  // namespace rysen_apexhand
