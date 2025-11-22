#!/usr/bin/env python3
"""
Procedural Mesh Generator for Movement Dojo

Generates mesh data (vertices, normals, UVs, indices) in code.
Outputs OBJ format for easy import, or raw vertex data as headers.

These serve as placeholders until artist-created assets are available.
All geometry is reproducible from code.

Usage:
    python3 generate_meshes.py --output assets/meshes/
    python3 generate_meshes.py --mesh lightsaber --output assets/meshes/lightsaber.obj
"""

import argparse
import os
import math
from typing import List, Tuple


# =============================================================================
# Math Helpers
# =============================================================================

def normalize(v: Tuple[float, float, float]) -> Tuple[float, float, float]:
    """Normalize a 3D vector."""
    length = math.sqrt(v[0]**2 + v[1]**2 + v[2]**2)
    if length < 1e-10:
        return (0.0, 1.0, 0.0)
    return (v[0]/length, v[1]/length, v[2]/length)


def cross(a: Tuple[float, float, float], b: Tuple[float, float, float]) -> Tuple[float, float, float]:
    """Cross product of two 3D vectors."""
    return (
        a[1]*b[2] - a[2]*b[1],
        a[2]*b[0] - a[0]*b[2],
        a[0]*b[1] - a[1]*b[0]
    )


# =============================================================================
# Mesh Generators
# =============================================================================

class MeshData:
    """Container for mesh geometry data."""
    def __init__(self):
        self.vertices: List[Tuple[float, float, float]] = []
        self.normals: List[Tuple[float, float, float]] = []
        self.uvs: List[Tuple[float, float]] = []
        self.faces: List[Tuple[int, ...]] = []  # 1-indexed for OBJ format

    def to_obj(self, name: str = "mesh") -> str:
        """Export to OBJ format string."""
        lines = [f"# Generated mesh: {name}", f"# Vertices: {len(self.vertices)}", ""]

        # Vertices
        for v in self.vertices:
            lines.append(f"v {v[0]:.6f} {v[1]:.6f} {v[2]:.6f}")

        # Texture coordinates
        if self.uvs:
            lines.append("")
            for uv in self.uvs:
                lines.append(f"vt {uv[0]:.6f} {uv[1]:.6f}")

        # Normals
        if self.normals:
            lines.append("")
            for n in self.normals:
                lines.append(f"vn {n[0]:.6f} {n[1]:.6f} {n[2]:.6f}")

        # Faces
        lines.append("")
        lines.append(f"g {name}")
        for face in self.faces:
            if self.normals and self.uvs:
                face_str = " ".join(f"{v}/{v}/{v}" for v in face)
            elif self.normals:
                face_str = " ".join(f"{v}//{v}" for v in face)
            elif self.uvs:
                face_str = " ".join(f"{v}/{v}" for v in face)
            else:
                face_str = " ".join(str(v) for v in face)
            lines.append(f"f {face_str}")

        return "\n".join(lines)

    def to_c_header(self, name: str = "mesh") -> str:
        """Export as C header with vertex arrays."""
        lines = [
            f"// Generated mesh: {name}",
            f"// Vertices: {len(self.vertices)}",
            "",
            f"static const float {name}_vertices[] = {{"
        ]

        # Interleaved: position (3) + normal (3) + uv (2)
        for i, v in enumerate(self.vertices):
            n = self.normals[i] if i < len(self.normals) else (0, 1, 0)
            uv = self.uvs[i] if i < len(self.uvs) else (0, 0)
            lines.append(f"    {v[0]:.6f}f, {v[1]:.6f}f, {v[2]:.6f}f,  "
                        f"{n[0]:.6f}f, {n[1]:.6f}f, {n[2]:.6f}f,  "
                        f"{uv[0]:.6f}f, {uv[1]:.6f}f,")

        lines.append("};")
        lines.append(f"static const int {name}_vertex_count = {len(self.vertices)};")

        # Indices
        lines.append("")
        lines.append(f"static const unsigned int {name}_indices[] = {{")
        for face in self.faces:
            # Convert from 1-indexed to 0-indexed
            indices = ", ".join(str(v-1) for v in face)
            lines.append(f"    {indices},")
        lines.append("};")
        lines.append(f"static const int {name}_index_count = {len(self.faces) * 3};")

        return "\n".join(lines)


