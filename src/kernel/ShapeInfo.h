#pragma once
#include <TopoDS_Shape.hxx>
#include <string>

namespace hcad::kernel {

struct ShapeStats {
  int solids = 0, shells = 0, faces = 0, edges = 0, vertices = 0;
  double volume = 0;    // mm^3
  double area = 0;      // mm^2
  double bbox[6] = {};  // xmin ymin zmin xmax ymax zmax
  bool valid = false;   // BRepCheck_Analyzer
};

ShapeStats shapeStats(const TopoDS_Shape& s);
bool isValidSolid(const TopoDS_Shape& s);
std::string describe(const ShapeStats& st);

} // namespace hcad::kernel
