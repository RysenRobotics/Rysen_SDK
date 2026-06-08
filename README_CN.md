<div align="right">
  <strong>🇨🇳 中文</strong> | <a href="README.md">🇺🇸 English</a>
</div>

# 🦾 Rysen ApexHand SDK

本仓库提供 Rysen Apexhand 的库文件（包含 x86_64 架构和 aarch64 架构）以及提供 C++、Python 和 ROS2 三种使用方式的操作示例，演示怎么使用 sdk 的 API。

## 📁 仓库结构

```
.
├── cpp
│   ├── CMakeLists.txt
│   ├── README.md
│   └── rysen_example.cpp
├── docker
│   ├── docker-compose.arm64.yml
│   ├── docker-compose.yml
│   ├── Dockerfile.rysen_sdk
│   ├── Dockerfile.rysen_sdk.arm64
│   └── README.md
├── install_rysen_deps.sh
├── LICENSE
├── python
│   ├── example.py
│   ├── __init__.py
│   ├── MANIFEST.in
│   ├── README.md
│   ├── rysen_apexhand_sdk.py
│   └── setup.py
├── README.md
├── ros2
│   ├── rysen_apexhand
│   │   ├── CMakeLists.txt
│   │   ├── include
│   │   ├── launch
│   │   ├── package.xml
│   │   ├── README.md
│   │   ├── src
│   │   └── thirdparty
│   └── rysen_apexhand_msgs
│       ├── CMakeLists.txt
│       ├── msg
│       ├── package.xml
│       ├── README.md
│       └── srv
├── rysen_sdk
│   ├── include
│   │   ├── rysen_apexhand_data.hpp
│   │   └── rysen_apexhand_sdk.hpp
│   └── lib
│       ├── aarch64
│       └── x86_64
├── urdf
│   ├── apex_hand_left.urdf
│   └── apex_hand_right.urdf
└── VERSION

```

---

## 🚀 快速开始 (推荐)

强烈推荐使用我们提供的智能环境启动脚本。该脚本会自动侦测您的系统架构（amd64 / arm64），并按最优策略（本地缓存 -> GHCR 云端 -> 离线 Tar 包 -> 源码构建）为您准备好包含所有依赖的 Docker 容器。

### 1. 一键启动开发环境

**🐧 Linux / macOS 用户:**

```bash
# 在仓库根目录下执行
chmod +x ./scripts/start_env.sh
./scripts/start_env.sh

```

**🪟 Windows 用户:**
直接双击根目录下的 `./scripts/start_env.bat` 文件，或在命令行中执行：

```cmd
./scripts/start_env.bat

```

