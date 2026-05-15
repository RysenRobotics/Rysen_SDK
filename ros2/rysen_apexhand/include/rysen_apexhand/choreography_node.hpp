/**
 * @file    choreography_node.hpp
 * @brief   ROS 2 node for Rysen ApexHand choreography execution.
 * Receives JSON scripts via Service and streams JointStates to Driver Node.
 */

#ifndef RYSEN_APEXHAND__CHOREOGRAPHY_NODE_HPP_
#define RYSEN_APEXHAND__CHOREOGRAPHY_NODE_HPP_

#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/joint_state.hpp>
#include <string>
#include <vector>

#include "rysen_apexhand_msgs/msg/choreography_status.hpp"
#include "rysen_apexhand_msgs/srv/control_choreography.hpp"
#include "rysen_apexhand_msgs/srv/start_choreography.hpp"
// 🌟 新增：引入获取连接信息的服务
#include <nlohmann/json.hpp>

#include "rysen_apexhand_msgs/srv/get_connection_info.hpp"

namespace rysen_apexhand {

// --- 基础数据结构 (保持不变) ---
struct JointData {
    double value;
    double min;
    double max;
    std::vector<std::string> child;
};

struct Keyframe {
    double time;
    std::map<std::string, JointData> joint_states;
};

struct PlaybackConfig {
    double rate = 1.0;
    bool loop = false;
    bool yoyo = false;
    double loop_transition_s = 0.0;
    double keyframe_hold_s = 0.0;
    double max_joint_speed_deg_s = 100.0;
};

enum class PlayState {
    IDLE = 0,
    PLAYING = 1,
    PAUSED = 2,
    HOLDING = 3,
    TRANSITIONING = 4,
    RESETTING = 5,
    HOMING = 6,
    RESUMING = 7,
    PAUSING = 8
};

// ==========================================
// 单手运行上下文 (Hand Context)
// ==========================================
struct HandContext {
    std::string ip;
    std::string topic_key;  // 例如: ip_192_168_0_102

    // 该手专属的 ROS 通信句柄
    rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr follow_pub;
    rclcpp::Publisher<rysen_apexhand_msgs::msg::ChoreographyStatus>::SharedPtr status_pub;
    rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr joint_state_sub;

    // 该手独立的状态机与配置
    PlayState state{PlayState::IDLE};
    PlaybackConfig config;

    std::vector<Keyframe> keyframes;
    double max_t{0.0};

    // 时间轴控制
    double anim_t{0.0};
    int direction{1};
    std::chrono::time_point<std::chrono::steady_clock> last_time;

    // 内部记忆状态
    double current_hold_timer{0.0};
    double loop_trans_timer{0.0};
    std::map<std::string, double> loop_trans_start_pose;

    // 柔性移动
    double custom_move_timer{0.0};
    double custom_move_duration{2.0};
    std::map<std::string, double> move_start_pose_deg;
    std::map<std::string, double> move_target_pose_deg;

    // 硬件驻留与前端UI视觉锁定
    int settle_ticks_remaining{0};
    bool pin_status_to_end_frame{false};
    double end_target_time_s{0.0};

    // 硬件反馈与滤波器
    std::map<std::string, double> current_hardware_pose_deg;
    std::vector<double> last_sent_rad;
    bool is_first_frame{true};

    // 构造函数初始化滤波器数组
    HandContext() : last_sent_rad(21, 0.0) {
        for (int i = 0; i < 21; ++i)
            current_hardware_pose_deg["f" /* 需要辅助函数初始化，见cpp */] = 0.0;
    }
};

// ==========================================
// 大脑节点定义
// ==========================================
class ChoreographyNode : public rclcpp::Node {
   public:
    explicit ChoreographyNode(const rclcpp::NodeOptions& options = rclcpp::NodeOptions());
    ~ChoreographyNode() override = default;

   private:
    // --- 动态感知与管理 ---
    rclcpp::TimerBase::SharedPtr connection_check_timer_;  // 低频轮询定时器
    rclcpp::Client<rysen_apexhand_msgs::srv::GetConnectionInfo>::SharedPtr
        get_connection_info_client_;

    void OnCheckConnections();               // 轮询底层硬件连接状态
    void AddHand(const std::string& ip);     // 动态创建上下文
    void RemoveHand(const std::string& ip);  // 动态销毁上下文

    // --- ROS 对外服务接口 ---
    // 服务端只有一个，负责接收所有手的请求
    rclcpp::Service<rysen_apexhand_msgs::srv::StartChoreography>::SharedPtr start_srv_;
    rclcpp::Service<rysen_apexhand_msgs::srv::ControlChoreography>::SharedPtr control_srv_;
    rclcpp::TimerBase::SharedPtr control_timer_;  // 90Hz 主心跳

    void OnStartChoreography(
        const std::shared_ptr<rysen_apexhand_msgs::srv::StartChoreography::Request> request,
        std::shared_ptr<rysen_apexhand_msgs::srv::StartChoreography::Response> response);
    void OnControlChoreography(
        const std::shared_ptr<rysen_apexhand_msgs::srv::ControlChoreography::Request> request,
        std::shared_ptr<rysen_apexhand_msgs::srv::ControlChoreography::Response> response);
    void OnTimer();  // 核心：遍历所有 Context 的 90Hz 主循环

    // --- 数据与锁 ---
    std::map<std::string, std::shared_ptr<HandContext>> hands_map_;
    std::mutex map_mutex_;  // 保护多线程访问 (定时器和Service可能并发)

    // 全局配置参数
    std::string multi_hand_topic_prefix_;
    std::string frame_id_;

    // --- 算法辅助函数 (签名需传入 Context 指针) ---
    bool ParseJson(const std::string& json_str, HandContext* ctx);
    std::map<std::string, double> GetInterpolatedJointValues(HandContext* ctx, double t);
    std::map<std::string, double> ApplyCouplingAndLimits(std::map<std::string, double> target_deg);
    void PublishCommand(HandContext* ctx,
                        const std::map<std::string, double>& target_deg,
                        double dt_real);
    void ChangeState(HandContext* ctx, PlayState new_state);
    std::string IndexToName(int offset);
};

}  // namespace rysen_apexhand

#endif  // RYSEN_APEXHAND__CHOREOGRAPHY_NODE_HPP_