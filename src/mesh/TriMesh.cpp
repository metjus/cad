#include "mesh/TriMesh.h"

#include <cmath>
#include <unordered_map>

namespace hcad::mesh {

void TriMesh::scale(double f) {
  for (auto& v : vertices) for (auto& c : v) c *= f;
}

namespace {
struct KeyHash {
  size_t operator()(const std::array<int64_t, 3>& k) const noexcept {
    uint64_t h = 1469598103934665603ull;
    for (int64_t x : k) { h ^= static_cast<uint64_t>(x); h *= 1099511628211ull; }
    return static_cast<size_t>(h);
  }
};
} // namespace

size_t weldVertices(TriMesh& m, double tol) {
  if (tol <= 0) tol = 1e-9;
  std::unordered_map<std::array<int64_t, 3>, uint32_t, KeyHash> grid;
  std::vector<Vec3> out;
  std::vector<uint32_t> remap(m.vertices.size());
  out.reserve(m.vertices.size());
  for (size_t i = 0; i < m.vertices.size(); ++i) {
    const auto& v = m.vertices[i];
    std::array<int64_t, 3> key{std::llround(v[0] / tol), std::llround(v[1] / tol), std::llround(v[2] / tol)};
    auto [it, inserted] = grid.try_emplace(key, static_cast<uint32_t>(out.size()));
    if (inserted) out.push_back(v);
    remap[i] = it->second;
  }
  for (auto& t : m.triangles) for (auto& idx : t) idx = remap[idx];
  size_t removed = m.vertices.size() - out.size();
  m.vertices = std::move(out);
  return removed;
}

} // namespace hcad::mesh