def generate_cylinder(radius: float, height: float, segments: int = 16) -> MeshData:
    """Generate a cylinder mesh centered at origin, extending along Y axis."""
    mesh = MeshData()

    # Generate vertices for top and bottom caps
    half_h = height / 2

    # Bottom center
    mesh.vertices.append((0, -half_h, 0))
    mesh.normals.append((0, -1, 0))
    mesh.uvs.append((0.5, 0.5))
    bottom_center = 1

    # Bottom ring
    bottom_start = len(mesh.vertices) + 1
    for i in range(segments):
        angle = (2 * math.pi * i) / segments
        x = radius * math.cos(angle)
        z = radius * math.sin(angle)
        mesh.vertices.append((x, -half_h, z))
        mesh.normals.append((0, -1, 0))
        mesh.uvs.append((0.5 + 0.5*math.cos(angle), 0.5 + 0.5*math.sin(angle)))

    # Top center
    mesh.vertices.append((0, half_h, 0))
    mesh.normals.append((0, 1, 0))
    mesh.uvs.append((0.5, 0.5))
    top_center = len(mesh.vertices)

    # Top ring
    top_start = len(mesh.vertices) + 1
    for i in range(segments):
        angle = (2 * math.pi * i) / segments
        x = radius * math.cos(angle)
        z = radius * math.sin(angle)
        mesh.vertices.append((x, half_h, z))
        mesh.normals.append((0, 1, 0))
        mesh.uvs.append((0.5 + 0.5*math.cos(angle), 0.5 + 0.5*math.sin(angle)))

    # Side vertices (need separate normals)
    side_bottom_start = len(mesh.vertices) + 1
    for i in range(segments + 1):  # +1 for seam
        angle = (2 * math.pi * i) / segments
        x = radius * math.cos(angle)
        z = radius * math.sin(angle)
        nx, nz = math.cos(angle), math.sin(angle)

        mesh.vertices.append((x, -half_h, z))
        mesh.normals.append((nx, 0, nz))
        mesh.uvs.append((i / segments, 0))

        mesh.vertices.append((x, half_h, z))
        mesh.normals.append((nx, 0, nz))
        mesh.uvs.append((i / segments, 1))

    # Bottom cap faces
    for i in range(segments):
        next_i = (i + 1) % segments
        mesh.faces.append((bottom_center, bottom_start + next_i, bottom_start + i))

    # Top cap faces
    for i in range(segments):
        next_i = (i + 1) % segments
        mesh.faces.append((top_center, top_start + i, top_start + next_i))

    # Side faces
    for i in range(segments):
        b1 = side_bottom_start + i * 2
        t1 = b1 + 1
        b2 = b1 + 2
        t2 = b2 + 1
        mesh.faces.append((b1, b2, t2))
        mesh.faces.append((b1, t2, t1))

    return mesh


def generate_sphere(radius: float, segments: int = 16, rings: int = 8) -> MeshData:
    """Generate a UV sphere."""
    mesh = MeshData()

    for ring in range(rings + 1):
        phi = math.pi * ring / rings
        y = radius * math.cos(phi)
        ring_radius = radius * math.sin(phi)

        for seg in range(segments + 1):
            theta = 2 * math.pi * seg / segments
            x = ring_radius * math.cos(theta)
            z = ring_radius * math.sin(theta)

            mesh.vertices.append((x, y, z))
            mesh.normals.append(normalize((x, y, z)))
            mesh.uvs.append((seg / segments, ring / rings))

    # Generate faces
    for ring in range(rings):
        for seg in range(segments):
            v1 = ring * (segments + 1) + seg + 1
            v2 = v1 + 1
            v3 = v1 + segments + 1
            v4 = v3 + 1

            if ring > 0:
                mesh.faces.append((v1, v3, v2))
            if ring < rings - 1:
                mesh.faces.append((v2, v3, v4))

    return mesh


def generate_lightsaber_hilt(length: float = 0.25, grip_radius: float = 0.018) -> MeshData:
    """Generate a stylized lightsaber hilt."""
    mesh = MeshData()

    # Hilt is composed of multiple sections
    sections = [
        # (y_start, y_end, radius_start, radius_end, segments)
        (0.0, 0.02, 0.015, 0.022, 8),      # Pommel
        (0.02, 0.04, 0.022, 0.018, 8),     # Pommel taper
        (0.04, 0.16, 0.018, 0.018, 16),    # Main grip
        (0.16, 0.18, 0.018, 0.022, 8),     # Emitter base
        (0.18, 0.22, 0.022, 0.020, 8),     # Emitter shroud
        (0.22, length, 0.020, 0.015, 8),   # Emitter tip
    ]

    vertex_offset = 0
    for y_start, y_end, r_start, r_end, segs in sections:
        # Create a tapered cylinder section
        for ring in range(2):
            t = ring
            y = y_start + (y_end - y_start) * t
            r = r_start + (r_end - r_start) * t

            for seg in range(segs + 1):
                angle = 2 * math.pi * seg / segs
                x = r * math.cos(angle)
                z = r * math.sin(angle)

                mesh.vertices.append((x, y, z))
                mesh.normals.append(normalize((math.cos(angle), 0, math.sin(angle))))
                mesh.uvs.append((seg / segs, y / length))

        # Faces for this section
        for seg in range(segs):
            base = vertex_offset + 1  # 1-indexed
            v1 = base + seg
            v2 = base + seg + 1
            v3 = base + segs + 1 + seg
            v4 = base + segs + 1 + seg + 1

            mesh.faces.append((v1, v3, v2))
            mesh.faces.append((v2, v3, v4))

        vertex_offset += (segs + 1) * 2

    return mesh


