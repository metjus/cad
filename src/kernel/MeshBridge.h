#pragma once
#include "mesh/TriMesh.h"
#include <Poly_Triangulation.hxx>

namespace hcad::kernel {

// Converts reference mesh data into an OCCT triangulation for *display only*.
// The result is never used as B-Rep geometry.
Handle(Poly_Triangulation) toPolyTriangulation(const mesh::TriMesh& mesh);

} // namespace hcad::kernel
