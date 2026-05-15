# Rysen ApexHand SDK Python 示例

本目录提供 Rysen ApexHand SDK 的 Python 封装和示例程序。

## 目录结构

```
python/
├── README.md                    # 本说明文件
├── example.py                   # Python 使用示例
├── rysen_apexhand_sdk.py   # Python 封装模块
├── __init__.py                  # Python 包初始化文件
├── setup.py                     # Python 安装配置
├── MANIFEST.in                  # 打包清单文件
└── lib/                         # SDK 库文件目录（构建时自动生成）
    ├── librysen_sdk.so*    # SDK 核心库
    └── _rysen_sdk*.so      # Python 绑定库
```

## 安装

### 前置要求

1. **Python 版本**: Python 3.10
2. **系统依赖库**:

   **快速安装（推荐）**:
   ```bash
   # 在 examples 目录下
   chmod +x install_rysen_deps.sh
   ./install_rysen_deps.sh
   ```

   **手动安装运行时库**:
   ```bash
   sudo apt-get update
   sudo apt-get install -y \
     libspdlog1 \
     libfmt8 \
     libboost-system1.74.0 \
     libboost-thread1.74.0
   sudo ldconfig
   ```

### 安装步骤

1. **进入 Python 目录**:
   ```bash
   cd examples/python
   ```

2. **安装 Python 包**:
   ```bash
   pip install -e .
   ```
   
   或者使用 `pip3`:
   ```bash
   pip3 install -e .
   ```

   安装完成后，`rysen_apexhand_sdk` 模块即可在 Python 中使用。

## 使用示例

### 基本使用

```python
from rysen_apexhand_sdk import RysenApexHandSDK

# 创建 SDK 实例
sdk = RysenApexHandSDK()

# 连接机器人（Ethernet 方式）
if sdk.Connect("192.168.0.102", connection_type=1):
    print("连接成功")
    
    # 使能所有手指
    sdk.SetFingerEnabled([0, 1, 2, 3, 4], True)
    
    # 执行控制操作...
    
    # 断开连接
    sdk.Disconnect()
else:
    print("连接失败")
```

### 运行示例程序

```bash
cd examples/python
python example.py
```

或使用 Python 3:
```bash
python3 example.py
```

## API 说明

### 连接管理

- `Connect(address, connection_type)`: 连接机器人
  - `address`: 机器人 IP 地址（字符串）
  - `connection_type`: 连接类型（1=Ethernet，其他类型后续支持）
  - 返回: `True` 表示成功，`False` 表示失败

- `Disconnect()`: 断开连接
  - 返回: `True` 表示成功

### 手指控制

- `SetFingerEnabled(finger_ids, enable)`: 使能/禁用手指
  - `finger_ids`: 手指 ID 列表（0-4，分别对应拇指、食指、中指、无名指、小指）
  - `enable`: `True` 为使能，`False` 为禁用
  - 返回: `True` 表示成功

### 运动控制

- `MoveJoint(joint_ids, positions, velocities, accelerations)`: 位置控制（阻塞式）
  - `joint_ids`: 关节 ID 列表
  - `positions`: 目标位置列表（弧度）
  - `velocities`: 速度列表（弧度/秒）
  - `accelerations`: 加速度列表（弧度/秒²）
  - 返回: `True` 表示成功

- `MoveJPositionFollow(joint_ids, positions)`: 位置跟随控制（非阻塞）
  - `joint_ids`: 关节 ID 列表
  - `positions`: 目标位置列表（弧度）
  - 返回: `True` 表示成功

### 参数设置

- `SetMaxJointSpeed(joint_ids, speeds)`: 设置最大关节速度
- `SetMaxJointAccel(joint_ids, accels)`: 设置最大关节加速度
- `SetMaxFingerTorque(finger_ids, torques)`: 设置最大手指扭矩

### 状态获取

- `GetJointStates()`: 获取关节状态
  - 返回: 关节状态字典

- `GetMotorStates()`: 获取电机状态
  - 返回: 电机状态字典

- `GetHandSensorImage()`: 获取手部触觉图像
  - 返回: 图像数据（numpy 数组）

### 回调函数

可以注册回调函数来接收实时数据：

- `SetJointStatesCallback(callback)`: 设置关节状态回调
- `SetMotorStatesCallback(callback)`: 设置电机状态回调
- `SetHandSensorImageCallback(callback)`: 设置触觉图像回调

回调函数示例：
```python
def joint_states_callback(joint_states):
    print(f"收到关节状态: {joint_states}")

sdk.SetJointStatesCallback(joint_states_callback)
```

## 注意事项

1. **库文件版本匹配**: `lib/` 目录下的 `.so` 文件应与目标 Python 版本匹配
   - 例如：`_rysen_sdk.cpython-310-*.so` 对应 Python 3.10

2. **连接方式**: 目前仅支持 Ethernet 连接（`connection_type=1`）
   - 设备ip: 192.168.0.102 

3. **网络配置**: 确保机器人和主机在同一网络中，防火墙允许相关端口通信

4. **实时内核**: 建议使用实时内核（PREEMPT_RT）以获得更好的控制性能

5. **错误处理**: 所有 API 函数返回布尔值表示操作是否成功，建议检查返回值

## 故障排除

### 导入错误

如果遇到 `ImportError`，请检查：
1. Python 包是否正确安装：`pip list | grep rysen`
2. 库文件是否存在：`ls examples/python/lib/`
3. 系统依赖库是否安装：`ldconfig -p | grep spdlog`

### 连接失败

1. 检查机器人 IP 地址是否正确
2. 检查网络连接：`ping <机器人IP>`
3. 检查端口是否被占用：`netstat -an | grep 5856`

### 运行时错误

1. 确保系统依赖库已正确安装
2. 检查库文件权限：`ls -l examples/python/lib/`
3. 查看详细错误信息，检查 Python 版本兼容性

## 更多信息

- 完整示例代码：参考 `example.py`
- C++ API 文档：参考 `../cpp/rysen_example.cpp`
- ROS2 接口：参考 `../ros2/rysen_apexhand/README.md`
