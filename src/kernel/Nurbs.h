#pragma once
#include "kernel/NamedShape.h"

#include <Geom_BSplineSurface.hxx>
#include <gp_Pnt.hxx>

#include <string>
#include <vector>

namespace hcad::kernel {

// Editable definition of a NURBS surface. This is the data stored in the
// document; the OCCT Geom_BSplineSurface is always rebuilt from it.
struct NurbsSurfaceDef {
  int degreeU = 3;
  int degreeV = 3;
  int countU = 4;                 // poles in U
  int countV = 4;                 // poles in V
  std::vector<gp_Pnt> poles;      // row-major: index = u * countV + v
  std::vector<double> weights;    // same layout; empty = all 1.0 (non-rational)

  gp_Pnt& pole(int u, int v) { return poles[static_cast<size_t>(u * countV + v)]; }
  const gp_Pnt& pole(int u, int v) const { return poles[static_cast<size_t>(u * countV + v)]; }

  // Planar grid of poles spanning sizeU x sizeV in XY at height z.
  static NurbsSurfaceDef planarGrid(int countU, int countV, double sizeU, double sizeV, double z = 0.0, int degree = 3);
};

struct NurbsBuildResult {
  Handle(Geom_BSplineSurface) surface;
  NamedShape face;
  std::string error;
  bool ok() const { return error.empty(); }
};

// Builds a clamped, uniform-knot B-spline surface and a trimmed-to-bounds face.
NurbsBuildResult buildNurbsSurface(const NurbsSurfaceDef& def, const std::string& tag);

// Edges of the control polygon (for the control cage display).
TopoDS_Shape controlCage(const NurbsSurfaceDef& def);

} // namespace hcad::kernel
