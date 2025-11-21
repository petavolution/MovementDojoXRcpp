#!/usr/bin/env python3
"""
USD Scene Generator for Lightsaber Trainer

Generates USD scene files for testing and development.
Can be used without the full USD library installed.
"""

import argparse
import os
import math
from typing import List, Tuple, Optional

class Vec3:
    """Simple 3D vector class."""
    def __init__(self, x: float = 0, y: float = 0, z: float = 0):
        self.x = x
        self.y = y
        self.z = z

    def __str__(self):
        return f"({self.x}, {self.y}, {self.z})"

class Color:
    """RGB color class."""
    def __init__(self, r: float = 1, g: float = 1, b: float = 1):
        self.r = r
        self.g = g
        self.b = b

    def __str__(self):
        return f"({self.r}, {self.g}, {self.b})"

class USDAWriter:
    """Writes USD ASCII (.usda) files."""

    def __init__(self, filepath: str):
        self.filepath = filepath
        self.content = []
        self.indent_level = 0
        self.default_prim = "World"
        self.up_axis = "Y"
        self.meters_per_unit = 1.0

    def _indent(self) -> str:
        return "    " * self.indent_level

    def _write(self, line: str):
        self.content.append(f"{self._indent()}{line}")

    def begin(self):
        """Start the USD file."""
        self.content.append("#usda 1.0")
        self.content.append("(")
        self.content.append(f'    defaultPrim = "{self.default_prim}"')
        self.content.append(f'    upAxis = "{self.up_axis}"')
        self.content.append(f"    metersPerUnit = {self.meters_per_unit}")
        self.content.append(")")
        self.content.append("")

    def begin_def(self, prim_type: str, name: str, kind: Optional[str] = None):
        """Start a prim definition."""
        if kind:
            self._write(f'def {prim_type} "{name}" (')
            self.indent_level += 1
            self._write(f'kind = "{kind}"')
            self.indent_level -= 1
            self._write(")")
        else:
            self._write(f'def {prim_type} "{name}"')
        self._write("{")
        self.indent_level += 1

    def end_def(self):
        """End a prim definition."""
        self.indent_level -= 1
        self._write("}")
        self._write("")

    def add_attribute(self, attr_type: str, name: str, value: str):
        """Add an attribute."""
        self._write(f"{attr_type} {name} = {value}")

    def add_transform(self, translate: Optional[Vec3] = None,
                      rotate: Optional[Tuple[float, Vec3]] = None,
                      scale: Optional[Vec3] = None):
        """Add transform operations."""
        ops = []
        if translate:
            self._write(f"double3 xformOp:translate = {translate}")
            ops.append('"xformOp:translate"')
        if rotate:
            angle, axis = rotate
            self._write(f"float3 xformOp:rotateXYZ = ({rotate[0]}, 0, 0)")
            ops.append('"xformOp:rotateXYZ"')
        if scale:
            self._write(f"double3 xformOp:scale = {scale}")
            ops.append('"xformOp:scale"')
        if ops:
            ops_str = ", ".join(ops)
            self._write(f"uniform token[] xformOpOrder = [{ops_str}]")

    def add_display_color(self, color: Color):
        """Add display color attribute."""
        self._write(f"color3f[] primvars:displayColor = [{color}]")

    def add_cube(self, name: str, size: float, position: Vec3, color: Color):
        """Add a cube primitive."""
        self.begin_def("Cube", name)
        self.add_attribute("double", "size", str(size))
        self.add_transform(translate=position)
        self.add_display_color(color)
        self.end_def()

    def add_sphere(self, name: str, radius: float, position: Vec3, color: Color):
        """Add a sphere primitive."""
        self.begin_def("Sphere", name)
        self.add_attribute("double", "radius", str(radius))
        self.add_transform(translate=position)
        self.add_display_color(color)
        self.end_def()

    def add_cylinder(self, name: str, radius: float, height: float,
                     position: Vec3, color: Color):
        """Add a cylinder primitive."""
        self.begin_def("Cylinder", name)
        self.add_attribute("double", "radius", str(radius))
        self.add_attribute("double", "height", str(height))
        self.add_transform(translate=position)
        self.add_display_color(color)
        self.end_def()

    def add_plane(self, name: str, width: float, length: float,
                  position: Vec3, color: Color):
        """Add a plane as a mesh."""
        hw = width / 2
        hl = length / 2

        self.begin_def("Mesh", name)
        self._write(f"int[] faceVertexCounts = [4]")
        self._write(f"int[] faceVertexIndices = [0, 1, 2, 3]")
        self._write(f"point3f[] points = [({-hw}, 0, {-hl}), ({hw}, 0, {-hl}), ({hw}, 0, {hl}), ({-hw}, 0, {hl})]")
        self._write(f"normal3f[] normals = [(0, 1, 0), (0, 1, 0), (0, 1, 0), (0, 1, 0)]")
        self.add_transform(translate=position)
        self.add_display_color(color)
        self.end_def()

    def add_distant_light(self, name: str, intensity: float, color: Color,
                          angle: Tuple[float, float, float] = (45, 0, 0)):
        """Add a distant light."""
        self.begin_def("DistantLight", name)
        self.add_attribute("float", "inputs:intensity", str(intensity))
        self.add_attribute("color3f", "inputs:color", str(color))
        self._write(f"float3 xformOp:rotateXYZ = ({angle[0]}, {angle[1]}, {angle[2]})")
        self._write('uniform token[] xformOpOrder = ["xformOp:rotateXYZ"]')
        self.end_def()

    def add_point_light(self, name: str, intensity: float, color: Color,
                        position: Vec3):
        """Add a point light."""
        self.begin_def("SphereLight", name)
        self.add_attribute("float", "inputs:intensity", str(intensity))
        self.add_attribute("color3f", "inputs:color", str(color))
        self.add_transform(translate=position)
        self.end_def()

    def save(self):
        """Save the USD file."""
        with open(self.filepath, 'w') as f:
            f.write('\n'.join(self.content))
        print(f"Saved: {self.filepath}")


