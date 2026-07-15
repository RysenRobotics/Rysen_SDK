"""
Setup script for Rysen SDK Python bindings

This setup.py installs pre-compiled Python bindings (.so files) and Python SDK files.
No compilation is required - just install the pre-built package.

Usage:
    pip install -e .
    or
    pip install .

Copyright (c) 2024-2026, Rysen Robotics (Shenzhen) Co. Ltd.
All rights reserved.

Use of this source code is governed by a BSD 3-Clause license that can be
found in the LICENSE file.
"""

from setuptools import setup
import os
import shutil
import sys  

# Get the directory containing this file (python/)
here = os.path.abspath(os.path.dirname(__file__))
# 定义源路径 (仓库根目录的 rysen_sdk/lib) 和目标路径 (当前的 python/lib)
sdk_lib_src = os.path.abspath(os.path.join(here, "..", "rysen_sdk", "lib"))
local_lib_dst = os.path.join(here, "lib")

# 检查当前执行的命令是否是为了打包发布 (bdist_wheel) 或构建源码包 (sdist)
# 如果是 pip install -e .，sys.argv 里面会有 'develop' 或 'egg_info'
is_packaging = any(arg in sys.argv for arg in ['bdist_wheel', 'sdist'])

# 仅在真实打包时，且源库存在时，才进行拷贝
if is_packaging and os.path.exists(sdk_lib_src):
    print(f"📦 打包模式：正在从统一 SDK 目录同步动态库用于构建 Wheel 包...")
    if os.path.exists(local_lib_dst):
        shutil.rmtree(local_lib_dst) # 清理旧的
    shutil.copytree(sdk_lib_src, local_lib_dst) # 拷贝新的
else:
    print(f"🔧 开发/元数据模式：跳过同步动态库...")

# Read version from VERSION file (single source of truth), fallback to __init__.py, then default
version = "1.4.7"  # Default fallback version (updated by update_version.sh)
try:
    # First try to read from VERSION file (project root)
    version_file = os.path.join(here, "..", "..", "VERSION")
    if os.path.exists(version_file):
        with open(version_file, "r") as f:
            version = f.read().strip()
    else:
        # Fallback to __init__.py
        init_file = os.path.join(here, "__init__.py")
        if os.path.exists(init_file):
            with open(init_file, "r") as f:
                for line in f:
                    if "__version__" in line:
                        # Extract version from: __version__ = "0.1.0"
                        version = line.split("=")[1].strip().strip('"').strip("'")
                        break
except Exception:
    pass

# Read long description from README if available
long_description = ""
readme_path = os.path.join(here, "README.md")
if os.path.exists(readme_path):
    with open(readme_path, "r", encoding="utf-8") as f:
        long_description = f.read()

setup(
    name="rysen-sdk",
    version=version,
    author="Rysen",
    author_email="support@rysen.com",
    description="Python bindings for Rysen SDK - Robotic hand control library",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://todo.com/rysen/rysen_sdk",
    # Package structure:
    # - python/__init__.py (package entry point)
    # - python/rysen_apexhand_sdk.py (main Python SDK module)
    # - python/lib/ (pre-compiled shared objects for SDK + bindings)
    package_dir={"rysen_sdk": "."},
    packages=["rysen_sdk"] if os.path.exists(os.path.join(here, "__init__.py")) else [],
    py_modules=["rysen_apexhand_sdk"] if os.path.exists(os.path.join(here, "rysen_apexhand_sdk.py")) else [],
    # Include pre-compiled .so files in the package
    # These patterns will match all Python version-specific .so files
    package_data={
        "": [ # 使用空字符串匹配根目录下的数据文件
            "lib/x86_64/*.so",
            "lib/aarch64/*.so",
            "lib/*/*.pyd",  # Windows 匹配
            "lib/*/*.dll",
            "lib/*/*.dylib",
        ],
    },
    # Include .so files even if they're not in package_data
    include_package_data=True,
    zip_safe=False,  # .so files cannot be in zip
    python_requires=">=3.8",
    # No build dependencies needed - using pre-compiled .so files
    install_requires=[],
    extras_require={
        "dev": [
            "pytest>=6.0",
            "pytest-cov>=2.0",
        ],
    },
    classifiers=[
        "Development Status :: 4 - Beta",
        "Intended Audience :: Developers",
        "Intended Audience :: Science/Research",
        "License :: OSI Approved :: MIT License",
        "Operating System :: POSIX :: Linux",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: Python :: 3.12",
        "Topic :: Scientific/Engineering",
        "Topic :: Software Development :: Libraries :: Python Modules",
    ],
    keywords="robotics, robotic-hand, control, sdk",
)
