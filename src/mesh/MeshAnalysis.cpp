#include "mesh/MeshAnalysis.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>
#include <set>
#include <sstream>

namespace hcad::mesh {
namespace {

Vec3 sub(const Vec3& a, const Vec3& b) { return {a[0] - b[0], a[1] - b[1], a[2] - b[2]}; }
Vec3 cross(const Vec3& a, const Vec3& b) {
  return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}
double norm(const Vec3& a) { return std::sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]); }

struct UnionFind {
  std::vector<uint32_t> p;
  explicit UnionFind(size_t n) : p(n) { std::iota(p.begin(), p.end(), 0u); }
  uint32_t find(uint32_t x) { while (p[x] != x) x = p[x] = p[p[x]]; return x; }
  void unite(uint32_t a, uint32_t b) { p[find(a)] = find(b); }
};

} // namespace

MeshReport analyzeMesh(const TriMesh& m) {
  MeshReport r;
  r.vertexCount = m.vertices.size();
  r.triangleCount = m.triangles.size();
  if (m.vertices.empty()) return r;

  r.bboxMin = r.bboxMax = m.vertices.front();
  for (const auto& v : m.vertices)
    for (int i = 0; i < 3; ++i) { r.bboxMin[i] = std::min(r.bboxMin[i], v[i]); r.bboxMax[i] = std::max(r.bboxMax[i], v[i]); }

  double diag = norm(sub(r.bboxMax, r.bboxMin));
  double areaEps = std::max(1e-12, 1e-12 * diag * diag);

  // Undirected edge -> (count, sum of orientation signs)
  std::map<std::pair<uint32_t, uint32_t>, std::pair<int, int>> edges;
  std::set<std::array<uint32_t, 3>> seenTris;
  UnionFind uf(m.vertices.size());
  std::vector<bool> used(m.vertices.size(), false);

  for (const auto& t : m.triangles) {
    if (t[0] == t[1] || t[1] == t[2] || t[0] == t[2]) { ++r.degenerateTriangles; continue; }
    const Vec3 &a = m.vertices[t[0]], &b = m.vertices[t[1]], &c = m.vertices[t[2]];
    Vec3 n = cross(sub(b, a), sub(c, a));
    double area2 = norm(n);
    if (area2 * 0.5 <= areaEps) ++r.degenerateTriangles;
    r.surfaceArea += 0.5 * area2;
    r.signedVolume += (a[0] * (b[1] * c[2] - b[2] * c[1]) - a[1] * (b[0] * c[2] - b[2] * c[0]) + a[2] * (b[0] * c[1] - b[1] * c[0])) / 6.0;

    auto key = t;
    std::sort(key.begin(), key.end());
    if (!seenTris.insert(key).second) ++r.duplicateTriangles;

    for (int k = 0; k < 3; ++k) {
      uint32_t u = t[k], v = t[(k + 1) % 3];
      used[u] = true;
      uf.unite(u, v);
      auto e = u < v ? std::make_pair(u, v) : std::make_pair(v, u);
      auto& rec = edges[e];
      rec.first += 1;
      rec.second += u < v ? 1 : -1;
    }
  }

  std::map<uint32_t, std::vector<uint32_t>> boundaryAdj;
  for (const auto& [e, rec] : edges) {
    if (rec.first == 1) {
      ++r.boundaryEdges;
      boundaryAdj[e.first].push_back(e.second);
      boundaryAdj[e.second].push_back(e.first);
    } else if (rec.first > 2) {
      ++r.nonManifoldEdges;
    } else if (rec.second != 0) {
      ++r.inconsistentEdges;
    }
  }

  // Boundary loops = connected components of the boundary-edge graph.
  std::set<uint32_t> visited;
  for (const auto& [v, _] : boundaryAdj) {
    if (visited.count(v)) continue;
    ++r.boundaryLoops;
    std::vector<uint32_t> stack{v};
    while (!stack.empty()) {
      uint32_t x = stack.back(); stack.pop_back();
      if (!visited.insert(x).second) continue;
      for (uint32_t y : boundaryAdj[x]) if (!visited.count(y)) stack.push_back(y);
    }
  }

  std::set<uint32_t> roots;
  for (uint32_t i = 0; i < used.size(); ++i) if (used[i]) roots.insert(uf.find(i));
  r.componentCount = roots.size();

  r.closed = r.boundaryEdges == 0 && r.nonManifoldEdges == 0 && r.triangleCount > 0;
  r.normalsInverted = r.closed && r.inconsistentEdges == 0 && r.signedVolume < 0;
  return r;
}

std::string MeshReport::summary() const {
  std::ostringstream s;
  s.setf(std::ios::fixed);
  s.precision(3);
  s << "Vertices: " << vertexCount << "\n"
    << "Triangles: " << triangleCount << "\n"
    << "Connected components: " << componentCount << "\n"
    << "Open boundary edges: " << boundaryEdges << " (" << boundaryLoops << " loop(s))\n"
    << "Non-manifold edges: " << nonManifoldEdges << "\n"
    << "Inconsistently oriented edges: " << inconsistentEdges << "\n"
    << "Degenerate triangles: " << degenerateTriangles << "\n"
    << "Duplicate triangles: " << duplicateTriangles << "\n"
    << "Size: " << (bboxMax[0] - bboxMin[0]) << " x " << (bboxMax[1] - bboxMin[1]) << " x " << (bboxMax[2] - bboxMin[2]) << " mm\n"
    << "Surface area: " << surfaceArea << " mm2\n";
  if (closed) s << "Enclosed volume: " << std::abs(signedVolume) << " mm3\n";
  s << "Watertight: " << (closed ? "yes" : "no");
  if (normalsInverted) s << "\nWarning: normals point inwards";
  return s.str();
}

} // namespace hcad::mesh
