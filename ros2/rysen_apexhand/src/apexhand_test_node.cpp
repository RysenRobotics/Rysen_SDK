#include <chrono>
#include <cmath>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/joint_state.hpp"
#include "rysen_apexhand_msgs/srv/connect.hpp"
#include "rysen_apexhand_msgs/srv/set_all_fingers_enable.hpp"

using namespace std::chrono_literals;

class ApexHandTestNode : public rclcpp::Node {
public:
    ApexHandTestNode() : Node("apexhand_test_node") {
        // 声明参数：模式 (1: 仅左手102, 2: 仅右手103, 3: 双手同时)
        this->declare_parameter<int>("test_mode", 3);
        int mode = this->get_parameter("test_mode").as_int();

        if (mode == 1 || mode == 3) active_ips_.push_back("192.168.0.102");
        if (mode == 2 || mode == 3) active_ips_.push_back("192.168.0.103");

        RCLCPP_INFO(this->get_logger(), "🛡️ 安全测试节点启动，当前模式: %d (控制 %zu 只手)", mode, active_ips_.size());

        // 初始化客户端
        connect_client_ = this->create_client<rysen_apexhand_msgs::srv::Connect>("rysen/apexhand/connect");
        enable_client_ = this->create_client<rysen_apexhand_msgs::srv::SetAllFingersEnable>("rysen/apexhand/set_all_fingers");

        // 等待服务上线
        while (!connect_client_->wait_for_service(2s)) {
            RCLCPP_INFO(this->get_logger(), "等待 Connect 服务上线...");
        }
        while (!enable_client_->wait_for_service(2s)) {
            RCLCPP_INFO(this->get_logger(), "等待 SetAllFingers 服务上线...");
        }

        // 初始化控制话题发布者 (按 IP 动态生成话题名)
        for (const auto& ip : active_ips_) {
            std::string topic_name = "rysen/apexhand/ip_" + ReplaceDots(ip) + "/move_j_position_follow_command";
            auto pub = this->create_publisher<sensor_msgs::msg::JointState>(topic_name, 10);
            follow_pubs_[ip] = pub;
        }

        // 启动高频控制定时器 (50Hz = 20ms)，但初始状态下不发送数据
        control_timer_ = this->create_wall_timer(20ms, std::bind(&ApexHandTestNode::ControlLoop, this));
        
        // 启动独立的工作线程，执行 连->使能->控制->去使能->断开 的流程
        sequence_thread_ = std::thread(&ApexHandTestNode::RunTestSequence, this);
    }

    ~ApexHandTestNode() {
        if (sequence_thread_.joinable()) {
            sequence_thread_.join();
        }
    }

private:
    // 工具函数：把 IP 里的点换成下划线
    std::string ReplaceDots(std::string ip) {
        std::replace(ip.begin(), ip.end(), '.', '_');
        return ip;
    }

    // 核心测试序列 (在独立线程中运行，允许阻塞调用服务)
    void RunTestSequence() {
        // 1. 连接设备
        for (const auto& ip : active_ips_) {
            CallConnectService(ip, true);
        }
        std::this_thread::sleep_for(1s); 

        // 2. 使能所有手指
        for (const auto& ip : active_ips_) {
            CallEnableService(ip, true);
        }
        std::this_thread::sleep_for(1s);

        // 3. 开始随动控制 (MoveJPositionFollow)
        RCLCPP_INFO(this->get_logger(), ">>> 🌊 开始安全抓握测试 (持续 8 秒) <<<");
        start_time_ = this->now();
        is_controlling_ = true; 
        
        std::this_thread::sleep_for(8s); // 运行 8 秒（刚好完整走完 4 个抓握周期）
        
        is_controlling_ = false; 
        RCLCPP_INFO(this->get_logger(), ">>> 🛑 停止发送随动指令 <<<");
        std::this_thread::sleep_for(500ms);

        // 4. 去使能
        for (const auto& ip : active_ips_) {
            CallEnableService(ip, false);
        }
        std::this_thread::sleep_for(1s);

        // 5. 断开连接
        for (const auto& ip : active_ips_) {
            CallConnectService(ip, false);
        }

        RCLCPP_INFO(this->get_logger(), "🎉 测试流程全部完成，准备退出节点！");
        rclcpp::shutdown();
    }