def generate_stage1_scene(output_dir: str):
    """Generate Stage 1 test scene: simple cube."""
    filepath = os.path.join(output_dir, "stage1_cube.usda")
    writer = USDAWriter(filepath)
    writer.default_prim = "World"
    writer.begin()

    # Root xform
    writer.begin_def("Xform", "World", kind="assembly")

    # Ground plane
    writer.add_plane("Ground", 10.0, 10.0, Vec3(0, 0, 0), Color(0.3, 0.3, 0.35))

    # Test cube
    writer.add_cube("TestCube", 0.5, Vec3(0, 0.25, -1.5), Color(0.8, 0.2, 0.2))

    # Light
    writer.add_distant_light("Sun", 1.0, Color(1, 0.95, 0.9), (50, -30, 0))

    writer.end_def()
    writer.save()


def generate_dojo_basic_scene(output_dir: str):
    """Generate basic dojo training environment."""
    filepath = os.path.join(output_dir, "dojo_basic.usda")
    writer = USDAWriter(filepath)
    writer.default_prim = "Dojo"
    writer.begin()

    # Root
    writer.begin_def("Xform", "Dojo", kind="assembly")

    # Floor
    writer.add_plane("Floor", 20.0, 20.0, Vec3(0, 0, 0), Color(0.15, 0.15, 0.2))

    # Training platform
    writer.add_cube("Platform", 4.0, Vec3(0, 0.1, 0), Color(0.2, 0.2, 0.25))

    # Walls (back)
    writer.add_cube("BackWall", 8.0, Vec3(0, 2, -5), Color(0.1, 0.1, 0.15))

    # Target pillars
    for i, angle in enumerate([45, -45, 135, -135]):
        rad = math.radians(angle)
        x = math.sin(rad) * 3
        z = math.cos(rad) * 3
        writer.add_cylinder(f"Pillar_{i}", 0.15, 2.5,
                           Vec3(x, 1.25, z), Color(0.25, 0.25, 0.3))

    # Training targets (spheres)
    for i in range(5):
        angle = math.radians(i * 72 - 144)
        x = math.sin(angle) * 2
        z = math.cos(angle) * 2 - 1.5
        writer.add_sphere(f"Target_{i}", 0.15,
                         Vec3(x, 1.0 + (i % 2) * 0.3, z),
                         Color(1.0, 0.3, 0.1))

    # Lights
    writer.begin_def("Xform", "Lights")
    writer.add_distant_light("KeyLight", 0.8, Color(1, 0.95, 0.9), (60, -45, 0))
    writer.add_point_light("FillLight", 200.0, Color(0.3, 0.4, 0.8), Vec3(2, 3, 2))
    writer.add_point_light("RimLight", 150.0, Color(0.8, 0.3, 0.2), Vec3(-2, 2, -3))
    writer.end_def()

    writer.end_def()
    writer.save()


