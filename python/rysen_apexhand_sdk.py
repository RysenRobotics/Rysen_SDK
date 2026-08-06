"""
Rysen ApexHand SDK Python Interface

This module provides a Python interface to the Rysen ApexHand SDK.

Copyright (c) 2024-2026, Rysen Robotics (Shenzhen) Co. Ltd.
All rights reserved.

Use of this source code is governed by a BSD 3-Clause license that can be
found in the LICENSE file.
"""

from typing import Callable, List, Optional
import time
import os
import sys
import glob
import ctypes

import platform

_CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
_ROOT_DIR = os.path.abspath(os.path.join(_CURRENT_DIR, ".."))

_ARCH = platform.machine()
if _ARCH == 'x86_64':
    _LIB_DIR = os.path.join(_ROOT_DIR, "rysen_sdk", "lib", "x86_64")
elif _ARCH in ['aarch64', 'arm64']:
    _LIB_DIR = os.path.join(_ROOT_DIR, "rysen_sdk", "lib", "aarch64")
else:
    raise RuntimeError(f"Unsupported architecture: {_ARCH}")

# 把库所在的目录强行插到 Python 搜索路径的最前面
if _LIB_DIR not in sys.path:
    sys.path.insert(0, _LIB_DIR)

# Ensure the native library directory is discoverable by Python
if os.path.isdir(_LIB_DIR) and _LIB_DIR not in sys.path:
    sys.path.insert(0, _LIB_DIR)


def _preload_native_libs() -> None:
    """Preload dependent shared libraries so the binding can be resolved."""
    if not os.path.isdir(_LIB_DIR):
        return

    load_mode = getattr(ctypes, "RTLD_GLOBAL", None)
    lib_patterns = (
        "librysen*.so",
        "librysen*.dylib",
        "librysen*.dll",
    )
    for pattern in lib_patterns:
        for candidate in glob.glob(os.path.join(_LIB_DIR, pattern)):
            try:
                if load_mode is None:
                    ctypes.CDLL(candidate)
                else:
                    ctypes.CDLL(candidate, mode=load_mode)
            except OSError:
                # Ignore failures - the loader will report a clear error later
                continue


_preload_native_libs()

# Import the native extension module
# Try multiple import strategies to handle different installation scenarios
_sdk = None
_import_errors = []

# Strategy 1: Try relative import (when installed as a package)
if _sdk is None:
    try:
        from . import _rysen_sdk as _sdk
    except ImportError as e:
        _import_errors.append(f"Relative import failed: {e}")

# Strategy 2: Try absolute import from current directory
if _sdk is None:
    try:
        import _rysen_sdk as _sdk
    except ImportError as e:
        _import_errors.append(f"Absolute import from current dir failed: {e}")

# Strategy 3: Try importing from parent directory (development scenario)
if _sdk is None:
    try:
        # Add package directory to sys.path if needed
        if _CURRENT_DIR not in sys.path:
            sys.path.insert(0, _CURRENT_DIR)
        import _rysen_sdk as _sdk
    except ImportError as e:
        _import_errors.append(f"Import from package dir failed: {e}")

# Strategy 4: Try finding .so file by pattern matching
if _sdk is None:
    so_patterns = [
        os.path.join(_CURRENT_DIR, "_rysen_sdk*.so"),
        os.path.join(_LIB_DIR, "_rysen_sdk*.so"),
        os.path.join(_LIB_DIR, "_rysen_sdk*.pyd"),
        os.path.join(os.path.dirname(_CURRENT_DIR), "python", "_rysen_sdk*.so"),
    ]
    for pattern in so_patterns:
        so_files = glob.glob(pattern)
        if so_files:
            # Add directory to sys.path and try import
            so_dir = os.path.dirname(so_files[0])
            if so_dir not in sys.path:
                sys.path.insert(0, so_dir)
            try:
                import _rysen_sdk as _sdk
                break
            except ImportError:
                continue

if _sdk is None:
    error_msg = "Failed to import _rysen_sdk module.\n"
    error_msg += "Tried the following import strategies:\n"
    for i, err in enumerate(_import_errors, 1):
        error_msg += f"  {i}. {err}\n"
    error_msg += "\nPossible solutions:\n"
    error_msg += "1. Make sure the package is installed: pip install -e .\n"
    error_msg += "2. Ensure the shared libraries exist under rysen_sdk/lib/ (after running cmake --build ...).\n"
    error_msg += "3. Set LD_LIBRARY_PATH to include the SDK library directory if your platform requires it:\n"
    error_msg += "   export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/path/to/rysen_sdk/python/lib\n"
    raise ImportError(error_msg)


# Dynamically expose pybind11 enums/structs so Python users can inspect
# the available data types without digging into the native .so.
_EXPORTED_NATIVE_TYPES: List[str] = []
for _name in dir(_sdk):
    if _name.startswith("_"):
        continue
    # Skip Rysen because we wrap it with a higher-level Python class below.
    if _name == "Rysen":
        continue

    _attr = getattr(_sdk, _name)
    if isinstance(_attr, type):
        globals()[_name] = _attr
        _EXPORTED_NATIVE_TYPES.append(_name)