def generate_lightsaber_blade(length: float = 1.0, radius: float = 0.015) -> MeshData:
    """Generate a lightsaber blade (simple cylinder with rounded tip)."""
    mesh = MeshData()

    segments = 12

    # Main blade cylinder
    for ring in range(2):
        y = ring * (length - radius)
        for seg in range(segments + 1):
            angle = 2 * math.pi * seg / segments
            x = radius * math.cos(angle)
            z = radius * math.sin(angle)

            mesh.vertices.append((x, y, z))
            mesh.normals.append(normalize((math.cos(angle), 0, math.sin(angle))))
            mesh.uvs.append((seg / segments, y / length))

    # Rounded tip (hemisphere)
    tip_rings = 4
    for ring in range(tip_rings + 1):
        phi = (math.pi / 2) * ring / tip_rings
        y = (length - radius) + radius * math.sin(phi)
        ring_r = radius * math.cos(phi)

        for seg in range(segments + 1):
            angle = 2 * math.pi * seg / segments
            x = ring_r * math.cos(angle)
            z = ring_r * math.sin(angle)

            mesh.vertices.append((x, y, z))
            n = normalize((math.cos(angle) * math.cos(phi), math.sin(phi), math.sin(angle) * math.cos(phi)))
            mesh.normals.append(n)
            mesh.uvs.append((seg / segments, y / length))

    # Cylinder faces
    for seg in range(segments):
        v1 = seg + 1
        v2 = seg + 2
        v3 = v1 + segments + 1
        v4 = v2 + segments + 1
        mesh.faces.append((v1, v3, v2))
        mesh.faces.append((v2, v3, v4))

    # Tip faces
    base = (segments + 1) * 2 + 1
    for ring in range(tip_rings):
        for seg in range(segments):
            v1 = base + ring * (segments + 1) + seg
            v2 = v1 + 1
            v3 = v1 + segments + 1
            v4 = v3 + 1

            if ring < tip_rings - 1:
                mesh.faces.append((v1, v3, v2))
            mesh.faces.append((v2, v3, v4))

    return mesh


def generate_target_orb(radius: float = 0.08) -> MeshData:
    """Generate a training target orb."""
    return generate_sphere(radius, segments=12, rings=6)


def generate_boundary_post(radius: float = 0.05, height: float = 2.0) -> MeshData:
    """Generate a boundary post."""
    return generate_cylinder(radius, height, segments=8)


def generate_floor_tile(size: float = 1.0) -> MeshData:
    """Generate a simple floor tile (quad)."""
    mesh = MeshData()
    half = size / 2

    mesh.vertices = [
        (-half, 0, -half),
        (half, 0, -half),
        (half, 0, half),
        (-half, 0, half),
    ]
    mesh.normals = [(0, 1, 0)] * 4
    mesh.uvs = [(0, 0), (1, 0), (1, 1), (0, 1)]
    mesh.faces = [(1, 2, 3), (1, 3, 4)]

    return mesh


# =============================================================================
# Main
# =============================================================================

MESH_GENERATORS = {
    "lightsaber_hilt": lambda: generate_lightsaber_hilt(),
    "lightsaber_blade": lambda: generate_lightsaber_blade(),
    "target_orb": lambda: generate_target_orb(),
    "boundary_post": lambda: generate_boundary_post(),
    "floor_tile": lambda: generate_floor_tile(),
    "sphere_small": lambda: generate_sphere(0.05, 8, 4),
    "sphere_medium": lambda: generate_sphere(0.1, 12, 6),
    "cylinder": lambda: generate_cylinder(0.1, 0.5),
}


def main():
    parser = argparse.ArgumentParser(
        description="Generate procedural meshes for Movement Dojo"
    )
    parser.add_argument(
        "--output", "-o",
        default="assets/meshes/",
        help="Output directory or file path"
    )
    parser.add_argument(
        "--mesh", "-m",
        choices=list(MESH_GENERATORS.keys()) + ["all"],
        default="all",
        help="Mesh to generate"
    )
    parser.add_argument(
        "--format", "-f",
        choices=["obj", "header"],
        default="obj",
        help="Output format"
    )

    args = parser.parse_args()

    # Prepare output directory
    if args.output.endswith(('.obj', '.h')):
        output_dir = os.path.dirname(args.output) or "."
        single_file = args.output
    else:
        output_dir = args.output
        single_file = None
    os.makedirs(output_dir, exist_ok=True)

    # Generate meshes
    meshes_to_generate = list(MESH_GENERATORS.keys()) if args.mesh == "all" else [args.mesh]

    for mesh_name in meshes_to_generate:
        mesh = MESH_GENERATORS[mesh_name]()

        if single_file and len(meshes_to_generate) == 1:
            output_path = single_file
        else:
            ext = ".h" if args.format == "header" else ".obj"
            output_path = os.path.join(output_dir, f"{mesh_name}{ext}")

        if args.format == "header":
            content = mesh.to_c_header(mesh_name)
        else:
            content = mesh.to_obj(mesh_name)

        with open(output_path, "w") as f:
            f.write(content)

        print(f"Generated: {output_path} ({len(mesh.vertices)} vertices)")

    print(f"\nDone! Generated {len(meshes_to_generate)} mesh(es)")


if __name__ == "__main__":
    main()
