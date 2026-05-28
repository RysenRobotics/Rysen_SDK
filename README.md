<div align="right">
  <a href="README_CN.md">🇨🇳 中文</a> | <strong>🇺🇸 English</strong>
</div>

# 🦾 Rysen ApexHand SDK

This repository provides library files for Rysen ApexHand (supporting x86_64 and aarch64 architectures) along with operational examples for C++, Python, and ROS2. These examples demonstrate how to use the SDK's APIs.

## 📁 Repository Structure

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

## 🚀 Quick Start

### 🛠️ C++

```bash
cd cpp
mkdir build && cd build
cmake ..
make
./bin/rysen_example
```
> 💡 For detailed instructions, refer to [cpp/README.md](cpp/README.md).

### 🐍 Python
```bash
cd python
pip install -e .
python example.py
```
> 💡 For detailed instructions, refer to [python/README.md](python/README.md).

### 🐢 ROS2

```bash
cd ros2
colcon build
source install/setup.bash

# Launch the main control node (driver layer)
ros2 launch rysen_apexhand rysen_apexhand.launch.py
```

```bash
# Launch the test node (application layer)
cd ros2
source install/setup.bash
ros2 run rysen_apexhand rysen_apexhand_ros_example_node_exe
```

**Features**:

* Publishes joint states and subscribes to follow control using standard ROS2 message type (`sensor_msgs/JointState`)
* Provides position control service (`MoveJoint`) and finger enable service (`SetFingerEnabled`)
* Publishes motor status and tactile sensor data

> 📖 **For detailed instructions, refer to**:
> * [ros2/rysen_apexhand_msgs/README.md](ros2/rysen_apexhand_msgs/README.md) - Message and service definitions
> * [ros2/rysen_apexhand/README.md](ros2/rysen_apexhand/README.md) - ROS2 node usage instructions
>
>

---

### 🐳 Docker Environment
```bash
# Linux (x86_64)
cd docker
docker compose -f docker-compose.yml up -d
docker exec -it rysen_sdk_env /bin/bash
```

```bash
# Linux (aarch64)
cd docker
docker compose -f docker-compose.arm64.yml up -d
docker exec -it rysen_sdk_arm64_env /bin/bash
```

> 💡 For detailed instructions, refer to [docker/README.md](docker/README.md).

---

## 💻 System Requirements

### 📌 Basic Requirements
* **Operating System**: Linux (Ubuntu 22.04 recommended)
* **Dependent Libraries**: 
  * Boost (libboost-all-dev)
  * libserialport (libserialport-dev)
  * spdlog (libspdlog-dev)
  * fmt (libfmt-dev)
  * yaml-cpp (libyaml-cpp-dev)

### 🔧 Dependency Installation

**Quick Installation (Recommended)**:

```bash
# In the root directory
chmod +x install_rysen_deps.sh
./install_rysen_deps.sh
```

> ℹ️ This script automatically installs all required dependent libraries. If some dependencies are already installed, the script will skip them.

### 🧩 Additional Requirements
* **C++ Examples**: GCC 9+ or Clang 10+ (supports C++17), CMake 3.10+
* **Python Examples**: Python 3.10
* **ROS2 Examples**: ROS2 Humble

---

## 🔌 Connection Configuration
> ⚠️ All example programs use **Ethernet** connection by default: (Ensure the local IP address is in the same network segment as the device address: `192.168.0.xxx`)

> **Default IP Address**: `192.168.0.102` (Can be modified in the code. Location: `cpp/rysen_example.cpp`, `python/example.py`, `ros2/rysen_apexhand/src/rysen_ros_example_node.cpp`)

**Notes**:
1. Ensure the robot and host are in the same network
2. The firewall allows communication on relevant ports
3. The robot's IP address is configured correctly
4. Cloning this repository on Windows may cause dynamic library links to become text files (resulting in corrupted library files). Solution:
    ```bash
    cd /workspace/rysen_sdk/lib/x86_64
    # Delete fake links damaged by Windows
    rm librysen_sdk.so librysen_sdk.so.1

    # Recreate native Linux soft links
    ln -s librysen_sdk.so.1.x.x librysen_sdk.so.1
    ln -s librysen_sdk.so.1 librysen_sdk.so
    ```

---

## 📦 SDK Library File Description
This project provides a unified `rysen_sdk/` directory in the root directory, and all example programs dynamically link to the library files here.

* `rysen_sdk/include/` - C++ header files of the SDK
* `rysen_sdk/lib/x86_64/` - Dynamic link libraries for general PC/server platforms
* `rysen_sdk/lib/aarch64/` - Dynamic link libraries for ARM edge computing platforms such as Raspberry Pi and NVIDIA Jetson

**🧠 Intelligent Architecture Routing**:
When building C++ or ROS 2 projects, the CMake script automatically identifies the physical architecture of the current system (`uname -m`) and intelligently and seamlessly links the `librysen_sdk.so` in the directory corresponding to the platform. Developers do not need to modify the path manually.

---

## 📚 More Information
* **C++ API Documentation**: Refer to comments in [cpp/README.md](cpp/README.md) and `cpp/rysen_example.cpp`
* **Python API Documentation**: Refer to [python/README.md](python/README.md)
* **ROS2 API Documentation**: Refer to [ros2/rysen_apexhand/README.md](ros2/rysen_apexhand/README.md)
* **Docker Environment**: Refer to [docker/README.md](docker/README.md)

## 📬 Contact us
For any questions, please contact support@rysen.com