#include "rysen_apexhand/choreography_node.hpp"

#include <algorithm>
#include <cmath>

using json = nlohmann::json;
using namespace std::chrono_literals;

namespace rysen_apexhand {

// ================== SDK 角度物理限制 ==================
const double SDK_K_MIN_RAD[21] = {0.0,     -0.1745, 0.0,     -0.349, -0.349,  -0.436,  -0.349,
                                  -0.0873, -0.0873, -0.436,  -0.349, -0.0873, -0.0873, -0.436,
                                  -0.349,  -0.0873, -0.0873, -0.436, -0.349,  -0.0873, -0.0873};
const double SDK_K_MAX_RAD[21] = {1.3963, 1.047, 1.3963, 1.3963, 1.3963, 0.436, 1.5708,
                                  1.745,  1.745, 0.436,  1.5708, 1.745,  1.745, 0.436,
                                  1.5708, 1.745, 1.745,  0.436,  1.5708, 1.745, 1.745};

ChoreographyNode::ChoreographyNode(const rclcpp::NodeOptions& options)
    : Node("choreography_node", options) {
    // 1. 获取全局参数
    this->declare_parameter<std::string>("multi_hand_topic_prefix", "rysen/apexhand");
    this->declare_parameter<std::string>("frame_id", "base_link");
    multi_hand_topic_prefix_ = this->get_parameter("multi_hand_topic_prefix").as_string();
    frame_id_ = this->get_parameter("frame_id").as_string();

    RCLCPP_INFO(this->get_logger(), "=== Central Choreography Brain Starting ===");

    // 2. 初始化全局控制服务 (统一入口)
    std::string start_srv_name = multi_hand_topic_prefix_ + "/start_choreography";
    std::string control_srv_name = multi_hand_topic_prefix_ + "/control_choreography";

    start_srv_ = this->create_service<rysen_apexhand_msgs::srv::StartChoreography>(
        start_srv_name, std::bind(&ChoreographyNode::OnStartChoreography, this,
                                  std::placeholders::_1, std::placeholders::_2));

    control_srv_ = this->create_service<rysen_apexhand_msgs::srv::ControlChoreography>(
        control_srv_name, std::bind(&ChoreographyNode::OnControlChoreography, this,
                                    std::placeholders::_1, std::placeholders::_2));

    // 3. 初始化动态连接感知客户端
    std::string get_info_srv_name = multi_hand_topic_prefix_ + "/get_connection_info";
    get_connection_info_client_ =
        this->create_client<rysen_apexhand_msgs::srv::GetConnectionInfo>(get_info_srv_name);

    // 4. 启动 1Hz 的连接轮询定时器
    connection_check_timer_ =
        this->create_wall_timer(1000ms, std::bind(&ChoreographyNode::OnCheckConnections, this));

    // 5. 启动 90Hz 的核心插值控制定时器 (群发引擎)
    control_timer_ = this->create_wall_timer(11ms, std::bind(&ChoreographyNode::OnTimer, this));

    RCLCPP_INFO(this->get_logger(), "Central Brain is ready. Polling for hand connections...");
}

// -----------------------------------------------------------------------------
// 🌟 动态感知与生命周期管理
// -----------------------------------------------------------------------------
void ChoreographyNode::OnCheckConnections() {
    // 如果服务不可用，跳过本次轮询
    if (!get_connection_info_client_->wait_for_service(0s))
        return;

    auto request = std::make_shared<rysen_apexhand_msgs::srv::GetConnectionInfo::Request>();
    auto future = get_connection_info_client_->async_send_request(
        request,
        [this](rclcpp::Client<rysen_apexhand_msgs::srv::GetConnectionInfo>::SharedFuture future) {
            auto response = future.get();
            std::lock_guard<std::mutex> lock(map_mutex_);

            // 将当前底层在线的 IP 放入集合
            std::vector<std::string> active_ips;
            for (size_t i = 0; i < response->ips.size(); ++i) {
                if (response->connected[i]) {
                    active_ips.push_back(response->ips[i]);
                }
            }

            // 1. 添加新手
            for (const auto& ip : active_ips) {
                if (hands_map_.find(ip) == hands_map_.end()) {
                    AddHand(ip);
                }
            }

            // 2. 移除掉线的手
            std::vector<std::string> ips_to_remove;
            for (const auto& [ip, ctx] : hands_map_) {
                if (std::find(active_ips.begin(), active_ips.end(), ip) == active_ips.end()) {
                    ips_to_remove.push_back(ip);
                }
            }
            for (const auto& ip : ips_to_remove) {
                RemoveHand(ip);
            }
        });
}

void ChoreographyNode::AddHand(const std::string& ip) {
    auto ctx = std::make_shared<HandContext>();
    ctx->ip = ip;
    std::string ip_key = ip;
    std::replace(ip_key.begin(), ip_key.end(), '.', '_');
    ctx->topic_key = "ip_" + ip_key;

    // 为该手创建专属的话题收发器
    std::string follow_topic =
        multi_hand_topic_prefix_ + "/" + ctx->topic_key + "/move_j_position_follow_command";
    std::string joint_states_topic =
        multi_hand_topic_prefix_ + "/" + ctx->topic_key + "/joint_states";
    std::string status_topic =
        multi_hand_topic_prefix_ + "/" + ctx->topic_key + "/choreography_status";

    ctx->follow_pub = this->create_publisher<sensor_msgs::msg::JointState>(follow_topic, 10);
    ctx->status_pub =
        this->create_publisher<rysen_apexhand_msgs::msg::ChoreographyStatus>(status_topic, 10);

    // 订阅硬件真实反馈
    ctx->joint_state_sub = this->create_subscription<sensor_msgs::msg::JointState>(
        joint_states_topic, 10,
        [this, ctx_ptr = ctx.get()](const sensor_msgs::msg::JointState::SharedPtr msg) {
            for (size_t i = 0; i < msg->name.size(); ++i) {
                ctx_ptr->current_hardware_pose_deg[msg->name[i]] = msg->position[i] * 180.0 / M_PI;
            }
        });

    ctx->last_time = std::chrono::steady_clock::now();
    for (int i = 0; i < 21; ++i) ctx->current_hardware_pose_deg[IndexToName(i)] = 0.0;

    hands_map_[ip] = ctx;
    RCLCPP_INFO(this->get_logger(), "➕ Assigned Brain Context for Hand: %s", ip.c_str());
}

void ChoreographyNode::RemoveHand(const std::string& ip) {
    hands_map_.erase(ip);  // 智能指针会自动销毁内部的 Publisher 和 Subscriber
    RCLCPP_INFO(this->get_logger(), "➖ Released Brain Context for Hand: %s", ip.c_str());
}

// -----------------------------------------------------------------------------
// 🌟 路由分发层 (Service Callbacks)
// -----------------------------------------------------------------------------
void ChoreographyNode::OnStartChoreography(
    const std::shared_ptr<rysen_apexhand_msgs::srv::StartChoreography::Request> request,
    std::shared_ptr<rysen_apexhand_msgs::srv::StartChoreography::Response> response) {
    std::lock_guard<std::mutex> lock(map_mutex_);
    auto it = hands_map_.find(request->ip);
    if (it == hands_map_.end()) {
        response->success = false;
        response->message = "Failed: Target IP not connected or brain not yet assigned.";
        return;
    }

    HandContext* ctx = it->second.get();

    if (ctx->state != PlayState::IDLE && ctx->state != PlayState::PAUSED) {
        response->success = false;
        response->message = "Failed: Node is busy. Stop current task first.";
        return;
    }

    if (!ParseJson(request->json_content, ctx)) {
        response->success = false;
        response->message = "Failed: Invalid JSON format or empty keyframes.";
        return;
    }

    // 处理并校验传入的时间戳 (timestamp)
    // 如果前端传了负数或者大于总时长的时间，我们使用 std::clamp (或 max/min)
    // 将其强行限制在合法范围内
    double desired_start_time = request->start_time;
    desired_start_time = std::max(0.0, std::min(desired_start_time, ctx->max_t));

    ctx->config.rate = request->playback_rate > 0 ? request->playback_rate : 1.0;
    ctx->config.loop = request->loop;
    ctx->config.yoyo = request->yoyo;
    ctx->config.loop_transition_s =
        request->loop_transition_s > 0.0 ? request->loop_transition_s : 1.5;
    ctx->config.keyframe_hold_s = request->keyframe_hold_s > 0.0 ? request->keyframe_hold_s : 0.0;
    ctx->config.max_joint_speed_deg_s =
        request->max_joint_speed_deg_s > 0.0 ? request->max_joint_speed_deg_s : 100.0;

    ctx->current_hold_timer = 0.0;
    ctx->anim_t = desired_start_time;  // 1. 进度条直接定位到目标时间戳
    ctx->direction = 1;
    ctx->move_start_pose_deg = ctx->current_hardware_pose_deg;

    // 2. 柔性归位的目标姿态，设置为该时间戳对应的插值姿态，而不是 0.0
    ctx->move_target_pose_deg = GetInterpolatedJointValues(ctx, desired_start_time);

    // 动态计算 Homing 所需的最短时间
    double max_diff_deg = 0.0;
    for (int i = 0; i < 21; ++i) {
        std::string name = IndexToName(i);
        double s_val = ctx->move_start_pose_deg.count(name) ? ctx->move_start_pose_deg[name] : 0.0;
        double t_val =
            ctx->move_target_pose_deg.count(name) ? ctx->move_target_pose_deg[name] : 0.0;
        max_diff_deg = std::max(max_diff_deg, std::abs(t_val - s_val));
    }

    double required_time = (max_diff_deg / ctx->config.max_joint_speed_deg_s) + 0.05;

    ctx->custom_move_timer = 0.0;
    ctx->custom_move_duration = std::max(0.01, required_time);
    ctx->is_first_frame = true;

    ChangeState(ctx, PlayState::HOMING);

    response->success = true;
    response->message = "Choreography loaded on " + request->ip + ". Seeking to " +
                        std::to_string(desired_start_time) + "s. Auto-homing will take " +
                        std::to_string(ctx->custom_move_duration) + "s.";
}

void ChoreographyNode::OnControlChoreography(
    const std::shared_ptr<rysen_apexhand_msgs::srv::ControlChoreography::Request> request,
    std::shared_ptr<rysen_apexhand_msgs::srv::ControlChoreography::Response> response) {
    std::lock_guard<std::mutex> lock(map_mutex_);
    auto it = hands_map_.find(request->ip);
    if (it == hands_map_.end()) {
        response->success = false;
        response->message = "Failed: Target IP not connected.";
        return;
    }

    HandContext* ctx = it->second.get();

    if (request->command == 1) {  // CMD_PAUSE
        if (ctx->state == PlayState::PLAYING || ctx->state == PlayState::HOLDING ||
            ctx->state == PlayState::TRANSITIONING) {
            // 不要使用带有延迟的硬件反馈作为起点！
            // 而是使用上一帧刚刚下发的平滑指令作为起点，避免指令回跳
            for (int i = 0; i < 21; ++i) {
                ctx->move_start_pose_deg[IndexToName(i)] = ctx->last_sent_rad[i] * 180.0 / M_PI;
            }

            ctx->move_target_pose_deg = GetInterpolatedJointValues(ctx, ctx->anim_t);

            // 采用固定缓冲时间，让底层电机平滑到位
            ctx->custom_move_timer = 0.0;
            ctx->custom_move_duration = 0.15;
            ctx->is_first_frame = true;

            // 手动暂停也需要短暂驻留与UI锁定 (约 10 个 Tick 的稳定期)
            ctx->settle_ticks_remaining = 10;
            ctx->pin_status_to_end_frame = true;
            ctx->end_target_time_s = ctx->anim_t;

            ChangeState(ctx, PlayState::PAUSING);

            response->success = true;
            response->message = "Pausing... Hardware is syncing to target over " +
                                std::to_string(ctx->custom_move_duration) + "s.";
        }
    } else if (request->command == 2) {  // CMD_RESUME
        if (ctx->state == PlayState::PAUSED) {
            if (ctx->anim_t >= ctx->max_t && ctx->direction > 0)
                ctx->anim_t = 0.0;

            // 暂停期间释放了控制权，必须先从当前物理位置平滑拉回
            ctx->move_start_pose_deg = ctx->current_hardware_pose_deg;
            ctx->move_target_pose_deg = GetInterpolatedJointValues(ctx, ctx->anim_t);

            // 动态计算拉回所需的安全时间
            double max_diff_deg = 0.0;
            for (int i = 0; i < 21; ++i) {
                std::string name = IndexToName(i);
                double s_val =
                    ctx->move_start_pose_deg.count(name) ? ctx->move_start_pose_deg[name] : 0.0;
                double t_val =
                    ctx->move_target_pose_deg.count(name) ? ctx->move_target_pose_deg[name] : 0.0;
                max_diff_deg = std::max(max_diff_deg, std::abs(t_val - s_val));
            }
            double required_time = (max_diff_deg / ctx->config.max_joint_speed_deg_s) + 0.05;

            ctx->custom_move_timer = 0.0;
            ctx->custom_move_duration = std::max(0.01, required_time);
            ctx->is_first_frame = true;

            // 进入拉回状态
            ChangeState(ctx, PlayState::RESUMING);

            response->success = true;
            response->message = "Resuming. Pulling hand back smoothly over " +
                                std::to_string(ctx->custom_move_duration) + "s.";
        }
    } else if (request->command == 3) {  // CMD_STOP_AND_RESET
        ctx->current_hold_timer = 0.0;
        ctx->move_start_pose_deg = ctx->current_hardware_pose_deg;
        ctx->move_target_pose_deg.clear();
        for (int i = 0; i < 21; ++i) ctx->move_target_pose_deg[IndexToName(i)] = 0.0;
        ctx->custom_move_timer = 0.0;
        ctx->custom_move_duration = 1.5;
        ChangeState(ctx, PlayState::RESETTING);
        response->success = true;
        response->message = "Resetting...";
    } else if (request->command == 4) {  // CMD_UPDATE_PARAMS
        if (request->max_joint_speed_deg_s > 0.0)
            ctx->config.max_joint_speed_deg_s = request->max_joint_speed_deg_s;
        if (request->playback_rate > 0.0)
            ctx->config.rate = request->playback_rate;
        if (request->keyframe_hold_s >= 0.0)
            ctx->config.keyframe_hold_s = request->keyframe_hold_s;
        ctx->config.loop = request->loop;
        ctx->config.yoyo = request->yoyo;

        if (ctx->state != PlayState::TRANSITIONING && request->loop_transition_s >= 0.0) {
            ctx->config.loop_transition_s = request->loop_transition_s;
            response->message = "Params updated.";
        } else {
            response->message = "Params updated, but loop_transition_s ignored.";
        }
        response->success = true;
    } else if (request->command == 5) {  // CMD_JUMP_TO_TIME
        if (ctx->state == PlayState::IDLE || ctx->state == PlayState::PAUSED) {
            ctx->current_hold_timer = 0.0;
            ctx->anim_t =
                std::max(0.0, std::min(static_cast<double>(request->target_time), ctx->max_t));
            response->success = true;
            response->message = "Jumped.";
        }
    }
}

// -----------------------------------------------------------------------------
// 🌟 核心并发引擎 (Core Orchestrator)
// -----------------------------------------------------------------------------
void ChoreographyNode::OnTimer() {
    std::lock_guard<std::mutex> lock(map_mutex_);
    auto now = std::chrono::steady_clock::now();

    // 遍历并发处理每一只在线的手
    for (auto& [ip, ctx_ptr] : hands_map_) {
        HandContext* ctx = ctx_ptr.get();

        double dt_real = std::chrono::duration<double>(now - ctx->last_time).count();
        ctx->last_time = now;

        // 1. 发布该手专属状态
        rysen_apexhand_msgs::msg::ChoreographyStatus status_msg;
        status_msg.ip = ctx->ip;
        status_msg.state = static_cast<uint8_t>(ctx->state);

        // 处于驻留阶段时锁定前端时间显示，防止进度条跳动
        if (ctx->pin_status_to_end_frame) {
            status_msg.current_time = ctx->end_target_time_s;
        } else {
            status_msg.current_time = ctx->anim_t;
        }

        status_msg.max_time = ctx->max_t;
        ctx->status_pub->publish(status_msg);

        if (ctx->state == PlayState::IDLE)
            continue;

        // --- HOMING / RESETTING / RESUMING / PAUSING ---
        if (ctx->state == PlayState::HOMING || ctx->state == PlayState::RESETTING ||
            ctx->state == PlayState::RESUMING || ctx->state == PlayState::PAUSING) {
            ctx->custom_move_timer += dt_real;
            double ratio = std::min(1.0, ctx->custom_move_timer / ctx->custom_move_duration);
            std::map<std::string, double> current_target;
            for (int i = 0; i < 21; ++i) {
                std::string name = IndexToName(i);
                double s_val =
                    ctx->move_start_pose_deg.count(name) ? ctx->move_start_pose_deg[name] : 0.0;
                double t_val =
                    ctx->move_target_pose_deg.count(name) ? ctx->move_target_pose_deg[name] : 0.0;
                current_target[name] = s_val + (t_val - s_val) * ratio;
            }
            PublishCommand(ctx, ApplyCouplingAndLimits(current_target), dt_real);

            // 当动作完成时，根据不同状态去往不同的归宿
            if (ratio >= 1.0) {
                if (ctx->state == PlayState::HOMING) {
                    ChangeState(ctx, PlayState::PAUSED);
                } else if (ctx->state == PlayState::PAUSING) {
                    // 不要立刻切换到 PAUSED，而是倒数驻留 Tick 消除抖动
                    if (ctx->settle_ticks_remaining > 0) {
                        ctx->settle_ticks_remaining--;
                    } else {
                        ctx->pin_status_to_end_frame = false;  // 解除UI锁定
                        ChangeState(ctx, PlayState::PAUSED);  // 驻留结束，彻底释放控制权
                    }
                } else if (ctx->state == PlayState::RESETTING) {
                    ChangeState(ctx, PlayState::IDLE);
                } else if (ctx->state == PlayState::RESUMING) {
                    ChangeState(ctx, PlayState::PLAYING);
                }
            }
            continue;
        }

        // --- PAUSED ---
        if (ctx->state == PlayState::PAUSED) {
            //停止时释放movePositionFollow的控制权
            continue;
        }

        // --- HOLDING ---
        if (ctx->state == PlayState::HOLDING) {
            ctx->current_hold_timer -= dt_real;
            PublishCommand(
                ctx, ApplyCouplingAndLimits(GetInterpolatedJointValues(ctx, ctx->anim_t)), dt_real);
            if (ctx->current_hold_timer <= 0.0)
                ChangeState(ctx, PlayState::PLAYING);
            continue;
        }

        // --- TRANSITIONING ---
        if (ctx->state == PlayState::TRANSITIONING) {
            ctx->loop_trans_timer += dt_real;
            double duration =
                ctx->config.loop_transition_s > 0.0 ? ctx->config.loop_transition_s : 0.01;
            double ratio = std::min(1.0, ctx->loop_trans_timer / duration);

            std::map<std::string, double> frame0_pose = GetInterpolatedJointValues(ctx, 0.0);
            std::map<std::string, double> current_target;
            for (int i = 0; i < 21; ++i) {
                std::string name = IndexToName(i);
                double s_val =
                    ctx->loop_trans_start_pose.count(name) ? ctx->loop_trans_start_pose[name] : 0.0;
                double t_val = frame0_pose.count(name) ? frame0_pose[name] : 0.0;
                current_target[name] = s_val + (t_val - s_val) * ratio;
            }
            PublishCommand(ctx, ApplyCouplingAndLimits(current_target), dt_real);

            if (ratio >= 1.0) {
                ctx->anim_t = 0.0;
                ChangeState(ctx, PlayState::PLAYING);
            }
            continue;
        }

        // --- PLAYING ---
        if (ctx->state == PlayState::PLAYING) {
            double dt = dt_real * ctx->config.rate;
            double next_t = ctx->anim_t + dt * ctx->direction;

            bool hit_keyframe = false;
            if (ctx->direction > 0) {
                for (const auto& kf : ctx->keyframes) {
                    if (kf.time > ctx->anim_t && kf.time <= next_t) {
                        next_t = kf.time;
                        hit_keyframe = true;
                        break;
                    }
                }
            } else {
                for (auto it = ctx->keyframes.rbegin(); it != ctx->keyframes.rend(); ++it) {
                    if (it->time < ctx->anim_t && it->time >= next_t) {
                        next_t = it->time;
                        hit_keyframe = true;
                        break;
                    }
                }
            }

            ctx->anim_t = next_t;
            if (hit_keyframe && ctx->config.keyframe_hold_s > 0.0) {
                ctx->current_hold_timer = ctx->config.keyframe_hold_s;
                ChangeState(ctx, PlayState::HOLDING);
            }

            if (ctx->anim_t >= ctx->max_t && ctx->direction > 0) {
                ctx->anim_t = ctx->max_t;
                if (ctx->config.yoyo) {
                    ctx->direction = -1;
                    // 🌟 复用 HOLDING：给硬件 0.15s 追赶时间，拒绝引入新状态！
                    // 如果原本因为关键帧有了 hold 时间，则取较长者
                    ctx->current_hold_timer = std::max(0.15, ctx->current_hold_timer);
                    ChangeState(ctx, PlayState::HOLDING);
                } else if (ctx->config.loop) {
                    ctx->loop_trans_timer = 0.0;
                    ctx->loop_trans_start_pose = GetInterpolatedJointValues(ctx, ctx->max_t);
                    ChangeState(ctx, PlayState::TRANSITIONING);
                } else {
                    // 动作正向自然结束：进入自动收尾状态
                    for (int i = 0; i < 21; ++i) {
                        ctx->move_start_pose_deg[IndexToName(i)] =
                            ctx->last_sent_rad[i] * 180.0 / M_PI;
                    }
                    ctx->move_target_pose_deg = GetInterpolatedJointValues(ctx, ctx->max_t);

                    ctx->custom_move_timer = 0.0;
                    ctx->custom_move_duration = 0.15;  // 固定 0.15 秒缓冲
                    ctx->is_first_frame = true;

                    // 设置驻留时间与 UI 锁定
                    ctx->settle_ticks_remaining = 10;
                    ctx->pin_status_to_end_frame = true;
                    ctx->end_target_time_s = ctx->max_t;

                    ChangeState(ctx, PlayState::PAUSING);
                }
            } else if (ctx->anim_t <= 0.0 && ctx->direction < 0) {
                ctx->anim_t = 0.0;
                ctx->direction = 1;
                if (ctx->config.yoyo) {
                    // 🌟 复用 HOLDING 状态处理反向极点缓冲
                    ctx->current_hold_timer = std::max(0.15, ctx->current_hold_timer);
                    ChangeState(ctx, PlayState::HOLDING);
                } else if (!ctx->config.loop) {
                    // 动作反向自然结束：进入自动收尾状态
                    for (int i = 0; i < 21; ++i) {
                        ctx->move_start_pose_deg[IndexToName(i)] =
                            ctx->last_sent_rad[i] * 180.0 / M_PI;
                    }
                    ctx->move_target_pose_deg = GetInterpolatedJointValues(ctx, 0.0);

                    ctx->custom_move_timer = 0.0;
                    ctx->custom_move_duration = 0.15;  // 固定 0.15 秒缓冲
                    ctx->is_first_frame = true;

                    // 设置驻留时间与 UI 锁定
                    ctx->settle_ticks_remaining = 10;
                    ctx->pin_status_to_end_frame = true;
                    ctx->end_target_time_s = 0.0;

                    ChangeState(ctx, PlayState::PAUSING);
                }
            }

            // 防双重下发！
            // 只有当状态依然是 PLAYING（没有在上面几行变成 PAUSING 或 HOLDING）时，才下发正常帧
            if (ctx->state == PlayState::PLAYING) {
                PublishCommand(ctx,
                               ApplyCouplingAndLimits(GetInterpolatedJointValues(ctx, ctx->anim_t)),
                               dt_real);
            }
        }
    }
}

// -----------------------------------------------------------------------------
// 算法辅助层 (上下文隔离)
// -----------------------------------------------------------------------------
void ChoreographyNode::PublishCommand(HandContext* ctx,
                                      const std::map<std::string, double>& target_deg,
                                      double dt_real) {
    auto msg = std::make_shared<sensor_msgs::msg::JointState>();
    msg->header.stamp = this->now();
    msg->header.frame_id = frame_id_;

    for (int i = 0; i < 21; ++i) {
        std::string name = IndexToName(i);
        double final_deg = target_deg.count(name) ? target_deg.at(name) : 0.0;
        double target_rad =
            std::max(SDK_K_MIN_RAD[i], std::min(SDK_K_MAX_RAD[i], final_deg * M_PI / 180.0));

        if (!ctx->is_first_frame) {
            double max_delta_rad = ctx->config.max_joint_speed_deg_s * (M_PI / 180.0) * dt_real;
            double diff = target_rad - ctx->last_sent_rad[i];
            if (diff > max_delta_rad)
                target_rad = ctx->last_sent_rad[i] + max_delta_rad;
            else if (diff < -max_delta_rad)
                target_rad = ctx->last_sent_rad[i] - max_delta_rad;
        }

        ctx->last_sent_rad[i] = target_rad;
        msg->name.push_back(name);
        msg->position.push_back(target_rad);
    }

    ctx->is_first_frame = false;
    ctx->follow_pub->publish(*msg);
}

bool ChoreographyNode::ParseJson(const std::string& json_str, HandContext* ctx) {
    try {
        json j = json::parse(json_str);
        ctx->keyframes.clear();
        if (j.contains("keyframes")) {
            for (const auto& j_frame : j["keyframes"]) {
                Keyframe frame;
                frame.time = j_frame["time"].get<double>();
                for (const auto& [finger, f_data] : j_frame["data"].items()) {
                    for (const auto& [joint, j_data] : f_data.items()) {
                        frame.joint_states[joint] = {j_data["value"].get<double>(), 0, 0, {}};
                    }
                }
                ctx->keyframes.push_back(frame);
            }
        }
        std::sort(ctx->keyframes.begin(), ctx->keyframes.end(),
                  [](const Keyframe& a, const Keyframe& b) { return a.time < b.time; });
        ctx->max_t = ctx->keyframes.empty() ? 0.0 : ctx->keyframes.back().time;
        return !ctx->keyframes.empty();
    } catch (...) {
        return false;
    }
}

std::map<std::string, double> ChoreographyNode::GetInterpolatedJointValues(HandContext* ctx,
                                                                           double t) {
    std::map<std::string, double> res;
    if (ctx->keyframes.empty())
        return res;
    const Keyframe *prev = &ctx->keyframes.front(), *next = &ctx->keyframes.back();
    for (const auto& f : ctx->keyframes) {
        if (f.time <= t)
            prev = &f;
        if (f.time >= t) {
            next = &f;
            break;
        }
    }
    double ratio = (prev == next)
                       ? 0.0
                       : std::max(0.0, std::min(1.0, (t - prev->time) / (next->time - prev->time)));
    for (int i = 0; i < 21; ++i) {
        std::string name = IndexToName(i);
        if (prev->joint_states.count(name) && next->joint_states.count(name)) {
            res[name] =
                prev->joint_states.at(name).value +
                (next->joint_states.at(name).value - prev->joint_states.at(name).value) * ratio;
        } else if (prev->joint_states.count(name))
            res[name] = prev->joint_states.at(name).value;
        else if (next->joint_states.count(name))
            res[name] = next->joint_states.at(name).value;
    }
    return res;
}

std::map<std::string, double> ChoreographyNode::ApplyCouplingAndLimits(
    std::map<std::string, double> res) {
    if (res.count("f0_joint0") && res.count("f0_joint1")) {
        if (res["f0_joint0"] < 30.36 && res["f0_joint1"] >= 30.36)
            res["f0_joint1"] = 30.35;
    }
    return res;
}

void ChoreographyNode::ChangeState(HandContext* ctx, PlayState new_state) {
    ctx->state = new_state;
}

std::string ChoreographyNode::IndexToName(int offset) {
    int counts[] = {5, 4, 4, 4, 4};
    int o = offset;
    for (int fi = 0; fi < 5; ++fi) {
        if (o < counts[fi])
            return "f" + std::to_string(fi) + "_joint" + std::to_string(o);
        o -= counts[fi];
    }
    return "";
}

}  // namespace rysen_apexhand