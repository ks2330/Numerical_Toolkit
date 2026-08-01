#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <pybind11/functional.h>

#include <utility>  

#include "app_support/solver_api.h"
#include "mesh_generation/mesh_generation.h" 

namespace py = pybind11;


static py::array_t<double> nodesToArray(const std::vector<meshgeneration::Node>& nodes) {
    py::array_t<double> a({static_cast<py::ssize_t>(nodes.size()), py::ssize_t{2}});
    auto v = a.mutable_unchecked<2>();
    for (size_t i = 0; i < nodes.size(); ++i) {
        v(i, 0) = nodes[i].x;
        v(i, 1) = nodes[i].y;
    }
    return a;
}

static py::array_t<int> elementsToArray(const std::vector<meshgeneration::Element>& elems) {
    py::array_t<int>    a({static_cast<py::ssize_t>(elems.size()), py::ssize_t{3}});
    auto v = a.mutable_unchecked<2>();
    for (size_t i = 0; i < elems.size(); ++i) {
        v(i, 0) = elems[i].n0_id;
        v(i, 1) = elems[i].n1_id;
        v(i, 2) = elems[i].n2_id;
    }
    return a;
}

PYBIND11_MODULE(pycfd, m) {
    m.doc() = "Numerical Toolkit bindings — CFD solvers callable from Python (no CSV).";

    py::class_<app_support::NacaSpec>(m, "NacaSpec")
        .def(py::init<>())
        .def_readwrite("digits4", &app_support::NacaSpec::digits4)
        .def_readwrite("n_points", &app_support::NacaSpec::nPoints);

    py::class_<app_support::FvmConfig>(m, "FvmConfig")
        .def(py::init<>())
        .def_readwrite("naca", &app_support::FvmConfig::naca)
        .def_readwrite("density", &app_support::FvmConfig::density)
        .def_readwrite("mach", &app_support::FvmConfig::mach)
        .def_readwrite("alpha_deg", &app_support::FvmConfig::alphaDeg)
        .def_readwrite("rho_inf", &app_support::FvmConfig::rhoInf)
        .def_readwrite("p_inf", &app_support::FvmConfig::pInf)
        .def_readwrite("gamma", &app_support::FvmConfig::gamma)
        .def_readwrite("R", &app_support::FvmConfig::R)
        .def_readwrite("cfl", &app_support::FvmConfig::cfl)
        .def_readwrite("tolerance", &app_support::FvmConfig::tolerance)
        .def_readwrite("max_iters", &app_support::FvmConfig::maxIters)
        .def_readwrite("pressure_field_csv", &app_support::FvmConfig::pressureFieldCSV)
        .def_readwrite("forces_csv", &app_support::FvmConfig::forcesCSV);

    py::class_<app_support::HeatConfig>(m, "HeatConfig")
        .def(py::init<>())
        .def_readwrite("width",    &app_support::HeatConfig::width)
        .def_readwrite("height",   &app_support::HeatConfig::height)
        .def_readwrite("t_inlet",  &app_support::HeatConfig::T_inlet)
        .def_readwrite("density",  &app_support::HeatConfig::density);

    py::class_<meshgeneration::Mesh>(m, "Mesh")
        .def_property_readonly("nodes", [](const meshgeneration::Mesh& mesh) {
            return nodesToArray(mesh.nodes);
        })
        .def_property_readonly("elements", [](const meshgeneration::Mesh& mesh) {
            return elementsToArray(mesh.elements);
        })
        .def_property_readonly("chord", [](const meshgeneration::Mesh& mesh) {
            return mesh.chord;
        });

    py::class_<app_support::FvmResult>(m, "FvmResult")
        .def_property_readonly("nodes", [](const app_support::FvmResult& r) {
            return nodesToArray(r.mesh.nodes);
        })
        .def_property_readonly("elements", [](const app_support::FvmResult& r) {
            return elementsToArray(r.mesh.elements);
        })
        .def_property_readonly("pressure", [](const app_support::FvmResult& r) {
            return py::array_t<double>(r.pressure.size(), r.pressure.data());
        })
        .def_property_readonly("mach", [](const app_support::FvmResult& r) {
            return py::array_t<double>(r.mach.size(), r.mach.data());
        })
        .def_property_readonly("residual_history", [](const app_support::FvmResult& r) {
            return py::array_t<float>(r.residualHistory.size(), r.residualHistory.data());
        })
        .def_property_readonly("cl", [](const app_support::FvmResult& r) { return r.forces.cl; })
        .def_property_readonly("cd", [](const app_support::FvmResult& r) { return r.forces.cd; })
        .def_property_readonly("lift", [](const app_support::FvmResult& r) { return r.forces.lift; })
        .def_property_readonly("drag", [](const app_support::FvmResult& r) { return r.forces.drag; })
        .def_property_readonly("fx", [](const app_support::FvmResult& r) { return r.forces.fx; })
        .def_property_readonly("fy", [](const app_support::FvmResult& r) { return r.forces.fy; })
        .def_property_readonly("watertight", [](const app_support::FvmResult& r) { return r.watertight; });


    py::class_<app_support::FemResult>(m, "FemResult")
        .def_property_readonly("nodes",    [](const app_support::FemResult& r){ return nodesToArray(r.mesh.nodes); })
        .def_property_readonly("elements", [](const app_support::FemResult& r){ return elementsToArray(r.mesh.elements); })
        .def_property_readonly("field",    [](const app_support::FemResult& r){ return py::array_t<double>(r.field.size(), r.field.data()); });

    py::class_<app_support::BenchmarkResult>(m, "BenchmarkResult")
        .def_property_readonly("num_nodes",    [](const app_support::BenchmarkResult& r){ return py::array_t<int>(r.numNodes.size(), r.numNodes.data()); })
        .def_property_readonly("dense_times",  [](const app_support::BenchmarkResult& r){ return py::array_t<double>(r.denseTimes.size(), r.denseTimes.data()); })
        .def_property_readonly("sparse_times", [](const app_support::BenchmarkResult& r){ return py::array_t<double>(r.sparseTimes.size(), r.sparseTimes.data()); })
        .def_property_readonly("speedups",     [](const app_support::BenchmarkResult& r){ return py::array_t<double>(r.speedups.size(), r.speedups.data()); })
        .def_property_readonly("max_diffs",    [](const app_support::BenchmarkResult& r){ return py::array_t<double>(r.maxDiffs.size(), r.maxDiffs.data()); });



    m.def("naca_outline", [](int digits4, int n_points) {
        meshgeneration::Mesh mesh;
        mesh.generateNACA4(digits4, n_points);
        return nodesToArray(mesh.holeNodes);
    }, py::arg("digits4"), py::arg("n_points") = 160);

    m.def("build_aerofoil_mesh", [](const app_support::FvmConfig& cfg) {
        meshgeneration::Mesh mesh;
        {
            py::gil_scoped_release rel;     // without this a GUI thread stalls for the whole build
            mesh = app_support::buildAerofoilMesh(cfg);
        }
        return mesh;
    }, py::arg("cfg"));



    
    m.def("run_potential", [](app_support::FvmConfig cfg, py::object mesh) {
        app_support::FemResult r;
        if (mesh.is_none()) {
            py::gil_scoped_release rel;
            r = app_support::runPotential(cfg);
        } else {
            meshgeneration::Mesh& held = mesh.cast<meshgeneration::Mesh&>();
            py::gil_scoped_release rel;
            r = app_support::runPotential(cfg, std::move(held));
        }
        return r;
    }, py::arg("cfg"), py::arg("mesh") = py::none());




    m.def("build_heat_mesh", [](const app_support::HeatConfig& cfg) {
        meshgeneration::Mesh mesh;
        {
            py::gil_scoped_release rel;
            mesh = app_support::buildHeatMesh(cfg);
        }
        return mesh;
    }, py::arg("cfg"));

    m.def("run_heat", [](app_support::HeatConfig cfg, py::object mesh) {
        app_support::FemResult r;
        if (mesh.is_none()) {
            py::gil_scoped_release rel;
            r = app_support::runHeat(cfg);
        } else {
            meshgeneration::Mesh& held = mesh.cast<meshgeneration::Mesh&>();
            py::gil_scoped_release rel;
            r = app_support::runHeat(cfg, std::move(held));
        }
        return r;
    }, py::arg("cfg"), py::arg("mesh") = py::none());

    m.def("run_benchmark", [](std::vector<double> densities, int reps, int warmup) {
        py::gil_scoped_release rel;
        return app_support::runBenchmark(densities, reps, warmup);
    }, py::arg("densities") = std::vector<double>{5.0, 10.0, 20.0},
       py::arg("reps") = 3, py::arg("warmup") = 1);

    m.def("run_fvm", [](app_support::FvmConfig cfg, py::object cb, py::object mesh) {
        app_support::ProgressCallback ccb;
        if (!cb.is_none()) {
            ccb = [cb](int it, double res) {
                py::gil_scoped_acquire g;
                return py::cast<bool>(cb(it, res));
            };
        }
        app_support::FvmResult r;
        if (mesh.is_none()) {
            py::gil_scoped_release rel;
            r = app_support::runFvm(cfg, ccb);           
        } else {
            meshgeneration::Mesh& held = mesh.cast<meshgeneration::Mesh&>();
            py::gil_scoped_release rel;
            r = app_support::runFvm(cfg, ccb, std::move(held)); 
        }
        return r;
    }, py::arg("cfg"), py::arg("cb") = py::none(), py::arg("mesh") = py::none());

}
