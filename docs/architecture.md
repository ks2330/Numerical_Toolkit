# Numerical Toolkit: Subsonic CFD Aerofoil Solver Architecture

This document serves as a comprehensive offline guide to transforming the existing `Numerical_Toolkit` foundation into a functional 2D subsonic CFD solver for aerofoils.

## Current State Analysis
You already have a remarkably strong foundation:
1.  **Robust Unstructured Mesher (`mesh_generation.h`)**: A fully functional 2D Delaunay mesher with Bowyer-Watson, CSV boundary loading, Poisson disk sampling for interior nodes, quality refinement, and crucial support for holes.
2.  **Laplacian FEM (`FEM_Global_Stiffness_Matrix.h`)**: Implementation of the element stiffness matrix for the Laplace operator (linear triangular elements) and global assembly.
3.  **Linear Algebra**: Integration with `Eigen`.
4.  **Boundary Conditions**: Infrastructure for Dirichlet boundary conditions.

---

## Roadmap Overview

To simulate a subsonic aerofoil, there are two distinct paths.
*   **Path A: Potential Flow** (Inviscid, Irrotational). Highly recommended as the first milestone because it perfectly utilizes the existing Laplacian code ($\nabla^2 \phi = 0$).
*   **Path B: Incompressible Navier-Stokes** (Viscous Flow). The advanced approach required for simulating flow separation, drag, and true viscous effects.

---

## Path A: The Potential Flow Solver (First Milestone)

Subsonic potential flow is governed by Laplace's equation: $\nabla^2 \phi = 0$, where $\phi$ is the velocity potential.

### 1. Geometry and Domain
*   **Aerofoil Profile**: Generate NACA 4-digit aerofoil coordinates (e.g., using a Python script) and save them to a CSV.
*   **Computational Domain**: Define a large outer boundary (far-field), e.g., a circle with a radius 20x the aerofoil chord length. The aerofoil acts as a "hole" in the middle.
*   **Meshing**: Triangulate the annular region between the far-field and the aerofoil surface.

### 2. Boundary Conditions
*   **Far-field Boundary**: Enforce the freestream velocity $(u_\infty, v_\infty)$.
    *   Since $\mathbf{V} = \nabla \phi$, the potential at the far-field can be prescribed as a Dirichlet condition: $\phi(x,y) = u_\infty x + v_\infty y$.
*   **Aerofoil Surface (Solid Wall)**: Flow tangency implies zero penetration: $\frac{\partial \phi}{\partial n} = \mathbf{V} \cdot \mathbf{n} = 0$.
    *   *FEM Advantage*: In the Galerkin finite element method, a homogeneous Neumann boundary condition ($\frac{\partial \phi}{\partial n} = 0$) is the "natural" boundary condition. If you do not explicitly add boundary integrals to your Right-Hand Side (RHS) load vector for these surface nodes, the solver automatically enforces zero flow through the surface.

### 3. Solving the System
You assemble the global stiffness matrix $K$ and solve the linear system $K \Phi = F$, where $F$ incorporates the Dirichlet BCs from the far-field.

### 4. Post-Processing
Once you have the potential $\phi$ at every node:
1.  **Compute Velocity**: Calculate the gradient of the potential within each element. For a linear triangle, the velocity $(u,v)$ is constant across the element.
    $u_e = \frac{\partial \phi}{\partial x}$, $v_e = \frac{\partial \phi}{\partial y}$
2.  **Compute Pressure**: Use Bernoulli's equation to find the pressure coefficient $C_p$:
    $C_p = 1 - \left( \frac{|\mathbf{V}|^2}{|\mathbf{V}_\infty|^2} \right)$

### 5. Advanced: The Kutta Condition (Lifting Flow)
A pure potential flow around a cylinder or aerofoil produces zero lift (d'Alembert's paradox). To generate lift, circulation $\Gamma$ must be introduced.
*   You must enforce the **Kutta Condition**: flow must leave the sharp trailing edge smoothly.
*   This requires introducing a "branch cut" or wake line extending from the trailing edge to the far-field boundary. The potential $\phi$ will have a jump across this line equal to the circulation $\Gamma$.

---

## Path B: Incompressible Navier-Stokes (Advanced)

For viscous fluids (required for skin friction, flow separation, etc.), you solve the Incompressible Navier-Stokes equations:
1.  Conservation of Momentum: $\frac{\partial \mathbf{u}}{\partial t} + (\mathbf{u} \cdot \nabla)\mathbf{u} = -\frac{1}{\rho}\nabla P + \nu \nabla^2 \mathbf{u}$
2.  Conservation of Mass (Incompressibility): $\nabla \cdot \mathbf{u} = 0$

