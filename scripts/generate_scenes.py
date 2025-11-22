#!/usr/bin/env python3
"""
Procedural USD Scene Generator for Movement Dojo

Generates complete .usda scene files without any GUI tools.
All content is defined in code for reproducibility and CI/CD.

Usage:
    python3 generate_scenes.py --output scenes/
    python3 generate_scenes.py --variant advanced --output scenes/dojo_advanced.usda
"""

import argparse
import os
import math
from datetime import datetime

# =============================================================================
# Geometry Generators
# =============================================================================

def generate_boundary_posts(count: int, radius: float, height: float = 2.0) -> str:
    """Generate cylindrical boundary posts in a circle."""
    lines = []
    for i in range(count):
        angle = (2 * math.pi * i) / count
        x = radius * math.cos(angle)
        z = radius * math.sin(angle)
        lines.append(f'''
        def Cylinder "BoundaryPost_{i}"
        {{
            double height = {height}
            double radius = 0.05
            double3 xformOp:translate = ({x:.3f}, {height/2}, {z:.3f})
            uniform token[] xformOpOrder = ["xformOp:translate"]
            color3f[] primvars:displayColor = [(0.4, 0.4, 0.5)]
        }}''')
    return '\n'.join(lines)


def generate_floor_grid(size: float, divisions: int) -> str:
    """Generate a procedural floor with grid lines."""
    lines = []
    step = size / divisions
    half = size / 2

    for i in range(divisions + 1):
        offset = -half + i * step
        # X-parallel lines
        lines.append(f'''
        def Cube "GridLineX_{i}"
        {{
            double size = 1.0
            double3 xformOp:scale = ({size}, 0.002, 0.002)
            double3 xformOp:translate = (0, 0.001, {offset:.3f})
            uniform token[] xformOpOrder = ["xformOp:translate", "xformOp:scale"]
            color3f[] primvars:displayColor = [(0.3, 0.3, 0.35)]
        }}''')
        # Z-parallel lines
        lines.append(f'''
        def Cube "GridLineZ_{i}"
        {{
            double size = 1.0
            double3 xformOp:scale = (0.002, 0.002, {size})
            double3 xformOp:translate = ({offset:.3f}, 0.001, 0)
            uniform token[] xformOpOrder = ["xformOp:translate", "xformOp:scale"]
            color3f[] primvars:displayColor = [(0.3, 0.3, 0.35)]
        }}''')

    return '\n'.join(lines)


def generate_target_spheres(count: int, pattern: str = "circle") -> str:
    """Generate training target spheres."""
    lines = []

    if pattern == "circle":
        radius = 1.5
        for i in range(count):
            angle = (2 * math.pi * i) / count
            x = radius * math.cos(angle)
            z = radius * math.sin(angle)
            y = 1.0 + 0.3 * math.sin(angle * 2)
            lines.append(f'''
        def Sphere "Target_{i}" (
            customData = {{
                bool isTrainingTarget = true
                int targetId = {i}
            }}
        )
        {{
            double radius = 0.08
            double3 xformOp:translate = ({x:.3f}, {y:.3f}, {z:.3f})
            uniform token[] xformOpOrder = ["xformOp:translate"]
            color3f[] primvars:displayColor = [(1.0, 0.3, 0.1)]
        }}''')

    elif pattern == "grid":
        idx = 0
        for row in range(int(math.sqrt(count))):
            for col in range(int(math.sqrt(count))):
                x = (col - 1) * 0.8
                z = -2.0
                y = 0.8 + row * 0.6
                lines.append(f'''
        def Sphere "Target_{idx}" (
            customData = {{
                bool isTrainingTarget = true
                int targetId = {idx}
            }}
        )
        {{
            double radius = 0.06
            double3 xformOp:translate = ({x:.3f}, {y:.3f}, {z:.3f})
            uniform token[] xformOpOrder = ["xformOp:translate"]
            color3f[] primvars:displayColor = [(0.2, 0.8, 1.0)]
        }}''')
                idx += 1

    return '\n'.join(lines)


def generate_path_spline(name: str, points: list, color: tuple = (0.3, 1.0, 0.5)) -> str:
    """Generate a visual path for path-following exercises."""
    lines = [f'''
        def Xform "{name}"
        {{''']

    for i, (x, y, z) in enumerate(points):
        lines.append(f'''
            def Sphere "PathPoint_{i}"
            {{
                double radius = 0.02
                double3 xformOp:translate = ({x:.3f}, {y:.3f}, {z:.3f})
                uniform token[] xformOpOrder = ["xformOp:translate"]
                color3f[] primvars:displayColor = [({color[0]}, {color[1]}, {color[2]})]
            }}''')

    lines.append('        }')
    return '\n'.join(lines)


