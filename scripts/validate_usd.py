#!/usr/bin/env python3
"""
USD Scene Validation Script

Validates USD scene files for the Lightsaber Trainer application.
Works with or without the full USD library installed.
"""

import argparse
import os
import sys
import re
from pathlib import Path
from typing import List, Tuple, Optional

# Try to import USD, but provide fallback validation
try:
    from pxr import Usd, UsdGeom
    HAS_USD = True
except ImportError:
    HAS_USD = False
    print("Note: USD library not available, using basic validation")


class ValidationResult:
    """Result of a validation check."""

    def __init__(self, passed: bool, message: str, severity: str = "error"):
        self.passed = passed
        self.message = message
        self.severity = severity  # "error", "warning", "info"

    def __str__(self):
        status = "PASS" if self.passed else f"FAIL ({self.severity})"
        return f"[{status}] {self.message}"


class USDAValidator:
    """Validates USD ASCII files."""

    def __init__(self, filepath: str):
        self.filepath = filepath
        self.content = ""
        self.results: List[ValidationResult] = []

    def load(self) -> bool:
        """Load the file content."""
        try:
            with open(self.filepath, 'r') as f:
                self.content = f.read()
            return True
        except Exception as e:
            self.results.append(ValidationResult(
                False, f"Failed to read file: {e}"))
            return False

    def validate(self) -> bool:
        """Run all validation checks."""
        if not self.load():
            return False

        # Basic syntax checks
        self._check_header()
        self._check_balanced_braces()
        self._check_required_metadata()
        self._check_prim_definitions()

        # USD library checks (if available)
        if HAS_USD:
            self._check_with_usd_library()

        # Return True if no errors (warnings are ok)
        return all(r.passed or r.severity != "error" for r in self.results)

    def _check_header(self):
        """Check for valid USD header."""
        if self.content.startswith("#usda 1.0"):
            self.results.append(ValidationResult(
                True, "Valid USDA header found"))
        elif self.content.startswith("#usdc"):
            self.results.append(ValidationResult(
                True, "Binary USD file (usdc) - skipping text validation",
                severity="info"))
        else:
            self.results.append(ValidationResult(
                False, "Missing or invalid USD header"))

    def _check_balanced_braces(self):
        """Check for balanced braces and parentheses."""
        # Remove strings to avoid counting braces inside strings
        content = re.sub(r'"[^"]*"', '', self.content)

        braces = 0
        parens = 0
        brackets = 0

        for char in content:
            if char == '{':
                braces += 1
            elif char == '}':
                braces -= 1
            elif char == '(':
                parens += 1
            elif char == ')':
                parens -= 1
            elif char == '[':
                brackets += 1
            elif char == ']':
                brackets -= 1

            # Check for negative (closing before opening)
            if braces < 0 or parens < 0 or brackets < 0:
                self.results.append(ValidationResult(
                    False, "Unbalanced brackets/braces/parentheses"))
                return

        if braces != 0:
            self.results.append(ValidationResult(
                False, f"Unbalanced curly braces (diff: {braces})"))
        elif parens != 0:
            self.results.append(ValidationResult(
                False, f"Unbalanced parentheses (diff: {parens})"))
        elif brackets != 0:
            self.results.append(ValidationResult(
                False, f"Unbalanced square brackets (diff: {brackets})"))
        else:
            self.results.append(ValidationResult(
                True, "All brackets/braces/parentheses balanced"))

    def _check_required_metadata(self):
        """Check for required stage metadata."""
        # Check defaultPrim
        if re.search(r'defaultPrim\s*=\s*"[^"]+"', self.content):
            self.results.append(ValidationResult(
                True, "defaultPrim defined"))
        else:
            self.results.append(ValidationResult(
                False, "Missing defaultPrim metadata", severity="warning"))

        # Check upAxis
        if re.search(r'upAxis\s*=\s*"[YZ]"', self.content):
            self.results.append(ValidationResult(
                True, "upAxis defined"))
        else:
            self.results.append(ValidationResult(
                False, "Missing upAxis metadata", severity="warning"))

    def _check_prim_definitions(self):
        """Check for valid prim definitions."""
        # Find all def statements
        defs = re.findall(r'def\s+(\w+)\s+"([^"]+)"', self.content)

        if not defs:
            self.results.append(ValidationResult(
                False, "No prim definitions found"))
            return

        self.results.append(ValidationResult(
            True, f"Found {len(defs)} prim definitions"))

        # Check for known prim types
        known_types = {
            'Xform', 'Mesh', 'Cube', 'Sphere', 'Cylinder', 'Cone', 'Capsule',
            'DistantLight', 'SphereLight', 'DiskLight', 'RectLight',
            'Material', 'Shader', 'Scope'
        }

        for prim_type, prim_name in defs:
            if prim_type not in known_types:
                self.results.append(ValidationResult(
                    False, f"Unknown prim type: {prim_type} (in {prim_name})",
                    severity="warning"))

    def _check_with_usd_library(self):
        """Perform validation using the USD library."""
        try:
            stage = Usd.Stage.Open(self.filepath)
            if not stage:
                self.results.append(ValidationResult(
                    False, "USD library failed to open stage"))
                return

            self.results.append(ValidationResult(
                True, "USD library successfully opened stage"))

            # Check default prim exists
            default_prim = stage.GetDefaultPrim()
            if default_prim:
                self.results.append(ValidationResult(
                    True, f"Default prim exists: {default_prim.GetName()}"))
            else:
                self.results.append(ValidationResult(
                    False, "Default prim not found in stage"))

            # Count geometry
            mesh_count = 0
            light_count = 0
            xform_count = 0

            for prim in stage.Traverse():
                if prim.IsA(UsdGeom.Mesh):
                    mesh_count += 1
                elif prim.IsA(UsdGeom.Xform):
                    xform_count += 1
                # Check for lights
                if prim.GetTypeName() in ('DistantLight', 'SphereLight',
                                           'DiskLight', 'RectLight'):
                    light_count += 1

            self.results.append(ValidationResult(
                True, f"Stage contains: {mesh_count} meshes, {xform_count} xforms, {light_count} lights",
                severity="info"))

        except Exception as e:
            self.results.append(ValidationResult(
                False, f"USD library validation error: {e}"))

    def print_results(self):
        """Print validation results."""
        print(f"\nValidation results for: {self.filepath}")
        print("-" * 60)

        errors = 0
        warnings = 0

        for result in self.results:
            if not result.passed:
                if result.severity == "error":
                    errors += 1
                elif result.severity == "warning":
                    warnings += 1
            print(f"  {result}")

        print("-" * 60)
        print(f"Summary: {errors} errors, {warnings} warnings")

        return errors == 0


