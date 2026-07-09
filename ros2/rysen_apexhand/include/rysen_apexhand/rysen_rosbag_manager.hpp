#ifndef RYSEN_APEXHAND_RYSEN_ROSBAG_MANAGER_HPP_
#define RYSEN_APEXHAND_RYSEN_ROSBAG_MANAGER_HPP_

#include <memory>
#include <mutex>
#include <rysen_apexhand_msgs/msg/hand_tactile_forces.hpp>
#include <sensor_msgs/msg/joint_state.hpp>
#include <string>
#include <thread>
#include <unordered_map>

#include "rclcpp/rclcpp.hpp"
#include "rosbag2_cpp/reader.hpp"
#include "rosbag2_cpp/writer.hpp"

// 前置声明 Node 类，避免循环包含
namespace rysen_apexhand {
class RysenApexHandNode;
}

namespace rysen_apexhand {

class ApexHandBagManager {
   public:
    // 构造函数传入 ROS 节点的弱指针或共享指针，以便调用节点的日志、时间戳或发布器
    explicit ApexHandBagManager(rclcpp::Node* node_ptr);
    ~ApexHandBagManager();

    // 录制控制接口
    bool startRecording(const std::string& ip, const std::string& folder_path, double frequency);
    void stopRecording(const std::string& ip);

    // 每当 SDK 推送新数据时，由 Node 调用此接口将数据压入录制队列
    void writeJointState(const std::string& ip, const sensor_msgs::msg::JointState& msg);
    void writeTactileImage(const std::string& ip,
                           const rysen_apexhand_msgs::msg::HandTactileForces& msg);

    // 定义回调函数类型：当回播读出一帧 JointState 时触发
    using PlaybackJointCallback =
        std::function<void(const std::string& ip, const sensor_msgs::msg::JointState& msg)>;

    // 供主节点注册回调的接口
    void setPlaybackJointCallback(PlaybackJointCallback cb) {
        playback_joint_cb_ = cb;
    }

    // 回播控制接口
    bool startPlayback(const std::string& ip, const std::string& folder_path, double speed_ratio);
    void stopPlayback(const std::string& ip);

   private:
    rclcpp::Node* node_ptr_{nullptr};  // 借用主节点的指针用于日志打印

    // 内部结构体：管理单只手的录制上下文
    struct RecordContext {
        std::unique_ptr<rosbag2_cpp::Writer> writer;
        double target_frequency;
        rclcpp::TimerBase::SharedPtr sampling_timer;  // 用定时器来控制裁剪后的采样频率
        // 用于暂存最新的数据快照（配合读写锁）
        sensor_msgs::msg::JointState latest_joint;
        rysen_apexhand_msgs::msg::HandTactileForces latest_tactile;
        std::mutex data_mutex;
    };

    // 内部结构体：管理单只手的回播上下文
    struct PlaybackContext {
        std::unique_ptr<rosbag2_cpp::Reader> reader;
        std::thread playback_thread;
        bool is_running{false};
    };

    PlaybackJointCallback playback_joint_cb_;  // 存放回调

    std::mutex manager_mutex_;
    std::unordered_map<std::string, std::unique_ptr<RecordContext>> active_records_;
    std::unordered_map<std::string, std::unique_ptr<PlaybackContext>> active_playbacks_;

    // 内部回播线程执行函数
    void playbackLoop(const std::string& ip, PlaybackContext* ctx, double speed_ratio);
};

}  // namespace rysen_apexhand

#endif  // RYSEN_APEXHAND_RYSEN_ROSBAG_MANAGER_HPP_