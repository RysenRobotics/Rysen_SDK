# Rysen ApexHand SDK C++ Examples

This directory provides C++ usage examples for the Rysen ApexHand SDK.

## Directory Structure

```
cpp/
├── CMakeLists.txt
├── README.md
└── rysen_example.cpp
```

## Prerequisites

1. **C++ Compiler**: GCC 9+ or Clang 10+ (supports C++17)
2. **CMake**: 3.10 or higher
3. **System Dependencies**:

   **Quick Installation (Recommended)**:
   ```bash
   # In the root directory of the repository
   chmod +x install_rysen_deps.sh
   ./install_rysen_deps.sh
   ```

   **Manual Installation**:
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

## Building

### Build Independently

```bash
cd cpp/
mkdir build && cd build
cmake ..
make
```

The executable file is located at `cpp/build/bin/rysen_example`.

## Running

```bash
cd cpp/build
./bin/rysen_example
```

## Example Code Explanation

`rysen_example.cpp` demonstrates the following features:

1. **Connection and Initialization**
   - Ethernet connection
   - Version verification

2. **Finger Control**
   - Enable/disable fingers
   - Set maximum finger torque

3. **Motion Parameter Configuration**
   - Set maximum joint velocity
   - Set maximum joint acceleration

4. **Callback Functions**
   - Joint state callback
   - Motor state callback
   - Hand tactile image callback

5. **Control Commands**
   - MoveJposition control (blocking)
   - GetJointStates, GetMotorStates, GetHandSensorImage

6. **Position Following Control**
   - MoveJPositionFollow (non-blocking)

## API Usage Examples

### Basic Usage Flow

```cpp
#include <rysen_apexhand_sdk.hpp>

// Create SDK instance
RysenApexHandSDK sdk;

// Connect to the robot
if (sdk.Connect("192.168.0.102", ConnectionType::ETHERNET)) {
    // Enable all fingers
    sdk.SetFingerEnabled({0, 1, 2, 3, 4}, true);
    
    // Set callback function
    sdk.SetJointStatesCallback([](const JointStates& states) {
        // Process joint states
    });
    
    // Execute control operations
    std::vector<uint8_t> joint_ids = {0, 1, 2};
    std::vector<double> positions = {0.5, 0.3, 0.2};
    std::vector<double> velocities = {1.0, 1.0, 1.0};
    std::vector<double> accelerations = {2.0, 2.0, 2.0};
    
    sdk.MoveJoint(joint_ids, positions, velocities, accelerations);
    
    // Disconnect
    sdk.Disconnect();
}
```

### Position Following Control

```cpp
// Non-blocking position following control
std::vector<uint8_t> joint_ids = {0, 1, 2};
std::vector<double> positions = {0.5, 0.3, 0.2};

sdk.MoveJPositionFollow(joint_ids, positions);
```

### Get States

```cpp
// Get joint states
JointStates joint_states;
if (sdk.GetJointStates(joint_states) == ErrorCode::ERROR_CODE_OK) {
    // Process joint states
}

// Get motor states
MotorStates motor_states;
if (sdk.GetMotorStates(motor_states) == ErrorCode::ERROR_CODE_OK) {
    // Process motor states
}

// Get tactile image
std::vector<uint16_t> tactile_image;
if (sdk.GetHandSensorImage(tactile_image) == ErrorCode::ERROR_CODE_OK) {
    // Process tactile image
}
```

## Connection Configuration

- **Connection Type**: Ethernet (`ConnectionType::ETHERNET`)
- **Default IP Address**: 192.168.0.102 (modifiable in code)

## Notes

1. **Real-time Kernel**: It is recommended to use a real-time kernel (PREEMPT_RT) for better control performance.

2. **Network Configuration**: 
   - Ensure the robot and host are on the same network
   - Firewall allows communication on relevant ports
   - Correct robot IP address

3. **Error Handling**: All API functions return `ErrorCode`; it is recommended to check the return values.

## Troubleshooting

### Compilation Errors

1. Check CMake version: `cmake --version`
2. Check compiler version: `g++ --version`
3. Ensure all dependent libraries are installed

### Runtime Errors

1. **Connection Failed**:
   - Check the robot IP address
   - Check network connection: `ping <Robot IP>`
   - Check if the port is occupied

2. **Permission Issues**:
   - Ensure network access permissions
   - Some operations may require root privileges (related to real-time kernel)

## More Information

- Complete example code: Refer to `rysen_example.cpp`
- Python API: Refer to `../python/README.md`
- ROS2 Interface: Refer to `../ros2/rysen_apexhand/README.md`
- Main example directory: Refer to `../README.md`