# Commonly used aliases (kept for backwards compatibility / type hints)
LogLevel = getattr(_sdk, "LogLevel", None)
ConnectionType = getattr(_sdk, "ConnectionType", None)
ErrorCode = getattr(_sdk, "ErrorCode", None)
FingerId = getattr(_sdk, "FingerId", None)
HandDir = getattr(_sdk, "HandDir", None)
MotorId = getattr(_sdk, "MotorId", None)
JointId = getattr(_sdk, "JointId", None)

VersionInfo = getattr(_sdk, "VersionInfo", None)
ParamInfo = getattr(_sdk, "ParamInfo", None)
JointState = getattr(_sdk, "JointState", None)
JointStates = getattr(_sdk, "JointStates", None)
JointControlParam = getattr(_sdk, "JointControlParam", None)
MoveJPositionFollowParam = getattr(_sdk, "MoveJPositionFollowParam", None)
MotorState = getattr(_sdk, "MotorState", None)
MotorStates = getattr(_sdk, "MotorStates", None)
MaxJointSpeed = getattr(_sdk, "MaxJointSpeed", None)
MaxJointAccel = getattr(_sdk, "MaxJointAccel", None)
MaxFingerTorque = getattr(_sdk, "MaxFingerTorque", None)
HardwareErrorCodes = getattr(_sdk, "HardwareErrorCodes", None)

# Add detailed documentation for enums and data structures
# Note: Some types may have read-only __doc__ attributes, so we use try-except
def _set_doc(obj, doc_string):
    """Safely set __doc__ attribute, ignoring errors if it's read-only."""
    try:
        obj.__doc__ = doc_string
    except (AttributeError, TypeError):
        pass  # Ignore if __doc__ is read-only

if LogLevel is not None:
    _set_doc(LogLevel, """Log level enumeration.
    
    Values:
        LOG_LEVEL_DEBUG (0): Debug level logging
        LOG_LEVEL_INFO (1): Information level logging
        LOG_LEVEL_WARN (2): Warning level logging
        LOG_LEVEL_ERROR (3): Error level logging
        LOG_LEVEL_FATAL (4): Fatal error level logging
    """)

if ConnectionType is not None:
    _set_doc(ConnectionType, """Connection type enumeration.
    
    Values:
        CONNECTION_TYPE_RS485 (0): RS485 serial connection
        CONNECTION_TYPE_ETHERNET (1): Ethernet network connection
    """)

if ErrorCode is not None:
    _set_doc(ErrorCode, """Error code enumeration.
    
    Values:
        ERROR_CODE_OK (0): Operation successful (操作成功)
        ERROR_CODE_COMM_ERROR (1): Communication error (通讯错误：连接中断、收发失败等)
        ERROR_CODE_TIMEOUT (2): Timeout error (超时错误：等待设备响应超时)
        ERROR_CODE_OUT_OF_RANGE (3): Value out of range (参数超出允许范围，例如位置/速度/加速度)
        ERROR_CODE_OVER_SPEED (4): Overspeed / over-acceleration (速度/加速度过大)
        ERROR_CODE_OTHER_ERROR (5): Other error (其他错误，逐步废弃，建议使用下方更具体错误码)
        ERROR_CODE_INVALID_ARGUMENT (6): Invalid argument (非法参数：数量不匹配、频率不合法、空列表等)
        ERROR_CODE_CONFIG_ERROR (7): Configuration / environment error (配置或环境错误：日志配置失败、传感器不可用等)
        ERROR_CODE_NOT_IMPLEMENTED (8): Feature not implemented (功能未实现)
        ERROR_CODE_HARDWARE_ERROR (9): Hardware error detected (硬件错误，通过 HardwareErrorCodes 检测到故障)
    """)

if FingerId is not None:
    _set_doc(FingerId, """Finger identifier enumeration.
    
    Values:
        FINGER_ID_THUMB (0): Thumb finger
        FINGER_ID_INDEX (1): Index finger
        FINGER_ID_MIDDLE (2): Middle finger
        FINGER_ID_RING (3): Ring finger
        FINGER_ID_LITTLE (4): Little finger (小拇指)
    """)

if HandDir is not None:
    _set_doc(HandDir, """Hand direction enumeration.
    
    Values:
        LEFT (0): Left hand
        RIGHT (1): Right hand
    """)

