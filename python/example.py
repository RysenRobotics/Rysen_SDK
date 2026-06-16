"""
Example usage of Rysen ApexHand SDK Python interface.
Rysen ApexHand SDK Python 接口示例程序

This script demonstrates comprehensive usage of the SDK, including:
本示例演示了 SDK 的全面使用方法，包括：

  - Connection and initialization / 连接和初始化
  - Finger enable/disable / 手指使能/禁用
  - Motion parameter configuration / 运动参数配置
  - Joint, motor, and hand sensor image callbacks / 关节、电机和手传感器图像回调
  - MoveJ control commands / MoveJ 控制命令
  - Get functions (GetJointStates, GetMotorStates, GetHandSensorImage) / Get 函数
  - MoveJPositionFollow (position following control) / MoveJPositionFollow（位置跟随控制）

This script assumes that:
本脚本假设：

  - `rysen_apexhand_sdk.py` is available on PYTHONPATH
  - Pre-built shared libraries (`librysen_sdk*.so`, `_rysen_sdk*.so`) live in ./lib

This example is based on the C++ example (examples/cpp/rysen_example.cpp).
本示例基于 C++ 示例（examples/cpp/rysen_example.cpp）编写。

Copyright (c) 2024-2026, Rysen Robotics (Shenzhen) Co. Ltd.
All rights reserved.

Use of this source code is governed by a BSD 3-Clause license that can be
found in the LICENSE file.
"""

import os
import signal
import sys
import time
import argparse  # 新增引入 argparse
from typing import List

from rysen_apexhand_sdk import (
    ConnectionType,
    Rysen,
    ErrorCode,
    FingerId,
    HandDir,
    JointControlParam,
    JointId,
    LogLevel,
    MaxFingerTorque,
    MaxJointAccel,
    MaxJointSpeed,
    MoveJPositionFollowParam,
)


def create_joint_control_param(joint_id: JointId, position: float, velocity: float, acceleration: float) -> JointControlParam:
    """
    Helper function to create JointControlParam / 辅助函数：创建关节控制参数
    
    @param joint_id: Joint identifier / 关节标识符
    @param position: Target position in radians / 目标位置（弧度）
    @param velocity: Target velocity in rad/s / 目标速度（rad/s）
    @param acceleration: Target acceleration in rad/s² / 目标加速度（rad/s²）
    @return: JointControlParam object / 关节控制参数对象
    """
    param = JointControlParam()
    param.joint_id = joint_id
    param.position = position
    param.velocity = velocity
    param.acceleration = acceleration
    return param


# Global flag for graceful shutdown / 全局标志位，用于优雅退出
running = True


def signal_handler(sig, frame):
    """
    Handle Ctrl+C signal / Ctrl+C 信号处理函数
    
    @param sig: Signal number / 信号编号
    @param frame: Current stack frame / 当前堆栈帧
    """
    global running
    print(f"\nCaught signal {sig}, stopping...")
    running = False




