#include "rysen_apexhand/rysen_rosbag_manager.hpp"

// 引入 ROS 2 基础组件
#include <rclcpp/rclcpp.hpp>

// 引入序列化相关 (读取时必须用到)
#include <rclcpp/serialization.hpp>
#include <rclcpp/serialized_message.hpp>

// 引入 ROS 2 自带的文件系统工具，替代 std::filesystem
#include <rcpputils/filesystem_helper.hpp>

// 引入 Rosbag2 的 API 和存储结构
#include <rosbag2_cpp/reader.hpp>
#include <rosbag2_cpp/writer.hpp>
#include <rosbag2_storage/serialized_bag_message.hpp>

// 引入消息头文件
#include <algorithm>
#include <chrono>
#include <memory>
#include <rysen_apexhand_msgs/msg/hand_tactile_forces.hpp>
#include <sensor_msgs/msg/joint_state.hpp>
#include <thread>

namespace rysen_apexhand {

ApexHandBagManager::ApexHandBagManager(rclcpp::Node* node_ptr) : node_ptr_(node_ptr) {
    if (!node_ptr_) {
        throw std::runtime_error("BagManager requires a valid Node pointer!");
    }
}

ApexHandBagManager::~ApexHandBagManager() {
    // 析构时安全停止所有正在进行的录制和回播
    std::lock_guard<std::mutex> lock(manager_mutex_);
    active_records_.clear();
    active_playbacks_.clear();
}

bool ApexHandBagManager::startRecording(const std::string& ip,
                                        const std::string& folder_path,
                                        double frequency) {
    std::lock_guard<std::mutex> lock(manager_mutex_);

    // 严格限制频率在 10.0 到 500.0 之间
    double safe_frequency = std::clamp(frequency, 10.0, 500.0);

    if (active_records_.find(ip) != active_records_.end()) {
        RCLCPP_WARN(node_ptr_->get_logger(), "Hand %s is already recording.", ip.c_str());
        return false;
    }

    auto ctx = std::make_unique<RecordContext>();
    ctx->target_frequency = safe_frequency;  // 使用安全频率

    ctx->writer = std::make_unique<rosbag2_cpp::Writer>();

    // 1. 处理传入的路径
    std::string safe_path = folder_path;

    // 如果客户没有传参，赋予默认的绝对路径
    if (safe_path.empty()) {
        safe_path = "/opt/rysen_teach/data/rosbag";
        RCLCPP_INFO(node_ptr_->get_logger(), "No folder path provided, defaulting to: %s",
                    safe_path.c_str());
    }

    // 去除末尾的斜杠防呆
    if (!safe_path.empty() && safe_path.back() == '/') {
        safe_path.pop_back();
    }

    // 2. 尝试创建目录并带上权限降级保护
    try {
        rcpputils::fs::path dir_path(safe_path);
        if (!rcpputils::fs::exists(dir_path)) {
            rcpputils::fs::create_directories(dir_path);
        }
    } catch (const std::exception& e) {
        // 如果权限不足（比如在普通 PC 上没用 sudo 运行），降级到 /tmp
        RCLCPP_WARN(node_ptr_->get_logger(),
                    "Failed to create %s (Missing sudo?). Falling back to /tmp/rosbag. Error: %s",
                    safe_path.c_str(), e.what());

        safe_path = "/tmp/rosbag";
        rcpputils::fs::create_directories(rcpputils::fs::path(safe_path));
    }

    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << safe_path << "/apexhand_" << ip << "_"
       << std::put_time(std::localtime(&in_time_t), "%Y%m%d_%H%M%S");

    std::string final_bag_uri = ss.str();

    rosbag2_storage::StorageOptions storage_options;
    storage_options.uri = final_bag_uri;
    storage_options.storage_id = "sqlite3";  // 暂时用 sqlite3 测试

    // 标准化 topic 名称：替换 . 为 _，并加上前导 /
    std::string ip_key = ip;
    std::replace(ip_key.begin(), ip_key.end(), '.', '_');
    ip_key = "ip_" + ip_key;

    std::string joint_topic = "/rysen/apexhand/" + ip_key + "/joint_states";
    std::string tactile_topic = "/rysen/apexhand/" + ip_key + "/hand_tactile_forces";

    try {
        ctx->writer->open(storage_options);

        ctx->writer->create_topic({joint_topic, "sensor_msgs/msg/JointState", "cdr", ""});
        ctx->writer->create_topic(
            {tactile_topic, "rysen_apexhand_msgs/msg/HandTactileForces", "cdr", ""});

    } catch (const std::exception& e) {
        RCLCPP_ERROR(node_ptr_->get_logger(), "Failed to open bag: %s", e.what());
        return false;
    }

    auto period = std::chrono::duration<double>(1.0 / safe_frequency);
    RecordContext* ctx_ptr = ctx.get();

    // 定时器回调：现在变得极其简单！
    ctx->sampling_timer = node_ptr_->create_wall_timer(
        std::chrono::duration_cast<std::chrono::nanoseconds>(period),
        [this, ip, ctx_ptr, joint_topic, tactile_topic]() {
            sensor_msgs::msg::JointState joint_copy;
            rysen_apexhand_msgs::msg::HandTactileForces tactile_copy;

            {
                std::lock_guard<std::mutex> data_lock(ctx_ptr->data_mutex);
                joint_copy = ctx_ptr->latest_joint;
                tactile_copy = ctx_ptr->latest_tactile;
            }

            auto time_stamp = node_ptr_->now();

            // 直接调用泛型 write 方法，底层全自动处理序列化和内存！
            if (!joint_copy.name.empty()) {
                ctx_ptr->writer->write(joint_copy, joint_topic, time_stamp);
            }

            // 等你后续加上了最新 tactile 的缓存，这里可以直接解开注释：
            // ctx_ptr->writer->write(tactile_copy, tactile_topic, time_stamp);
        });

    active_records_[ip] = std::move(ctx);
    RCLCPP_INFO(node_ptr_->get_logger(), "Started recording to %s", final_bag_uri.c_str());
    return true;
}

void ApexHandBagManager::writeJointState(const std::string& ip,
                                         const sensor_msgs::msg::JointState& msg) {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    auto it = active_records_.find(ip);
    if (it != active_records_.end()) {
        // 只更新最新状态，不阻塞 SDK 底层！
        std::lock_guard<std::mutex> data_lock(it->second->data_mutex);
        it->second->latest_joint = msg;
    }
}

void ApexHandBagManager::stopRecording(const std::string& ip) {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    auto it = active_records_.find(ip);
    if (it != active_records_.end()) {
        it->second->sampling_timer->cancel();  // 停止定时器
        it->second->writer->close();           // 关闭文件句柄，触发刷盘
        active_records_.erase(it);             // 释放上下文
        RCLCPP_INFO(node_ptr_->get_logger(), "Stopped recording for hand %s", ip.c_str());
    }
}

// ==========================================
// 回播逻辑 (Playback)
// ==========================================

bool ApexHandBagManager::startPlayback(const std::string& ip,
                                       const std::string& folder_path,
                                       double speed_ratio) {
    std::lock_guard<std::mutex> lock(manager_mutex_);

    // 惰性清理已经自然结束的回播线程
    auto it = active_playbacks_.find(ip);
    if (it != active_playbacks_.end()) {
        if (!it->second->is_running) {
            // 线程已经跑完了，进行安全回收
            if (it->second->playback_thread.joinable()) {
                it->second->playback_thread.join();
            }
            active_playbacks_.erase(it);
            RCLCPP_INFO(node_ptr_->get_logger(), "Cleaned up finished playback context for hand %s",
                        ip.c_str());
        } else {
            RCLCPP_WARN(node_ptr_->get_logger(), "Hand %s is currently playing back.", ip.c_str());
            return false;
        }
    }

    auto ctx = std::make_unique<PlaybackContext>();
    ctx->is_running = true;
    ctx->reader = std::make_unique<rosbag2_cpp::Reader>();

    try {
        ctx->reader->open(folder_path);
    } catch (const std::exception& e) {
        RCLCPP_ERROR(node_ptr_->get_logger(), "Failed to open bag for playback: %s", e.what());
        return false;
    }

    // 启动独立线程读取数据，避免卡死主节点
    PlaybackContext* ctx_ptr = ctx.get();
    ctx->playback_thread =
        std::thread(&ApexHandBagManager::playbackLoop, this, ip, ctx_ptr, speed_ratio);

    active_playbacks_[ip] = std::move(ctx);
    RCLCPP_INFO(node_ptr_->get_logger(), "Started playback for hand %s", ip.c_str());
    return true;
}

void ApexHandBagManager::playbackLoop(const std::string& ip,
                                      PlaybackContext* ctx,
                                      double speed_ratio) {
    if (speed_ratio <= 0.0) {
        speed_ratio = 1.0;  // 防止除以 0 或负数导致时间计算异常
    }

    // 确保回播监听的话题名称和录制时完全一致
    std::string ip_key = ip;
    std::replace(ip_key.begin(), ip_key.end(), '.', '_');
    ip_key = "ip_" + ip_key;
    std::string target_topic = "/rysen/apexhand/" + ip_key + "/joint_states";

    // 实例化序列化工具
    rclcpp::Serialization<sensor_msgs::msg::JointState> serializer;

    bool is_first_msg = true;
    rcutils_time_point_value_t last_bag_time = 0;
    auto last_real_time = std::chrono::steady_clock::now();

    while (ctx->is_running && ctx->reader->has_next()) {
        auto serialized_msg = ctx->reader->read_next();

        // 1. 话题过滤：目前我们只关心关节数据
        if (serialized_msg->topic_name != target_topic) {
            continue;
        }

        // 2. 时间戳同步逻辑
        if (is_first_msg) {
            last_bag_time = serialized_msg->time_stamp;
            last_real_time = std::chrono::steady_clock::now();
            is_first_msg = false;
        } else {
            // 计算两帧在录制时的间隔时间 (纳秒)
            auto bag_diff_ns = serialized_msg->time_stamp - last_bag_time;

            // 根据倍速调整需要等待的时间
            auto wait_duration =
                std::chrono::nanoseconds(static_cast<long long>(bag_diff_ns / speed_ratio));

            auto target_time = last_real_time + wait_duration;

            // 使用 sleep_until 比 sleep_for 更准，因为它可以吸收代码执行的耗时
            std::this_thread::sleep_until(target_time);

            last_bag_time = serialized_msg->time_stamp;
            last_real_time = std::chrono::steady_clock::now();
        }

        // 睡眠期间如果用户调用了 stopPlayback，立即退出循环
        if (!ctx->is_running) {
            break;
        }

        // 3. 反序列化出 ROS Message
        sensor_msgs::msg::JointState joint_cmd;
        rclcpp::SerializedMessage extracted_msg(*serialized_msg->serialized_data);
        serializer.deserialize_message(&extracted_msg, &joint_cmd);

        // 4. 通过回调将指令丢给主节点执行
        if (playback_joint_cb_) {
            playback_joint_cb_(ip, joint_cmd);
        } else {
            RCLCPP_WARN_ONCE(
                node_ptr_->get_logger(),
                "Playback read a message, but no callback is registered to execute it!");
        }
    }

    RCLCPP_INFO(node_ptr_->get_logger(), "Playback thread for %s finished or stopped.", ip.c_str());
    ctx->is_running = false;
}

void ApexHandBagManager::stopPlayback(const std::string& ip) {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    auto it = active_playbacks_.find(ip);
    if (it != active_playbacks_.end()) {
        it->second->is_running = false;  // 通知线程退出
        if (it->second->playback_thread.joinable()) {
            it->second->playback_thread.join();
        }
        active_playbacks_.erase(it);
        RCLCPP_INFO(node_ptr_->get_logger(), "Stopped playback for hand %s", ip.c_str());
    }
}

}  // namespace rysen_apexhand