# =============================================================================
# Scene Variants
# =============================================================================

def generate_basic_variant() -> str:
    """Basic dojo - simple floor, minimal targets."""
    return f'''
    # Basic Training Setup
    def Xform "BasicSetup"
    {{
        {generate_floor_grid(8, 8)}
        {generate_boundary_posts(6, 3.5)}
    }}
'''


def generate_advanced_variant() -> str:
    """Advanced dojo - targets, paths, more complex layout."""
    # Generate a figure-8 path
    path_points = []
    for t in range(32):
        angle = (t / 32) * 2 * math.pi
        x = math.sin(angle) * 0.8
        y = 1.0 + math.sin(angle * 2) * 0.3
        z = -1.5 + math.sin(angle * 2) * 0.5
        path_points.append((x, y, z))

    return f'''
    # Advanced Training Setup
    def Xform "AdvancedSetup"
    {{
        {generate_floor_grid(10, 10)}
        {generate_boundary_posts(8, 4.0)}

        def Xform "Targets"
        {{
            {generate_target_spheres(8, "circle")}
        }}

        {generate_path_spline("TrainingPath", path_points)}
    }}
'''


def generate_master_variant() -> str:
    """Master dojo - all features, complex paths, many targets."""
    # Spiral path
    spiral_points = []
    for t in range(48):
        angle = (t / 48) * 4 * math.pi
        radius = 0.3 + (t / 48) * 0.8
        x = math.cos(angle) * radius
        y = 0.5 + (t / 48) * 1.5
        z = math.sin(angle) * radius - 1.5
        spiral_points.append((x, y, z))

    return f'''
    # Master Training Setup
    def Xform "MasterSetup"
    {{
        {generate_floor_grid(12, 12)}
        {generate_boundary_posts(12, 5.0, 2.5)}

        def Xform "TargetArray"
        {{
            {generate_target_spheres(9, "grid")}
        }}

        def Xform "CircleTargets"
        {{
            {generate_target_spheres(6, "circle")}
        }}

        {generate_path_spline("SpiralPath", spiral_points, (0.8, 0.3, 1.0))}
    }}
'''


def generate_zen_variant() -> str:
    """Zen dojo - minimal, meditation-focused."""
    return f'''
    # Zen Training Setup - Minimal for Meditation
    def Xform "ZenSetup"
    {{
        # Simple circular floor
        def Cylinder "ZenFloor"
        {{
            double height = 0.05
            double radius = 3.0
            double3 xformOp:translate = (0, -0.025, 0)
            uniform token[] xformOpOrder = ["xformOp:translate"]
            color3f[] primvars:displayColor = [(0.15, 0.15, 0.18)]
        }}

        # Center meditation point
        def Sphere "CenterPoint"
        {{
            double radius = 0.1
            double3 xformOp:translate = (0, 0.05, 0)
            uniform token[] xformOpOrder = ["xformOp:translate"]
            color3f[] primvars:displayColor = [(0.5, 0.3, 0.8)]
        }}

        # Subtle boundary ring
        def Torus "BoundaryRing"
        {{
            double radius = 2.5
            double secondaryRadius = 0.02
            double3 xformOp:translate = (0, 0.01, 0)
            uniform token[] xformOpOrder = ["xformOp:translate"]
            color3f[] primvars:displayColor = [(0.4, 0.4, 0.5)]
        }}
    }}
'''


# =============================================================================
# Main Scene Generator
# =============================================================================

