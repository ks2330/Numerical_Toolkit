# Mesh Generation Audit

---

## 1. Bugs

### 1.1 U-shape exterior triangles never deleted (root cause of ushape failure)

After CDT enforces the boundary edges, the fan triangulation creates new triangles inside the notch region (the open part of the U). These are *outside* the domain but are never removed because `deleteHoles` only removes triangles inside `holes` (i.e. circles). There is no equivalent function for concave outer boundaries.

**Fix needed — add `deleteExterior()`:**
```cpp
void deleteExterior() {
    if (outerBoundary.empty()) return;
    elements.erase(std::remove_if(elements.begin(), elements.end(), [&](const Element& e) {
        Node centroid = computeCentroid(e);
        return !isPointInPolygon(centroid, outerBoundary);
    }), elements.end());
}
```
Then call it in `triangulate()` after `deleteHoles()`.

---

### 1.2 `initialize` ushape branch does not reset state

`setBoundary` calls `nodes.clear()`, `edges.clear()`, `holes.clear()`, `outerBoundary.clear()` before populating. The ushape branch does none of this. Calling `initialize` twice (or switching shapes) accumulates stale state.

**Fix:** add the same clear calls at the top of the ushape branch.

---

### 1.3 `isboth` / `isRectangular` flags not reset on re-initialisation

If `initialize("both")` is called and then `initialize("ushape")` is called, `isboth` is still `true`. `generateRandomNodes` will then enter the wrong branch.

**Fix:** reset all flags at the start of `initialize`:
```cpp
isRectangular = false;
isboth = false;
```

---

### 1.4 `segsPerUnit / 4` integer division can produce zero

In `rectangle` and `uShape`, `nx = static_cast<int>(width * segsPerUnit / 4)`. If `segsPerUnit < 4`, `nx = 0`, causing division by zero in `i * width / nx`.

**Fix:** use `std::max(1, ...)` as the guard:
```cpp
const int nx = std::max(1, static_cast<int>(width * segsPerUnit / 4));
```

---

### 1.5 `run_FEM` truncates dimensions to int

```cpp
mesh.initialize(shape, static_cast<int>(dim1), static_cast<int>(dim2), segsPerUnit);
```
`dim1` and `dim2` are already doubles — casting to int silently truncates e.g. 6.5 → 6.

**Fix:** remove the casts.

---

### 1.6 `triangulate(width, height)` receives grid counts, not shape dimensions

In `main.cpp`, `run_Triangulation(mesh, nx, ny)` passes `nx=6, ny=6` as width/height. These are used to build the super-triangle in `bowyerWatson`. If the shape is larger than 6×6 units, the super-triangle won't encompass all nodes and BW will silently fail.

**Fix:** derive the super-triangle size from the actual bounding box of nodes rather than passing it as a parameter:
```cpp
// inside bowyerWatson, replace the parameter with:
auto bbox = GetBoundingBox(nodes);
double w = bbox[1].x - bbox[0].x;
double h = bbox[2].y - bbox[0].y;
auto [superNodes, ...] = generateLargeTriangle(w, h, h, 10);
```

---

### 1.7 `enforceConstraint` variable shadowing (line ~240)

