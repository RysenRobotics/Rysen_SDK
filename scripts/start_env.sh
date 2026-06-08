#!/bin/bash

# 1. 架构侦测
ARCH=$(uname -m)
echo "=== 🦾 Rysen ApexHand SDK Linux/macOS 环境启动 ==="
echo "💻 检测到系统架构: $ARCH"

# 2. 根据架构动态分配变量
if [[ "$ARCH" == "aarch64" || "$ARCH" == "arm64" ]]; then
    IMAGE_NAME="rysen_sdk_arm64:latest"
    GHCR_IMAGE="ghcr.io/rysenrobotics/rysen_sdk_arm64:latest"
    TAR_FILE="rysen_sdk_arm64_image.tar"
    COMPOSE_FILE="docker/docker-compose.arm64.yml"
    DOCKERFILE="docker/Dockerfile.rysen_sdk.arm64"
    CONTAINER_NAME="rysen_sdk_arm64_env"
else
    IMAGE_NAME="rysen_sdk:latest"
    GHCR_IMAGE="ghcr.io/rysenrobotics/rysen_sdk:latest"
    TAR_FILE="rysen_sdk_image.tar"
    COMPOSE_FILE="docker/docker-compose.yml"
    DOCKERFILE="docker/Dockerfile.rysen_sdk"
    CONTAINER_NAME="rysen_sdk_env"
fi

# 3. 执行多级获取策略
if docker image inspect "$IMAGE_NAME" >/dev/null 2>&1; then
    echo "✅ [1/4] 本地已存在镜像 '$IMAGE_NAME'，跳过拉取/构建环节。"
else
    echo "🔍 [1/4] 本地未找到镜像 '$IMAGE_NAME'。"
    
    echo "🌐 [2/4] 尝试从 GHCR (GitHub) 拉取镜像..."
    if docker pull "$GHCR_IMAGE"; then
        echo "✅ 从 GHCR 拉取成功！"
        docker tag "$GHCR_IMAGE" "$IMAGE_NAME"
    else
        echo "⚠️ 从 GHCR 拉取失败 (可能是网络连接问题)。"
        
        echo "📦 [3/4] 尝试查找并加载离线镜像包..."
        TAR_LOADED=false
        if [ -f "$TAR_FILE" ]; then
            echo "找到离线包 '$TAR_FILE'，正在加载..."
            if docker load -i "$TAR_FILE"; then
                 echo "✅ 离线包加载成功！"
                 TAR_LOADED=true
            else
                 echo "⚠️ 离线包加载失败 (文件可能损坏)，将尝试后备方案..."
            fi
        else
            echo "⚠️ 当前目录下未找到离线包 '$TAR_FILE'。"
        fi

        # 如果 tar 包未加载成功，最后尝试构建
        if [ "$TAR_LOADED" = false ]; then
            echo "🔨 [4/4] 尝试通过 Dockerfile 本地构建..."
            if docker build -t "$IMAGE_NAME" -f "$DOCKERFILE" .; then
                echo "✅ 本地构建成功！"
            else
                echo "❌ 本地构建失败 (基础镜像拉取超时等原因)。"
                echo "👉 最终解决方案：请确保网络畅通，或前往 GitHub Releases 下载离线镜像包放置在当前目录下后重试。"
                exit 1
            fi
        fi
    fi
fi

echo "=== 🚀 正在启动容器环境 ==="
docker compose -f "$COMPOSE_FILE" up -d

echo "🎉 容器启动完成！可以使用以下命令进入环境："
echo "docker exec -it $CONTAINER_NAME /bin/bash"