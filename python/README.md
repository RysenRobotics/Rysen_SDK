# Rysen ApexHand SDK Python Examples

This directory provides the Python wrapper and example programs for the Rysen ApexHand SDK.

## Directory Structure

```
python/
├── example.py
├── __init__.py
├── MANIFEST.in
├── README.md
├── rysen_apexhand_sdk.py
└── setup.py
```

## Installation

### Prerequisites

1. **Python Version**: Python 3.10
2. **System Dependencies**:

   **Quick Installation (Recommended)**:
   ```bash
   # In the root directory of the repository
   chmod +x install_rysen_deps.sh
   ./install_rysen_deps.sh
   ```

   **Manual Installation of Runtime Libraries**:
   ```bash
   sudo apt-get update
   sudo apt-get install -y \
     libspdlog1 \
     libfmt8 \
     libboost-system1.74.0 \
     libboost-thread1.74.0
   sudo ldconfig
   ```

### Installation Steps

1. **Navigate to the Python directory**:
   ```bash
   cd python/
   ```

2. **Install the Python package**:
   ```bash
   pip install -e .
   ```
   
   Or use `pip3`:
   ```bash
   pip3 install -e .
   ```

   After installation, the `rysen_apexhand_sdk` module can be used in Python.

## Usage Examples

### Basic Usage

```python
from rysen_apexhand_sdk import RysenApexHandSDK

# Create an SDK instance
sdk = RysenApexHandSDK()

# Connect to the robot (Ethernet mode)
if sdk.Connect("192.168.0.102", connection_type=1):
    print("Connection successful")
    
    # Enable all fingers
    sdk.SetFingerEnabled([0, 1, 2, 3, 4], True)
    
    # Perform control operations...
    
    # Disconnect
    sdk.Disconnect()
else:
    print("Connection failed")
```

### Run the Example Program

```bash
cd python
python example.py
```

Or use Python 3:
```bash
cd python
python3 example.py
```

## API Documentation

### Connection Management

- `Connect(address, connection_type)`: Connect to the robot
  - `address`: Robot IP address (string)
  - `connection_type`: Connection type (1=Ethernet, other types to be supported later)
  - Returns: `True` for success, `False` for failure

- `Disconnect()`: Disconnect from the robot
  - Returns: `True` for success

### Finger Control

- `SetFingerEnabled(finger_ids, enable)`: Enable/disable fingers
  - `finger_ids`: List of finger IDs (0-4, corresponding to thumb, index finger, middle finger, ring finger, little finger respectively)
  - `enable`: `True` for enable, `False` for disable
  - Returns: `True` for success

### Motion Control

- `MoveJoint(joint_ids, positions, velocities, accelerations)`: Position control (blocking)
  - `joint_ids`: List of joint IDs
  - `positions`: List of target positions (radians)
  - `velocities`: List of velocities (radians/second)
  - `accelerations`: List of accelerations (radians/second²)
  - Returns: `True` for success

- `MoveJPositionFollow(joint_ids, positions)`: Position follow control (non-blocking)
  - `joint_ids`: List of joint IDs
  - `positions`: List of target positions (radians)
  - Returns: `True` for success

### Parameter Configuration

- `SetMaxJointSpeed(joint_ids, speeds)`: Set maximum joint speeds
- `SetMaxJointAccel(joint_ids, accels)`: Set maximum joint accelerations
- `SetMaxFingerTorque(finger_ids, torques)`: Set maximum finger torques

### Status Retrieval

- `GetJointStates()`: Retrieve joint states
  - Returns: Dictionary of joint states

- `GetMotorStates()`: Retrieve motor states
  - Returns: Dictionary of motor states

- `GetHandSensorImage()`: Retrieve hand tactile image
  - Returns: Image data (numpy array)

### Callback Functions

You can register callback functions to receive real-time data:

- `SetJointStatesCallback(callback)`: Set joint state callback
- `SetMotorStatesCallback(callback)`: Set motor state callback
- `SetHandSensorImageCallback(callback)`: Set tactile image callback

Callback function example:
```python
def joint_states_callback(joint_states):
    print(f"Received joint states: {joint_states}")

sdk.SetJointStatesCallback(joint_states_callback)
```

## Notes

1. **Library File Version Compatibility**: The `.so` files in the `../rysen_sdk/lib/**/` directory must match the target Python version
   - Example: `_rysen_sdk.cpython-310-*.so` corresponds to Python 3.10

2. **Connection Method**: Only Ethernet connection (`connection_type=1`) is supported currently
   - Device IP: 192.168.0.102

3. **Network Configuration**: Ensure the robot and host are on the same network, and the firewall allows communication on relevant ports

4. **Real-Time Kernel**: It is recommended to use a real-time kernel (PREEMPT_RT) for better control performance

5. **Error Handling**: All API functions return a boolean value indicating the success of the operation; it is recommended to check the return value

## Troubleshooting

### Import Errors

If you encounter an `ImportError`, check the following:
1. Whether the Python package is installed correctly: `pip list | grep rysen`
2. Whether the library files exist: `ls ../rysen_sdk/lib/`
3. Whether the system dependencies are installed: `ldconfig -p | grep spdlog`

### Connection Failures

1. Check if the robot IP address is correct
2. Check the network connection: `ping <Robot IP>`
3. Check if the port is occupied: `netstat -an | grep 5856`

### Runtime Errors

1. Ensure the system dependencies are installed correctly
2. Check library file permissions: `ls -l examples/python/lib/`
3. Check the detailed error information and verify Python version compatibility

## More Information

- Complete example code: Refer to `example.py`
- C++ API documentation: Refer to `../cpp/rysen_example.cpp`
- ROS2 interface: Refer to `../ros2/rysen_apexhand/README.md`