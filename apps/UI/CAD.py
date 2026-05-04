from build123d import *
import re
import csv

# 1. Create a rectangle as a 'Face'
# This method is much more direct for 2D FEM work
rect_face = Sketch() + Rectangle(600, 200)

# 2. Export the resulting geometry
# In build123d, the export_step function is a standalone utility
# that takes the object and the filename.
export_step(rect_face, "rectangle.step")

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
print("CSV generated with Coordinates!")
