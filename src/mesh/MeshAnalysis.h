#pragma once
#include "mesh/TriMesh.h"
#include <string>

namespace hcad::mesh {

struct MeshReport {
  size_t vertexCount = 0;
  size_t triangleCount = 0;
  size_t componentCount = 0;
  size_t boundaryEdges = 0;       // edges used by exactly one triangle
  size_t boundaryLoops = 0;       // ~ number of holes / open borders
  size_t nonManifoldEdges = 0;    // edges used by more than two triangles
  size_t inconsistentEdges = 0;   // shared edges with equal direction (flipped neighbour)
  size_t degenerateTriangles = 0; // zero area or repeated index
  size_t duplicateTriangles = 0;
  Vec3 bboxMin{}, bboxMax{};
  double surfaceArea = 0;
  double signedVolume = 0;        // meaningful only when closed
  bool closed = false;            // watertight + manifold
  bool normalsInverted = false;   // closed and signed volume < 0

  std::string summary() const;
};

MeshReport analyzeMesh(const TriMesh& mesh);

} // namespace hcad::mesh
