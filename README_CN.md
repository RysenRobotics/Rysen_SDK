<div align="right">
  <strong>🇨🇳 中文</strong> | <a href="README.md">🇺🇸 English</a>
</div>

# 🦾 Rysen ApexHand SDK

本仓库提供 Rysen Apexhand 的库文件（包含 x86_64 架构和 aarch64 架构）以及提供 C++、Python 和 ROS2 三种使用方式的操作示例，演示怎么使用 sdk 的 API。

> 🚨 **出厂默认 IP 地址重要提示 (必看)**：
> * **右手**: `192.168.0.103`
> * **左手**: `192.168.0.102`
>
> *下文所有的示例代码均默认以**右手 (`192.168.0.103`)** 为例。如果您购买的是左手，请在运行测试命令时，将对应的 IP 替换为 `192.168.0.102`。*

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

### 0. 必备前提：安装并运行 Docker

在执行启动脚本之前，**必须确保您的系统中已安装与您处理器架构相匹配的 Docker，并且 Docker 后台服务（Daemon）正在运行。**


### 1. 一键启动开发环境

**🐧 Linux / macOS 用户:**

```bash
# 在仓库根目录下执行
chmod +x ./scripts/start_env.sh
./scripts/start_env.sh

```

**🪟 Windows 用户 (⚠️ 极不推荐，仅供编译/接口体验):**
请前往 Docker Desktop 官网下载并安装符合您处理器架构的Docker版本，并确保右下角托盘图标显示 "Engine running"。由于网络延迟问题，不建议在 Windows 上进行实机硬件联调。

```cmd
./scripts/start_env.bat

```

> **📦 离线/弱网环境部署说明 (必看)**：
> 如果您所在的设备（如机器人内部的工控机）无法连接外网，或拉取镜像极其缓慢：
> 1. 请前往本仓库的 [Releases 页面](https://github.com/RysenRobotics/Rysen_SDK/releases/tag/v1.3.1)下载对应的离线镜像包：
> * x86_64 设备（PC/服务器）请下载：`rysen_sdk_amd64_image.tar`
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
# 默认演示连接右手(103)。如使用左手，请更改为 192.168.0.102
./bin/rysen_example --ip 192.168.0.103
```

> 💡 详细说明请参考 [cpp/README.md](cpp/README.md)。

### 🐍 Python
⚠️ 注意：请务必使用 python3 命令执行脚本，以避免环境或版本冲突导致的语法报错。
```bash
cd python
pip install -e .
# 默认演示连接右手(103)。如使用左手，请更改为 192.168.0.102
python3 example.py --ip 192.168.0.103
```

> 💡 详细说明请参考 [python/README.md](python/README.md)。

### 🐢 ROS2

ROS2 的运行需要同时开启两个终端，一个用于启动底层通信服务器节点，另一个用于发送控制指令的客户端测试节点。
>💡普通终端进入docker容器的命令：
```bash
#如果你没有用docker，而是在宿主机运行的程序，无需执行
docker exec -it rysen_sdk_env bash
```
编译工作空间（仅首次需要）:
```bash
source /opt/ros/humble/setup.bash
cd ros2
colcon build
```

【终端 1】：启动主控制节点 (服务器节点 / 驱动层)
此节点负责与硬件建立实际的网络通信。
```bash
source /opt/ros/humble/setup.bash
cd ros2
source install/setup.bash
# 启动主控制节点 (驱动层)
ros2 launch rysen_apexhand rysen_apexhand.launch.py
```
【终端 2】：启动测试节点 (客户端节点 / 应用层)
保持终端 1 运行，新建一个终端。此节点负责发送运动指令并接收状态反馈。
```bash
# 启动测试节点 (应用层)
source /opt/ros/humble/setup.bash
cd ros2
source install/setup.bash
# 默认演示连接右手(103)。如使用左手，请更改为 192.168.0.102
ros2 run rysen_apexhand rysen_apexhand_ros_example_node_exe --ip 192.168.0.103
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

> **🔧 宿主机网络设置教程**：
> 请按照以下步骤，将您的宿主机有线网卡配置到 192.168.0.x 网段：
> 将机械手通过网线物理连接至主机的有线网口。
> 1. 打开 Ubuntu 的 设置 (Settings) -> 网络 (Network) -> 找到您的 有线连接 (Wired) 并点击齿轮设置图标。
> 
> 
> 2. 切换到 IPv4 标签页。
> 3. 将 IPv4 方法从“自动(DHCP)”更改为 手动 (Manual)。
> 4. 在地址栏中填入以下信息：
> 地址 (Address): 192.168.0.50 （或除连接灵巧手ip外的任意 192.168.0.x 地址）
> 子网掩码 (Netmask): 255.255.255.0
> 5. 点击右上角的“应用 (Apply)”，然后关闭并重新打开网络开关使设置生效。
>
>
                   
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

如果您有任何疑问，可以通过 support@rysenbot.com 联系我们。
