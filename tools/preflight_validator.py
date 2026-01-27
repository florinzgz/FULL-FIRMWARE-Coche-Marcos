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

Simple, conservative, build-time only.
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
        "forbidden": ["Wire."]
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


def strip_comments(line: str, in_block_comment: bool):
    # handle block comments
    if in_block_comment:
        if "*/" in line:
            return strip_comments(line.split("*/", 1)[1], False), False
        return "", True

    if "/*" in line:
        before, after = line.split("/*", 1)
        cleaned, new_state = strip_comments(after, True)
        return before, new_state

    # remove // comments
    line = re.sub(r'//.*', '', line)
    # remove strings
    line = re.sub(r'"[^"]*"', '', line)
    return line.strip(), False


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
    brace_depth = 0
    in_block_comment = False
    init_state = {k: False for k in FORBIDDEN_RULES.keys()}

    for lineno, raw_line in enumerate(lines, 1):
        line, in_block_comment = strip_comments(raw_line, in_block_comment)
        if not line:
            continue

        # detect setup()
        if not in_setup and re.match(r'\s*void\s+setup\s*\(', line):
            in_setup = True
            brace_depth = 0
            continue

        if in_setup:
            brace_depth += line.count("{")
            brace_depth -= line.count("}")
            if brace_depth <= 0 and "}" in line:
                in_setup = False
                continue

        scope = "setup()" if in_setup else "global"

        # skip declarations
        if line.endswith(";") and "(" not in line:
            continue

        for resource, rule in FORBIDDEN_RULES.items():
            # detect init
            for init_call in rule["init"]:
                if init_call in line:
                    init_state[resource] = True

            # detect forbidden usage before init
            if not init_state[resource]:
                for forbidden in rule["forbidden"]:
                    if forbidden in line and "(" in line:
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
