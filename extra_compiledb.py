"""
PlatformIO extra script to generate compile_commands.json in each environment's build directory.
This script configures PlatformIO to place compile_commands.json files in .pio/build/[env]/.
"""
import os
Import("env")

# Place compile_commands.json in the build directory of each environment
env.Replace(COMPILATIONDB_PATH=os.path.join("$BUILD_DIR", "compile_commands.json"))
env.Replace(COMPILATIONDB_INCLUDE_TOOLCHAIN=True)
