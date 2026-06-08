@echo off
:: 强制使用 UTF-8 编码，防止中文乱码
chcp 65001 >nul
setlocal enabledelayedexpansion

echo === 🦾 Rysen ApexHand SDK Windows 环境启动 ===

:: 默认按 amd64/x86_64 处理
set IMAGE_NAME=rysen_sdk:latest
set GHCR_IMAGE=ghcr.io/rysenrobotics/rysen_sdk:latest
set TAR_FILE=rysen_sdk_image.tar
set COMPOSE_FILE=docker/docker-compose.yml
set DOCKERFILE=docker/Dockerfile.rysen_sdk
set CONTAINER_NAME=rysen_sdk_env

:: 智能侦测：ARM64 架构
if /i "%PROCESSOR_ARCHITECTURE%"=="ARM64" (
    set IMAGE_NAME=rysen_sdk_arm64:latest
    set GHCR_IMAGE=ghcr.io/rysenrobotics/rysen_sdk_arm64:latest
    set TAR_FILE=rysen_sdk_arm64_image.tar
    set COMPOSE_FILE=docker\docker-compose.arm64.yml
    set DOCKERFILE=docker\Dockerfile.rysen_sdk.arm64
    set CONTAINER_NAME=rysen_sdk_arm64_env
)

echo 💻 检测到系统架构: %PROCESSOR_ARCHITECTURE%

echo 🔍 [1/4] 检查本地镜像...
docker image inspect %IMAGE_NAME% >nul 2>&1
if %errorlevel% equ 0 (
    echo ✅ 本地已存在镜像 '%IMAGE_NAME%'，跳过拉取/构建环节。
    goto :start_container
)

echo 🌐 [2/4] 尝试从 GHCR (GitHub) 拉取镜像...
docker pull %GHCR_IMAGE%
if %errorlevel% equ 0 (
    echo ✅ 从 GHCR 拉取成功！
    docker tag %GHCR_IMAGE% %IMAGE_NAME%
    goto :start_container
)
echo ⚠️ 从 GHCR 拉取失败 (可能是网络连接问题)。

echo 📦 [3/4] 尝试查找并加载离线镜像包...
if exist "%TAR_FILE%" (
    echo 找到离线包 '%TAR_FILE%'，正在加载...
    docker load -i "%TAR_FILE%"
    if !errorlevel! equ 0 (
        echo ✅ 离线包加载成功！
        goto :start_container
    ) else (
        echo ⚠️ 离线包加载失败 (文件可能损坏)，将尝试后备方案...
    )
) else (
    echo ⚠️ 当前目录下未找到离线包 '%TAR_FILE%'。
)

echo 🔨 [4/4] 尝试通过 Dockerfile 本地构建...
docker build -t %IMAGE_NAME% -f "%DOCKERFILE%" .
if %errorlevel% equ 0 (
    echo ✅ 本地构建成功！
    goto :start_container
)
echo ❌ 本地构建失败 (基础镜像拉取超时等原因)。
echo 👉 最终解决方案：请确保网络畅通，或前往 GitHub Releases 下载离线镜像包 '%TAR_FILE%' 放置在当前目录下后重试。
goto :error_end

:start_container
echo === 🚀 正在启动容器环境 ===
docker compose -f "%COMPOSE_FILE%" up -d
echo 🎉 容器启动完成！可以使用以下命令进入环境：
echo docker exec -it %CONTAINER_NAME% /bin/bash
goto :eof

:error_end
echo.
pause
exit /b 1