if MotorId is not None:
    _set_doc(MotorId, """Motor identifier enumeration (16 motors total).
    
    Values:
        MOTOR_ID_THUMB_CMC_ABD (0): Thumb CMC motor 0
        MOTOR_ID_THUMB_CMC_ROT (1): Thumb CMC motor 1
        MOTOR_ID_THUMB_CMC_FLEX (2): Thumb CMC motor 2
        MOTOR_ID_THUMB_MCP_FLEX (3): Thumb MCP motor
        MOTOR_ID_INDEX_MCP_ABD_FLEX_0 (4): Index MCP motor 0
        MOTOR_ID_INDEX_MCP_ABD_FLEX_1 (5): Index MCP motor 1
        MOTOR_ID_INDEX_PIP_FLEX (6): Index PIP motor
        MOTOR_ID_MIDDLE_MCP_ABD_FLEX_0 (7): Middle MCP coupled motor 0
        MOTOR_ID_MIDDLE_MCP_ABD_FLEX_1 (8): Middle MCP coupled motor 1
        MOTOR_ID_MIDDLE_PIP_FLEX (9): Middle PIP motor
        MOTOR_ID_RING_MCP_ABD_FLEX_0 (10): Ring MCP coupled motor 0
        MOTOR_ID_RING_MCP_ABD_FLEX_1 (11): Ring MCP coupled motor 1
        MOTOR_ID_RING_PIP_FLEX (12): Ring PIP motor
        MOTOR_ID_LITTLE_MCP_ABD_FLEX_0 (13): Little MCP coupled motor 0
        MOTOR_ID_LITTLE_MCP_ABD_FLEX_1 (14): Little MCP coupled motor 1
        MOTOR_ID_LITTLE_PIP_FLEX (15): Little PIP motor
    """)

if JointId is not None:
    _set_doc(JointId, """Joint identifier enumeration (21 joints total).
    
    Values:
        JOINT_ID_THUMB_CMC_ABD (0): Thumb CMC abduction/adduction
        JOINT_ID_THUMB_CMC_ROT (1): Thumb CMC rotation
        JOINT_ID_THUMB_CMC_FLEX (2): Thumb CMC flexion
        JOINT_ID_THUMB_MCP_FLEX (3): Thumb MCP flexion
        JOINT_ID_THUMB_IP_FLEX (4): Thumb IP flexion
        JOINT_ID_INDEX_MCP_ABD (5): Index MCP abduction/adduction
        JOINT_ID_INDEX_MCP_FLEX (6): Index MCP flexion
        JOINT_ID_INDEX_PIP_FLEX (7): Index PIP joint
        JOINT_ID_INDEX_DIP_FLEX (8): Index DIP joint
        JOINT_ID_MIDDLE_MCP_ABD (9): Middle MCP abduction/adduction
        JOINT_ID_MIDDLE_MCP_FLEX (10): Middle MCP flexion
        JOINT_ID_MIDDLE_PIP_FLEX (11): Middle PIP joint
        JOINT_ID_MIDDLE_DIP_FLEX (12): Middle DIP joint
        JOINT_ID_RING_MCP_ABD (13): Ring MCP abduction/adduction
        JOINT_ID_RING_MCP_FLEX (14): Ring MCP flexion
        JOINT_ID_RING_PIP_FLEX (15): Ring PIP joint
        JOINT_ID_RING_DIP_FLEX (16): Ring DIP joint
        JOINT_ID_LITTLE_MCP_ABD (17): Little MCP abduction/adduction
        JOINT_ID_LITTLE_MCP_FLEX (18): Little MCP flexion
        JOINT_ID_LITTLE_PIP_FLEX (19): Little PIP flexion
        JOINT_ID_LITTLE_DIP_FLEX (20): Little DIP flexion
    """)

if VersionInfo is not None:
    _set_doc(VersionInfo, """Version information structure.
    
    Attributes:
        touch_sensor_version (str): Touch sensor firmware version
        hand_firmware_version (str): Hand firmware version
        sdk_version (str): SDK library version
    """)

if ParamInfo is not None:
    _set_doc(ParamInfo, """Parameter information structure.
    
    Attributes:
        max_speed (List[float]): Maximum joint speeds (rad/s)
        max_accel (List[float]): Maximum joint accelerations (rad/s²)
        max_current (List[float]): Maximum finger currents/torques (%)
    """)

if JointState is not None:
    _set_doc(JointState, """Single joint state structure.
    
    Attributes:
        joint_id (JointId): Joint identifier
        position (float): Current joint position (rad)
        velocity (float): Current joint velocity (rad/s)
        acceleration (float): Current joint acceleration (rad/s²)
    """)

if JointStates is not None:
    _set_doc(JointStates, """Joint states collection with timestamp.
    
    Attributes:
        timestamp: Timestamp of the joint states measurement
        joint_states (List[JointState]): List of joint state data
    """)

if JointControlParam is not None:
    _set_doc(JointControlParam, """Joint control parameter structure.
    
    Attributes:
        joint_id (JointId): Joint identifier
        position (float): Target joint position (rad)
        velocity (float): Target joint velocity (rad/s)
        acceleration (float): Target joint acceleration (rad/s²);
            protocol TgJointAcc, can be used as current substitute channel
    """)

if MoveJPositionFollowParam is not None:
    _set_doc(MoveJPositionFollowParam, """MoveJ position follow parameter structure.
    
    Used for smooth trajectory following with automatic interpolation.
    
    Attributes:
        id (JointId): Joint identifier
        position (float): Target joint position (rad)
    """)

if MotorState is not None:
    _set_doc(MotorState, """Single motor state structure.
    
    Attributes:
        motor_id (MotorId): Motor identifier
        temperature (float): Motor temperature
        current (float): Motor current
    """)

if MotorStates is not None:
    _set_doc(MotorStates, """Motor states collection with timestamp.
    
    Attributes:
        timestamp: Timestamp of the motor states measurement
        motors (List[MotorState]): List of motor state data
    """)

