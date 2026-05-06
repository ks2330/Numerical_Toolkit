from build123d import *
import re
import csv
import os
from pathlib import Path


# Define dimensions
height = 600
width = 200
thickness = 60

with BuildSketch() as h_sketch:
    # 1. Create the two vertical side bars
    # We place them at +/- (width/2 - thickness/2) on the X axis
    with Locations((-width / 2 + thickness / 2, 0), (width / 2 - thickness / 2, 0)):
        Rectangle(thickness, height)

    # 2. Create the central horizontal crossbar
    # The width needs to span the gap between the bars
    Rectangle(width, thickness)

export_step(h_sketch.sketch, "rectangle.step")

print("Generated rectangle.step successfully.")


def parse_step_to_csv(step_filename, csv_filename):
    points = []

    with open(step_filename, "r") as f:
        content = f.read()

        # Regex breakdown:
        # #\d+          -> Matches the ID (e.g., #23)
        # CARTESIAN_POINT -> The keyword
        # \(([^)]+)\)   -> Captures everything inside the parentheses
        pattern = re.compile(
            r"#(\d+)\s*=\s*CARTESIAN_POINT\s*\(\s*\'[^\']*\'\s*,\s*\(([^)]+)\)\s*\)\s*;"
        )
        # 1. Find all VERTEX_POINT lines and grab the ID they point to
        # Example: #22 = VERTEX_POINT('', #23) -> Store '23' in a set
        vertex_refs = set(
            re.findall(r"VERTEX_POINT\s*\(\s*\'\'\s*,\s*#(\d+)\s*\)", content)
        )

        # 2. When parsing CARTESIAN_POINTS, only save them if their ID is in vertex_refs
        for match in pattern.finditer(content):
            point_id = match.group(1)
            if point_id in vertex_refs:
                coords_str = match.group(2)

                # Split the coordinates (e.g., "-300., -100., 0.")
                coords = [c.strip().rstrip(".") for c in coords_str.split(",")]

                # We only want 3D points (x, y, z) to avoid the 2D parametric points
                if len(coords) == 3:
                    x, y, z = coords
                    points.append({"X": x, "Y": y})

    # Write to CSV
    with open(csv_filename, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=["X", "Y"])
        writer.writeheader()
        writer.writerows(points)


# Execute
parse_step_to_csv("rectangle.step", "Nodes.csv")

root_path = Path(__file__).resolve().parents[2]


destination_path = root_path / "results" / "step"
destination_path.mkdir(parents=True, exist_ok=True)

source_file = root_path / "apps" / "UI" / "rectangle.step"

destination_file = destination_path / "rectangle.step"

try:
    os.remove(destination_file)
except FileNotFoundError:
    pass
except PermissionError:
    print("Error: You do not have permission to delete this file.")

source_file.rename(destination_file)

destination_path = root_path / "results" / "csv"
destination_path.mkdir(parents=True, exist_ok=True)

source_file = root_path / "apps" / "UI" / "Nodes.csv"

destination_file = destination_path / "Nodes.csv"

try:
    os.remove(destination_file)
except FileNotFoundError:
    pass
except PermissionError:
    print("Error: You do not have permission to delete this file.")

source_file.rename(destination_file)
print("Generate CSV File and put into csv folder")


print("CSV generated with Coordinates!")