Inside the cavity edge loop, `for (const auto& edge : e)` shadows the outer constraint `edge`. The outer `edge` is inaccessible inside this loop. Not currently a bug (outer `edge` isn't needed there), but fragile.

**Fix:** rename the inner variable: `for (const auto& elemEdge : e)`.

---

## 2. Hardcoded Logic

### 2.1 U-shape geometry parameters are hardcoded

`t = width / 3.0` and `d = height * 0.6` are baked in with no way to override them.

**Suggested signature:**
```cpp
uShape(double width, double height, int segsPerUnit,
       double wallThickness = -1.0,  // defaults to width/3
       double notchDepth    = -1.0,  // defaults to height*0.6
       int id_offset = 0)
```

---

### 2.2 `segsPerUnit / 4` magic number

The `/4` divisor appears in every shape generator with no explanation. It controls node density per unit length but is not documented or configurable.

---

### 2.3 Heat equation BCs hardcoded to `x=0` and `x=MaxX`

`run_FEM_Heat_Equation` applies Dirichlet BCs by checking `node.x == 0` and `node.x == MaxX`. This only makes sense for a rectangle. For ushape or circle meshes this applies BCs on the wrong nodes.

---

### 2.4 `run_Triangulation` passes `nx, ny` as geometric width/height

See bug 1.6 — the grid count and the physical dimension are being conflated.

---

## 3. Redundancies / Dead Code

### 3.1 `boundaryEdges` local vector is created but never used

In `setBoundary`, the ushape branch, and `AddHole`, a local `boundaryEdges` vector is populated alongside `edges` but is never returned or stored. It can be deleted.

```cpp
// delete this:
std::vector<Edge> boundaryEdges;
// and all .push_back calls into it
```

---

### 3.2 `isInPolygon` variable declared but never used

In `generateRandomNodes`, `isRectangular` branch:
```cpp
bool isInPolygon = true;  // never read
```
Delete it.

---

### 3.3 `generateRandomNodes` has three near-identical branches

`isboth`, `isRectangular`, and `!outerBoundary.empty()` all run the same while-loop with minor differences. Extract the loop into a private helper:

```cpp
void placeRandomNodes(int numNodes, double minX, double maxX, double minY, double maxY,
                      std::function<bool(const Node&)> accept) {
    int generated = 0, attempts = 0, maxAttempts = numNodes * 100;
    int id = nodes.size();
    while (generated < numNodes && attempts++ < maxAttempts) {
        double x = minX + static_cast<double>(rand()) / RAND_MAX * (maxX - minX);
        double y = minY + static_cast<double>(rand()) / RAND_MAX * (maxY - minY);
        Node n = {x, y, id};
        if (accept(n)) { nodes.push_back(n); id++; generated++; }
    }
}
```

---

### 3.4 `deleteElementsOutsideDomain` is dead code

This function does a hardcoded circle distance check and is never called anywhere. It predates `deleteHoles`. Delete it.

---

### 3.5 `drawCircle` is never called

`drawCircle` computes a circumcircle but `isInCircle` uses the determinant method directly. `drawCircle` is unreachable dead code.

---

### 3.6 Commented-out `circle` signature in `shape_generators.h`

```cpp
//inline std::vector<Node> circle(double radius = 6, ...
```
Delete it.

---

## 4. Architecture / Design Concerns

### 4.1 Shape type encoded as string flags (`isRectangular`, `isboth`)

Adding each new shape requires adding a new `bool` member. A single `enum class ShapeType { Rectangle, Both, UShape, ... }` member would be cleaner and make the flag-reset problem in bug 1.3 go away.

---

### 4.2 `initialize` is doing too much

It builds geometry, builds edges, and sets flags. `setBoundary` already does the same job for rectangles. The ushape branch duplicates the edge-building loop that's also in `setBoundary` and `AddHole`. Extracting a private `buildBoundaryEdges(const std::vector<Node>&)` helper would remove the duplication.

---

### 4.3 `enforceConstraint` fan is not guaranteed star-shaped

Fanning all cavity edges back to A works only if the cavity polygon is star-shaped from A. For long constraint edges crossing many triangles, A may not "see" all cavity boundary nodes. A more robust approach fans from the midpoint of AB or uses an ear-clipping approach on the cavity polygon.

---

### 4.4 No `deleteExterior` step — needed for all concave shapes

See bug 1.1. Currently the pipeline is:
```
bowyerWatson → enforceConstraint → deleteHoles
```
It should be:
```
bowyerWatson → enforceConstraint → deleteHoles → deleteExterior
```
`deleteExterior` uses `isPointInPolygon(centroid, outerBoundary)` and is already implementable with existing helpers.
