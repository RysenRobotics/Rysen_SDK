# Rysen SDK Docker 开发环境

Docker 开发环境配置，用于构建和运行 Rysen SDK 示例程序。

## 环境说明

### 基础环境

- **操作系统**: Ubuntu 22.04
- **ROS2**: Humble
- **时区**: Asia/Shanghai

**注意**: Docker 镜像已包含所有依赖库，无需手动安装。如果在本机环境使用，请先运行 `../install_rysen_deps.sh` 安装依赖。

### 包含组件

#### 构建工具
- CMake, GCC, Git
- build-essential, pkg-config

#### 依赖库
- Boost (网络通信)
- libserialport (串口通信)
- spdlog, fmt (日志和格式化)
- yaml-cpp (配置文件解析)
- urdfdom (URDF 支持)

#### 开发工具
- clang-format (代码格式化)
- gdb, valgrind (调试工具)

#### ROS2 工具
- colcon (ROS2 构建工具)
- rosdep (依赖管理)
- vcstool (版本控制工具)

## 使用方式

### 快速开始

```bash
# 进入 docker 目录
cd docker

# 1. 构建镜像（必须先构建）
docker build -f Dockerfile.rysen_sdk -t rysen_sdk:latest ..

# 2. 启动容器
docker compose up -d

# 3. 进入容器
docker compose exec rysen_sdk bash
```

### 详细步骤

#### 1. 构建镜像

**手动构建（推荐）**:
```bash
cd docker
docker build -f Dockerfile.rysen_sdk -t rysen_sdk:latest ..
```

**使用 Docker Compose 构建**:
```bash
cd docker
docker compose build
```

#### 2. 启动容器

**使用 Docker Compose**:
```bash
cd docker
docker compose up -d
```

**手动运行**:
```bash
cd docker
docker run -it --rm \
  -v $(pwd)/..:/workspace \
  -v rysen_sdk_build:/workspace/build \
  --name rysen_sdk_env \
  rysen_sdk:latest
```

#### 3. 进入容器

```bash
docker compose exec rysen_sdk bash
```

#### 4. 使用示例程序

进入容器后，工作目录为 `/workspace`（对应 examples 目录）：

**C++ 示例**:
```bash
cd /workspace/cpp
mkdir build && cd build
cmake ..
make
./rysen_example
```

**Python 示例**:
```bash
cd /workspace/python
pip install -e .
python example.py
```

**ROS2 示例**:
```bash
cd /workspace/ros2
colcon build
source install/setup.bash
ros2 launch rysen_apexhand rysen_apexhand.launch.py
```

### 停止容器

```bash
docker compose down
```

### 清理

```bash
# 停止并删除容器
docker compose down

# 删除镜像
docker rmi rysen_sdk:latest

# 删除构建缓存卷（可选）
docker volume rm build_cache
```

## 文件说明

- `Dockerfile.rysen_sdk` - Docker 镜像定义文件
- `docker-compose.yml` - Docker Compose 配置文件
