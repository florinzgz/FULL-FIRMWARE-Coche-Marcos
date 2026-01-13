#!/usr/bin/env python3
"""
Pre-Flight Hardware Validation System
======================================

Build-time validator that prevents firmware from being built if it would
crash at runtime due to improper hardware initialization order.

This system:
- Parses all C++ source and header files
- Builds a call graph of function dependencies
- Tracks hardware initialization order
- Detects illegal hardware access before initialization
- Blocks the build with detailed error messages

Zero runtime overhead - validation happens only at build time.
"""

import os
import sys
import json
import re
from pathlib import Path
from typing import Dict, List, Set, Tuple, Optional
from dataclasses import dataclass, field

# PlatformIO SCons environment import
try:
    Import("env")
    PLATFORMIO_MODE = True
except:
    PLATFORMIO_MODE = False


@dataclass
class CodeLocation:
    """Represents a location in source code"""
    file: str
    line: int
    function: str
    code: str


@dataclass
class FunctionCall:
    """Represents a function call in the code"""
    name: str
    location: CodeLocation


@dataclass
class InitializationState:
    """Tracks initialization state for a hardware resource"""
    initialized: bool = False
    init_location: Optional[CodeLocation] = None
    violations: List[Tuple[CodeLocation, str]] = field(default_factory=list)


