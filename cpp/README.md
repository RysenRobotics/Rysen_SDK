# Rysen ApexHand SDK C++ 示例

本目录提供 Rysen ApexHand SDK 的 C++ 使用示例。

## 目录结构

```
cpp/
├── README.md                    # 本说明文件
├── rysen_example.cpp        # C++ 示例代码
├── CMakeLists.txt              # CMake 构建配置
└── rysen_sdk/               # SDK 文件（构建时自动复制，不跟踪）
    ├── include/                # SDK 头文件
    │   ├── rysen_apexhand_sdk.hpp
    │   └── rysen_apexhand_data.hpp
    └── lib/                    # SDK 库文件
        └── librysen_sdk.so*
```

## 前置要求

1. **C++ 编译器**: GCC 9+ 或 Clang 10+（支持 C++17）
2. **CMake**: 3.10 或更高版本
3. **系统依赖库**:

   **快速安装（推荐）**:
   ```bash
   # 在 examples 目录下
   chmod +x install_rysen_deps.sh
   ./install_rysen_deps.sh
   ```

   **手动安装**:
   ```bash
   sudo apt-get update
   sudo apt-get install -y \
     build-essential \
     cmake \
     libboost-all-dev \
     libserialport-dev \
     libspdlog-dev \
     libfmt-dev \
     libyaml-cpp-dev
   ```

## 构建

### 方法 1: 使用项目构建脚本（推荐）

```bash
# 在项目根目录
cd /path/to/rysen_sdk
./build.sh
```

构建完成后，可执行文件位于 `build/bin/rysen_example`。

### 方法 2: 单独构建

```bash
cd examples/cpp
mkdir build && cd build
cmake ..
make
```

可执行文件位于 `examples/cpp/build/rysen_example`。

## 运行

```bash
# 如果使用项目构建脚本
cd /path/to/rysen_sdk
./build/bin/rysen_example

# 如果单独构建
cd examples/cpp/build
./rysen_example
```

## 示例代码说明

`rysen_example.cpp` 演示了以下功能：

1. **连接和初始化**
   - Ethernet 连接
   - 版本验证

2. **手指控制**
   - 使能/禁用手指
   - 设置最大手指扭矩

3. **运动参数配置**
   - 设置最大关节速度
   - 设置最大关节加速度

4. **回调函数**
   - 关节状态回调
   - 电机状态回调
   - 手部触觉图像回调

5. **控制命令**
   - MoveJ 位置控制（阻塞式）
   - GetJointStates、GetMotorStates、GetHandSensorImage

6. **位置跟随控制**
   - MoveJPositionFollow（非阻塞式）

## API 使用示例

### 基本使用流程

```cpp
#include <rysen_apexhand_sdk.hpp>

// 创建 SDK 实例
RysenApexHandSDK sdk;

// 连接机器人
if (sdk.Connect("192.168.0.102", ConnectionType::ETHERNET)) {
    // 使能所有手指
    sdk.SetFingerEnabled({0, 1, 2, 3, 4}, true);
    
    // 设置回调函数
    sdk.SetJointStatesCallback([](const JointStates& states) {
        // 处理关节状态
    });
    
    // 执行控制操作
    std::vector<uint8_t> joint_ids = {0, 1, 2};
    std::vector<double> positions = {0.5, 0.3, 0.2};
    std::vector<double> velocities = {1.0, 1.0, 1.0};
    std::vector<double> accelerations = {2.0, 2.0, 2.0};
    
    sdk.MoveJoint(joint_ids, positions, velocities, accelerations);
    
    // 断开连接
    sdk.Disconnect();
}
```

### 位置跟随控制

```cpp
// 非阻塞式位置跟随控制
std::vector<uint8_t> joint_ids = {0, 1, 2};
std::vector<double> positions = {0.5, 0.3, 0.2};

sdk.MoveJPositionFollow(joint_ids, positions);
```

### 获取状态

```cpp
// 获取关节状态
JointStates joint_states;
if (sdk.GetJointStates(joint_states) == ErrorCode::ERROR_CODE_OK) {
    // 处理关节状态
}

// 获取电机状态
MotorStates motor_states;
if (sdk.GetMotorStates(motor_states) == ErrorCode::ERROR_CODE_OK) {
    // 处理电机状态
}

// 获取触觉图像
std::vector<uint16_t> tactile_image;
if (sdk.GetHandSensorImage(tactile_image) == ErrorCode::ERROR_CODE_OK) {
    // 处理触觉图像
}
```

## 连接配置

- **连接方式**: Ethernet（`ConnectionType::ETHERNET`）
- **默认 IP 地址**: 192.168.0.102（可在代码中修改）

## 注意事项

1. **实时内核**: 建议使用实时内核（PREEMPT_RT）以获得更好的控制性能

2. **网络配置**: 
   - 确保机器人和主机在同一网络中
   - 防火墙允许相关端口通信
   - 机器人 IP 地址正确

3. **SDK 文件**: 构建时会自动复制 SDK 头文件和库文件到 `rysen_sdk/` 目录

4. **库路径**: 如果运行时找不到库文件，可以设置 `LD_LIBRARY_PATH`:
   ```bash
   export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/path/to/examples/cpp/rysen_sdk/lib
   ```

5. **错误处理**: 所有 API 函数返回 `ErrorCode`，建议检查返回值

## 故障排除

### 编译错误

1. 检查 CMake 版本: `cmake --version`
2. 检查编译器版本: `g++ --version`
3. 确保所有依赖库已安装

### 运行时错误

1. **找不到库文件**:
   ```bash
   export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/examples/cpp/rysen_sdk/lib
   ```

2. **连接失败**:
   - 检查机器人 IP 地址
   - 检查网络连接: `ping <机器人IP>`
   - 检查端口是否被占用

3. **权限问题**:
   - 确保有网络访问权限
   - 某些操作可能需要 root 权限（实时内核相关）

## 更多信息

- 完整示例代码：参考 `rysen_example.cpp`
- Python API：参考 `../python/README.md`
- ROS2 接口：参考 `../ros2/rysen_apexhand/README.md`
- 主示例目录：参考 `../README.md`
