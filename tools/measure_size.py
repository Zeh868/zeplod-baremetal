#!/usr/bin/env python3
"""tools/measure_size.py — parse size output and generate resource budget table"""
import subprocess
import sys

def measure(name, cmake_args):
    build_dir = f"build_measure_{name}"
    cmd = ["cmake", "-B", build_dir, f"-DBOARD=native_sim"] + cmake_args
    subprocess.run(cmd, check=True)
    subprocess.run(["cmake", "--build", build_dir], check=True)
    result = subprocess.run(["size", f"{build_dir}/test_skeleton"],
                            capture_output=True, text=True)
    print(f"=== {name} ===")
    print(result.stdout)

if __name__ == "__main__":
    configs = [
        ("ultra",          ["-DBM_ULTRA_MODE=1"]),
        ("core",           ["-DBM_ENABLE_MODULE=OFF", "-DBM_ENABLE_WDG=OFF"]),
        ("core+module",    ["-DBM_ENABLE_MODULE=ON",  "-DBM_ENABLE_WDG=OFF"]),
        ("core+module+wdg", ["-DBM_ENABLE_MODULE=ON", "-DBM_ENABLE_WDG=ON"]),
    ]
    for name, args in configs:
        measure(name, args)
