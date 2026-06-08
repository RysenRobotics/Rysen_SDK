<div align="right">
  <a href="README_CN.md">🇨🇳 中文</a> | <strong>🇺🇸 English</strong>
</div>

# 🦾 Rysen ApexHand SDK

This repository provides library files for Rysen ApexHand (including x86_64 and aarch64 architectures) as well as operational examples using C++, Python, and ROS2. These examples demonstrate how to use the SDK's APIs.

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
├── README_CN.md
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

## 🚀 Quick Start (Recommended)

We highly recommend using our provided intelligent environment startup script. This script automatically detects your system architecture (amd64 / arm64) and prepares a Docker container with all dependencies following an optimal strategy (local cache -> GHCR cloud -> offline Tar package -> source code build).

### 1. One-click Launch of Development Environment

**🐧 Linux / macOS Users:**

```bash
# Execute in the root directory of the repository
chmod +x ./scripts/start_env.sh
./scripts/start_env.sh
```

**🪟 Windows Users:**
Double-click the `./scripts/start_env.bat` file in the root directory directly, or execute it in the command line:

```cmd
./scripts/start_env.bat
```

> **📦 Deployment Instructions for Offline/Low-Network Environments (Must Read)**:
> If the device you are using (such as the industrial computer inside a robot) cannot connect to the external network, or pulling the image is extremely slow:
> 1. Please go to the [Releases page](https://github.com/RysenRobotics/Rysen_SDK/releases/tag/v1.3.1) of this repository to download the corresponding offline image package:
> * For x86_64 devices (PC/Server), download: `rysen_sdk_image.tar`
> * For ARM64 devices (Raspberry Pi/Jetson, etc.), download: `rysen_sdk_arm64_image.tar`
> 
> 2. Place the downloaded `.tar` file in the **root directory** of the repository.
> 3. Run the startup script again, and the script will automatically detect and load the offline package at high speed!

### 2. Get Started

After successful startup, the script will automatically guide you to the `/workspace` directory of the `rysen_sdk_env` container. Inside the container, you can directly run any C++, Python, or ROS2 examples, and all dependencies are ready.

---

## 💻 Local Bare-Metal Development Guide

If you do not want to use Docker and hope to develop natively directly on the host system, please configure your local environment according to the following steps.

### 📌 System and Dependency Requirements

* **Operating System**: Linux (Ubuntu 22.04 is highly recommended)
* **Compiler**: GCC 9+ / Clang 10+ (supports C++17), CMake 3.10+
* **Environment Requirements**: Python 3.10, ROS2 Humble (if you need to use the corresponding module)

**Quick Installation of Dependencies (Ubuntu Only)**:

```bash
# Run the one-click installation script in the root directory
chmod +x install_rysen_deps.sh
./install_rysen_deps.sh
```

*(Note: This script will automatically install required libraries such as Boost, libserialport, spdlog, fmt, yaml-cpp.)*

### 🛠️ Run C++ Examples

```bash
cd cpp
mkdir build && cd build
cmake ..
make
./bin/rysen_example
```

> 💡 For detailed instructions, please refer to [cpp/README.md](cpp/README.md).

### 🐍 Python

```bash
cd python
pip install -e .
python example.py
```

> 💡 For detailed instructions, please refer to [python/README.md](python/README.md).

### 🐢 ROS2

```bash
source /opt/ros/humble/setup.bash
cd ros2
colcon build
source install/setup.bash

# Launch the main control node (driver layer)
ros2 launch rysen_apexhand rysen_apexhand.launch.py
```

```bash
# Launch the test node (application layer)
source /opt/ros/humble/setup.bash
cd ros2
source install/setup.bash
ros2 run rysen_apexhand rysen_apexhand_ros_example_node_exe
```

**Features**:

* Publishes joint states and subscribes to follow control using standard ROS2 message type (`sensor_msgs/JointState`)
* Provides position control service (`MoveJoint`) and finger enable service (`SetFingerEnabled`)
* Publishes motor status and tactile sensor data

> 📖 **For detailed instructions, please refer to**:
> * [ros2/rysen_apexhand_msgs/README.md](ros2/rysen_apexhand_msgs/README.md) - Message and service definitions
> * [ros2/rysen_apexhand/README.md](ros2/rysen_apexhand/README.md) - ROS2 node usage instructions

---

## 🔌 Connection Configuration

> ⚠️ All example programs use **Ethernet** connection by default (ensure the local address and the device address are in the same network segment: `192.168.0.xxx`).

> **Default IP Address**: `192.168.0.102` (can be modified in the code, specific locations are in `cpp/rysen_example.cpp`,
`python/example.py`,
`ros2/rysen_apexhand/src/rysen_ros_example_node.cpp`)

**Notes**:
1. Ensure the robot and the host are in the same network.
2. The firewall allows communication on relevant ports.
3. The robot's IP address is configured correctly.
4. Cloning this repository on Windows may cause dynamic library links to become text files (resulting in library file corruption). The solution is as follows:

    ```bash
    cd /workspace/rysen_sdk/lib/x86_64
    # Delete the fake links damaged by Windows
    rm librysen_sdk.so librysen_sdk.so.1

    # Re-establish native Linux soft links
    ln -s librysen_sdk.so.1.x.x librysen_sdk.so.1
    ln -s librysen_sdk.so.1 librysen_sdk.so
    ```

---

## 📦 SDK Library File Description

This project provides a unified `rysen_sdk/` directory in the root directory, and all example programs dynamically link to the library files here:

* `rysen_sdk/include/` - C++ header files of the SDK
* `rysen_sdk/lib/x86_64/` - Dynamic link libraries suitable for ordinary PC/server platforms
* `rysen_sdk/lib/aarch64/` - Dynamic link libraries suitable for ARM edge computing platforms such as Raspberry Pi and NVIDIA Jetson

**🧠 Intelligent Architecture Routing**:
When building C++ or ROS 2 projects, the CMake script will automatically identify the physical architecture of the current system (`uname -m`) and intelligently and seamlessly link to the `librysen_sdk.so` in the directory corresponding to the platform. Developers do not need to modify the path manually.

---

## 📚 More Information
* **C++ API Documentation**: Refer to comments in [cpp/README.md](cpp/README.md) and `cpp/rysen_example.cpp`
* **Python API Documentation**: Refer to [python/README.md](python/README.md)
* **ROS2 API Documentation**: Refer to [ros2/rysen_apexhand/README.md](ros2/rysen_apexhand/README.md)
* **Docker Environment**: Refer to [docker/README.md](docker/README.md)

## 📬 Contact us
For any questions, please contact support@rysen.com