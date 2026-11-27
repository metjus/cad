#include "kernel/ShapeInfo.h"

#include <BRepBndLib.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>

#include <cstdio>

namespace hcad::kernel {

static int count(const TopoDS_Shape& s, TopAbs_ShapeEnum t) {
  TopTools_IndexedMapOfShape m;
  TopExp::MapShapes(s, t, m);
  return m.Extent();
}

ShapeStats shapeStats(const TopoDS_Shape& s) {
  ShapeStats st;
  if (s.IsNull()) return st;
  st.solids = count(s, TopAbs_SOLID);
  st.shells = count(s, TopAbs_SHELL);
  st.faces = count(s, TopAbs_FACE);
  st.edges = count(s, TopAbs_EDGE);
  st.vertices = count(s, TopAbs_VERTEX);
  GProp_GProps vp, sp;
  if (st.solids > 0) { BRepGProp::VolumeProperties(s, vp); st.volume = vp.Mass(); }
  BRepGProp::SurfaceProperties(s, sp);
  st.area = sp.Mass();
  Bnd_Box b;
  BRepBndLib::Add(s, b);
  if (!b.IsVoid()) b.Get(st.bbox[0], st.bbox[1], st.bbox[2], st.bbox[3], st.bbox[4], st.bbox[5]);
  st.valid = BRepCheck_Analyzer(s).IsValid();
  return st;
}

bool isValidSolid(const TopoDS_Shape& s) {
  return !s.IsNull() && count(s, TopAbs_SOLID) >= 1 && BRepCheck_Analyzer(s).IsValid();
}

std::string describe(const ShapeStats& st) {
  char buf[512];
  std::snprintf(buf, sizeof buf, "B-Rep %s: %d face(s), %d edge(s), %d vertex(es)\n",
                st.solids > 0 ? "solid" : "sheet", st.faces, st.edges, st.vertices);
  std::string s = buf;
  if (st.solids > 0) { std::snprintf(buf, sizeof buf, "Volume: %.3f mm3\n", st.volume); s += buf; }
  std::snprintf(buf, sizeof buf, "Area: %.3f mm2\nBounds: %.3f x %.3f x %.3f mm\nKernel check: %s", st.area,
                st.bbox[3] - st.bbox[0], st.bbox[4] - st.bbox[1], st.bbox[5] - st.bbox[2], st.valid ? "valid" : "INVALID");
  return s + buf;
}

} // namespace hcad::kernel