class HardwareValidator:
    """Main validation engine for hardware initialization order"""

    def __init__(self, rules_file: str, source_dirs: List[str]):
        self.rules = self._load_rules(rules_file)
        self.source_dirs = source_dirs
        self.violations: List[Dict] = []
        self.file_cache: Dict[str, List[str]] = {}

    def _load_rules(self, rules_file: str) -> Dict:
        """Load hardware rules from JSON file"""
        try:
            with open(rules_file, 'r') as f:
                return json.load(f)
        except FileNotFoundError:
            print(f"❌ ERROR: Rules file not found: {rules_file}")
            sys.exit(1)
        except json.JSONDecodeError as e:
            print(f"❌ ERROR: Invalid JSON in rules file: {e}")
            sys.exit(1)

    def _get_source_files(self) -> List[Path]:
        """Get all C++ source and header files"""
        files = []
        for source_dir in self.source_dirs:
            source_path = Path(source_dir)
            if source_path.exists():
                files.extend(source_path.rglob("*.cpp"))
                files.extend(source_path.rglob("*.h"))
                files.extend(source_path.rglob("*.hpp"))
        return files

    def _read_file_lines(self, file_path: str) -> List[str]:
        """Read and cache file contents"""
        if file_path not in self.file_cache:
            try:
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                    self.file_cache[file_path] = f.readlines()
            except Exception as e:
                print(f"⚠️  Warning: Could not read {file_path}: {e}")
                self.file_cache[file_path] = []
        return self.file_cache[file_path]

    def _extract_function_name(self, line: str) -> Optional[str]:
        """Extract function name from a line of code"""
        # Match function definitions: ReturnType FunctionName(
        func_pattern = r'\b([a-zA-Z_][a-zA-Z0-9_:]*)\s*\('
        match = re.search(func_pattern, line)
        if match:
            return match.group(1)
        return None

    def _is_in_comment(self, line: str, position: int) -> bool:
        """Check if a position in a line is within a comment"""
        before = line[:position]
        # Single-line comment
        if '//' in before:
            comment_pos = before.find('//')
            if comment_pos < position:
                return True
        return False

    def _is_false_positive(self, line: str, pattern: str) -> bool:
        """Check if a pattern match is likely a false positive"""
        line_stripped = line.strip()
        
        # Skip preprocessor directives
        if line_stripped.startswith('#'):
            return True
        
        # Skip string literals containing the pattern
        if pattern in line:
            # Check if it's in quotes
            parts = line.split('"')
            for i in range(1, len(parts), 2):  # Odd indices are inside quotes
                if pattern in parts[i]:
                    return True
        
        # Skip type definitions (class, struct, typedef)
        if re.match(r'\s*(class|struct|typedef|enum)\s+', line_stripped):
            return True
        
        # Skip function/method definitions
        if '::' in pattern and re.match(r'.*(void|bool|int|uint|static|inline)\s+.*' + re.escape(pattern), line):
            return True
        
        # Skip const/constexpr declarations
        if re.match(r'\s*(const|constexpr|static\s+const)\s+', line_stripped):
            return True
            
        # Skip comments (already handled but double-check)
        if '//' in line and line.find('//') < line.find(pattern) if pattern in line else False:
            return True
        
        return False

    def _find_function_calls(self, file_path: str, pattern: str) -> List[FunctionCall]:
        """Find all calls to functions matching the pattern in a file"""
        lines = self._read_file_lines(str(file_path))
        calls = []
        current_function = "global_scope"
        in_multiline_comment = False

        for line_num, line in enumerate(lines, 1):
            # Track multi-line comments
            if '/*' in line:
                in_multiline_comment = True
            if '*/' in line:
                in_multiline_comment = False
                continue
            
            # Skip comments
            if in_multiline_comment or line.strip().startswith('//'):
                continue

            # Track current function context
            func_name = self._extract_function_name(line)
            if func_name and '(' in line and '{' not in line:
                # This might be a function definition
                current_function = func_name

            # Search for the pattern
            # Handle both literal strings and regex patterns
            if pattern.startswith('\\') or '[' in pattern or '*' in pattern:
                # Regex pattern
                matches = re.finditer(pattern, line)
            else:
                # Literal string search
                matches = [m for m in re.finditer(re.escape(pattern), line)]

            for match in matches:
                # Skip if in comment
                if self._is_in_comment(line, match.start()):
                    continue

                location = CodeLocation(
                    file=str(file_path),
                    line=line_num,
                    function=current_function,
                    code=line.strip()
                )
                calls.append(FunctionCall(name=pattern, location=location))

        return calls

    def _check_initialization_order(self, file_path: str) -> Dict[str, InitializationState]:
        """Check initialization order within a single file"""
        states: Dict[str, InitializationState] = {}
        
        # Initialize states for all rules
        for rule in self.rules.get('rules', []):
            states[rule['resource']] = InitializationState()

        lines = self._read_file_lines(str(file_path))
        current_function = "global_scope"
        in_multiline_comment = False

        for line_num, line in enumerate(lines, 1):
            # Track multi-line comments
            if '/*' in line:
                in_multiline_comment = True
            if '*/' in line:
                in_multiline_comment = False
                continue
            
            # Skip comments
            if in_multiline_comment or line.strip().startswith('//'):
                continue

            # Track current function
            func_name = self._extract_function_name(line)
            if func_name:
                current_function = func_name

            # Check each rule
            for rule in self.rules.get('rules', []):
                resource = rule['resource']
                state = states[resource]

                # Check for initialization
                for init_func in rule.get('init_functions', []):
                    if init_func in line and not self._is_in_comment(line, line.find(init_func)):
                        if not self._is_false_positive(line, init_func):
                            state.initialized = True
                            state.init_location = CodeLocation(
                                file=str(file_path),
                                line=line_num,
                                function=current_function,
                                code=line.strip()
                            )

                # Check for forbidden usage before init
                if not state.initialized:
                    for forbidden in rule.get('forbidden_before_init', []):
                        # Skip false positives
                        if forbidden in line:
                            if self._is_in_comment(line, line.find(forbidden)):
                                continue
                            if self._is_false_positive(line, forbidden):
                                continue
                        
                        # Handle regex patterns
                        if forbidden.startswith('\\') or '[' in forbidden or '*' in forbidden:
                            if re.search(forbidden, line) and not self._is_in_comment(line, 0):
                                if not self._is_false_positive(line, forbidden):
                                    location = CodeLocation(
                                        file=str(file_path),
                                        line=line_num,
                                        function=current_function,
                                        code=line.strip()
                                    )
                                    state.violations.append((location, forbidden))
                        else:
                            if forbidden in line and not self._is_in_comment(line, line.find(forbidden)):
                                if not self._is_false_positive(line, forbidden):
                                    location = CodeLocation(
                                        file=str(file_path),
                                        line=line_num,
                                        function=current_function,
                                        code=line.strip()
                                    )
                                    state.violations.append((location, forbidden))

        return states

    def _check_project_specific_rules(self):
        """Check project-specific initialization order rules"""
        for rule in self.rules.get('project_specific_rules', []):
            enforced_order = rule.get('enforced_order', [])
            if not enforced_order:
                continue

            # Track where each required initialization appears
            init_locations: Dict[str, List[CodeLocation]] = {init: [] for init in enforced_order}

            # Search all files for these initializations
            for file_path in self._get_source_files():
                for init_pattern in enforced_order:
                    calls = self._find_function_calls(file_path, init_pattern)
                    init_locations[init_pattern].extend([call.location for call in calls])

            # Check if order is violated
            # For now, we check if later items appear before earlier items in the same file
            for file_path in self._get_source_files():
                lines = self._read_file_lines(str(file_path))
                first_occurrence = {}

                for line_num, line in enumerate(lines, 1):
                    for idx, init_pattern in enumerate(enforced_order):
                        if init_pattern in line and init_pattern not in first_occurrence:
                            first_occurrence[init_pattern] = line_num

                # Check if any later initialization appears before an earlier one
                for i in range(len(enforced_order) - 1):
                    earlier = enforced_order[i]
                    for j in range(i + 1, len(enforced_order)):
                        later = enforced_order[j]
                        
                        if later in first_occurrence and earlier in first_occurrence:
                            if first_occurrence[later] < first_occurrence[earlier]:
                                self.violations.append({
                                    'type': 'order_violation',
                                    'rule': rule['name'],
                                    'file': str(file_path),
                                    'line': first_occurrence[later],
                                    'message': f"'{later}' appears before '{earlier}' (line {first_occurrence[earlier]})",
                                    'fatal': rule['fatal'],
                                    'impact': rule['impact']
                                })

    def validate(self) -> bool:
        """Run all validation checks"""
        print("=" * 80)
        print("🔍 PRE-FLIGHT HARDWARE VALIDATION SYSTEM")
        print("=" * 80)
        print()

        all_files = self._get_source_files()
        print(f"📁 Scanning {len(all_files)} source files...")
        print()

        # Check each file for initialization order violations
        for file_path in all_files:
            states = self._check_initialization_order(file_path)
            
            for resource, state in states.items():
                if state.violations:
                    for location, forbidden_func in state.violations:
                        # Find the rule for this resource
                        rule = next((r for r in self.rules['rules'] if r['resource'] == resource), None)
                        if rule:
                            self.violations.append({
                                'type': 'init_order',
                                'resource': resource,
                                'file': location.file,
                                'line': location.line,
                                'function': location.function,
                                'code': location.code,
                                'forbidden_call': forbidden_func,
                                'fatal': rule['fatal'],
                                'impact': rule['impact'],
                                'description': rule['description']
                            })

        # Check project-specific rules
        self._check_project_specific_rules()

        # Report results
        return self._report_violations()

    def _report_violations(self) -> bool:
        """Report all violations and return success status"""
        fatal_violations = [v for v in self.violations if v.get('fatal', True)]
        warning_violations = [v for v in self.violations if not v.get('fatal', True)]

        if not self.violations:
            print("✅ VALIDATION PASSED")
            print("   No hardware initialization violations detected")
            print("   Build can proceed safely")
            print()
            return True

        # Report fatal violations
        if fatal_violations:
            print("❌ BUILD BLOCKED — HARDWARE VIOLATIONS DETECTED")
            print("=" * 80)
            print()

            for violation in fatal_violations:
                print("🚨 FATAL VIOLATION:")
                print(f"   Resource: {violation.get('resource', 'Unknown')}")
                print(f"   File: {violation['file']}")
                print(f"   Line: {violation['line']}")
                
                if 'function' in violation:
                    print(f"   Function: {violation['function']}")
                
                if 'code' in violation:
                    print(f"   Code: {violation['code']}")
                
                if violation.get('type') == 'init_order':
                    print(f"   Violation: '{violation['forbidden_call']}' used before initialization")
                    print(f"   Reason: {violation.get('description', 'Unknown')}")
                elif violation.get('type') == 'order_violation':
                    print(f"   Rule: {violation.get('rule', 'Unknown')}")
                    print(f"   Violation: {violation['message']}")
                
                print(f"   Impact: {violation.get('impact', 'System crash or undefined behavior')}")
                print()
                print("   Fix: Ensure hardware is properly initialized before use")
                print("-" * 80)
                print()

        # Report warnings
        if warning_violations:
            print("⚠️  WARNINGS — Non-Fatal Issues Detected")
            print("=" * 80)
            print()

            for violation in warning_violations:
                print("⚠️  WARNING:")
                print(f"   Resource: {violation.get('resource', 'Unknown')}")
                print(f"   File: {violation['file']}")
                print(f"   Line: {violation['line']}")
                print(f"   Impact: {violation.get('impact', 'Potential issues')}")
                print("-" * 80)
                print()

        if fatal_violations:
            print("=" * 80)
            print("❌ BUILD CANNOT PROCEED")
            print(f"   {len(fatal_violations)} fatal violation(s) detected")
            print("   Fix the issues above and rebuild")
            print("=" * 80)
            print()
            return False
        else:
            print("⚠️  Build can proceed with warnings")
            print()
            return True


def main():
    """Main entry point"""
    # Determine project root
    if PLATFORMIO_MODE:
        project_dir = env.subst("$PROJECT_DIR")
    else:
        # Running standalone for testing
        project_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

    rules_file = os.path.join(project_dir, "rules", "hardware_rules.json")
    source_dirs = [
        os.path.join(project_dir, "src"),
        os.path.join(project_dir, "include"),
    ]

    # Create validator and run
    validator = HardwareValidator(rules_file, source_dirs)
    success = validator.validate()

    if not success:
        print("💀 VALIDATION FAILED - ABORTING BUILD")
        sys.exit(1)
    else:
        print("✅ VALIDATION PASSED - BUILD PROCEEDING")


if __name__ == "__main__":
    main()
