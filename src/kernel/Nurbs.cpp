#include "kernel/Nurbs.h"

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRep_Builder.hxx>
#include <Precision.hxx>
#include <Standard_Failure.hxx>
#include <TColStd_Array1OfInteger.hxx>
#include <TColStd_Array1OfReal.hxx>
#include <TColStd_Array2OfReal.hxx>
#include <TColgp_Array2OfPnt.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>

namespace hcad::kernel {

NurbsSurfaceDef NurbsSurfaceDef::planarGrid(int nu, int nv, double su, double sv, double z, int degree) {
  NurbsSurfaceDef d;
  d.countU = nu;
  d.countV = nv;
  d.degreeU = std::min(degree, nu - 1);
  d.degreeV = std::min(degree, nv - 1);
  for (int u = 0; u < nu; ++u)
    for (int v = 0; v < nv; ++v)
      d.poles.emplace_back(su * u / (nu - 1), sv * v / (nv - 1), z);
  return d;
}

namespace {
// Clamped uniform knot vector for `count` poles of degree `deg`.
void clampedKnots(int count, int deg, TColStd_Array1OfReal& knots, TColStd_Array1OfInteger& mults) {
  const int nk = count - deg + 1;
  for (int i = 1; i <= nk; ++i) {
    knots(i) = static_cast<double>(i - 1) / (nk - 1);
    mults(i) = (i == 1 || i == nk) ? deg + 1 : 1;
  }
}
} // namespace

NurbsBuildResult buildNurbsSurface(const NurbsSurfaceDef& d, const std::string& tag) {
  NurbsBuildResult r;
  if (d.degreeU < 1 || d.degreeV < 1) { r.error = "Degree must be at least 1"; return r; }
  if (d.countU <= d.degreeU || d.countV <= d.degreeV) { r.error = "Need more control points than the degree in each direction"; return r; }
  if (static_cast<int>(d.poles.size()) != d.countU * d.countV) { r.error = "Control point count does not match the grid size"; return r; }
  if (!d.weights.empty() && d.weights.size() != d.poles.size()) { r.error = "Weight count does not match control point count"; return r; }
  for (double w : d.weights) if (w <= 0) { r.error = "Weights must be positive"; return r; }

  try {
    TColgp_Array2OfPnt poles(1, d.countU, 1, d.countV);
    for (int u = 0; u < d.countU; ++u)
      for (int v = 0; v < d.countV; ++v) poles(u + 1, v + 1) = d.pole(u, v);
    const int nku = d.countU - d.degreeU + 1, nkv = d.countV - d.degreeV + 1;
    TColStd_Array1OfReal ku(1, nku), kv(1, nkv);
    TColStd_Array1OfInteger mu(1, nku), mv(1, nkv);
    clampedKnots(d.countU, d.degreeU, ku, mu);
    clampedKnots(d.countV, d.degreeV, kv, mv);
    if (d.weights.empty()) {
      r.surface = new Geom_BSplineSurface(poles, ku, kv, mu, mv, d.degreeU, d.degreeV);
    } else {
      TColStd_Array2OfReal w(1, d.countU, 1, d.countV);
      for (int u = 0; u < d.countU; ++u)
        for (int v = 0; v < d.countV; ++v) w(u + 1, v + 1) = d.weights[static_cast<size_t>(u * d.countV + v)];
      r.surface = new Geom_BSplineSurface(poles, w, ku, kv, mu, mv, d.degreeU, d.degreeV);
    }
    BRepBuilderAPI_MakeFace mf(r.surface, Precision::Confusion());
    if (!mf.IsDone()) { r.error = "Could not build a face from the NURBS surface"; return r; }
    NamedShape ns(mf.Face());
    ns.nameFace(tag + ".Surface", mf.Face());
    r.face = ns;
  } catch (const Standard_Failure& e) {
    r.error = std::string("Kernel exception: ") + e.GetMessageString();
  }
  return r;
}

TopoDS_Shape controlCage(const NurbsSurfaceDef& d) {
  BRep_Builder b;
  TopoDS_Compound c;
  b.MakeCompound(c);
  auto add = [&](const gp_Pnt& a, const gp_Pnt& p) {
    if (a.Distance(p) > Precision::Confusion()) b.Add(c, BRepBuilderAPI_MakeEdge(a, p).Edge());
  };
  for (int u = 0; u < d.countU; ++u)
    for (int v = 0; v < d.countV; ++v) {
      if (u + 1 < d.countU) add(d.pole(u, v), d.pole(u + 1, v));
      if (v + 1 < d.countV) add(d.pole(u, v), d.pole(u, v + 1));
    }
  return c;
}

} // namespace hcad::kernel
