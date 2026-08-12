<div align="right">
  <a href="README_CN.md">🇨🇳 中文</a> | <strong>🇺🇸 English</strong>
</div>

# 🦾 Rysen ApexHand SDK

This repository provides library files for Rysen ApexHand (including x86_64 and aarch64 architectures) as well as operation examples using C++, Python, and ROS2, demonstrating how to use the SDK's APIs.

> 🚨 **Important Note on Default IP Addresses (Must Read)**：
> * **Right Hand**: `192.168.0.103`
> * **Left Hand**: `192.168.0.102`
>
> *All example codes below use the **Right Hand (`192.168.0.103`)** by default. If you purchased the Left Hand, replace the corresponding IP with `192.168.0.102` when running test commands.*

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
├── docs
│   ├── apexhand_1.0_tactile_array.md
│   └── apexhand_1.0_tactile_array_en.md
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

## ⚙️ Prerequisites

Before getting started, ensure your host system meets the following hardware and software requirements. Rysen Explorer is deployed via Docker — there is **no need** to install Python, a C++ toolchain, or ROS2 locally; runtime dependencies are included in the pre-built image.

### Hardware

| Item | Requirement |
|------|-------------|
| **Ethernet** | At least one available network port on the host; gigabit recommended, same subnet as the dexterous hand |
| **Connection** | Direct Ethernet cable or low-latency switch; avoid high-latency Wi‑Fi |
| **Device Ports** | Port 1 **5856**, Port 2 **5857** (TCP) |

### Recommended Host Configuration

| Item | Requirement |
|------|-------------|
| **CPU** | x86_64 |
| **Memory** | ≥ 8 GB |
| **Storage** | ≥ 16 GB free space |

### Operating System & Dependencies

| Item | Requirement |
|------|-------------|
| **Operating System** | Linux; **Ubuntu 22.04** recommended |
| **Architecture** | x86_64 (CPU) |
| **Docker Engine** | ≥ **20.10** |
| **Docker Compose** | ≥ **2.0** (Compose V2 plugin; use the `docker compose` command) |

### Network Connectivity

Before connecting to the dexterous hand, confirm:

- The host can reach the hand's IP address (`ping` succeeds)
- Firewall allows TCP **5856** and **5857**
- Control/data port RTT: ≤ **5 ms** on a standard kernel, ≤ **1 ms** with a real-time kernel (PREEMPT_RT)