def main():
    """
    Main example function / 主示例函数
    
    @return: 0 on success, 1 on error / 成功返回 0，失败返回 1
    """
    global running

    # 解析命令行参数
    parser = argparse.ArgumentParser(description="Rysen ApexHand SDK Python Example")
    parser.add_argument("--ip", type=str, default="192.168.0.102", help="Device IP address")
    args = parser.parse_args()
    device_ip = args.ip

    # Register signal handler for Ctrl+C / 注册 Ctrl+C 信号处理函数
    signal.signal(signal.SIGINT, signal_handler)

    # Create SDK instance / 创建 SDK 实例
    print("\n=== 创建 SDK 实例 ===")
    sdk = Rysen()

    # Set logging / 设置日志
    # Logs will be saved to ./log directory / 日志将保存到 ./log 目录
    print("\n=== 设置日志 ===")
    sdk.set_log_path("./log")
    sdk.enable_logging(True)
    print("✅ 日志已启用，路径: ./log")

    # Optional: load robot model from URDF (for planning/kinematics) / 可选：从 URDF 加载机器人模型（用于规划/运动学）
    # 使用“当前脚本文件所在目录”为基准的相对路径，避免依赖运行时工作目录。
    print("\n=== 从 URDF 加载机器人模型 ===")
    script_dir = os.path.dirname(
        os.path.abspath(__file__))  # .../examples/python
    urdf_dir = os.path.abspath(os.path.join(script_dir, "..",
                                            "urdf"))  # .../examples/urdf
    urdf_path = os.path.join(urdf_dir, "apex_hand_right.urdf")
    ret = sdk.set_robot_model_from_urdf(urdf_path)
    if ret != ErrorCode.ERROR_CODE_OK:
        print(f"⚠️ 从 URDF 加载机器人模型失败，错误码: {ret}，继续仅基于关节命令运行示例")
    else:
        print(f"✅ 已从 URDF 加载机器人模型: {urdf_path}")

    # Connect to device / 连接设备
    # Note: Connection is required before receiving data / 注意：需要先连接才能接收数据
    print("\n=== 连接设备 ===")
    print(f"正在连接到设备: {device_ip}")

    ret = sdk.connect(device_ip, ConnectionType.CONNECTION_TYPE_ETHERNET)
    if ret != ErrorCode.ERROR_CODE_OK:
        print(f"❌ 连接失败，错误码: {ret}")
        return 1
    print("✅ 连接成功")

    # Test disable all fingers / 测试禁用所有手指
    print("\n=== 测试 SetAllFingersDisabled 接口 ===")
    ret = sdk.set_all_fingers_disabled()
    if ret != ErrorCode.ERROR_CODE_OK:
        print(f"❌ 禁用所有手指失败，错误码: {ret}")
    else:
        print("✅ 所有手指已禁用")

    # Enable single finger / 使能单个手指
    print("\n=== 测试 SetFingerEnabled 接口 ===")
    ret = sdk.set_finger_enabled([FingerId.FINGER_ID_INDEX])
    if ret != ErrorCode.ERROR_CODE_OK:
        print(f"❌ 使能手指失败，错误码: {ret}")
    else:
        print("✅ 手指已使能")

    # Disable single finger / 禁用单个手指
    print("\n=== 测试 SetFingerDisabled 接口 ===")
    ret = sdk.set_finger_disabled([FingerId.FINGER_ID_INDEX])
    if ret != ErrorCode.ERROR_CODE_OK:
        print(f"❌ 禁用手指失败，错误码: {ret}")
    else:
        print("✅ 手指已禁用")

    # Enable all fingers again for subsequent operations / 再次使能所有手指，准备后续操作
    print("\n=== 再次使能所有手指 ===")
    ret = sdk.set_all_fingers_enabled()
    if ret != ErrorCode.ERROR_CODE_OK:
        print(f"❌ 使能失败，错误码: {ret}")
    else:
        print("✅ 所有手指已使能")

    # ========== Motion Parameter Configuration / 运动参数配置 ==========
    # Configure maximum joint speeds, accelerations, and finger torques
    # 配置最大关节速度、加速度和手指扭矩
    print("\n=== 设置运动参数 ===")

    # Set maximum joint speeds (21 joints) / 设置最大关节速度（21个关节）
    speeds: List[MaxJointSpeed] = []
    for i in range(21):
        speed = MaxJointSpeed()
        speed.joint_id = JointId(i)
        speed.speed = 8.0  # Default speed 8.0 rad/s / 默认速度 8.0 rad/s
        speeds.append(speed)

    # Set maximum joint accelerations (21 joints) / 设置最大关节加速度（21个关节）
    accels: List[MaxJointAccel] = []
    for i in range(21):
        accel = MaxJointAccel()
        accel.joint_id = JointId(i)
        accel.accel = 260.0  # Default acceleration 260.0 rad/s² / 默认加速度 260.0 rad/s²
        accels.append(accel)

    # Set maximum finger torques (5 fingers) / 设置最大手指扭矩（5个手指）
    torques: List[MaxFingerTorque] = []
    for i in range(5):
        torque = MaxFingerTorque()
        torque.finger_id = FingerId(i)
        torque.torque = 50.0  # Default torque 50% / 默认扭矩 50%
        torques.append(torque)

    sdk.set_max_joint_speed(speeds)
    sdk.set_max_joint_accel(accels)
    sdk.set_max_finger_torque(torques)
    print("✅ 运动参数已设置")

    # ========== Register Callbacks / 注册回调函数 ==========

    # Register joint states callback (100Hz) / 注册关节状态回调（100Hz）
    # The callback will be called periodically to receive joint position, velocity, and acceleration data
    # 回调函数将周期性调用，接收关节位置、速度和加速度数据
    print("\n=== 注册关节状态回调 (100Hz) ===")
    program_start = time.time(
    )  # Record program start time for calculating relative timestamps / 记录程序开始时间

    def on_joint_states(states):
        """Joint states callback / 关节状态回调函数"""
        elapsed = time.time() - program_start
        print(
            f"[JointStates] 时间戳: {elapsed*1000:.0f} ms, 关节数量: {len(states.joint_states)}"
        )

    sdk.register_joint_states_callback(on_joint_states, freq_hz=100)
    print("✅ 关节状态回调已注册")

    # Register motor states callback (50Hz) / 注册电机状态回调（50Hz）
    # The callback will be called periodically to receive motor temperature and current data
    # 回调函数将周期性调用，接收电机温度和电流数据
    print("\n=== 注册电机状态回调 (50Hz) ===")

    def on_motor_states(states):
        """Motor states callback / 电机状态回调函数"""
        elapsed = time.time() - program_start
        print(
            f"[MotorStates] 时间戳: {elapsed*1000:.0f} ms, 电机数量: {len(states.motors)}"
        )

    sdk.register_motor_states_callback(on_motor_states, freq_hz=50)
    print("✅ 电机状态回调已注册")

    # Register hand sensor image callback (50Hz) / 注册手传感器图像回调（50Hz）
    # The callback will be called periodically to receive tactile sensor image data
    # Note: Requires tactile sensor version to be available / 注意：需要触觉传感器版本号可用
    # 回调函数将周期性调用，接收触觉传感器图像数据
    print("\n=== 注册手传感器图像回调 (50Hz) ===")

    def on_hand_sensor_image(image):
        """Hand sensor image callback / 手传感器图像回调函数"""
        elapsed = time.time() - program_start
        print(f"[HandSensorImage] 时间戳: {elapsed*1000:.0f} ms")

    ret = sdk.register_hand_sensor_image_callback(on_hand_sensor_image,
                                                  freq_hz=50)
    if ret != ErrorCode.ERROR_CODE_OK:
        print(f"❌ 注册手传感器图像回调失败，错误码: {ret}")
        print("   提示: 如果触觉传感器版本号未获取到，触觉相关接口无法使用")
    else:
        print("✅ 手传感器图像回调已注册")

    # Wait for data to stabilize / 等待数据稳定
    print("\n=== 等待数据稳定（2秒）===")
    time.sleep(2.0)

    # ========== MoveJ Control Test / MoveJ 控制测试 ==========
    # MoveJ is a blocking function that moves joints to target positions and waits until they reach
    # MoveJ 是阻塞函数，将关节移动到目标位置并等待到达
    print("\n=== 测试 MoveJ 控制（所有21个关节） ===")
    movej_commands: List[JointControlParam] = []

    # Thumb (5个关节)
    movej_commands.append(
        create_joint_control_param(JointId.JOINT_ID_THUMB_CMC_ABD, 1.2, 2.0,
                                   8.0))
    movej_commands.append(
        create_joint_control_param(JointId.JOINT_ID_THUMB_CMC_ROT, 0.2, 2.0,
                                   8.0))
    movej_commands.append(
        create_joint_control_param(JointId.JOINT_ID_THUMB_CMC_FLEX, 0.25, 2.0,
                                   8.0))
    movej_commands.append(
        create_joint_control_param(JointId.JOINT_ID_THUMB_MCP_FLEX, 0.4, 2.0,
                                   8.0))
    movej_commands.append(
        create_joint_control_param(JointId.JOINT_ID_THUMB_IP_FLEX, 0.4, 2.0,
                                   8.0))

    # Index (4个关节)
    movej_commands.append(
        create_joint_control_param(JointId.JOINT_ID_INDEX_MCP_ABD, 0.3, 2.0,
                                   8.0))
    movej_commands.append(
        create_joint_control_param(JointId.JOINT_ID_INDEX_MCP_FLEX, 0.8, 2.0,
                                   8.0))
    movej_commands.append(
        create_joint_control_param(JointId.JOINT_ID_INDEX_PIP_FLEX, 0.8, 2.0,
                                   8.0))
    movej_commands.append(
        create_joint_control_param(JointId.JOINT_ID_INDEX_DIP_FLEX, 0.8, 2.0,
                                   8.0))

    # Middle (4个关节)
    movej_commands.append(
        create_joint_control_param(JointId.JOINT_ID_MIDDLE_MCP_ABD, 0.15, 2.0,
                                   8.0))
    movej_commands.append(
        create_joint_control_param(JointId.JOINT_ID_MIDDLE_MCP_FLEX, 0.8, 2.0,
                                   8.0))
    movej_commands.append(
        create_joint_control_param(JointId.JOINT_ID_MIDDLE_PIP_FLEX, 0.8, 2.0,
                                   8.0))
    movej_commands.append(
        create_joint_control_param(JointId.JOINT_ID_MIDDLE_DIP_FLEX, 0.8, 2.0,
                                   8.0))

    # Ring (4个关节)
    movej_commands.append(
        create_joint_control_param(JointId.JOINT_ID_RING_MCP_ABD, -0.15, 2.0,
                                   8.0))
    movej_commands.append(
        create_joint_control_param(JointId.JOINT_ID_RING_MCP_FLEX, 0.8, 2.0,
                                   8.0))
    movej_commands.append(
        create_joint_control_param(JointId.JOINT_ID_RING_PIP_FLEX, 0.8, 2.0,
                                   8.0))
    movej_commands.append(
        create_joint_control_param(JointId.JOINT_ID_RING_DIP_FLEX, 0.8, 2.0,
                                   8.0))

    # Little finger (4 joints)
    movej_commands.append(
        create_joint_control_param(JointId.JOINT_ID_LITTLE_MCP_ABD, -0.3, 2.0,
                                   8.0))
    movej_commands.append(
        create_joint_control_param(JointId.JOINT_ID_LITTLE_MCP_FLEX, 0.8, 2.0,
                                   8.0))
    movej_commands.append(
        create_joint_control_param(JointId.JOINT_ID_LITTLE_PIP_FLEX, 0.8, 2.0,
                                   8.0))
    movej_commands.append(
        create_joint_control_param(JointId.JOINT_ID_LITTLE_DIP_FLEX, 0.8, 2.0,
                                   8.0))

    print(f"发送 MoveJ 命令，控制 {len(movej_commands)} 个关节...")
    print("等待关节到达目标位置（MoveJoint 是阻塞函数，会等待直到到达）...")
    ret = sdk.move_joint(movej_commands)
    if ret != ErrorCode.ERROR_CODE_OK:
        print(f"❌ MoveJ 控制失败，错误码: {ret}")
    else:
        print("✅ MoveJ 完成，关节已到达目标位置")

    # Test again: return to initial position / 再次测试：回到初始位置
    # Move all 21 joints back to 0 position / 将所有21个关节都回到0位置
    print("\n=== 测试 MoveJ 回到初始位置（所有21个关节） ===")
    movej_commands.clear()

    # Thumb (5 joints)
    movej_commands.append(
        create_joint_control_param(JointId.JOINT_ID_THUMB_CMC_ABD, 0.0, 0.5,
                                   2.0))
    movej_commands.append(
        create_joint_control_param(JointId.JOINT_ID_THUMB_CMC_ROT, 0.0, 0.5,
                                   2.0))
    movej_commands.append(
        create_joint_control_param(JointId.JOINT_ID_THUMB_CMC_FLEX, 0.0, 0.5,
                                   2.0))
    movej_commands.append(
        create_joint_control_param(JointId.JOINT_ID_THUMB_MCP_FLEX, 0.0, 0.5,
                                   2.0))
    movej_commands.append(
        create_joint_control_param(JointId.JOINT_ID_THUMB_IP_FLEX, 0.0, 0.5,
                                   2.0))

    # Index (4 joints)
    movej_commands.append(
        create_joint_control_param(JointId.JOINT_ID_INDEX_MCP_ABD, 0.0, 0.5,
                                   2.0))
    movej_commands.append(
        create_joint_control_param(JointId.JOINT_ID_INDEX_MCP_FLEX, 0.0, 0.5,
                                   2.0))
    movej_commands.append(
        create_joint_control_param(JointId.JOINT_ID_INDEX_PIP_FLEX, 0.0, 0.5,
                                   2.0))
    movej_commands.append(
        create_joint_control_param(JointId.JOINT_ID_INDEX_DIP_FLEX, 0.0, 0.5,
                                   2.0))

    # Middle (4 joints)
    movej_commands.append(
        create_joint_control_param(JointId.JOINT_ID_MIDDLE_MCP_ABD, 0.0, 0.5,
                                   2.0))
    movej_commands.append(
        create_joint_control_param(JointId.JOINT_ID_MIDDLE_MCP_FLEX, 0.0, 0.5,
                                   2.0))
    movej_commands.append(
        create_joint_control_param(JointId.JOINT_ID_MIDDLE_PIP_FLEX, 0.0, 0.5,
                                   2.0))
    movej_commands.append(
        create_joint_control_param(JointId.JOINT_ID_MIDDLE_DIP_FLEX, 0.0, 0.5,
                                   2.0))

    # Ring (4 joints)
    movej_commands.append(
        create_joint_control_param(JointId.JOINT_ID_RING_MCP_ABD, 0.0, 0.5,
                                   2.0))
    movej_commands.append(
        create_joint_control_param(JointId.JOINT_ID_RING_MCP_FLEX, 0.0, 0.5,
                                   2.0))
    movej_commands.append(
        create_joint_control_param(JointId.JOINT_ID_RING_PIP_FLEX, 0.0, 0.5,
                                   2.0))
    movej_commands.append(
        create_joint_control_param(JointId.JOINT_ID_RING_DIP_FLEX, 0.0, 0.5,
                                   2.0))

    # Little finger (4 joints)
    movej_commands.append(
        create_joint_control_param(JointId.JOINT_ID_LITTLE_MCP_ABD, 0.0, 0.5,
                                   2.0))
    movej_commands.append(
        create_joint_control_param(JointId.JOINT_ID_LITTLE_MCP_FLEX, 0.0, 0.5,
                                   2.0))
    movej_commands.append(
        create_joint_control_param(JointId.JOINT_ID_LITTLE_PIP_FLEX, 0.0, 0.5,
                                   2.0))
    movej_commands.append(
        create_joint_control_param(JointId.JOINT_ID_LITTLE_DIP_FLEX, 0.0, 0.5,
                                   2.0))

    print("发送 MoveJ 命令，回到初始位置（MoveJoint 是阻塞函数，会等待直到到达）...")
    ret = sdk.move_joint(movej_commands)

    if ret != ErrorCode.ERROR_CODE_OK:
        print(f"❌ MoveJ 控制失败，错误码: {ret}")
    else:
        print("✅ MoveJ 完成，关节已回到初始位置")

    # ========== Get Functions Test / Get 函数测试 ==========
    # Test synchronous data retrieval functions / 测试同步数据获取函数
    print("\n=== 测试 Get 函数 ===")

    # Test GetJointStates / 测试 GetJointStates
    # Synchronously get current joint states / 同步获取当前关节状态
    print("\n--- 测试 GetJointStates ---")
    try:
        joint_states = sdk.get_joint_states()
        if joint_states and joint_states.joint_states:
            print(f"✅ 成功获取关节状态，关节数量: {len(joint_states.joint_states)}")
            print_count = min(5, len(joint_states.joint_states))
            print(f"前 {print_count} 个关节的状态:")
            for i in range(print_count):
                joint = joint_states.joint_states[i]
                print(f"  关节[{joint.joint_id}] "
                      f"位置: {joint.position:.6f} rad, "
                      f"速度: {joint.velocity:.6f} rad/s, "
                      f"加速度: {joint.acceleration:.6f} rad/s²")
        else:
            print("❌ 获取关节状态失败，数据为空")
    except Exception as e:
        print(f"❌ 获取关节状态失败，错误: {e}")

    # Test GetMotorStates / 测试 GetMotorStates
    # Synchronously get current motor states / 同步获取当前电机状态
    print("\n--- 测试 GetMotorStates ---")
    try:
        motor_states = sdk.get_motor_states()
        if motor_states and motor_states.motors:
            print(f"✅ 成功获取电机状态，电机数量: {len(motor_states.motors)}")
            print_count = min(5, len(motor_states.motors))
            print(f"前 {print_count} 个电机的状态:")
            for i in range(print_count):
                motor = motor_states.motors[i]
                print(f"  电机[{motor.motor_id}] "
                      f"温度: {motor.temperature:.1f} °C, "
                      f"电流: {motor.current:.3f} A")
        else:
            print("❌ 获取电机状态失败，数据为空")
    except Exception as e:
        print(f"❌ 获取电机状态失败，错误: {e}")

    # Test GetHandSensorImage / 测试 GetHandSensorImage
    # Synchronously get current tactile sensor image / 同步获取当前触觉传感器图像
    print("\n--- 测试 GetHandSensorImage ---")
    try:
        sensor_image = sdk.get_hand_sensor_image()
        if sensor_image:
            print("✅ 成功获取手传感器图像")
            print(f"  食指 PIP 图像尺寸: {sensor_image.index_image.prox_pad.width}x"
                  f"{sensor_image.index_image.prox_pad.height}")
            print(
                f"  图像数据大小: {len(sensor_image.index_image.prox_pad.gray_image)} 像素"
            )
        else:
            print("❌ 获取手传感器图像失败，数据为空")
    except Exception as e:
        print(f"❌ 获取手传感器图像失败，错误: {e}")

    # ========== MoveJPositionFollow Test / MoveJPositionFollow 测试 ==========
    # MoveJPositionFollow is a non-blocking function that continuously follows target positions
    # It uses smooth interpolation for trajectory planning
    # MoveJPositionFollow 是非阻塞函数，持续跟踪目标位置，使用平滑插值进行轨迹规划
    print("\n=== 测试 MoveJPositionFollow（单个关节往复运动，一系列中间点） ===")
    print("控制食指 MCP_1 关节往复运动，包含多个中间点，时间间隔不同...")

    # Move to starting position first / 先移动到起始位置
    print("\n--- 移动到起始位置 (0.0 rad) ---")
    start_pos: List[JointControlParam] = []
    start_pos.append(
        create_joint_control_param(JointId.JOINT_ID_INDEX_PIP_FLEX, 0.0, 1.0,
                                   2.0))
    ret = sdk.move_joint(start_pos)
    if ret != ErrorCode.ERROR_CODE_OK:
        print(f"❌ 移动到起始位置失败，错误码: {ret}")
    else:
        print("✅ 已到达起始位置")

    # Reciprocating motion cycles (2 cycles) / 往复运动循环（2次）
    cycle_count = 2

    # Define waypoint structure / 定义路径点结构
    class Waypoint:

        def __init__(self, position: float, delay_ms: int):
            self.position = position  # Target position (rad) / 目标位置 (rad)
            self.delay_ms = delay_ms  # Wait time after reaching this point (ms) / 到达该点后的等待时间 (ms)

    # Define path: from 0.0 to 0.8 and back to 0.0, with dense intermediate points
    # 定义路径：从 0.0 到 0.8 再回到 0.0，包含更多密集的中间点
    waypoints = [
        Waypoint(0.0, 100),  # 起始点
        Waypoint(0.1, 50),  # 中间点
        Waypoint(0.2, 50),  # 中间点
        Waypoint(0.3, 50),  # 中间点
        Waypoint(0.4, 50),  # 中间点
        Waypoint(0.5, 50),  # 中间点
        Waypoint(0.6, 50),  # 中间点
        Waypoint(0.7, 50),  # 中间点
        Waypoint(0.8, 100),  # 最大位置
        Waypoint(0.7, 50),  # 返回路径
        Waypoint(0.6, 50),  # 返回路径
        Waypoint(0.5, 50),  # 返回路径
        Waypoint(0.4, 50),  # 返回路径
        Waypoint(0.3, 50),  # 返回路径
        Waypoint(0.2, 50),  # 返回路径
        Waypoint(0.1, 50),  # 返回路径
        Waypoint(0.0, 100)  # 回到起始位置
    ]

    # 定义5个手指对应的主要MCP关节ID（每个手指完成一个完整的往复运动循环）
    finger_joints = [
        JointId.JOINT_ID_THUMB_CMC_ABD,  # 拇指
        JointId.JOINT_ID_INDEX_MCP_FLEX,  # 食指
        JointId.JOINT_ID_MIDDLE_MCP_FLEX,  # 中指
        JointId.JOINT_ID_RING_MCP_FLEX,  # 无名指
        JointId.JOINT_ID_LITTLE_MCP_FLEX  # 小指
    ]
    finger_names = ["拇指", "食指", "中指", "无名指", "小指"]

    # 每个手指完成 cycle_count 次往复运动循环
    for finger_idx in range(len(finger_joints)):
        if not running:
            break
        target_joint = finger_joints[finger_idx]
        finger_name = finger_names[finger_idx]

        print(f"\n=== {finger_name} 开始往复运动 ===")

        for cycle in range(cycle_count):
            if not running:
                break
            print(f"\n--- {finger_name} 往复运动循环 {cycle + 1}/{cycle_count} ---")

            # Go through all waypoints sequentially / 依次经过所有中间点
            for i, waypoint in enumerate(waypoints):
                if not running:
                    break
                print(
                    f"  点 {i + 1}/{len(waypoints)}: 位置={waypoint.position} rad, 等待={waypoint.delay_ms} ms"
                )

                follow_params: List[MoveJPositionFollowParam] = []
                param = MoveJPositionFollowParam()
                param.id = target_joint
                param.position = waypoint.position
                follow_params.append(param)

                ret = sdk.move_j_position_follow(follow_params)
                if ret != ErrorCode.ERROR_CODE_OK:
                    print(f"❌ MoveJPositionFollow 失败，错误码: {ret}")
                    break

                # Wait for the specified time interval (last point doesn't need to wait)
                # 等待指定的时间间隔（最后一个点不需要等待）
                if i < len(waypoints) - 1:
                    time.sleep(waypoint.delay_ms / 1000.0)

            print(f"✅ {finger_name} 往复运动循环 {cycle + 1} 完成")

            # Wait between cycles / 循环之间的等待
            if cycle < cycle_count - 1:
                time.sleep(0.5)

        print(f"✅ {finger_name} 往复运动完成")

        # Wait between fingers / 手指之间的等待
        if finger_idx < len(finger_joints) - 1:
            time.sleep(0.5)

    print("\n✅ MoveJPositionFollow 往复运动测试完成")

    # Keep connection and receive data / 保持连接，接收数据
    print("\n=== 开始接收数据（按 Ctrl+C 停止）===")
    # Uncomment the following to keep receiving data / 取消注释以下代码以持续接收数据
    # while running:
    #     time.sleep(0.1)

    # ========== Cleanup / 清理资源 ==========
    print("\n=== 清理资源 ===")

    # Disable all fingers / 禁用所有手指
    sdk.set_all_fingers_disabled()
    print("✅ 所有手指已禁用")

    # Disconnect (will automatically unregister all callbacks) / 断开连接（会自动取消注册所有回调）
    print("正在断开连接...")
    ret = sdk.disconnect()
    if ret != ErrorCode.ERROR_CODE_OK:
        print(f"❌ 断开连接失败，错误码: {ret}")
    else:
        print("✅ 已断开连接")

    print("\n=== 测试完成 ===")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print("\nInterrupted by user")
        sys.exit(1)
    except Exception as exc:
        print(f"Error: {exc}")
        import traceback

        traceback.print_exc()
        sys.exit(1)