if MaxJointSpeed is not None:
    _set_doc(MaxJointSpeed, """Maximum joint speed parameter structure.
    
    Attributes:
        joint_id (JointId): Joint identifier
        speed (float): Maximum speed limit (rad/s)
    """)

if MaxJointAccel is not None:
    _set_doc(MaxJointAccel, """Maximum joint acceleration parameter structure.
    
    Attributes:
        joint_id (JointId): Joint identifier
        accel (float): Maximum acceleration limit (rad/s²)
    """)

if MaxFingerTorque is not None:
    _set_doc(MaxFingerTorque, """Maximum finger torque parameter structure.
    
    Attributes:
        finger_id (FingerId): Finger identifier
        torque (float): Maximum torque limit (0-100%)
    """)

_TangentialForce = getattr(_sdk, "TangentialForce", None)
if _TangentialForce is not None:
    _set_doc(_TangentialForce, """Tangential force structure.
    
    Attributes:
        theta (float): Tangential force direction (0-2π radians)
        magnitude (float): Tangential force magnitude
    """)

_TactileImage = getattr(_sdk, "TactileImage", None)
if _TactileImage is not None:
    _set_doc(_TactileImage, """Tactile image structure (2D force image).
    
    Attributes:
        width (int): Image width in pixels
        height (int): Image height in pixels
        gray_image (List[int]): Normal-force image in Pa
        tangential_forces (TangentialForce): Tangential force data
    """)

_CommonFingerSensorImage = getattr(_sdk, "CommonFingerSensorImage", None)
if _CommonFingerSensorImage is not None:
    _set_doc(_CommonFingerSensorImage, """Common finger sensor image structure.
    
    Attributes:
        prox_pad (TactileImage): Proximal interphalangeal joint surface image
        mid_pad (TactileImage): Distal interphalangeal joint surface image
        dist_pad (TactileImage): Fingertip surface image
    """)

_ThumbFingerSensorImage = getattr(_sdk, "ThumbFingerSensorImage", None)
if _ThumbFingerSensorImage is not None:
    _set_doc(_ThumbFingerSensorImage, """Thumb finger sensor image structure.
    
    Attributes:
        prox_pad (TactileImage): Carpometacarpal joint surface image
        mid_pad (TactileImage): Metacarpophalangeal joint surface image
        dist_pad (TactileImage): Thumbtip surface image
    """)

_HandSensorImage = getattr(_sdk, "HandSensorImage", None)
if _HandSensorImage is not None:
    _set_doc(_HandSensorImage, """Hand sensor image structure.
    
    Attributes:
        timestamp: Timestamp of the sensor image
        index_image (CommonFingerSensorImage): Index finger sensor images
        middle_image (CommonFingerSensorImage): Middle finger sensor images
        ring_image (CommonFingerSensorImage): Ring finger sensor images
        little_image (CommonFingerSensorImage): Little finger sensor images
        thumb_image (ThumbFingerSensorImage): Thumb finger sensor images
        palm_center (TactileImage): Palm surface image
    """)