> For detailed subnet and host IP configuration steps, see **[Network Connection Configuration](#-connection-configuration)** below.

---

## 🚀 Quick Start (Recommended)

We highly recommend using the intelligent environment startup script we provide. This script automatically detects your system architecture (amd64 / arm64) and prepares a Docker container with all dependencies for you according to the optimal strategy (local cache -> GHCR cloud -> offline Tar package -> source code build).

### 0. Prerequisite: Install and Run Docker

Before executing the startup script, **ensure that Docker matching your processor architecture is installed in your system and the Docker background service (Daemon) is running.**

### 1. Offline/Low-Bandwidth Deployment (Skip If Your Network Is Stable)

If your device (such as an industrial computer inside a robot) cannot access the internet, or pulling the image is extremely slow, prepare the appropriate offline image package first:

1. Go to this repository's [Releases page](https://github.com/RysenRobotics/Rysen_SDK/releases/tag/v1.3.1) and download the offline image package for your device architecture:
   * x86_64 devices (PC/server): `rysen_sdk_amd64_image.tar`
   * ARM64 devices (Raspberry Pi/Jetson, etc.): `rysen_sdk_arm64_image.tar`
2. Place the downloaded `.tar` file in the **root directory** of the repository.
3. Continue with the startup steps below. The script will automatically detect and quickly load the offline image package.

### 2. One-click Launch of Development Environment

**🐧 Linux / macOS Users:**

```bash
# Execute in the root directory of the repository
chmod +x ./scripts/start_env.sh
./scripts/start_env.sh
```

**🪟 Windows Users (⚠️ Not Recommended, for Compilation/Interface Experience Only):**
Please go to the Docker Desktop official website to download and install the Docker version suitable for your processor architecture, and ensure that the tray icon in the lower right corner displays "Engine running". Due to network latency issues, it is not recommended to perform physical hardware debugging on Windows.

```cmd
./scripts/start_env.bat
```

### 3. Get Started

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
# Defaults to connecting to the right hand (103). For left hand, change to 192.168.0.102
./bin/rysen_example --ip 192.168.0.103
```

> 💡 For detailed instructions, please refer to [cpp/README.md](cpp/README.md).

### 🐍 Python
⚠️ Note: Be sure to use the python3 command to execute the script to avoid syntax errors caused by environment or version conflicts.
```bash
cd python
pip install -e .
# Defaults to connecting to the right hand (103). For left hand, change to 192.168.0.102
python3 example.py --ip 192.168.0.103
```

> 💡 For detailed instructions, please refer to [python/README.md](python/README.md).

### 🐢 ROS2

Running ROS2 requires opening two terminals at the same time: one for starting the underlying communication server node, and the other for the client test node that sends control commands.
>💡 Command to enter the docker container from a normal terminal:
```bash
# If you are not using docker and running the program on the host machine, no need to execute this
docker exec -it rysen_sdk_env bash
```
Compile the workspace (required only for the first time):
```bash
source /opt/ros/humble/setup.bash
cd ros2
colcon build
```

【Terminal 1】: Start the main control node (server node / driver layer)
This node is responsible for establishing actual network communication with the hardware.
```bash
source /opt/ros/humble/setup.bash
cd ros2
source install/setup.bash
# Start the main control node (driver layer)
ros2 launch rysen_apexhand rysen_apexhand.launch.py
```
【Terminal 2】: Start the test node (client node / application layer)
Keep Terminal 1 running and open a new terminal. This node is responsible for sending motion commands and receiving status feedback.
```bash
# Start the test node (application layer)
source /opt/ros/humble/setup.bash
cd ros2
source install/setup.bash
# Defaults to connecting to the right hand (103). For left hand, change to 192.168.0.102
ros2 run rysen_apexhand rysen_apexhand_ros_example_node_exe --ip 192.168.0.103
```

**Features**:

* Publish joint states and subscribe to follow control using standard ROS2 message types (`sensor_msgs/JointState`)
* Provide position control service (`MoveJoint`) and finger enable service (`SetFingerEnabled`)
* Publish motor status and tactile sensor data

> 📖 **For detailed instructions, please refer to**:
> * [ros2/rysen_apexhand_msgs/README.md](ros2/rysen_apexhand_msgs/README.md) - Message and service definitions
> * [ros2/rysen_apexhand/README.md](ros2/rysen_apexhand/README.md) - ROS2 node usage instructions

---

## 🔌 Connection Configuration

> ⚠️ All example programs use **Ethernet** connection by default (ensure the local address and the device address are in the same network segment: `192.168.0.xxx`).

> **🔧 Host Network Settings Tutorial**:
> Please follow the steps below to configure your host's wired network card to the 192.168.0.x network segment:
> Physically connect the robotic hand to the host's wired network port via an Ethernet cable.
> 1. Open Ubuntu's Settings -> Network -> Find your Wired connection and click the gear settings icon.
> 
> 2. Switch to the IPv4 tab.
> 3. Change the IPv4 method from "Automatic (DHCP)" to Manual.
> 4. Fill in the following information in the address bar:
> Address: 192.168.0.50 (or any 192.168.0.x address except the IP of the connected dexterous hand)
> Netmask: 255.255.255.0
> 5. Click "Apply" in the upper right corner, then turn off and on the network switch to make the settings take effect.

**Notes**:
1. Ensure the robot and the host are in the same network.
2. The firewall allows communication on relevant ports.
3. The robot's IP address is configured correctly.
4. Cloning this repository on Windows may cause dynamic library links to become text files (resulting in library file corruption). The solution is as follows:

    ```bash
    cd /workspace/rysen_sdk/lib/x86_64
    # Delete the fake links damaged by Windows
    rm librysen_sdk.so librysen_sdk.so.1

    # Re-establish the native Linux soft links
    ln -s librysen_sdk.so.1.x.x librysen_sdk.so.1
    ln -s librysen_sdk.so.1 librysen_sdk.so
    ```

---

## 📦 SDK Library File Description

This project provides a unified `rysen_sdk/` directory in the root directory, and all example programs are dynamically linked to the library files here:

* `rysen_sdk/include/` - C++ header files of the SDK
* `rysen_sdk/lib/x86_64/` - Dynamic link libraries suitable for ordinary PC/server platforms
* `rysen_sdk/lib/aarch64/` - Dynamic link libraries suitable for ARM edge computing platforms such as Raspberry Pi and NVIDIA Jetson

**🧠 Intelligent Architecture Routing**:
When building C++ or ROS 2 projects, the CMake script will automatically identify the physical architecture of the current system (`uname -m`) and intelligently and seamlessly link the `librysen_sdk.so` in the directory of the corresponding platform. Developers do not need to modify the path manually.

---

## 📚 More Information

* **ApexHand 1.0 Tactile Array Mapping**: Refer to [docs/apexhand_1.0_tactile_array_en.md](docs/apexhand_1.0_tactile_array_en.md) for Taxel layouts, valid indices, and left/right palm mappings
* **C++ API Documentation**: Refer to the comments in [cpp/README.md](cpp/README.md) and `cpp/rysen_example.cpp`
* **Python API Documentation**: Refer to [python/README.md](python/README.md)
* **ROS2 API Documentation**: Refer to [ros2/rysen_apexhand/README.md](ros2/rysen_apexhand/README.md)
* **Docker Environment**: Refer to [docker/README.md](docker/README.md)

---

## 📬 Contact Us

If you have any questions, you can contact us at support@rysenbot.com.