### 1. Mandatory Core Upgrade: Sparse Matrices
Dense matrices (`std::vector<std::vector<double>>`) scale at $O(N^2)$ for memory. CFD meshes easily exceed 10,000 nodes.
**Crucial First Step**: Refactor `FEM_Global_Stiffness_Matrix.h` to use `Eigen::SparseMatrix<double>` and solvers like `Eigen::SparseLU`.

### 2. Required Matrices
You will need to implement integration routines for:
*   **Stiffness Matrix ($K$)**: (Already done!) $\int \nabla N_i \cdot \nabla N_j \, d\Omega$
*   **Mass Matrix ($M$)**: $\int N_i N_j \, d\Omega$
*   **Convection Matrix ($C(\mathbf{u})$)**: Non-linear, depends on current velocity. $\int N_i (\mathbf{u} \cdot \nabla) N_j \, d\Omega$
*   **Gradient ($G$) & Divergence ($D$) Operators**: To handle pressure gradients and velocity divergence.

### 3. Fractional Step / Projection Method (Chorin's Method)
A standard way to solve time-dependent incompressible N-S is by decoupling velocity and pressure over a time step $\Delta t$.

#### Pseudocode for the Time-Stepping Loop

```cpp
// Pseudocode for Chorin's Projection Method
void solveNavierStokes(Mesh& mesh) {
    // 0. Setup Sparse Matrices (Mass M, Stiffness K)
    Eigen::SparseMatrix<double> M = assembleMassMatrix(mesh);
    Eigen::SparseMatrix<double> K = assembleStiffnessMatrix(mesh);
    
    // Initialize state
    Vector u_n = initial_velocity_x;
    Vector v_n = initial_velocity_y;
    Vector p_n = initial_pressure;

    for (double t = 0; t < t_end; t += dt) {
        
        // 1. Predictor Step: Solve for intermediate velocity (u*, v*) ignoring pressure
        // (M/dt + C(u_n) + nu*K) u* = (M/dt) u_n
        Eigen::SparseMatrix<double> C = assembleConvectionMatrix(mesh, u_n, v_n);
        Eigen::SparseMatrix<double> A_momentum = (1.0/dt) * M + C + viscosity * K;
        
        Vector rhs_u = (1.0/dt) * M * u_n;
        Vector rhs_v = (1.0/dt) * M * v_n;
        
        // Apply Boundary Conditions (Dirichlet: no-slip on aerofoil, freestream at inlet)
        applyVelocityBCs(A_momentum, rhs_u, rhs_v);
        
        Vector u_star = solveLinearSystem(A_momentum, rhs_u);
        Vector v_star = solveLinearSystem(A_momentum, rhs_v);
        
        // 2. Pressure-Poisson Step: Solve for pressure to enforce incompressibility
        // K * p_{n+1} = (rho/dt) * Divergence(u*, v*)
        Vector rhs_p = (density/dt) * computeDivergenceLoadVector(mesh, u_star, v_star);
        
        // Apply Pressure BCs (Dirichlet: p=0 at outlet)
        Eigen::SparseMatrix<double> K_poisson = K; // Reuse Laplacian stiffness matrix!
        applyPressureBCs(K_poisson, rhs_p);
        
        Vector p_new = solveLinearSystem(K_poisson, rhs_p);
        
        // 3. Corrector Step: Project velocity onto divergence-free space
        // u_{n+1} = u* - (dt/rho) * Gradient_x(p_{n+1})
        // v_{n+1} = v* - (dt/rho) * Gradient_y(p_{n+1})
        Vector grad_px = computeGradientX(mesh, p_new);
        Vector grad_py = computeGradientY(mesh, p_new);
        
        u_n = u_star - (dt / density) * grad_px;
        v_n = v_star - (dt / density) * grad_py;
        p_n = p_new;
    }
}
```

### 4. Element Selection Consideration
For the Navier-Stokes pressure step, using simple P1-P1 elements (linear velocity, linear pressure on the same nodes) violates the **LBB (Ladyzhenskaya-Babuška-Brezzi) stability condition**, leading to non-physical pressure oscillations.
*   *Solution*: You will likely need to implement stabilization (like SUPG or PSPG) or move to mixed elements (e.g., Taylor-Hood P2-P1 elements, where velocity is quadratic and pressure is linear).

---

## Immediate Next Steps (Action Plan)

1.  **Eigen Sparse Matrix Refactor**: Before pursuing either Path A or B, the `FEM_Global_Stiffness_Matrix.h` must be rewritten to utilize `Eigen::SparseMatrix<double>` and `std::vector<Eigen::Triplet<double>>` for assembly.
2.  **Potential Flow Implementation**:
    *   Generate a NACA mesh.
    *   Set up far-field Dirichlet BCs for potential $\phi$.
    *   Run the solver and visualize the contours of $\phi$.
    *   Compute and plot the velocity vectors and $C_p$ surface distribution.
3.  **Kutta Condition**: Add the wake branch cut to enable lift generation.
