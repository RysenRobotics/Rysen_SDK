# Rysen ApexHand SDK 示例程序

本目录包含 Rysen ApexHand SDK 的完整示例程序，提供 C++、Python 和 ROS2 三种使用方式。

## 目录结构

```
examples/
├── README.md                    # 本说明文件
├── install_rysen_deps.sh   # 依赖安装脚本
├── cpp/                         # C++ 示例程序
│   ├── README.md               # C++ 使用说明
│   ├── rysen_example.cpp   # C++ 示例代码
│   ├── CMakeLists.txt          # CMake 构建配置
│   └── rysen_sdk/           # SDK 文件（构建时自动复制）
│       ├── include/            # SDK 头文件
│       └── lib/                # SDK 库文件
├── python/                      # Python 示例程序
│   ├── README.md               # Python 使用说明
│   ├── example.py              # Python 示例代码
│   ├── rysen_apexhand_sdk.py  # Python 封装
│   └── setup.py                # Python 安装配置
├── ros2/                        # ROS2 示例程序
│   ├── rysen_apexhand_msgs/  # ROS2 消息和服务定义
│   │   └── README.md
│   └── rysen_apexhand/     # ROS2 节点实现
│       └── README.md
└── docker/                      # Docker 开发环境
    ├── README.md               # Docker 使用说明
    ├── Dockerfile.rysen_sdk
    └── docker-compose.yml
```

## 快速开始

### C++ 示例

```bash
cd cpp
mkdir build && cd build
cmake ..
make
./rysen_example
```

详细说明请参考 [cpp/README.md](cpp/README.md)。

### Python 示例

```bash
cd python
pip install -e .
python example.py
```

详细说明请参考 [python/README.md](python/README.md)。

### ROS2 示例

```bash
cd ros2
colcon build
source install/setup.bash
ros2 launch rysen_apexhand rysen_apexhand.launch.py
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
cd docker
docker-compose up -d
docker-compose exec rysen_sdk bash
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

### 安装依赖

**快速安装（推荐）**:
```bash
# 在 examples 目录下
chmod +x install_rysen_deps.sh
./install_rysen_deps.sh
```

该脚本会自动安装所有必需的依赖库。如果已安装部分依赖，脚本会跳过已安装的包。

### 各示例特定要求

- **C++ 示例**: GCC 9+ 或 Clang 10+ (支持 C++17), CMake 3.10+
- **Python 示例**: Python 3.10
- **ROS2 示例**: ROS2 Humble

## 连接配置

所有示例程序默认使用 Ethernet 连接方式：(保证本机地址与设备地址在同一个网段：192.168.0.xxx)

- **默认 IP 地址**: 192.168.0.102（可在代码中修改）

**注意事项**:
1. 确保机器人和主机在同一网络中
2. 防火墙允许相关端口通信
3. 机器人 IP 地址配置正确

## SDK 文件说明

构建 SDK 后，以下文件会自动复制到示例目录（这些文件不会被 git 跟踪）：

- `cpp/rysen_sdk/` - C++ 示例所需的 SDK 头文件和库文件
- `ros2/rysen_apexhand/rysen_sdk/` - ROS2 包所需的 SDK 文件

这些文件在构建时自动生成，确保示例程序可以独立运行。

## 更多信息

- **C++ API 文档**: 参考 [cpp/README.md](cpp/README.md) 和 `cpp/rysen_example.cpp` 中的注释
- **Python API 文档**: 参考 [python/README.md](python/README.md)
- **ROS2 API 文档**: 参考 [ros2/rysen_apexhand/README.md](ros2/rysen_apexhand/README.md)
- **Docker 环境**: 参考 [docker/README.md](docker/README.md)
