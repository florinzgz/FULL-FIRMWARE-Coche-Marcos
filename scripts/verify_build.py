#!/usr/bin/env python3
"""
Build Verification Script v1.0
ESP32-S3 Car Control Firmware Build Integrity Checker

This script validates the build system and checks for common C++ issues that
could lead to silent compilation failures or linker errors.

Usage:
    python3 scripts/verify_build.py

Checks performed:
- Static member definitions match declarations
- Header guards are present
- Extern declarations are documented
- CPP files have corresponding headers
- Build output integrity (.o files exist)
"""

import os
import re
import sys
from pathlib import Path
from typing import List, Tuple, Set

class Colors:
    """ANSI color codes for terminal output"""
    RED = '\033[91m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    CYAN = '\033[96m'
    RESET = '\033[0m'
    BOLD = '\033[1m'

class BuildVerifier:
    def __init__(self, project_root: str):
        self.project_root = Path(project_root)
        self.errors: List[str] = []
        self.warnings: List[str] = []
        self.info: List[str] = []
        
    def log_error(self, msg: str):
        """Log an error message"""
        self.errors.append(msg)
        
    def log_warning(self, msg: str):
        """Log a warning message"""
        self.warnings.append(msg)
        
    def log_info(self, msg: str):
        """Log an info message"""
        self.info.append(msg)
    
    def check_static_members(self) -> bool:
        """
        Check that all static members declared in headers have definitions in cpp files.
        This is a common source of linker errors.
        """
        print(f"\n{Colors.CYAN}=== Checking Static Member Definitions ==={Colors.RESET}")
        
        header_files = list(self.project_root.glob("include/*.h"))
        checked_count = 0
        
        for header in header_files:
            # Find corresponding cpp file
            cpp_name = header.stem + ".cpp"
            cpp_files = list(self.project_root.glob(f"src/**/{cpp_name}"))
            
            if not cpp_files:
                continue  # No cpp file, might be header-only
                
            cpp_file = cpp_files[0]
            
            # Parse header for static member declarations
            with open(header, 'r', encoding='utf-8', errors='ignore') as f:
                header_content = f.read()
            
            # Find class name
            class_match = re.search(r'class\s+(\w+)\s*{', header_content)
            if not class_match:
                continue
            
            class_name = class_match.group(1)
            
            # Find static member variables (not methods)
            # Pattern: static TYPE NAME; (inside class)
            # Exclude methods: static TYPE NAME(...);
            static_pattern = r'static\s+(?:const\s+)?(\w+(?:<[^>]+>)?)\s+(\w+)\s*;'
            
            # Extract class body
            class_start = header_content.find('{', class_match.start())
            if class_start == -1:
                continue
            
            brace_count = 1
            class_end = class_start + 1
            while class_end < len(header_content) and brace_count > 0:
                if header_content[class_end] == '{':
                    brace_count += 1
                elif header_content[class_end] == '}':
                    brace_count -= 1
                class_end += 1
            
            class_body = header_content[class_start:class_end]
            
            # Find all static members in class body
            static_members = re.findall(static_pattern, class_body)
            
            if static_members:
                checked_count += len(static_members)
                # Check if they're defined in cpp
                with open(cpp_file, 'r', encoding='utf-8', errors='ignore') as f:
                    cpp_content = f.read()
                
                for member_type, member_name in static_members:
                    # Skip constexpr members (defined in header)
                    if f'constexpr' in class_body and member_name in class_body:
                        continue
                    
                    # Look for definition like: Type ClassName::memberName = ...;
                    definition_pattern = rf'\b{member_type}\s+{class_name}::{member_name}\s*='
                    if not re.search(definition_pattern, cpp_content, re.MULTILINE):
                        # Check if it's a template or inline definition
                        inline_pattern = rf'static\s+{member_type}\s+{member_name}\s*='
                        if not re.search(inline_pattern, class_body):
                            self.log_warning(
                                f"{header.name}: Static member '{member_name}' of type '{member_type}' "
                                f"may not be defined in {cpp_file.name}"
                            )
        
        print(f"  Checked {checked_count} static member declarations")
        return True
    
    def check_extern_declarations(self) -> bool:
        """
        Check extern declarations and document them.
        Extern declarations should have corresponding definitions somewhere.
        """
        print(f"\n{Colors.CYAN}=== Checking Extern Declarations ==={Colors.RESET}")
        
        cpp_files = list(self.project_root.glob("src/**/*.cpp"))
        extern_count = 0
        
        for cpp_file in cpp_files:
            with open(cpp_file, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
            
            # Find extern declarations
            extern_pattern = r'extern\s+(\w+(?:\s*<[^>]+>)?)\s+(\w+);'
            externs = re.findall(extern_pattern, content)
            
            for extern_type, extern_name in externs:
                extern_count += 1
                self.log_info(f"{cpp_file.name}: extern {extern_type} {extern_name}")
        
        print(f"  Found {extern_count} extern declarations")
        return True
    
    def check_header_guards(self) -> bool:
        """
        Check that all headers have proper include guards.
        Missing guards can cause duplicate definition errors.
        """
        print(f"\n{Colors.CYAN}=== Checking Header Guards ==={Colors.RESET}")
        
        header_files = list(self.project_root.glob("include/*.h"))
        missing_guards = 0
        
        for header in header_files:
            with open(header, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
            
            # Check for #pragma once or #ifndef guards
            has_pragma = '#pragma once' in content
            guard_pattern = rf'#ifndef\s+{header.stem.upper()}_H'
            has_ifndef = re.search(guard_pattern, content, re.IGNORECASE)
            
            if not has_pragma and not has_ifndef:
                self.log_error(f"{header.name}: Missing header guard (use #pragma once or #ifndef)")
                missing_guards += 1
        
        if missing_guards == 0:
            print(f"  {Colors.GREEN}✓{Colors.RESET} All {len(header_files)} headers have guards")
        return missing_guards == 0
    
    def check_cpp_has_header(self) -> bool:
        """
        Check that all cpp files have corresponding headers where expected.
        """
        print(f"\n{Colors.CYAN}=== Checking CPP/Header Pairs ==={Colors.RESET}")
        
        cpp_files = list(self.project_root.glob("src/**/*.cpp"))
        missing_headers = []
        
        for cpp_file in cpp_files:
            # Skip test files and main.cpp
            if 'test' in cpp_file.name.lower() or cpp_file.name == 'main.cpp':
                continue
            
            header_name = cpp_file.stem + ".h"
            header_path = self.project_root / "include" / header_name
            
            if not header_path.exists():
                missing_headers.append(cpp_file.name)
                self.log_info(f"{cpp_file.name} has no header in include/")
        
        if missing_headers:
            print(f"  {len(missing_headers)} cpp files without headers (may be intentional)")
        else:
            print(f"  {Colors.GREEN}✓{Colors.RESET} All non-test cpp files have headers")
        
        return True
    
    def check_build_artifacts(self) -> bool:
        """
        Check that .o files exist for all .cpp files in the last build.
        This helps detect silent compilation failures.
        """
        print(f"\n{Colors.CYAN}=== Checking Build Artifacts (.o files) ==={Colors.RESET}")
        
        # Find the most recent build directory
        build_dirs = list(self.project_root.glob(".pio/build/*/src"))
        if not build_dirs:
            print(f"  {Colors.YELLOW}⚠{Colors.RESET} No build directory found - run 'pio run' first")
            return True
        
        build_dir = build_dirs[0].parent
        src_cpp_files = list(self.project_root.glob("src/**/*.cpp"))
        missing_objects = []
        
        for cpp_file in src_cpp_files:
            # Construct expected .o file path
            rel_path = cpp_file.relative_to(self.project_root / "src")
            obj_path = build_dir / "src" / f"{rel_path}.o"
            
            if not obj_path.exists():
                missing_objects.append(str(cpp_file.relative_to(self.project_root)))
                self.log_warning(f"Missing .o file for: {cpp_file.relative_to(self.project_root)}")
        
        if missing_objects:
            print(f"  {Colors.YELLOW}⚠{Colors.RESET} {len(missing_objects)} .o files missing")
        else:
            print(f"  {Colors.GREEN}✓{Colors.RESET} All .cpp files compiled to .o files")
        
        return len(missing_objects) == 0
    
    def print_summary(self):
        """Print summary of all checks"""
        print(f"\n{Colors.BOLD}{'=' * 70}{Colors.RESET}")
        print(f"{Colors.BOLD}Build Verification Summary{Colors.RESET}")
        print(f"{Colors.BOLD}{'=' * 70}{Colors.RESET}")
        
        if self.errors:
            print(f"\n{Colors.RED}❌ Errors: {len(self.errors)}{Colors.RESET}")
            for error in self.errors:
                print(f"  {Colors.RED}•{Colors.RESET} {error}")
        
        if self.warnings:
            print(f"\n{Colors.YELLOW}⚠️  Warnings: {len(self.warnings)}{Colors.RESET}")
            for i, warning in enumerate(self.warnings[:10]):  # Show first 10
                print(f"  {Colors.YELLOW}•{Colors.RESET} {warning}")
            if len(self.warnings) > 10:
                print(f"  {Colors.YELLOW}•{Colors.RESET} ... and {len(self.warnings) - 10} more")
        
        if self.info and len(self.info) <= 15:  # Only show info if not too many
            print(f"\n{Colors.BLUE}ℹ️  Info:{Colors.RESET}")
            for info in self.info[:15]:
                print(f"  {Colors.BLUE}•{Colors.RESET} {info}")
        
        if not self.errors and not self.warnings:
            print(f"\n{Colors.GREEN}✅ All checks passed!{Colors.RESET}")
        
        print(f"\n{Colors.BOLD}{'=' * 70}{Colors.RESET}")
    
    def run_all_checks(self) -> bool:
        """Run all verification checks"""
        print(f"{Colors.BOLD}{'=' * 70}{Colors.RESET}")
        print(f"{Colors.BOLD}ESP32-S3 Car Control - Build Verification Tool v1.0{Colors.RESET}")
        print(f"{Colors.BOLD}{'=' * 70}{Colors.RESET}")
        print(f"Project: {self.project_root}")
        
        checks = [
            ("Header Guards", self.check_header_guards),
            ("Static Members", self.check_static_members),
            ("Extern Declarations", self.check_extern_declarations),
            ("CPP/Header Pairs", self.check_cpp_has_header),
            ("Build Artifacts", self.check_build_artifacts),
        ]
        
        all_passed = True
        for check_name, check_func in checks:
            try:
                if not check_func():
                    all_passed = False
            except Exception as e:
                self.log_error(f"Check '{check_name}' failed with exception: {e}")
                all_passed = False
        
        self.print_summary()
        
        return all_passed and len(self.errors) == 0

def main():
    """Main entry point"""
    # Get project root from command line or use current directory
    project_root = sys.argv[1] if len(sys.argv) > 1 else "."
    
    if not Path(project_root).exists():
        print(f"{Colors.RED}Error: Project root '{project_root}' does not exist{Colors.RESET}")
        return 1
    
    verifier = BuildVerifier(project_root)
    success = verifier.run_all_checks()
    
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(main())