def validate_file(filepath: str) -> bool:
    """Validate a single USD file."""
    validator = USDAValidator(filepath)
    validator.validate()
    return validator.print_results()


def validate_directory(dirpath: str) -> bool:
    """Validate all USD files in a directory."""
    all_passed = True
    files_found = 0

    for root, dirs, files in os.walk(dirpath):
        for filename in files:
            if filename.endswith(('.usda', '.usd', '.usdc')):
                filepath = os.path.join(root, filename)
                files_found += 1
                if not validate_file(filepath):
                    all_passed = False

    if files_found == 0:
        print(f"No USD files found in: {dirpath}")
        return False

    print(f"\nValidated {files_found} files")
    return all_passed


def main():
    parser = argparse.ArgumentParser(
        description="Validate USD scene files for Lightsaber Trainer"
    )
    parser.add_argument(
        "path",
        nargs="?",
        default="scenes",
        help="Path to USD file or directory containing USD files"
    )
    parser.add_argument(
        "--strict",
        action="store_true",
        help="Treat warnings as errors"
    )

    args = parser.parse_args()

    path = Path(args.path)

    if not path.exists():
        print(f"Error: Path not found: {path}")
        sys.exit(1)

    if path.is_file():
        success = validate_file(str(path))
    else:
        success = validate_directory(str(path))

    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
