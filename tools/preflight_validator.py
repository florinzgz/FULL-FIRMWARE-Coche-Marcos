#!/usr/bin/env python3
"""
Preflight Hardware Initialization Validator (Safe Version)

Checks for illegal hardware usage BEFORE initialization in:
- global scope
- setup()

Resources checked:
- I2C (Wire)
- SPI
- TFT (tft)

This script is intentionally simple and conservative:
- No call graph
- No regex heuristics
- No ESP-IDF
- No runtime impact
"""

Import("env")

import os
import re
import sys
from pathlib import Path

PROJECT_DIR = env.subst("$PROJECT_DIR")
SRC_DIRS = [
    os.path.join(PROJECT_DIR, "src"),
    os.path.join(PROJECT_DIR, "include"),
]

FORBIDDEN_RULES = {
    "I2C": {
        "init": ["Wire.begin"],
        "forbidden": ["Wire.", "i2c_"]
    },
    "SPI": {
        "init": ["SPI.begin"],
        "forbidden": ["SPI."]
    },
    "TFT": {
        "init": ["tft.begin"],
        "forbidden": ["tft.", "sprite."]
    }
}

violations = []

def strip_comments(line: str) -> str:
    # Remove inline comments
    line = re.sub(r'//.*', '', line)
    # Remove strings
    line = re.sub(r'"[^"]*"', '', line)
    return line.strip()


def get_source_files():
    files = []
    for d in SRC_DIRS:
        if os.path.isdir(d):
            files.extend(Path(d).rglob("*.cpp"))
            files.extend(Path(d).rglob("*.h"))
    return files


def analyze_file(filepath: Path):
    with open(filepath, "r", encoding="utf-8", errors="ignore") as f:
        lines = f.readlines()

    in_setup = False
    init_state = {k: False for k in FORBIDDEN_RULES.keys()}

    for lineno, raw_line in enumerate(lines, 1):
        line = strip_comments(raw_line)

        if not line:
            continue

        # detect setup()
        if re.match(r'\s*void\s+setup\s*\(', line):
            in_setup = True
            continue

        if in_setup and "}" in line:
            in_setup = False

        scope = "setup()" if in_setup else "global"

        for resource, rule in FORBIDDEN_RULES.items():
            # detect init
            for init_call in rule["init"]:
                if init_call in line:
                    init_state[resource] = True

            # detect forbidden usage before init
            if not init_state[resource]:
                for forbidden in rule["forbidden"]:
                    if forbidden in line:
                        violations.append({
                            "file": str(filepath),
                            "line": lineno,
                            "resource": resource,
                            "scope": scope,
                            "code": raw_line.strip()
                        })


def main():
    print("=" * 70)
    print("🔍 Preflight Hardware Validator (Safe Mode)")
    print("=" * 70)

    files = get_source_files()
    print(f"Scanning {len(files)} source files...\n")

    for f in files:
        analyze_file(f)

    if not violations:
        print("✅ VALIDATION PASSED")
        print("No illegal hardware usage detected.\n")
        return

    print("❌ HARDWARE INITIALIZATION VIOLATIONS DETECTED\n")

    for v in violations:
        print(f"Resource: {v['resource']}")
        print(f"File: {v['file']}")
        print(f"Line: {v['line']}")
        print(f"Scope: {v['scope']}")
        print(f"Code: {v['code']}")
        print("-" * 60)

    print(f"\n❌ BUILD BLOCKED: {len(violations)} violation(s)")
    sys.exit(1)


main()