    // 高频控制循环 (50Hz)
    void ControlLoop() {
        if (!is_controlling_) return;

        double elapsed_sec = (this->now() - start_time_).seconds();
        
        // 运动因子：0.5Hz 的正弦波，值域平滑在 [0.0, 1.0] 之间循环
        double motion_factor = 0.5 * (1.0 - std::cos(2.0 * M_PI * 0.5 * elapsed_sec));

        sensor_msgs::msg::JointState msg;
        msg.header.stamp = this->now();

        // ================== 安全红线 1：强制锁死四指侧摆关节 ==================
        std::vector<std::string> locked_abd_joints = {
            "index_mcp_abd", "middle_mcp_abd", "ring_mcp_abd", "little_mcp_abd"
        };
        for (const auto& j : locked_abd_joints) {
            msg.name.push_back(j);
            msg.position.push_back(0.0); // 锁死在 0 度，防止横向干涉
        }

        // ================== 姿态固定：大拇指基座固定避碰 ==================
        msg.name.push_back("thumb_cmc_abd");
        msg.position.push_back(0.5); // 理论 0~1.39，停在安全偏外侧位置
        msg.name.push_back("thumb_cmc_rot");
        msg.position.push_back(0.2); // 理论 -0.17~1.04，微小正转

        // ================== 安全红线 2：弯曲关节保守内缩区间 ==================
        // 所有弯曲关节的理论下限都 <= 0，理论上限都 >= 1.39
        // 我们安全起见，只让它们在 [0.1, 1.2] 弧度之间微幅抓握
        double safe_flex_min = 0.1;
        double safe_flex_max = 1.2;
        double current_flex_pos = safe_flex_min + motion_factor * (safe_flex_max - safe_flex_min);

        std::vector<std::string> flex_joints = {
            "thumb_cmc_flex", "thumb_mcp_flex", "thumb_ip_flex",
            "index_mcp_flex", "index_pip_flex", "index_dip_flex",
            "middle_mcp_flex", "middle_pip_flex", "middle_dip_flex",
            "ring_mcp_flex", "ring_pip_flex", "ring_dip_flex",
            "little_mcp_flex", "little_pip_flex", "little_dip_flex"
        };

        for (const auto& j : flex_joints) {
            msg.name.push_back(j);
            msg.position.push_back(current_flex_pos);
        }

        // 向所有激活的机械手发布指令
        for (const auto& ip : active_ips_) {
            follow_pubs_[ip]->publish(msg);
        }
    }

    // 阻塞调用 Connect 服务
    void CallConnectService(const std::string& ip, bool connect) {
        auto req = std::make_shared<rysen_apexhand_msgs::srv::Connect::Request>();
        req->ip = ip;
        req->connect = connect;
        req->connection_type = 1;

        RCLCPP_INFO(this->get_logger(), "请求 %s: %s", (connect ? "连接" : "断开"), ip.c_str());
        auto result = connect_client_->async_send_request(req);
        if (result.wait_for(5s) == std::future_status::ready) {
            auto res = result.get();
            if (!res->success) {
                RCLCPP_WARN(this->get_logger(), "%s 失败: %s", ip.c_str(), res->message.c_str());
            }
        } else {
            RCLCPP_ERROR(this->get_logger(), "%s 服务调用超时！", ip.c_str());
        }
    }

    // 阻塞调用 Enable 服务
    void CallEnableService(const std::string& ip, bool enable) {
        auto req = std::make_shared<rysen_apexhand_msgs::srv::SetAllFingersEnable::Request>();
        req->ip = ip;
        req->enable = enable;

        RCLCPP_INFO(this->get_logger(), "请求 %s: %s", (enable ? "使能" : "去使能"), ip.c_str());
        auto result = enable_client_->async_send_request(req);
        if (result.wait_for(5s) == std::future_status::ready) {
            auto res = result.get();
            if (!res->success) {
                RCLCPP_WARN(this->get_logger(), "%s 失败: %s", ip.c_str(), res->message.c_str());
            }
        } else {
            RCLCPP_ERROR(this->get_logger(), "%s 服务调用超时！", ip.c_str());
        }
    }

    std::vector<std::string> active_ips_;
    rclcpp::Client<rysen_apexhand_msgs::srv::Connect>::SharedPtr connect_client_;
    rclcpp::Client<rysen_apexhand_msgs::srv::SetAllFingersEnable>::SharedPtr enable_client_;
    std::unordered_map<std::string, rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr> follow_pubs_;
    
    rclcpp::TimerBase::SharedPtr control_timer_;
    std::thread sequence_thread_;
    
    bool is_controlling_ = false;
    rclcpp::Time start_time_;
};

int main(int argc, char **argv) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<ApexHandTestNode>();
    rclcpp::executors::MultiThreadedExecutor executor;
    executor.add_node(node);
    executor.spin();
    rclcpp::shutdown();
    return 0;
}