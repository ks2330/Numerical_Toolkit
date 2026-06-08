#pragma once

namespace meshgeneration {

class Mesh;

class TriangulationAlgorithm {
public:
    virtual void run(Mesh& mesh) = 0;
    virtual ~TriangulationAlgorithm() = default;
};

} // namespace meshgeneration