class Rysen:
    """
    Rysen ApexHand SDK Python Interface
    
    This class provides a high-level Python interface to control and interact
    with the Rysen ApexHand robotic hand.
    
    Example:
        >>> from rysen_apexhand_sdk import Rysen, ConnectionType, JointId, JointControlParam
        >>> 
        >>> # Create SDK instance
        >>> bot = Rysen()
        >>> 
        >>> # Connect to robot
        >>> result = bot.connect("192.168.0.102", ConnectionType.CONNECTION_TYPE_ETHERNET)
        >>> if result == ErrorCode.ERROR_CODE_OK:
        >>>     print("Connected successfully")
        >>> 
        >>> # Enable all fingers
        >>> bot.set_all_fingers_enabled()
        >>> 
        >>> # Move a joint
        >>> cmd = JointControlParam()
        >>> cmd.joint_id = JointId.JOINT_ID_THUMB_MCP_FLEX
        >>> cmd.position = 0.5
        >>> cmd.velocity = 0.1
        >>> bot.move_joint([cmd])
        >>> 
        >>> # Register callback for joint states
        >>> def on_joint_states(states):
        >>>     print(f"Received {len(states.joint_states)} joint states")
        >>> 
        >>> bot.register_joint_states_callback(on_joint_states, freq_hz=100)
        >>> 
        >>> # Disconnect
        >>> bot.disconnect()
    """
    
    def __init__(self):
        """Initialize the Rysen SDK instance."""
        self._sdk = _sdk.Rysen()
    
    def connect(self, address: str, connection_type: ConnectionType = ConnectionType.CONNECTION_TYPE_ETHERNET) -> ErrorCode:
        """
        Connect to the robot.
        
        连接机器人
        
        Args:
            address: Robot address (机器人地址)
            connection_type: Connection type (连接类型), default: CONNECTION_TYPE_ETHERNET
        
        Returns:
            ErrorCode: ERROR_CODE_OK if connection successful, error code otherwise
        """

        # 1. 拦截 connection_type 的类型错误
        if not isinstance(connection_type, ConnectionType):
            return ErrorCode.ERROR_CODE_INVALID_ARGUMENT

        # 2. address 的类型也拦截一下
        if not isinstance(address, str):
            return ErrorCode.ERROR_CODE_INVALID_ARGUMENT

        # 如果类型都合法，才给底层的 _rysen_sdk (也就是 pybind11) 去处理
        return self._sdk.connect(address, connection_type)

    def set_device_ip_address(self, ip_address: str) -> ErrorCode:
        """
        Set device IP address using default netmask/gateway and wait for device acknowledgment.

        设置设备 IP（子网掩码与网关使用默认值），并等待设备应答。

        Args:
            ip_address: Target IPv4 address (目标 IP 地址)

        Returns:
            ErrorCode: ERROR_CODE_OK if successful, otherwise an error code
        """
        if not isinstance(ip_address, str) or not ip_address.strip():
            return ErrorCode.ERROR_CODE_INVALID_ARGUMENT
        # NOTE: Python 示例接口只暴露 IP，子网掩码和网关使用默认值
        return self._sdk.set_device_ip_address(ip_address)
    
    def disconnect(self) -> ErrorCode:
        """
        Disconnect from the robot.
        
        断开与机器人的连接
        
        Returns:
            ErrorCode: ERROR_CODE_OK if disconnection successful
        """
        return self._sdk.disconnect()
    
    def is_connected(self) -> bool:
        """
        Check if connected to the robot.
        
        判断是否与机器人建立了连接
        
        Returns:
            bool: True if connected, False otherwise
        """
        return self._sdk.is_connected()
    
    def get_version_info(self) -> VersionInfo:
        """
        Get version information.
        
        获取版本信息
        
        Returns:
            VersionInfo: Version information structure (版本信息)
        """
        return self._sdk.get_version_info()
    
    def get_hardware_uid(self) -> str:
        """
        Get hardware UID.
        
        获取灵巧手硬件的 96位 全局唯一标识符 (UID)
        
        Returns:
            str: 格式化后的 96位十六进制字符串 (如 "XXXXXXXX-XXXXXXXX-XXXXXXXX")，未连接时返回空字符串
        """
        return self._sdk.get_hardware_uid()
    
    def set_all_fingers_enabled(self) -> ErrorCode:
        """
        Enable all fingers.
        
        使能所有手指
        
        Returns:
            ErrorCode: ERROR_CODE_OK if successful
        """
        return self._sdk.set_all_fingers_enabled()
    
    def set_all_fingers_disabled(self) -> ErrorCode:
        """
        Disable all fingers.
        
        禁用所有手指
        
        Returns:
            ErrorCode: ERROR_CODE_OK if successful
        """
        return self._sdk.set_all_fingers_disabled()
    
    def set_finger_enabled(self, fingers: List[FingerId]) -> ErrorCode:
        """
        Enable specified fingers.
        
        使能指定手指
        
        Args:
            fingers: List of finger IDs to enable (手指列表)
        
        Returns:
            ErrorCode: ERROR_CODE_OK if successful
        """
        if not isinstance(fingers, (list, tuple)):
            return ErrorCode.ERROR_CODE_INVALID_ARGUMENT
        if not all(isinstance(f, FingerId) for f in fingers):
            return ErrorCode.ERROR_CODE_INVALID_ARGUMENT
        return self._sdk.set_finger_enabled(fingers)
    
    def set_finger_disabled(self, fingers: List[FingerId]) -> ErrorCode:
        """
        Disable specified fingers.
        
        禁用指定手指
        
        Args:
            fingers: List of finger IDs to disable (手指列表)
        
        Returns:
            ErrorCode: ERROR_CODE_OK if successful
        """
        if not isinstance(fingers, (list, tuple)):
            return ErrorCode.ERROR_CODE_INVALID_ARGUMENT
        if not all(isinstance(f, FingerId) for f in fingers):
            return ErrorCode.ERROR_CODE_INVALID_ARGUMENT
        return self._sdk.set_finger_disabled(fingers)
    
    def is_finger_enabled(self, finger_id: FingerId) -> bool:
        """
        Check if a specific finger is enabled.
        
        检查指定手指是否已使能
        
        Args:
            finger_id: Finger ID to check (手指 ID)
            
        Returns:
            bool: True if enabled, False otherwise (未连接或参数错误也返回 False)
        """
        if not isinstance(finger_id, FingerId):
            return False
        return self._sdk.is_finger_enabled(finger_id)
    
    def clean_faults(self) -> ErrorCode:
        """
        Clear faults.
        
        清除机器人的故障状态
        
        Returns:
            ErrorCode: ERROR_CODE_OK if successful
        """
        return self._sdk.clean_faults()
    
    def set_log_path(self, path: str = "./") -> ErrorCode:
        """
        Set log path.
        
        设置日志路径
        
        Args:
            path: Log file path (日志路径), default: "./"
        
        Returns:
            ErrorCode: ERROR_CODE_OK if successful
        
        Note:
            The log level parameter is ignored. Log level is fixed to INFO internally
            and cannot be changed by users.
        """
        # 拦截 None 和其他非法类型，防止 pybind11 崩溃
        if not isinstance(path, str):
            return ErrorCode.ERROR_CODE_INVALID_ARGUMENT
            
        # 拦截空字符串，不让它走到 C++ 底层的自动降级逻辑
        if path.strip() == "":
            return ErrorCode.ERROR_CODE_INVALID_ARGUMENT
            
        # 拦截超长路径 (Linux 系统通常单级文件名最大 255 字符，总路径 4096 字符)
        # 这里设置一个合理的安全阈值，比如 1024。超过直接打回！
        if len(path) > 1024:
            return ErrorCode.ERROR_CODE_INVALID_ARGUMENT
        # level parameter is ignored - log level is fixed to INFO internally
        return self._sdk.set_log_path(path)
    
    def enable_logging(self, enable: bool = True) -> ErrorCode:
        """
        Enable or disable logging.
        
        开启/关闭日志
        
        Args:
            enable: True to enable, False to disable (开启/关闭)
        
        Returns:
            ErrorCode: ERROR_CODE_OK if successful
        """
        if not isinstance(enable, (bool, int)):
            return ErrorCode.ERROR_CODE_INVALID_ARGUMENT
        return self._sdk.enable_logging(enable)
    
    def set_max_joint_speed(self, speeds: List[MaxJointSpeed]) -> ErrorCode:
        """
        Set maximum joint speeds.
        
        设置最大关节速度
        
        Args:
            speeds: List of MaxJointSpeed structures (最大关节速度，21个关节，rad/s)
        
        Returns:
            ErrorCode: ERROR_CODE_OK if successful, ERROR_CODE_OUT_OF_RANGE if speed exceeds maximum allowed
        """
        if not isinstance(speeds, (list, tuple)):
            return ErrorCode.ERROR_CODE_INVALID_ARGUMENT
        if not all(isinstance(s, MaxJointSpeed) for s in speeds):
            return ErrorCode.ERROR_CODE_INVALID_ARGUMENT
        return self._sdk.set_max_joint_speed(speeds)
    
    def set_max_finger_torque(self, torques: List[MaxFingerTorque]) -> ErrorCode:
        """
        Set maximum finger torques.
        
        设置最大手指扭矩
        
        Args:
            torques: List of MaxFingerTorque structures (最大手指扭矩，五个手指，0-100%)
        
        Returns:
            ErrorCode: ERROR_CODE_OK if successful
        """
        if not isinstance(torques, (list, tuple)):
            return ErrorCode.ERROR_CODE_INVALID_ARGUMENT
        if not all(isinstance(t, MaxFingerTorque) for t in torques):
            return ErrorCode.ERROR_CODE_INVALID_ARGUMENT
        return self._sdk.set_max_finger_torque(torques)
    
    def set_max_joint_accel(self, accels: List[MaxJointAccel]) -> ErrorCode:
        """
        Set maximum joint accelerations.
        
        设置最大关节加速度
        
        Args:
            accels: List of MaxJointAccel structures (最大关节加速度，21个关节，rad/s²)
        
        Returns:
            ErrorCode: ERROR_CODE_OK if successful, ERROR_CODE_OUT_OF_RANGE if acceleration exceeds maximum allowed
        """
        if not isinstance(accels, (list, tuple)):
            return ErrorCode.ERROR_CODE_INVALID_ARGUMENT
        if not all(isinstance(a, MaxJointAccel) for a in accels):
            return ErrorCode.ERROR_CODE_INVALID_ARGUMENT
        return self._sdk.set_max_joint_accel(accels)

    def set_robot_model_from_urdf(self, urdf_path: str) -> ErrorCode:
        """
        Load robot kinematic model from URDF file.

        从 URDF 文件加载机器人模型（刚体/关节模型）。

        注意：
            - SDK 默认不会自动加载任何 URDF，必须显式调用本函数。
            - 该模型主要供实时规划与运动学计算使用。

        Args:
            urdf_path: Path to URDF file (URDF 文件路径，不能为空)

        Returns:
            ErrorCode:
                - ERROR_CODE_OK:              加载成功
                - ERROR_CODE_INVALID_ARGUMENT: urdf_path 为空
                - ERROR_CODE_CONFIG_ERROR:     URDF 文件不存在或解析失败
        """
        if not isinstance(urdf_path, str) or not urdf_path.strip():
            return ErrorCode.ERROR_CODE_INVALID_ARGUMENT
        return self._sdk.set_robot_model_from_urdf(urdf_path)
    
    def get_parameters(self) -> ParamInfo:
        """
        Get parameters.
        
        获取参数
        
        Returns:
            ParamInfo: Parameter information structure (参数信息)
        """
        return self._sdk.get_parameters()
    
    def move_joint(self, commands: List[JointControlParam]) -> ErrorCode:
        """
        Move joints to target positions.
        
        位置控制：控制机器人的关节位置，阻塞式控制，直到到达目标位置
        
        Args:
            commands: List of JointControlParam structures (控制关节，包含位置、速度、加速度，任意关节数量)
        
        Returns:
            ErrorCode: ERROR_CODE_OK if successful
        """
        if not isinstance(commands, (list, tuple)):
            return ErrorCode.ERROR_CODE_INVALID_ARGUMENT
        if not all(isinstance(cmd, JointControlParam) for cmd in commands):
            return ErrorCode.ERROR_CODE_INVALID_ARGUMENT
        return self._sdk.move_joint(commands)
    
    def move_j_position_follow(self, params: List[MoveJPositionFollowParam]) -> ErrorCode:
        """
        Move joints with position following (interpolated, non-blocking).
        
        跟随关节位置控制：适用于非固定周期的跟随指令输入，内部对相邻目标做样条规划
        
        Args:
            params: List of MoveJPositionFollowParam structures (目标关节位置，21个关节)
        
        Returns:
            ErrorCode: ERROR_CODE_OK if successful
        """
        if not isinstance(params, (list, tuple)):
            return ErrorCode.ERROR_CODE_INVALID_ARGUMENT
        if not all(isinstance(p, MoveJPositionFollowParam) for p in params):
            return ErrorCode.ERROR_CODE_INVALID_ARGUMENT
        return self._sdk.move_j_position_follow(params)

    def move_j_control_follow(self, commands: List[JointControlParam]) -> ErrorCode:
        """
        Direct joint control follow (no smoothing / tracker).

        直接跟随控制：将 position / velocity / acceleration 原样下发。
        acceleration 对应协议 TgJointAcc，可作为电流替代通道。

        Args:
            commands: List of JointControlParam (position rad, velocity rad/s,
                      acceleration rad/s² or current substitute)

        Returns:
            ErrorCode: ERROR_CODE_OK if successful
        """
        if not isinstance(commands, (list, tuple)):
            return ErrorCode.ERROR_CODE_INVALID_ARGUMENT
        if not all(isinstance(cmd, JointControlParam) for cmd in commands):
            return ErrorCode.ERROR_CODE_INVALID_ARGUMENT
        return self._sdk.move_j_control_follow(commands)

    def start_tactile_calibration(self) -> ErrorCode:
        """
        Start tactile calibration (normal force zero offset + tangential bias).
        
        开始触觉标定
        
        Returns:
            ErrorCode: ERROR_CODE_OK if successful
        """
        return self._sdk.start_tactile_calibration()

    def clear_tactile_calibration(self) -> ErrorCode:
        """
        Clear tactile calibration (normal force zero offset + tangential bias).
        
        清空触觉标定
        
        Returns:
            ErrorCode: ERROR_CODE_OK if successful
        """
        return self._sdk.clear_tactile_calibration()

    def get_hand_dir(self) -> HandDir:
        """
        Get hand direction (LEFT/RIGHT).
        
        获取手方向
        
        Returns:
            HandDir: HandDir.LEFT or HandDir.RIGHT
        """
        return self._sdk.get_hand_dir()
    
    def get_hardware_error_code(self) -> ErrorCode:
        """
        Get hardware error code.
        
        获取硬件错误码
        检查硬件错误码（设备及各手指的错误码），如果检测到错误则触发错误回调
        
        Returns:
            ErrorCode: Current error code (错误码)，ERROR_CODE_OK 表示无错误，
                       ERROR_CODE_HARDWARE_ERROR 表示检测到硬件错误
        """
        return self._sdk.get_hardware_error_code()
    
    def get_joint_states(self) -> JointStates:
        """
        Get joint states.
        
        获取关节状态
        
        Returns:
            JointStates: Joint states structure with timestamp (关节状态，包含时间戳和关节状态列表)
        """
        return self._sdk.get_joint_states()
    
    def get_motor_states(self) -> MotorStates:
        """
        Get motor states.
        
        获取电机状态
        
        Returns:
            MotorStates: Motor states structure with timestamp (电机状态，包含时间戳和电机列表)
        """
        return self._sdk.get_motor_states()
    
    def get_hand_sensor_image(self) -> _sdk.HandSensorImage:
        """
        Get hand sensor image.
        
        获取手传感器图像
        
        Returns:
            HandSensorImage: Hand sensor image data (手传感器图像)
        """
        return self._sdk.get_hand_sensor_image()
    
    def register_hardware_error_event_callback(self, callback: Optional[Callable[[HardwareErrorCodes], None]] = None) -> ErrorCode:
        """
        Register hardware error event callback.
        
        注册硬件错误事件回调函数
        
        Args:
            callback: Hardware error event callback function that takes HardwareErrorCodes as parameter.
                     硬件错误事件回调函数，接收 HardwareErrorCodes 作为参数。
                     Pass None to unregister.
        
        Returns:
            ErrorCode: ERROR_CODE_OK if successful
        """
        if callback is not None and not callable(callback):
            return ErrorCode.ERROR_CODE_INVALID_ARGUMENT
        return self._sdk.register_hardware_error_event_callback(callback)
    
    def unregister_hardware_error_event_callback(self) -> ErrorCode:
        """
        Unregister hardware error event callback.
        
        取消注册硬件错误事件回调函数
        
        Returns:
            ErrorCode: ERROR_CODE_OK if successful
        """
        return self._sdk.unregister_hardware_error_event_callback()
    
    def register_joint_states_callback(self, 
                                       callback: Optional[Callable[[JointStates], None]] = None,
                                       freq_hz: int = 100) -> ErrorCode:
        """
        Register joint states callback.
        
        注册关节状态回调函数
        
        Args:
            callback: Callback function that takes JointStates as argument (关节状态回调函数).
                     Pass None to unregister.
            freq_hz: Callback frequency in Hz (回调频率), default: 100
        
        Returns:
            ErrorCode: ERROR_CODE_OK if successful
        """
        if callback is not None and not callable(callback):
            return ErrorCode.ERROR_CODE_INVALID_ARGUMENT
        if not isinstance(freq_hz, int):
            return ErrorCode.ERROR_CODE_INVALID_ARGUMENT
        return self._sdk.register_joint_states_callback(callback, freq_hz)
    
    def unregister_joint_states_callback(self) -> ErrorCode:
        """
        Unregister joint states callback.
        
        取消注册关节状态回调函数
        
        Returns:
            ErrorCode: ERROR_CODE_OK if successful
        """
        return self._sdk.unregister_joint_states_callback()
    
    def register_motor_states_callback(self,
                                       callback: Optional[Callable[[MotorStates], None]] = None,
                                       freq_hz: int = 100) -> ErrorCode:
        """
        Register motor states callback.
        
        注册电机状态回调函数
        
        Args:
            callback: Callback function that takes MotorStates as argument (电机状态回调函数).
                     Pass None to unregister.
            freq_hz: Callback frequency in Hz (回调频率), default: 100
        
        Returns:
            ErrorCode: ERROR_CODE_OK if successful
        """
        if callback is not None and not callable(callback):
            return ErrorCode.ERROR_CODE_INVALID_ARGUMENT
        if not isinstance(freq_hz, int):
            return ErrorCode.ERROR_CODE_INVALID_ARGUMENT
        return self._sdk.register_motor_states_callback(callback, freq_hz)
    
    def unregister_motor_states_callback(self) -> ErrorCode:
        """
        Unregister motor states callback.
        
        取消注册电机状态回调函数
        
        Returns:
            ErrorCode: ERROR_CODE_OK if successful
        """
        return self._sdk.unregister_motor_states_callback()
    
    def register_hand_sensor_image_callback(self,
                                                  callback: Optional[Callable] = None,
                                                  freq_hz: int = 100) -> ErrorCode:
        """
        Register hand sensor image callback.
        
        注册手传感器图像回调函数
        
        Args:
            callback: Callback function that takes HandSensorImage as argument (手传感器图像回调函数).
                     Pass None to unregister.
            freq_hz: Callback frequency in Hz (回调频率), default: 100
        
        Returns:
            ErrorCode: ERROR_CODE_OK if successful
        """
        if callback is not None and not callable(callback):
            return ErrorCode.ERROR_CODE_INVALID_ARGUMENT
        if not isinstance(freq_hz, int):
            return ErrorCode.ERROR_CODE_INVALID_ARGUMENT
        return self._sdk.register_hand_sensor_image_callback(callback, freq_hz)
    
    def unregister_hand_sensor_image_callback(self) -> ErrorCode:
        """
        Unregister hand sensor image callback.
        
        取消注册手传感器图像回调函数
        
        Returns:
            ErrorCode: ERROR_CODE_OK if successful
        """
        return self._sdk.unregister_hand_sensor_image_callback()


