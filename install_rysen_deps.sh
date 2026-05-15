#!/bin/bash
###############################################################################
# Rysen SDK 开发环境依赖安装脚本
# 支持 Ubuntu  22.04 (x86_64 / ARM / 树莓派)
# 安装内容：
#   - 基础构建工具 (gcc/g++, cmake, make)
#   - 网络通信 (Boost.Asio)
#   - 串口通信 (libserialport)
#   - 日志系统 (spdlog)
#   - 其他实用库 (fmt, yaml-cpp)
###############################################################################

set -e  # 出错时退出
echo "📦 开始安装 Rysen SDK 依赖库..."
sudo apt update

echo "🚀 安装基础构建工具..."
sudo apt install -y build-essential cmake pkg-config git

echo "🌐 安装网络通信库 (Boost.Asio)..."
sudo apt install -y libboost-all-dev

echo "🔌 安装串口通信库..."
sudo apt install -y libserialport-dev

echo "🧩 安装日志库 spdlog 和 fmt..."
sudo apt install -y libspdlog-dev libfmt-dev

echo "🧾 安装配置文件解析库 yaml-cpp..."
sudo apt install -y libyaml-cpp-dev

echo "🧰 可选: 安装 gdb, valgrind 等调试工具"
sudo apt install -y gdb valgrind

echo "🤖 可选: 安装 URDF 相关库 (用于机器人描述)"
sudo apt install -y liburdfdom-dev liburdfdom-headers-dev libtinyxml2-dev

echo "🤖 可选: 安装 clang-format"
sudo apt install -y clang-format

echo "✅ 所有依赖安装完成！"
echo "---------------------------------------------------------"
echo " 包含内容："
echo "  - build-essential, cmake, pkg-config"
echo "  - Boost (Asio / Thread / System / Filesystem)"
echo "  - libserialport (串口通信)"
echo "  - spdlog (日志系统)"
echo "  - fmt (格式化输出)"
echo "  - yaml-cpp (参数配置)"
echo "---------------------------------------------------------"
echo "🌟 现在可以编译 Rysen SDK 项目了！"
