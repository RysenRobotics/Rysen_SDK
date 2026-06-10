@echo off
setlocal enabledelayedexpansion

:: Run from SDK root (parent of scripts/)
cd /d "%~dp0\.."

echo === Rysen ApexHand SDK Windows Environment Startup ===

:: Default: amd64 / x86_64
set IMAGE_NAME=rysen_sdk:latest
set GHCR_IMAGE=ghcr.io/rysenrobotics/rysen_sdk:latest
set TAR_FILE=rysen_sdk_amd64_image.tar
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
    echo Local image '%IMAGE_NAME%' found.
    goto :start_container
)

echo [2/4] Looking for offline image tarball...
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
    echo Offline tarball '%TAR_FILE%' not found. Skipping.
)

echo [3/4] Pulling image from GHCR...
docker pull %GHCR_IMAGE%
if !errorlevel! equ 0 (
    echo Pull from GHCR succeeded.
    docker tag %GHCR_IMAGE% %IMAGE_NAME%
    goto :start_container
)
echo WARNING: GHCR pull failed (network issue?).

echo [4/4] Building image from Dockerfile...
docker build -t %IMAGE_NAME% -f "%DOCKERFILE%" .
if !errorlevel! equ 0 (
    echo Local build succeeded.
    goto :start_container
)
echo ERROR: Local build failed.
echo Please ensure network access, or download '%TAR_FILE%' from GitHub Releases.
goto :error_end

:start_container
echo === Starting container environment ===
docker compose -f "%COMPOSE_FILE%" up -d
if !errorlevel! neq 0 (
    echo ERROR: docker compose failed. Try: docker-compose -f "%COMPOSE_FILE%" up -d
    goto :error_end
)

echo [Info] Auto-fixing Linux symlinks for Windows host...
docker exec %CONTAINER_NAME% /bin/bash -c "cd /workspace/rysen_sdk/lib/x86_64 2>/dev/null && rm -f librysen_sdk.so librysen_sdk.so.1 && ln -s librysen_sdk.so.1.* librysen_sdk.so.1 && ln -s librysen_sdk.so.1 librysen_sdk.so"
docker exec %CONTAINER_NAME% /bin/bash -c "cd /workspace/rysen_sdk/lib/aarch64 2>/dev/null && rm -f librysen_sdk.so librysen_sdk.so.1 && ln -s librysen_sdk.so.1.* librysen_sdk.so.1 && ln -s librysen_sdk.so.1 librysen_sdk.so"

echo.
echo ===================================================
echo SUCCESS: Environment is READY!
echo ===================================================
echo Enter the environment with:
echo docker exec -it %CONTAINER_NAME% /bin/bash
echo ===================================================
pause
goto :eof

:error_end
echo.
echo ===================================================
echo FATAL ERROR: Environment setup FAILED!
echo ===================================================
pause
exit /b 1
