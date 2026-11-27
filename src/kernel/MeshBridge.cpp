#include "kernel/MeshBridge.h"

namespace hcad::kernel {

Handle(Poly_Triangulation) toPolyTriangulation(const mesh::TriMesh& m) {
  Handle(Poly_Triangulation) t = new Poly_Triangulation(static_cast<int>(m.vertices.size()),
                                                        static_cast<int>(m.triangles.size()), false);
  for (size_t i = 0; i < m.vertices.size(); ++i) {
    const auto& v = m.vertices[i];
    t->SetNode(static_cast<int>(i + 1), gp_Pnt(v[0], v[1], v[2]));
  }
  for (size_t i = 0; i < m.triangles.size(); ++i) {
    const auto& f = m.triangles[i];
    t->SetTriangle(static_cast<int>(i + 1), Poly_Triangle(static_cast<int>(f[0] + 1), static_cast<int>(f[1] + 1),
                                                          static_cast<int>(f[2] + 1)));
  }
  return t;
}

} // namespace hcad::kernel