> **📦 离线/弱网环境部署说明 (必看)**：
> 如果您所在的设备（如机器人内部的工控机）无法连接外网，或拉取镜像极其缓慢：
> 1. 请前往本仓库的 [Releases 页面](https://github.com/RysenRobotics/Rysen_SDK/releases/tag/v1.3.1)下载对应的离线镜像包：
> * x86_64 设备（PC/服务器）请下载：`rysen_sdk_image.tar`
> * ARM64 设备（树莓派/Jetson等）请下载：`rysen_sdk_arm64_image.tar`
> 
> 
> 2. 将下载好的 `.tar` 文件放置在仓库的**根目录**下。
> 3. 再次运行启动脚本，脚本会自动侦测并极速加载离线包！
> 
> 

### 2. 开始使用

启动成功后，脚本会自动引导您进入 `rysen_sdk_env` 容器的 `/workspace` 目录。在容器内，您可以直接运行任何 C++、Python 或 ROS2 示例，所有依赖均已就绪。

---

## 💻 本地裸机开发指南

如果您不想使用 Docker，希望直接在宿主机系统上进行原生开发，请按照以下步骤配置您的本地环境。

### 📌 系统与依赖要求

* **操作系统**: Linux (强烈推荐 Ubuntu 22.04)
* **编译器**: GCC 9+ / Clang 10+ (支持 C++17), CMake 3.10+
* **环境要求**: Python 3.10, ROS2 Humble (如需使用对应模块)

**快速安装依赖（仅限 Ubuntu）**:

```bash
# 在根目录下运行一键安装脚本
chmod +x install_rysen_deps.sh
./install_rysen_deps.sh

```

*(注：该脚本会自动安装 Boost, libserialport, spdlog, fmt, yaml-cpp 等必需库。)*

### 🛠️ 运行 C++ 示例

```bash
cd cpp
mkdir build && cd build
cmake ..
make
./bin/rysen_example
```

> 💡 详细说明请参考 [cpp/README.md](cpp/README.md)。

### 🐍 Python

```bash
cd python
pip install -e .
python example.py
```

> 💡 详细说明请参考 [python/README.md](python/README.md)。

### 🐢 ROS2

```bash
cd ros2
colcon build
source install/setup.bash

# 启动主控制节点 (驱动层)
ros2 launch rysen_apexhand rysen_apexhand.launch.py
```

```bash
# 启动测试节点 (应用层)
cd ros2
source install/setup.bash
ros2 run rysen_apexhand rysen_apexhand_ros_example_node_exe
```

**特性**:

* 使用标准 ROS2 消息类型（`sensor_msgs/JointState`）发布关节状态和订阅跟随控制
* 提供位置控制服务（`MoveJoint`）和手指使能服务（`SetFingerEnabled`）
* 发布电机状态和触觉传感器数据

> 📖 **详细说明请参考**：
> * [ros2/rysen_apexhand_msgs/README.md](ros2/rysen_apexhand_msgs/README.md) - 消息和服务定义
> * [ros2/rysen_apexhand/README.md](ros2/rysen_apexhand/README.md) - ROS2 节点使用说明
> 
> 

---

## 🔌 连接配置

> ⚠️ 所有示例程序默认使用 **Ethernet** 连接方式（保证本机地址与设备地址在同一个网段：`192.168.0.xxx`）。

> **默认 IP 地址**: `192.168.0.102`（可在代码中修改，具体位置在 `cpp/rysen_example.cpp`，
`python/example.py`，
`ros2/rysen_apexhand/src/rysen_ros_example_node.cpp`）

**注意事项**:
1. 确保机器人和主机在同一网络中。
2. 防火墙允许相关端口通信。
3. 机器人 IP 地址配置正确。
4. Windows 克隆本仓库可能出现动态库链接变为文本文件（导致库文件破损问题），解决方案如下：

    ```bash
    cd /workspace/rysen_sdk/lib/x86_64
    # 删除被 Windows 破坏的假链接
    rm librysen_sdk.so librysen_sdk.so.1

    # 重新建立原生的 Linux 软链接
    ln -s librysen_sdk.so.1.x.x librysen_sdk.so.1
    ln -s librysen_sdk.so.1 librysen_sdk.so
    ```

---

## 📦 SDK 库文件说明

本项目在根目录下提供了一个统一的 `rysen_sdk/` 目录，所有的示例程序均动态链接此处的库文件：

* `rysen_sdk/include/` - SDK 的 C++ 头文件
* `rysen_sdk/lib/x86_64/` - 适用于普通 PC/服务器平台的动态链接库
* `rysen_sdk/lib/aarch64/` - 适用于树莓派、NVIDIA Jetson 等 ARM 边缘计算平台的动态链接库

**🧠 智能架构路由**:
在构建 C++ 或 ROS 2 项目时，CMake 脚本会自动识别当前系统的物理架构（`uname -m`），并智能且无缝地链接对应平台目录下的 `librysen_sdk.so`。开发者无需手动修改路径。

---

## 📚 更多信息

* **C++ API 文档**: 参考 [cpp/README.md](cpp/README.md) 和 `cpp/rysen_example.cpp` 中的注释
* **Python API 文档**: 参考 [python/README.md](python/README.md)
* **ROS2 API 文档**: 参考 [ros2/rysen_apexhand/README.md](ros2/rysen_apexhand/README.md)
* **Docker 环境**: 参考 [docker/README.md](docker/README.md)

---

## 📬 联系我们

如果您有任何疑问，可以通过 support@rysen.com 联系我们。