def generate_dojo_scene(variant: str = "basic") -> str:
    """Generate a complete dojo scene with the specified variant."""

    variant_content = {
        "basic": generate_basic_variant(),
        "advanced": generate_advanced_variant(),
        "master": generate_master_variant(),
        "zen": generate_zen_variant()
    }

    content = variant_content.get(variant, generate_basic_variant())
    timestamp = datetime.now().isoformat()

    return f'''#usda 1.0
(
    defaultPrim = "World"
    upAxis = "Y"
    metersPerUnit = 1.0
    doc = """
        Movement Dojo - {variant.title()} Variant
        Generated: {timestamp}

        This scene was procedurally generated.
        Edit generate_scenes.py to modify.
    """
    customLayerData = {{
        string generator = "generate_scenes.py"
        string variant = "{variant}"
    }}
)

def Xform "World" (
    variants = {{
        string dojoLayout = "{variant}"
        string difficulty = "normal"
        string timeOfDay = "day"
    }}
    prepend variantSets = ["dojoLayout", "difficulty", "timeOfDay"]
)
{{
    # =========================================================================
    # Core Floor
    # =========================================================================
    def Cube "MainFloor"
    {{
        double size = 1.0
        double3 xformOp:scale = (12, 0.1, 12)
        double3 xformOp:translate = (0, -0.05, 0)
        uniform token[] xformOpOrder = ["xformOp:translate", "xformOp:scale"]
        color3f[] primvars:displayColor = [(0.12, 0.12, 0.15)]
    }}

    # =========================================================================
    # Lighting
    # =========================================================================
    def Xform "Lighting"
    {{
        def DomeLight "AmbientDome"
        {{
            float inputs:intensity = 300
            color3f inputs:color = (0.7, 0.75, 0.85)
        }}

        def DistantLight "KeyLight"
        {{
            float inputs:intensity = 800
            float3 xformOp:rotateXYZ = (-45, 30, 0)
            uniform token[] xformOpOrder = ["xformOp:rotateXYZ"]
            color3f inputs:color = (1.0, 0.95, 0.9)
        }}

        def DistantLight "FillLight"
        {{
            float inputs:intensity = 200
            float3 xformOp:rotateXYZ = (-30, -60, 0)
            uniform token[] xformOpOrder = ["xformOp:rotateXYZ"]
            color3f inputs:color = (0.6, 0.7, 0.9)
        }}
    }}

    # =========================================================================
    # Variant-Specific Content
    # =========================================================================
    {content}

    # =========================================================================
    # Player Spawn Point
    # =========================================================================
    def Xform "PlayerSpawn" (
        customData = {{
            bool isSpawnPoint = true
        }}
    )
    {{
        double3 xformOp:translate = (0, 0, 2)
        uniform token[] xformOpOrder = ["xformOp:translate"]
    }}

    # =========================================================================
    # Training Area Bounds (for analytics)
    # =========================================================================
    def Xform "TrainingBounds" (
        customData = {{
            bool isTrainingArea = true
            double3 minBounds = (-3, 0, -3)
            double3 maxBounds = (3, 3, 3)
        }}
    )
    {{
    }}
}}
'''


def generate_training_guides_layer() -> str:
    """Generate a separate layer for training guides (can be toggled on/off)."""
    return '''#usda 1.0
(
    doc = "Training Guides Overlay Layer"
    customLayerData = {
        string generator = "generate_scenes.py"
        string purpose = "training_guides"
    }
)

over "World"
{
    # Hand position guides
    def Xform "TrainingGuides"
    {
        def Sphere "LeftHandGuide"
        {
            double radius = 0.05
            double3 xformOp:translate = (-0.4, 1.2, -0.5)
            uniform token[] xformOpOrder = ["xformOp:translate"]
            color3f[] primvars:displayColor = [(0.2, 1.0, 0.3)]
            float primvars:displayOpacity = 0.5
        }

        def Sphere "RightHandGuide"
        {
            double radius = 0.05
            double3 xformOp:translate = (0.4, 1.2, -0.5)
            uniform token[] xformOpOrder = ["xformOp:translate"]
            color3f[] primvars:displayColor = [(0.2, 0.3, 1.0)]
            float primvars:displayOpacity = 0.5
        }
    }
}
'''


# =============================================================================
# CLI Interface
# =============================================================================

def main():
    parser = argparse.ArgumentParser(
        description="Generate USD scenes for Movement Dojo"
    )
    parser.add_argument(
        "--output", "-o",
        default="scenes/",
        help="Output directory or file path"
    )
    parser.add_argument(
        "--variant", "-v",
        choices=["basic", "advanced", "master", "zen", "all"],
        default="all",
        help="Scene variant to generate"
    )
    parser.add_argument(
        "--guides",
        action="store_true",
        help="Also generate training guides layer"
    )

    args = parser.parse_args()

    # Ensure output directory exists
    if args.output.endswith('/') or not args.output.endswith('.usda'):
        os.makedirs(args.output, exist_ok=True)
        output_dir = args.output
    else:
        output_dir = os.path.dirname(args.output) or "."
        os.makedirs(output_dir, exist_ok=True)

    variants_to_generate = ["basic", "advanced", "master", "zen"] if args.variant == "all" else [args.variant]

    for variant in variants_to_generate:
        scene = generate_dojo_scene(variant)
        output_path = os.path.join(output_dir, f"dojo_{variant}.usda")
        with open(output_path, "w") as f:
            f.write(scene)
        print(f"Generated: {output_path}")

    if args.guides:
        guides = generate_training_guides_layer()
        guides_path = os.path.join(output_dir, "training_guides.usda")
        with open(guides_path, "w") as f:
            f.write(guides)
        print(f"Generated: {guides_path}")

    print(f"\nDone! Generated {len(variants_to_generate)} scene(s)")


if __name__ == "__main__":
    main()
