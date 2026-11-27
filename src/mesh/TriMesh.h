#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace hcad::mesh {

using Vec3 = std::array<double, 3>;
using Tri = std::array<uint32_t, 3>;

// Indexed triangle mesh. This is *reference* data (imported polygons), never
// authoritative CAD geometry. Units: millimetres after import scaling.
struct TriMesh {
  std::string name;
  std::vector<Vec3> vertices;
  std::vector<Tri> triangles;

  bool empty() const { return triangles.empty(); }
  void scale(double factor);
};

// Merges vertices closer than `tolerance` (grid hashing). Returns the number
// of removed vertices. Triangle indices are remapped; triangles are kept even
// if they become degenerate so that analysis can still report them.
size_t weldVertices(TriMesh& mesh, double tolerance);

} // namespace hcad::mesh
