#!/usr/bin/env python
"""
Pre-build script to ensure required Python dependencies are installed.
This script is executed before the build process starts.
"""

Import("env")
import subprocess
import sys
import importlib.util

def install_package(package_name):
    """Install a Python package if not already installed."""
    if importlib.util.find_spec(package_name) is not None:
        print(f"✓ {package_name} is already installed")
    else:
        print(f"Installing {package_name}...")
        try:
            subprocess.check_call([sys.executable, "-m", "pip", "install", package_name])
            print(f"✓ {package_name} installed successfully")
        except subprocess.CalledProcessError as e:
            print(f"✗ Failed to install {package_name}: {e}")
            sys.exit(1)

# Install required dependencies for ESP32-S3 platform 6.12.0+
print("Checking ESP32 build dependencies...")
install_package("intelhex")