def generate_dojo_full_scene(output_dir: str):
    """Generate full dojo with advanced features."""
    filepath = os.path.join(output_dir, "dojo_full.usda")
    writer = USDAWriter(filepath)
    writer.default_prim = "Dojo"
    writer.begin()

    # Add variants comment
    writer.content.append("# Dojo environment with training areas")
    writer.content.append("# Use variants to switch between layouts")
    writer.content.append("")

    # Root
    writer.begin_def("Xform", "Dojo", kind="assembly")

    # Main floor
    writer.add_plane("MainFloor", 30.0, 30.0, Vec3(0, 0, 0), Color(0.12, 0.12, 0.15))

    # Elevated training platform
    writer.add_cube("CentralPlatform", 6.0, Vec3(0, 0.15, 0), Color(0.18, 0.18, 0.22))

    # Circular pillar arrangement
    num_pillars = 8
    for i in range(num_pillars):
        angle = math.radians(i * 360 / num_pillars)
        x = math.sin(angle) * 5
        z = math.cos(angle) * 5
        writer.add_cylinder(f"OuterPillar_{i}", 0.3, 4.0,
                           Vec3(x, 2, z), Color(0.2, 0.2, 0.25))

    # Training zones
    zones = [
        ("PostureZone", Vec3(-4, 0.05, 0), Color(0.1, 0.3, 0.1)),
        ("CombatZone", Vec3(4, 0.05, 0), Color(0.3, 0.1, 0.1)),
        ("PathZone", Vec3(0, 0.05, 4), Color(0.1, 0.1, 0.3)),
    ]
    for name, pos, color in zones:
        writer.begin_def("Xform", name)
        writer.add_cube(f"{name}_Floor", 3.0, pos, color)
        writer.end_def()

    # Target drones (spheres on poles)
    for i in range(6):
        angle = math.radians(i * 60)
        x = math.sin(angle) * 3
        z = math.cos(angle) * 3
        height = 1.2 + (i % 3) * 0.4

        writer.begin_def("Xform", f"DroneMount_{i}")
        writer.add_cylinder(f"DronePole_{i}", 0.02, height,
                           Vec3(x, height/2, z), Color(0.3, 0.3, 0.35))
        writer.add_sphere(f"Drone_{i}", 0.12,
                         Vec3(x, height + 0.15, z),
                         Color(1.0, 0.4, 0.1))
        writer.end_def()

    # Ambient decoration spheres
    writer.begin_def("Xform", "Decorations")
    positions = [
        Vec3(-6, 0.5, -6), Vec3(6, 0.5, -6),
        Vec3(-6, 0.5, 6), Vec3(6, 0.5, 6)
    ]
    for i, pos in enumerate(positions):
        writer.add_sphere(f"AmbientOrb_{i}", 0.3, pos, Color(0.2, 0.5, 0.8))
    writer.end_def()

    # Lighting rig
    writer.begin_def("Xform", "LightRig")
    writer.add_distant_light("SunKey", 0.7, Color(1, 0.95, 0.9), (55, -40, 0))
    writer.add_distant_light("SkyFill", 0.2, Color(0.6, 0.7, 1.0), (-30, 60, 0))

    # Ring of point lights
    for i in range(4):
        angle = math.radians(i * 90 + 45)
        x = math.sin(angle) * 4
        z = math.cos(angle) * 4
        writer.add_point_light(f"RingLight_{i}", 300.0,
                              Color(0.8, 0.6, 0.4), Vec3(x, 4, z))
    writer.end_def()

    writer.end_def()
    writer.save()


def main():
    parser = argparse.ArgumentParser(
        description="Generate USD scenes for Lightsaber Trainer"
    )
    parser.add_argument(
        "--output", "-o",
        default="scenes",
        help="Output directory for generated scenes"
    )
    parser.add_argument(
        "--scene", "-s",
        choices=["stage1", "dojo_basic", "dojo_full", "all"],
        default="all",
        help="Scene to generate"
    )

    args = parser.parse_args()

    # Create output directory
    os.makedirs(args.output, exist_ok=True)

    # Generate requested scenes
    if args.scene in ("stage1", "all"):
        generate_stage1_scene(args.output)

    if args.scene in ("dojo_basic", "all"):
        generate_dojo_basic_scene(args.output)

    if args.scene in ("dojo_full", "all"):
        generate_dojo_full_scene(args.output)

    print("Done!")


if __name__ == "__main__":
    main()
