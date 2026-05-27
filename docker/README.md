# Rysen SDK Docker Development Environment

Docker development environment configuration for building and running Rysen SDK sample programs.

## Environment Description

### Basic Environment
- **Operating System**: Ubuntu 22.04
- **ROS2**: Humble
- **Time Zone**: Asia/Shanghai

**Note**: The Docker image includes all dependent libraries, so manual installation is not required. If using the native environment, run `../install_rysen_deps.sh` first to install dependencies.

### Included Components
#### Build Tools
- CMake, GCC, Git
- build-essential, pkg-config

#### Dependent Libraries
- Boost (Network Communication)
- libserialport (Serial Port Communication)
- spdlog, fmt (Logging and Formatting)
- yaml-cpp (Configuration File Parsing)
- urdfdom (URDF Support)

#### Development Tools
- clang-format (Code Formatting)
- gdb, valgrind (Debugging Tools)

#### ROS2 Tools
- colcon (ROS2 Build Tool)
- rosdep (Dependency Management)
- vcstool (Version Control Tool)

## Usage

### Quick Start
```bash
# Enter the docker directory
cd docker

# 1. Build the image (must be built first)
docker build -f Dockerfile.rysen_sdk -t rysen_sdk:latest ..

# 2. Start the container
docker compose up -d

# 3. Enter the container
docker compose exec rysen_sdk bash
```

### Detailed Steps

#### 1. Build the Image
**Manual Build (Recommended)**:
```bash
cd docker
docker build -f Dockerfile.rysen_sdk -t rysen_sdk:latest ..
```

**Build with Docker Compose**:
```bash
cd docker
docker compose build
```

#### 2. Start the Container
**Start with Docker Compose**:
```bash
cd docker
docker compose up -d
```

**Manual Run**:
```bash
cd docker
docker run -it --rm \
  -v $(pwd)/..:/workspace \
  -v rysen_sdk_build:/workspace/build \
  --name rysen_sdk_env \
  rysen_sdk:latest
```

#### 3. Enter the Container
```bash
docker compose exec rysen_sdk bash
```

#### 4. Use Example Programs
After entering the container, the working directory is `/workspace` (corresponding to the repository root directory):

**C++ Example**:
```bash
cd /workspace/cpp
mkdir build && cd build
cmake ..
make
./bin/rysen_example
```

**Python Example**:
```bash
cd /workspace/python
pip install -e .
python example.py
```

**ROS2 Example**:
```bash
cd /workspace/ros2
colcon build
source install/setup.bash
ros2 launch rysen_apexhand rysen_apexhand.launch.py
```

### Stop the Container
```bash
docker compose down
```

### Cleanup
```bash
# Stop and remove the container
docker compose down

# Remove the image
docker rmi rysen_sdk:latest

# Remove the build cache volume (optional)
docker volume rm build_cache
```

## File Description
- `Dockerfile.rysen_sdk` - Docker image definition file
- `docker-compose.yml` - Docker Compose configuration file