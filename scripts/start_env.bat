@echo off
setlocal enabledelayedexpansion

:: Run from SDK root (parent of scripts/)
cd /d "%~dp0\.."

echo === Rysen ApexHand SDK Windows Environment Startup ===

:: Default: amd64 / x86_64
set IMAGE_NAME=rysen_sdk:latest
set GHCR_IMAGE=ghcr.io/rysenrobotics/rysen_sdk:latest
set TAR_FILE=rysen_sdk_image.tar
set COMPOSE_FILE=docker\docker-compose.yml
set DOCKERFILE=docker\Dockerfile.rysen_sdk
set CONTAINER_NAME=rysen_sdk_env

:: ARM64 detection
if /i "%PROCESSOR_ARCHITECTURE%"=="ARM64" (
    set IMAGE_NAME=rysen_sdk_arm64:latest
    set GHCR_IMAGE=ghcr.io/rysenrobotics/rysen_sdk_arm64:latest
    set TAR_FILE=rysen_sdk_arm64_image.tar
    set COMPOSE_FILE=docker\docker-compose.arm64.yml
    set DOCKERFILE=docker\Dockerfile.rysen_sdk.arm64
    set CONTAINER_NAME=rysen_sdk_arm64_env
)

echo Detected architecture: %PROCESSOR_ARCHITECTURE%

echo [1/4] Checking local image...
docker image inspect %IMAGE_NAME% >nul 2>&1
if !errorlevel! equ 0 (
    echo Local image '%IMAGE_NAME%' found, skipping pull/build.
    goto :start_container
)

echo [2/4] Pulling image from GHCR...
docker pull %GHCR_IMAGE%
if !errorlevel! equ 0 (
    echo Pull from GHCR succeeded.
    docker tag %GHCR_IMAGE% %IMAGE_NAME%
    goto :start_container
)
echo WARNING: GHCR pull failed (network issue?).

echo [3/4] Looking for offline image tarball...
if exist "%TAR_FILE%" (
    echo Found '%TAR_FILE%', loading...
    docker load -i "%TAR_FILE%"
    if !errorlevel! equ 0 (
        echo Offline image loaded successfully.
        goto :start_container
    ) else (
        echo WARNING: Failed to load tarball, trying fallback...
    )
) else (
    echo WARNING: Offline tarball '%TAR_FILE%' not found in SDK root.
)

echo [4/4] Building image from Dockerfile...
docker build -t %IMAGE_NAME% -f "%DOCKERFILE%" .
if !errorlevel! equ 0 (
    echo Local build succeeded.
    goto :start_container
)
echo ERROR: Local build failed (base image pull timeout, etc.).
echo Please ensure network access, or download '%TAR_FILE%' from GitHub Releases
echo and place it in the SDK root directory, then retry.
goto :error_end

:start_container
echo === Starting container environment ===
docker compose -f "%COMPOSE_FILE%" up -d
if !errorlevel! neq 0 (
    echo ERROR: docker compose failed. Try: docker-compose -f "%COMPOSE_FILE%" up -d
    goto :error_end
)

:: ================= 自动化无痕修复软链接 =================
echo [Info] Auto-fixing Linux symlinks for Windows host...
:: 修复 x86_64 目录
docker exec %CONTAINER_NAME% /bin/bash -c "cd /workspace/rysen_sdk/lib/x86_64 2>/dev/null && rm -f librysen_sdk.so librysen_sdk.so.1 && ln -s librysen_sdk.so.1.* librysen_sdk.so.1 && ln -s librysen_sdk.so.1 librysen_sdk.so"
:: 修复 aarch64 目录
docker exec %CONTAINER_NAME% /bin/bash -c "cd /workspace/rysen_sdk/lib/aarch64 2>/dev/null && rm -f librysen_sdk.so librysen_sdk.so.1 && ln -s librysen_sdk.so.1.* librysen_sdk.so.1 && ln -s librysen_sdk.so.1 librysen_sdk.so"
:: ===================================================================

echo [Success] Container started. Enter the environment with:
echo    docker exec -it %CONTAINER_NAME% /bin/bash
goto :eof

:error_end
echo.
pause
exit /b 1