# Convenience functions
def create_joint_control_param(joint_id: JointId, 
                               position: float = 0.0,
                               velocity: float = 0.0,
                               acceleration: float = 0.0) -> JointControlParam:
    """
    Create a JointControlParam structure.
    
    Args:
        joint_id: Joint ID
        position: Target position (rad)
        velocity: Target velocity (rad/s)
        acceleration: Target acceleration (rad/s²)
    
    Returns:
        JointControlParam: Configured joint control parameter
    """
    param = JointControlParam()
    param.joint_id = joint_id
    param.position = position
    param.velocity = velocity
    param.acceleration = acceleration
    return param


def create_move_j_position_follow_param(joint_id: JointId, position: float = 0.0, torque: float = 200.0) -> MoveJPositionFollowParam:
    """
    Create a MoveJPositionFollowParam structure.
    
    Args:
        joint_id: Joint ID
        position: Target position (rad)
        torque: Target joint torque (Nmm)
    
    Returns:
        MoveJPositionFollowParam: Configured position follow parameter
    """
    param = MoveJPositionFollowParam()
    param.id = joint_id
    param.position = position
    param.torque = torque
    return param


__all__ = sorted(
    set(_EXPORTED_NATIVE_TYPES)
    | {
        'Rysen',
        'LogLevel',
        'ConnectionType',
        'ErrorCode',
        'FingerId',
        'MotorId',
        'JointId',
        'VersionInfo',
        'ParamInfo',
        'JointState',
        'JointStates',
        'JointControlParam',
        'MoveJPositionFollowParam',
        'MotorState',
        'MotorStates',
        'MaxJointSpeed',
        'HardwareErrorCodes',
        'MaxJointAccel',
        'MaxFingerTorque',
        'create_joint_control_param',
        'create_move_j_position_follow_param',
    }
)