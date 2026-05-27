<div align="right">
  <strong>🇨🇳 中文</strong> | <a href="README.md">🇺🇸 English</a>
</div>

# Rysen ApexHand SDK

本仓库提供Rysen Apexhand的库文件（包含x86_64架构和aarch64架构）以及提供C++、Python 和 ROS2 三种使用方式的操作示例，演示怎么使用sdk的API

## 仓库结构

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

## 快速开始

### C++

```bash
cd cpp
mkdir build && cd build
cmake ..
make
./bin/rysen_example
```

详细说明请参考 [cpp/README.md](cpp/README.md)。

### Python

```bash
cd python
pip install -e .
python example.py
```

详细说明请参考 [python/README.md](python/README.md)。

### ROS2

```bash
cd ros2
colcon build
source install/setup.bash

启动主控制节点 (驱动层)
ros2 launch rysen_apexhand rysen_apexhand.launch.py
```
```bash
启动测试节点 (应用层)
cd ros2
source install/setup.bash
ros2 run rysen_apexhand rysen_apexhand_ros_example_node_exe
```

**特性**:
- 使用标准 ROS2 消息类型（`sensor_msgs/JointState`）发布关节状态和订阅跟随控制
- 提供位置控制服务（MoveJoint）和手指使能服务（SetFingerEnabled）
- 发布电机状态和触觉传感器数据

详细说明请参考：
- [ros2/rysen_apexhand_msgs/README.md](ros2/rysen_apexhand_msgs/README.md) - 消息和服务定义
- [ros2/rysen_apexhand/README.md](ros2/rysen_apexhand/README.md) - ROS2 节点使用说明

### Docker 环境

```bash
#linux(x86_64)
cd docker
docker compose -f docker-compose.yml up -d
docker exec -it rysen_sdk_env /bin/bash
```
```bash
#linux(aarch64)
cd docker
docker compose -f docker-compose.arm64.yml up -d
docker exec -it rysen_sdk_arm64_env /bin/bash
```

详细说明请参考 [docker/README.md](docker/README.md)。

## 系统要求

### 基础要求

- **操作系统**: Linux (Ubuntu 22.04 推荐)
- **依赖库**: 
  - Boost (libboost-all-dev)
  - libserialport (libserialport-dev)
  - spdlog (libspdlog-dev)
  - fmt (libfmt-dev)
  - yaml-cpp (libyaml-cpp-dev)

### 依赖安装

**快速安装（推荐）**:
```bash
# 在 examples 目录下
chmod +x install_rysen_deps.sh
./install_rysen_deps.sh
```

该脚本会自动安装所有必需的依赖库。如果已安装部分依赖，脚本会跳过已安装的包。

### 其他要求

- **C++ 示例**: GCC 9+ 或 Clang 10+ (支持 C++17), CMake 3.10+
- **Python 示例**: Python 3.10
- **ROS2 示例**: ROS2 Humble

## 连接配置

所有示例程序默认使用 Ethernet 连接方式：(保证本机地址与设备地址在同一个网段：192.168.0.xxx)

- **默认 IP 地址**: 192.168.0.102（可在代码中修改，具体位置在cpp/rysen_example.cpp，python/example.py，ros2/rysen_apexhand/src/rysen_ros_example_node.cpp）

**注意事项**:
1. 确保机器人和主机在同一网络中
2. 防火墙允许相关端口通信
3. 机器人 IP 地址配置正确
4. windows克隆本仓库可能出现动态库链接变为文本文件（导致库文件破损问题）,解决方案：
    ```bash
      cd /workspace/rysen_sdk/lib/x86_64
      # 删除被 Windows 破坏的假链接
      rm librysen_sdk.so librysen_sdk.so.1

      # 重新建立原生的 Linux 软链接
      ln -s librysen_sdk.so.1.x.x librysen_sdk.so.1
      ln -s librysen_sdk.so.1 librysen_sdk.so
    ```

## SDK 库文件说明

本项目在根目录下提供了一个统一的 rysen_sdk/ 目录，所有的示例程序均动态链接此处的库文件

- rysen_sdk/include/ - SDK 的 C++ 头文件

- rysen_sdk/lib/x86_64/ - 适用于普通 PC/服务器平台的动态链接库

- rysen_sdk/lib/aarch64/ - 适用于树莓派、NVIDIA Jetson 等 ARM 边缘计算平台的动态链接库

智能架构路由：
在构建 C++ 或 ROS 2 项目时，CMake 脚本会自动识别当前系统的物理架构（uname -m），并智能且无缝地链接对应平台目录下的 librysen_sdk.so。开发者无需手动修改路径。

## 更多信息

- **C++ API 文档**: 参考 [cpp/README.md](cpp/README.md) 和 `cpp/rysen_example.cpp` 中的注释
- **Python API 文档**: 参考 [python/README.md](python/README.md)
- **ROS2 API 文档**: 参考 [ros2/rysen_apexhand/README.md](ros2/rysen_apexhand/README.md)
- **Docker 环境**: 参考 [docker/README.md](docker/README.md)

## 联系我们
如果您有任何疑问，可以通过 support@rysen.com 联系我们
